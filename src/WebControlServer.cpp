#include "WebControlServer.h"

#include <Arduino.h>
#include <WebServer.h>

#include "AppConfig.h"
#include "CommandRouter.h"
#include "MqttManager.h"
#include "RelayController.h"
#include "WiFiManager.h"

namespace
{
    WebServer gServer(80);

    String jsonEscape(const String &input)
    {
        String escaped;
        escaped.reserve(input.length() + 8);

        for (size_t i = 0; i < input.length(); i++)
        {
            const char c = input[i];
            if (c == '\\')
            {
                escaped += "\\\\";
            }
            else if (c == '"')
            {
                escaped += "\\\"";
            }
            else if (c == '\n')
            {
                escaped += "\\n";
            }
            else if (c == '\r')
            {
                escaped += "\\r";
            }
            else
            {
                escaped += c;
            }
        }

        return escaped;
    }

    String buildConfigJson(const WebControlContext &context)
    {
        String json = "{";
        json += "\"ok\":true,";
        json += "\"device_name\":\"";
        json += jsonEscape(DEVICE_NAME_DEFAULT);
        json += "\",";
        json += "\"device_type\":\"";
        json += jsonEscape(DEVICE_TYPE_DEFAULT);
        json += "\",";
        json += "\"firmware\":{\"name\":\"";
        json += jsonEscape(FIRMWARE_NAME);
        json += "\",\"version\":\"";
        json += jsonEscape(FIRMWARE_VERSION);
        json += "\"},";
        json += "\"hostname_default\":\"";
        json += jsonEscape(DEVICE_HOSTNAME_DEFAULT);
        json += "\",";
        json += "\"hostname\":\"";
        json += jsonEscape(context.getHostname != nullptr ? context.getHostname() : String(DEVICE_HOSTNAME_DEFAULT));
        json += "\",";
        json += "\"mdns_host\":\"";
        json += jsonEscape((context.getHostname != nullptr ? context.getHostname() : String(DEVICE_HOSTNAME_DEFAULT)) + String(".local"));
        json += "\",";
        json += "\"mqtt_host\":\"";
        json += jsonEscape(context.mqtt != nullptr ? context.mqtt->serverHost() : String(MQTT_HOST));
        json += "\",";
        json += "\"mqtt_port\":";
        json += context.mqtt != nullptr ? context.mqtt->serverPort() : MQTT_PORT;
        json += ",";
        json += "\"wifi_ssid\":\"";
        json += jsonEscape(context.wifi != nullptr ? context.wifi->ssid() : String(WIFI_SSID));
        json += "\",";
        json += "\"wifi_password_set\":";
        json += context.wifi != nullptr ? (context.wifi->ssid().length() > 0 ? "true" : "false") : "false";
        json += ",";
        json += "\"relay_pin\":";
        json += RELAY_PIN;
        json += ",";
        json += "\"relay_led_pin\":";
        json += RELAY_LED_PIN;
        json += ",";
        json += "\"wifi_led_pin\":";
        json += WIFI_LED_PIN;
        json += ",";
        json += "\"relay_button_pin\":";
        json += RELAY_BUTTON_PIN;
        json += ",";
        json += "\"reset_button_pin\":";
        json += RESET_BUTTON_PIN;
        json += ",";
        json += "\"nvs_health\":\"";
        json += jsonEscape(context.getNvsHealth != nullptr ? context.getNvsHealth() : String("nvs health unavailable"));
        json += "\"";
        json += "}";
        return json;
    }

