// SPDX-License-Identifier: Apache-2.0
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../../src/WebControlServer.cpp'), 'utf8');
const html = source.match(/const char WEB_UI\[\][\s\S]*?R"HTML\(([\s\S]*?)\)HTML"/)[1];
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];

async function consolePage() {
  const elements = new Map([...html.matchAll(/\bid="([^"]+)"/g)].map(([, id]) => [id, {
    value: '', textContent: '', innerHTML: '', checked: false, disabled: false,
    dataset: {}, classList: { add() {}, remove() {} }, listeners: {},
    addEventListener(event, fn) { this.listeners[event] = fn; },
    querySelectorAll() { return []; }
  }]));
  const calls = [];
  const context = vm.createContext({
    document: {
      title: '', getElementById(id) {
        assert.ok(elements.has(id), `Script refers to missing HTML element: ${id}`);
        return elements.get(id);
      }
    }, URLSearchParams, setInterval() {}, console,
    async fetch(url, options = {}) {
      calls.push({ url, ...options });
      const data = url === '/config'
        ? { ok: true, hostname: 'water', mdns_host: 'water.local', relay_auto_off_minutes: 60, ota_auto_schedule_enabled: true }
        : url === '/status' ? { ok: true, relay: 'off', hostname: 'water' }
        : { ok: true, enabled: true, events: [], count: 0, capacity: 10 };
      return { ok: true, status: 200, async json() { return data; }, async text() { return JSON.stringify(data); } };
    }
  });
  vm.runInContext(script, context);
  // Drain the page's initial asynchronous refresh without changing production code.
  await new Promise(resolve => setImmediate(resolve));
  calls.length = 0;
  return { context, elements, calls };
}

test('console has no MQTT or disconnected LED controls', async () => {
  assert.doesNotMatch(html, /MQTT|\bLED(?:S|s)?\b|mqtt_|\/led\//);
  await consolePage();
});

test('relay buttons still call the local on/off/toggle endpoints', async () => {
  const page = await consolePage();
  for (const [id, url] of [['btnOn', '/on'], ['btnOff', '/off'], ['btnToggle', '/toggle']]) {
    page.calls.length = 0;
    await page.elements.get(id).listeners.click();
    assert.ok(page.calls.some(call => call.url === url && call.method === 'POST'));
  }
});

test('configuration saves surviving settings without retired fields', async () => {
  const page = await consolePage();
  page.elements.get('hostnameInput').value = 'water';
  page.elements.get('relayAutoOffMinutesInput').value = '30';
  page.elements.get('otaAutoScheduleDisabledInput').checked = true;
  await page.context.applyConfig();
  const request = page.calls.find(call => call.url === '/config' && call.method === 'POST');
  assert.ok(request);
  assert.deepEqual(Object.fromEntries(new URLSearchParams(request.body)), {
    hostname: 'water', relay_auto_off_minutes: '30', ota_auto_schedule_enabled: '0'
  });
});

test('relay status presentation works without legacy MQTT/LED fields', async () => {
  const page = await consolePage();
  page.context.showJson({ ok: true, relay: 'on', hostname: 'water', uptime_ms: 2000 });
  assert.equal(page.elements.get('relayStateBadge').dataset.state, 'on');
  assert.equal(page.elements.get('error').textContent, 'none');
});

test('schedule time and day-mask validation still rejects invalid values', async () => {
  const { context } = await consolePage();
  assert.equal(context.validateTimeHm('25:00').ok, false);
  assert.equal(context.validateTimeHm('10:30').ok, true);
  assert.equal(context.validateDowMask('128').ok, false);
  assert.equal(context.validateDowMask('1').ok, true);
});