    const char WEB_UI[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover" />
  <title>Relay Control</title>
  <style>
    :root {
      color-scheme: dark;
      --bg-0: #020617;
      --bg-1: #0f172a;
      --bg-2: #111827;
      --card: rgba(15, 23, 42, 0.88);
      --card-border: rgba(148, 163, 184, 0.18);
      --text: #e2e8f0;
      --muted: #94a3b8;
      --line: rgba(148, 163, 184, 0.12);
      --accent: #22d3ee;
      --accent-2: #38bdf8;
      --shadow: 0 18px 40px rgba(2, 6, 23, 0.55);
    }
    html, body {
      width: 100%;
      min-width: 100%;
    }
    body {
      margin: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background:
        radial-gradient(circle at 15% 15%, rgba(34, 211, 238, 0.10), transparent 30%),
        radial-gradient(circle at 85% 15%, rgba(56, 189, 248, 0.10), transparent 28%),
        linear-gradient(180deg, var(--bg-1) 0%, var(--bg-0) 100%);
      color: var(--text);
      min-height: 100vh;
      overflow-x: hidden;
    }
    .wrap {
      width: 100%;
      max-width: none;
      margin: 0;
      padding: calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-top))
               calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-right))
               calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-bottom))
               calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-left));
      box-sizing: border-box;
    }
    .card {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 18px;
      margin-bottom: 16px;
      box-shadow: var(--shadow);
      width: 100%;
      box-sizing: border-box;
      backdrop-filter: blur(14px);
    }
    h1 {
      margin: 0 0 8px;
      font-size: clamp(1.5rem, 2vw, 2rem);
      letter-spacing: -0.03em;
    }
    .muted { color: var(--muted); margin: 0 0 10px; }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
      gap: 14px;
      margin-top: 6px;
    }
    .statusStrip {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
      margin: 8px 0 14px;
    }
    .statusBadge {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 10px 14px;
      border-radius: 999px;
      background: rgba(2, 6, 23, 0.68);
      border: 1px solid rgba(148, 163, 184, 0.18);
      color: var(--text);
      font-size: 0.92rem;
      font-weight: 700;
    }
    .statusDot {
      width: 10px;
      height: 10px;
      border-radius: 999px;
      background: #64748b;
      box-shadow: 0 0 0 4px rgba(100, 116, 139, 0.14);
    }
    .statusBadge[data-state="on"] .statusDot {
      background: #34d399;
      box-shadow: 0 0 0 4px rgba(52, 211, 153, 0.16);
    }
    .statusBadge[data-state="off"] .statusDot {
      background: #fb7185;
      box-shadow: 0 0 0 4px rgba(251, 113, 133, 0.16);
    }
    .statusNote {
      color: var(--muted);
      font-size: 0.9rem;
    }
    button {
      border: none;
      border-radius: 16px;
      padding: 16px 14px;
      font-weight: 700;
      color: #e2e8f0;
      cursor: pointer;
      transition: transform .08s ease, opacity .2s ease, box-shadow .2s ease;
      min-height: 54px;
      touch-action: manipulation;
    }
    button:active { transform: scale(0.985); }
    button:disabled { opacity: 0.55; cursor: not-allowed; }
    .on { background: linear-gradient(135deg, #115e59, #0f766e); box-shadow: 0 10px 24px rgba(15, 118, 110, 0.24); }
    .off { background: linear-gradient(135deg, #7f1d1d, #b91c1c); box-shadow: 0 10px 24px rgba(185, 28, 28, 0.24); }
    .toggle { background: linear-gradient(135deg, #7c2d12, #c2410c); box-shadow: 0 10px 24px rgba(194, 65, 12, 0.24); }
    .refresh { background: linear-gradient(135deg, #1e40af, #2563eb); box-shadow: 0 10px 24px rgba(37, 99, 235, 0.24); }
    .save { background: linear-gradient(135deg, #6d28d9, #7c3aed); color: #f8fafc; box-shadow: 0 10px 24px rgba(124, 58, 237, 0.24); }
    .is-active {
      position: relative;
      box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.12), 0 0 0 6px rgba(34, 211, 238, 0.10), 0 14px 30px rgba(2, 6, 23, 0.35);
      transform: translateY(-1px);
    }
    .is-active::after {
      content: "";
      position: absolute;
      inset: -6px;
      border-radius: 18px;
      border: 1px solid rgba(34, 211, 238, 0.22);
      animation: activePulse 1.6s ease-in-out infinite;
      pointer-events: none;
    }
    @keyframes activePulse {
      0% {
        opacity: 0.18;
        transform: scale(0.98);
      }
      60% {
        opacity: 0.55;
        transform: scale(1.02);
      }
      100% {
        opacity: 0.18;
        transform: scale(1.04);
      }
    }
    .is-inactive {
      opacity: 0.72;
      box-shadow: 0 8px 18px rgba(2, 6, 23, 0.2);
    }
    .kv { margin: 6px 0; font-size: 0.95rem; line-height: 1.45; }
    .sectionTitle { margin: 0 0 10px; font-size: 1rem; letter-spacing: 0.03em; text-transform: uppercase; color: var(--accent); }
    .row { display: flex; gap: 8px; flex-wrap: wrap; }
    .settingsGroup {
      margin-top: 12px;
      border-radius: 20px;
      overflow: hidden;
      background: rgba(2, 6, 23, 0.22);
      border: 1px solid rgba(148, 163, 184, 0.14);
    }
    .settingsRow {
      display: grid;
      grid-template-columns: minmax(138px, 220px) minmax(0, 1fr);
      gap: 12px;
      align-items: center;
      padding: 14px 16px;
      border-bottom: 1px solid rgba(148, 163, 184, 0.12);
      background: rgba(15, 23, 42, 0.35);
    }
    .settingsRow:last-child {
      border-bottom: none;
    }
    .settingsLabel {
      display: flex;
      flex-direction: column;
      gap: 2px;
      min-width: 0;
    }
    .settingsLabel span {
      font-size: 0.95rem;
      font-weight: 700;
      color: var(--text);
    }
    .settingsLabel small {
      font-size: 0.8rem;
      color: var(--muted);
      line-height: 1.35;
    }
    .settingsValue {
      width: 100%;
    }
    .settingsValue input[type=text] {
      width: 100%;
      min-width: 0;
      box-sizing: border-box;
    }
    .settingsRow--action {
      grid-template-columns: 1fr;
    }
    .settingsRow--action button {
      width: 100%;
      min-height: 52px;
    }
    .stackSection {
      margin-top: 14px;
    }
    .stateCard {
      margin-top: 12px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 14px 16px;
      border-radius: 18px;
      background: linear-gradient(135deg, rgba(15, 23, 42, 0.78), rgba(2, 6, 23, 0.68));
      border: 1px solid rgba(148, 163, 184, 0.18);
    }
    .stateCard strong {
      display: block;
      font-size: 0.92rem;
      color: var(--muted);
      letter-spacing: 0.02em;
      font-weight: 700;
      margin-bottom: 4px;
    }
    .stateCard span {
      font-size: 1.15rem;
      font-weight: 800;
      letter-spacing: -0.02em;
    }
    .list { margin: 10px 0 0; padding: 0; list-style: none; }
    .list li {
      border-top: 1px solid var(--line);
      padding: 10px 0;
    }
    .list small { color: var(--muted); display: block; margin-top: 4px; }
    input[type=text] {
      flex: 1;
      min-width: 0;
      border-radius: 14px;
      border: 1px solid rgba(148, 163, 184, 0.28);
      padding: 14px 16px;
      font-size: 1rem;
      background: rgba(2, 6, 23, 0.72);
      color: var(--text);
      box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.02);
    }
    input[type=text]::placeholder {
      color: #64748b;
    }
    pre {
      margin: 0;
      white-space: pre-wrap;
      background: rgba(2, 6, 23, 0.92);
      border-radius: 12px;
      border: 1px solid rgba(148, 163, 184, 0.18);
      padding: 12px;
      min-height: 92px;
      overflow-wrap: anywhere;
      color: #c7d2fe;
    }
    @media (max-width: 520px) {
      .grid { grid-template-columns: 1fr; }
      .settingsRow { grid-template-columns: 1fr; gap: 8px; padding: 14px; }
      .card { padding: 16px; border-radius: 14px; }
      .row { gap: 10px; }
      button { min-height: 52px; }
      .stateCard {
        align-items: flex-start;
        flex-direction: column;
      }
    }
    @media (pointer: coarse) {
      .grid { gap: 12px; }
      .settingsGroup { border-radius: 18px; }
      .settingsRow { padding: 14px 15px; }
      input[type=text] {
        padding: 14px;
        font-size: 1.05rem;
      }
      .muted {
        line-height: 1.5;
      }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>ESP32 Relay Console</h1>
      <p class="muted">Dark mode control panel with live relay, network, and configuration status.</p>
      <div class="stateCard">
        <div>
          <strong>Current Relay State</strong>
          <span id="relayStateLabel">unknown</span>
        </div>
        <div class="statusBadge" id="relayStateBadge" data-state="unknown">
          <span class="statusDot"></span>
          <span id="relayStateBadgeText">Waiting for status</span>
        </div>
      </div>
      <div class="grid stackSection">
        <button id="btnOn" class="on">ON</button>
        <button id="btnOff" class="off">OFF</button>
        <button id="btnToggle" class="toggle">TOGGLE</button>
        <button id="btnRefresh" class="refresh">REFRESH STATUS</button>
      </div>
      <div class="settingsGroup stackSection">
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Hostname</span>
            <small>Used for mDNS and device identity.</small>
          </div>
          <div class="settingsValue">
            <input id="hostnameInput" type="text" placeholder="homerelay" maxlength="32" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>MQTT Host</span>
            <small>Broker address or hostname.</small>
          </div>
          <div class="settingsValue">
            <input id="mqttHostInput" type="text" placeholder="192.168.0.10" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>MQTT Port</span>
            <small>Defaults to 1883 if empty.</small>
          </div>
          <div class="settingsValue">
            <input id="mqttPortInput" type="text" placeholder="1883" inputmode="numeric" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Wi-Fi SSID</span>
            <small>Tap a network below to fill this in.</small>
          </div>
          <div class="settingsValue">
            <input id="wifiSsidInput" type="text" placeholder="Wi-Fi network name" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Wi-Fi Password</span>
            <small>Leave empty to keep the saved password.</small>
          </div>
          <div class="settingsValue">
            <input id="wifiPassInput" type="text" placeholder="stored password or new password" />
          </div>
        </div>
        <div class="settingsRow settingsRow--action">
          <button id="btnSaveConfig" class="save">SAVE CONFIG</button>
        </div>
      </div>
      <div class="muted" style="margin-top:10px">mDNS hostnames are saved without the .local suffix, but the UI shows the full name as <span id="mdnsHostInline">n/a</span>.</div>
    </div>

    <div class="card">
      <div class="sectionTitle">Configuration</div>
      <div class="kv"><strong>Device:</strong> <span id="deviceName">n/a</span></div>
      <div class="kv"><strong>Type:</strong> <span id="deviceType">n/a</span></div>
      <div class="kv"><strong>Firmware:</strong> <span id="firmware">n/a</span></div>
      <div class="kv"><strong>mDNS host:</strong> <span id="mdnsHost">n/a</span></div>
      <div class="kv"><strong>MQTT Host:</strong> <span id="mqttHost">n/a</span></div>
      <div class="kv"><strong>MQTT Port:</strong> <span id="mqttPort">n/a</span></div>
      <div class="kv"><strong>Relay pin:</strong> <span id="relayPin">n/a</span></div>
      <div class="kv"><strong>Relay LED pin:</strong> <span id="relayLedPin">n/a</span></div>
      <div class="kv"><strong>Wi-Fi LED pin:</strong> <span id="wifiLedPin">n/a</span></div>
      <div class="kv"><strong>Relay button pin:</strong> <span id="relayButtonPin">n/a</span></div>
      <div class="kv"><strong>Reset button pin:</strong> <span id="resetButtonPin">n/a</span></div>
      <div class="kv"><strong>Hostname default:</strong> <span id="hostnameDefault">n/a</span></div>
      <div class="kv"><strong>NVS:</strong> <span id="nvsHealth">n/a</span></div>
    </div>

    <div class="card">
      <div class="sectionTitle">Wi-Fi</div>
      <div class="row">
        <button id="btnScanWifi" class="refresh">SCAN WI-FI</button>
      </div>
      <ul id="wifiList" class="list"></ul>
    </div>

    <div class="card">
      <div class="kv"><strong>Relay:</strong> <span id="relay">unknown</span></div>
      <div class="kv"><strong>Last command:</strong> <span id="last">none</span></div>
      <div class="kv"><strong>Uptime:</strong> <span id="uptime">n/a</span></div>
      <div class="kv"><strong>Hostname:</strong> <span id="hostname">n/a</span></div>
      <div class="kv"><strong>MQTT Client ID:</strong> <span id="mqttClientId">n/a</span></div>
      <div class="kv"><strong>Error:</strong> <span id="error">none</span></div>
    </div>

    <div class="card">
      <strong>Raw JSON</strong>
      <pre id="raw">(no response yet)</pre>
    </div>
  </div>

  <script>
    const ids = {
      relay: document.getElementById('relay'),
      last: document.getElementById('last'),
      uptime: document.getElementById('uptime'),
      hostname: document.getElementById('hostname'),
      mqttClientId: document.getElementById('mqttClientId'),
      error: document.getElementById('error'),
      relayStateLabel: document.getElementById('relayStateLabel'),
      relayStateBadge: document.getElementById('relayStateBadge'),
      relayStateBadgeText: document.getElementById('relayStateBadgeText'),
      mqttHostInput: document.getElementById('mqttHostInput'),
      mqttPortInput: document.getElementById('mqttPortInput'),
      wifiSsidInput: document.getElementById('wifiSsidInput'),
      wifiPassInput: document.getElementById('wifiPassInput'),
      deviceName: document.getElementById('deviceName'),
      deviceType: document.getElementById('deviceType'),
      firmware: document.getElementById('firmware'),
      mdnsHost: document.getElementById('mdnsHost'),
      mdnsHostInline: document.getElementById('mdnsHostInline'),
      mqttHost: document.getElementById('mqttHost'),
      mqttPort: document.getElementById('mqttPort'),
      relayPin: document.getElementById('relayPin'),
      relayLedPin: document.getElementById('relayLedPin'),
      wifiLedPin: document.getElementById('wifiLedPin'),
      relayButtonPin: document.getElementById('relayButtonPin'),
      resetButtonPin: document.getElementById('resetButtonPin'),
      hostnameDefault: document.getElementById('hostnameDefault'),
      nvsHealth: document.getElementById('nvsHealth'),
      wifiList: document.getElementById('wifiList'),
      raw: document.getElementById('raw'),
      hostnameInput: document.getElementById('hostnameInput'),
      buttons: [
        document.getElementById('btnOn'),
        document.getElementById('btnOff'),
        document.getElementById('btnToggle'),
        document.getElementById('btnRefresh'),
        document.getElementById('btnSaveConfig'),
        document.getElementById('btnScanWifi')
      ]
    };

    function setBusy(busy) {
      ids.buttons.forEach(b => b.disabled = busy);
    }

    function setRelayState(stateText) {
      const normalized = (stateText || 'unknown').toString().toLowerCase();
      const isOn = normalized === 'on';
      const isOff = normalized === 'off';

      ids.relayStateLabel.textContent = normalized;
      ids.relayStateBadge.dataset.state = normalized;
      ids.relayStateBadgeText.textContent = isOn ? 'Relay energized' : (isOff ? 'Relay idle' : 'State unknown');

      ids.buttons.forEach(button => {
        button.classList.remove('is-active', 'is-inactive');
      });

      if (isOn) {
        document.getElementById('btnOn').classList.add('is-active');
        document.getElementById('btnOff').classList.add('is-inactive');
      } else if (isOff) {
        document.getElementById('btnOff').classList.add('is-active');
        document.getElementById('btnOn').classList.add('is-inactive');
      }
    }

    function showJson(obj) {
      ids.raw.textContent = JSON.stringify(obj, null, 2);
      if (obj.relay) {
        ids.relay.textContent = obj.relay;
        setRelayState(obj.relay);
      }
      if (typeof obj.uptime_ms !== 'undefined') ids.uptime.textContent = obj.uptime_ms + ' ms';
      if (obj.hostname) {
        ids.hostname.textContent = obj.hostname;
        ids.hostnameInput.value = obj.hostname;
      }
      if (obj.mqtt_client_id) ids.mqttClientId.textContent = obj.mqtt_client_id;
      if (obj.command) ids.last.textContent = obj.command;
      ids.error.textContent = obj.ok ? 'none' : (obj.error || 'request failed');
    }

    function showConfig(obj) {
      if (obj.hostname) {
        ids.hostnameInput.value = obj.hostname;
      }
      if (obj.device_name) ids.deviceName.textContent = obj.device_name;
      if (obj.device_type) ids.deviceType.textContent = obj.device_type;
      if (obj.firmware) ids.firmware.textContent = obj.firmware.name + ' ' + obj.firmware.version;
      if (obj.mdns_host) ids.mdnsHost.textContent = obj.mdns_host;
      if (obj.mdns_host) ids.mdnsHostInline.textContent = obj.mdns_host;
      if (obj.mqtt_host) ids.mqttHost.textContent = obj.mqtt_host;
      if (typeof obj.mqtt_port !== 'undefined') ids.mqttPort.textContent = obj.mqtt_port;
      if (obj.mqtt_host) ids.mqttHostInput.value = obj.mqtt_host;
      if (typeof obj.mqtt_port !== 'undefined') ids.mqttPortInput.value = obj.mqtt_port;
      if (obj.wifi_ssid) ids.wifiSsidInput.value = obj.wifi_ssid;
      ids.wifiPassInput.placeholder = obj.wifi_password_set ? 'stored password' : 'new password';
      if (typeof obj.relay_pin !== 'undefined') ids.relayPin.textContent = obj.relay_pin;
      if (typeof obj.relay_led_pin !== 'undefined') ids.relayLedPin.textContent = obj.relay_led_pin;
      if (typeof obj.wifi_led_pin !== 'undefined') ids.wifiLedPin.textContent = obj.wifi_led_pin;
      if (typeof obj.relay_button_pin !== 'undefined') ids.relayButtonPin.textContent = obj.relay_button_pin;
      if (typeof obj.reset_button_pin !== 'undefined') ids.resetButtonPin.textContent = obj.reset_button_pin;
      if (obj.hostname_default) ids.hostnameDefault.textContent = obj.hostname_default;
      if (obj.nvs_health) ids.nvsHealth.textContent = obj.nvs_health;
    }

    function renderWifiList(obj) {
      const networks = Array.isArray(obj.networks) ? obj.networks : [];
      if (networks.length === 0) {
        ids.wifiList.innerHTML = '<li>No networks found.</li>';
        return;
      }

      ids.wifiList.innerHTML = networks.map(net => {
        const ssid = net.ssid && net.ssid.length ? net.ssid : '<hidden>';
        return `<li data-ssid="${ssid}"><strong>${ssid}</strong><small>BSSID ${net.bssid || 'n/a'} | RSSI ${net.rssi} dBm | Channel ${net.channel} | Enc ${net.encryption}</small></li>`;
      }).join('');

      ids.wifiList.querySelectorAll('li[data-ssid]').forEach(item => {
        item.style.cursor = 'pointer';
        item.addEventListener('click', () => {
          const ssid = item.getAttribute('data-ssid');
          if (ssid && ssid !== '<hidden>') {
            ids.wifiSsidInput.value = ssid;
          }
        });
      });
    }

    async function parseResponse(res) {
      let data;
      try {
        data = await res.json();
      } catch {
        data = { ok: false, error: 'Invalid JSON response' };
      }

      if (!res.ok) {
        throw data;
      }

      return data;
    }

    async function refreshStatus() {
      setBusy(true);
      try {
        const [status, config] = await Promise.all([
          parseResponse(await fetch('/status')),
          parseResponse(await fetch('/config'))
        ]);
        showJson(status);
        showConfig(config);
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Status request failed' });
      } finally {
        setBusy(false);
      }
    }

    async function sendCommand(path, commandName) {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch(path, { method: 'POST' }));
        showJson(data);

        const status = await parseResponse(await fetch('/status'));
        showJson(status);
        ids.last.textContent = commandName;
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Command request failed' });
      } finally {
        setBusy(false);
      }
    }

    async function scanWifi() {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch('/wifi/scan'));
        showJson(data);
        renderWifiList(data);
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Wi-Fi scan failed' });
      } finally {
        setBusy(false);
      }
    }

    async function applyConfig() {
      const hostname = ids.hostnameInput.value.trim();
      const mqttHost = ids.mqttHostInput.value.trim();
      const mqttPort = ids.mqttPortInput.value.trim();
      const wifiSsid = ids.wifiSsidInput.value.trim();
      const wifiPass = ids.wifiPassInput.value;

      if (!hostname && !mqttHost && !mqttPort && !wifiSsid && !wifiPass.trim()) {
        showJson({ ok: false, error: 'enter at least one setting to save' });
        return;
      }

      setBusy(true);
      try {
        const body = new URLSearchParams();
        if (hostname) body.set('hostname', hostname);
        if (mqttHost) body.set('mqtt_host', mqttHost);
        if (mqttPort) body.set('mqtt_port', mqttPort);
        if (wifiSsid) body.set('wifi_ssid', wifiSsid);
        if (wifiPass.trim()) body.set('wifi_pass', wifiPass);

        const response = await fetch('/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        });

        const data = await parseResponse(response);
        showJson(data);

        const status = await parseResponse(await fetch('/status'));
        const config = await parseResponse(await fetch('/config'));
        showJson(status);
        showConfig(config);
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Configuration save failed' });
      } finally {
        setBusy(false);
      }
    }

    document.getElementById('btnOn').addEventListener('click', () => sendCommand('/on', 'on'));
    document.getElementById('btnOff').addEventListener('click', () => sendCommand('/off', 'off'));
    document.getElementById('btnToggle').addEventListener('click', () => sendCommand('/toggle', 'toggle'));
    document.getElementById('btnRefresh').addEventListener('click', refreshStatus);
    document.getElementById('btnSaveConfig').addEventListener('click', applyConfig);
    document.getElementById('btnScanWifi').addEventListener('click', scanWifi);

    refreshStatus();
  </script>
</body>
</html>
)HTML";
} // namespace

void WebControlServer::configure(const WebControlContext &newContext)
{
    context = newContext;
}

void WebControlServer::beginIfNeeded(bool wifiConnected)
{
    if (started || !wifiConnected)
    {
        return;
    }

    registerRoutes();
    gServer.begin();
    started = true;

    Serial.println("[HTTP] Web control server started on port 80");
}

void WebControlServer::handleClient()
{
    if (!started)
    {
        return;
    }

    gServer.handleClient();
}

bool WebControlServer::isStarted() const
{
    return started;
}

void WebControlServer::registerRoutes()
{
    gServer.on("/", HTTP_GET, [this]()
               { handleRoot(); });
    gServer.on("/on", HTTP_POST, [this]()
               { handleOn(); });
    gServer.on("/off", HTTP_POST, [this]()
               { handleOff(); });
    gServer.on("/toggle", HTTP_POST, [this]()
               { handleToggle(); });
    gServer.on("/status", HTTP_GET, [this]()
               { handleStatus(); });
    gServer.on("/config", HTTP_GET, [this]()
               { handleConfig(); });
    gServer.on("/config", HTTP_POST, [this]()
               { handleConfigSave(); });
    gServer.on("/wifi/scan", HTTP_GET, [this]()
               { handleWifiScan(); });
    gServer.on("/hostname", HTTP_POST, [this]()
               { handleSetHostname(); });
    gServer.onNotFound([this]()
                       { handleNotFound(); });
}

bool WebControlServer::dispatchCommand(const char *command) const
{
    if (context.router == nullptr)
    {
        return false;
    }

    return context.router->dispatch(String(command));
}

const char *WebControlServer::relayStateText() const
{
    if (context.relay == nullptr)
    {
        return "unknown";
    }

    return context.relay->isOn() ? "on" : "off";
}

void WebControlServer::sendError(int statusCode, const char *error)
{
    String json = "{\"ok\":false,\"error\":\"";
    json += error;
    json += "\"}";
    gServer.send(statusCode, "application/json", json);
}

void WebControlServer::handleRoot()
{
    Serial.println("[HTTP] GET /");
    gServer.send_P(200, "text/html", WEB_UI);
}

void WebControlServer::handleOn()
{
    Serial.println("[HTTP] POST /on");

    if (!dispatchCommand("on"))
    {
        sendError(500, "Failed to dispatch on command");
        return;
    }

    String json = "{\"ok\":true,\"command\":\"on\",\"relay\":\"";
    json += relayStateText();
    json += "\"}";
    gServer.send(200, "application/json", json);
}

void WebControlServer::handleOff()
{
    Serial.println("[HTTP] POST /off");

    if (!dispatchCommand("off"))
    {
        sendError(500, "Failed to dispatch off command");
        return;
    }

    String json = "{\"ok\":true,\"command\":\"off\",\"relay\":\"";
    json += relayStateText();
    json += "\"}";
    gServer.send(200, "application/json", json);
}

void WebControlServer::handleToggle()
{
    Serial.println("[HTTP] POST /toggle");

    const char *previous = relayStateText();

    if (!dispatchCommand("toggle"))
    {
        const bool wasOn = context.relay != nullptr ? context.relay->isOn() : false;
        if (!dispatchCommand(wasOn ? "off" : "on"))
        {
            sendError(500, "Failed to dispatch toggle command");
            return;
        }
    }

    const char *current = relayStateText();

    String json = "{\"ok\":true,\"command\":\"toggle\",\"previous_relay\":\"";
    json += previous;
    json += "\",\"relay\":\"";
    json += current;
    json += "\"}";
    gServer.send(200, "application/json", json);
}

void WebControlServer::handleStatus()
{
    Serial.println("[HTTP] GET /status");

    String json = "{\"ok\":true,\"relay\":\"";
    json += relayStateText();
    json += "\",\"hostname\":\"";
    json += context.getHostname != nullptr ? context.getHostname() : String("unknown");
    json += "\",\"mqtt_client_id\":\"";
    json += context.getMqttClientId != nullptr ? context.getMqttClientId() : String("unknown");
    json += "\",\"uptime_ms\":";
    json += millis();
    json += "}";
    gServer.send(200, "application/json", json);
}

void WebControlServer::handleConfig()
{
    Serial.println("[HTTP] GET /config");
    gServer.send(200, "application/json", buildConfigJson(context));
}

void WebControlServer::handleConfigSave()
{
    Serial.println("[HTTP] POST /config");

    bool wifiChanged = false;
  bool settingsSaved = false;

    if (context.wifi != nullptr)
    {
        const String wifiSsid = gServer.arg("wifi_ssid");
        const String wifiPass = gServer.arg("wifi_pass");
        if (wifiSsid.length() > 0)
        {
            context.wifi->setCredentials(wifiSsid, wifiPass);
            wifiChanged = true;
      settingsSaved = true;
        }
    }

    if (context.mqtt != nullptr)
    {
        const String mqttHost = gServer.arg("mqtt_host");
        const String mqttPortText = gServer.arg("mqtt_port");
        if (mqttHost.length() > 0 || mqttPortText.length() > 0)
        {
            const String currentHost = context.mqtt->serverHost();
            const int currentPort = context.mqtt->serverPort();
            const String nextHost = mqttHost.length() > 0 ? mqttHost : currentHost;
            int nextPort = currentPort;
            if (mqttPortText.length() > 0)
            {
                nextPort = mqttPortText.toInt();
            }
            if (nextPort <= 0)
            {
                nextPort = MQTT_PORT;
            }
            context.mqtt->setServer(nextHost, nextPort);
            settingsSaved = true;
        }
    }

    const String requestedHostname = gServer.arg("hostname");
    if (requestedHostname.length() > 0)
    {
        if (context.setHostname == nullptr)
        {
            sendError(500, "Hostname setter is not available");
            return;
        }

        String error;
        if (!context.setHostname(requestedHostname, error))
        {
            sendError(400, error.length() > 0 ? error.c_str() : "Invalid hostname");
            return;
        }

        settingsSaved = true;
    }

    if (wifiChanged && requestedHostname.length() == 0 && context.wifi != nullptr)
    {
        context.wifi->forceReconnect();
    }

      if (!settingsSaved)
      {
        gServer.send(200, "application/json", buildConfigJson(context));
        return;
      }

      String json = buildConfigJson(context);
      if (json.endsWith("}"))
      {
        json.remove(json.length() - 1);
        json += ",\"restarting\":true}";
      }

      gServer.send(200, "application/json", json);
      delay(150);
      ESP.restart();
}

void WebControlServer::handleWifiScan()
{
    Serial.println("[HTTP] GET /wifi/scan");

    if (context.wifi == nullptr)
    {
        sendError(500, "Wi-Fi manager is not available");
        return;
    }

    gServer.send(200, "application/json", context.wifi->scanWifiJson());
}

void WebControlServer::handleSetHostname()
{
    Serial.println("[HTTP] POST /hostname");

    if (context.setHostname == nullptr)
    {
        sendError(500, "Hostname setter is not available");
        return;
    }

    const String requested = gServer.arg("name");
    if (requested.length() == 0)
    {
        sendError(400, "Missing name parameter");
        return;
    }

    String error;
    if (!context.setHostname(requested, error))
    {
        sendError(400, error.length() > 0 ? error.c_str() : "Invalid hostname");
        return;
    }

    String json = "{\"ok\":true,\"command\":\"hostname\",\"hostname\":\"";
    json += context.getHostname != nullptr ? context.getHostname() : String("unknown");
    json += "\",\"mqtt_client_id\":\"";
    json += context.getMqttClientId != nullptr ? context.getMqttClientId() : String("unknown");
    json += "\"}";
    gServer.send(200, "application/json", json);
}

void WebControlServer::handleNotFound()
{
    Serial.print("[HTTP] ");
    Serial.print(gServer.method() == HTTP_POST ? "POST " : "GET ");
    Serial.println(gServer.uri());
    sendError(404, "Not found");
}