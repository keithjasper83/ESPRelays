#include <unity.h>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>
#include <UnifiedManifest.h>
#include <UnifiedMdns.h>
#include <ManifestHttp.h>
#include "RelayManifest.h"

using namespace kjunified;
void setUp() {}
void tearDown() {}

Manifest fixture()
{
    Manifest m;
    std::ifstream f("test/fixtures/relay-manifest-v1.json");
    TEST_ASSERT_TRUE(f.good());
    TEST_ASSERT_FALSE(deserializeJson(m.document, f));
    return m;
}
JsonObject node(Manifest &m) { return m.document["capabilities"][0]["children"][0].as<JsonObject>(); }

void test_fixture_roundtrip_and_recursive_identity()
{
    auto m = fixture();
    std::string body, error;
    TEST_ASSERT_TRUE_MESSAGE(m.serialize(body, error), error.c_str());
    JsonDocument parsed;
    TEST_ASSERT_FALSE(deserializeJson(parsed, body));
    TEST_ASSERT_EQUAL_STRING("esp32-10003bba8d4c", parsed["hardware"]["id"]);
    auto relay = node(m);
    relay["label"] = "Renamed \"relay\"\n\t";
    auto children = relay["children"].to<JsonArray>();
    m.add(children, "channel", "vendor.future", "Channel", "read", true)["state"] = 4;
    TEST_ASSERT_TRUE(m.serialize(body, error));
    TEST_ASSERT_FALSE(deserializeJson(parsed, body));
    TEST_ASSERT_EQUAL_STRING("relay", parsed["capabilities"][0]["children"][0]["key"]);
    TEST_ASSERT_EQUAL_STRING("Renamed \"relay\"\n\t", parsed["capabilities"][0]["children"][0]["label"]);
    TEST_ASSERT_EQUAL_STRING("channel", parsed["capabilities"][0]["children"][0]["children"][0]["key"]);
}

void test_rejects_duplicate_keys_bad_bounds_and_nonfinite_state()
{
    std::string body, error;
    auto m = fixture();
    node(m)["key"] = "outputs";
    TEST_ASSERT_FALSE(m.serialize(body, error));
    TEST_ASSERT_TRUE(body.empty());
    node(m)["key"] = "relay";
    node(m)["min"] = 2;
    node(m)["max"] = 1;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    node(m)["max"] = 3;
    node(m)["step"] = 0;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    node(m)["step"] = 1;
    node(m)["state"] = INFINITY;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    node(m)["state"] = nullptr;
    TEST_ASSERT_TRUE(m.serialize(body, error));
}

void test_enforces_tree_and_document_limits()
{
    std::string body, error;
    auto m = fixture();
    auto children = node(m)["children"].to<JsonArray>();
    for (int depth = 3; depth <= 8; ++depth)
        children = m.add(children, "level" + std::to_string(depth), "group", "Level", "read", true)["children"].to<JsonArray>();
    TEST_ASSERT_TRUE(m.serialize(body, error));
    m.add(children, "too-deep", "group", "Too deep", "read", true);
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m = fixture();
    auto roots = m.document["capabilities"].to<JsonArray>();
    for (int i = 0; i < 128; ++i)
        m.add(roots, "item" + std::to_string(i), "value", "Item", "read", true);
    TEST_ASSERT_TRUE(m.serialize(body, error));
    m.add(roots, "overflow", "value", "Overflow", "read", true);
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m = fixture();
    node(m)["key"] = std::string(129, 'k');
    TEST_ASSERT_FALSE(m.serialize(body, error));
    node(m)["key"] = "relay";
    node(m)["metadata"]["large"] = std::string(65536, 'a');
    TEST_ASSERT_FALSE(m.serialize(body, error));
}

void test_factory_identity_ignores_hostname()
{
    uint8_t mac[] = {0x10, 0x00, 0x3b, 0xba, 0x8d, 0x4c};
    TEST_ASSERT_EQUAL_STRING("esp32-10003bba8d4c", hardwareId(mac).c_str());
}

struct MdnsRecorder {
    std::vector<std::string> calls;
    void addService(const char *service, const char *proto, uint16_t port) {
        calls.push_back(std::string(service) + ":" + proto + ":" + std::to_string(port));
    }
    void addServiceTxt(const char *service, const char *proto, const char *key, const char *value) {
        calls.push_back(std::string(service) + ":" + proto + ":" + key + "=" + value);
    }
};
void test_mdns_advertises_contract_on_http_port()
{
    MdnsRecorder mdns;
    advertise(mdns, 80);
    TEST_ASSERT_EQUAL(4, mdns.calls.size());
    TEST_ASSERT_EQUAL_STRING("kj-esp:tcp:80", mdns.calls[0].c_str());
    TEST_ASSERT_EQUAL_STRING("kj-esp:tcp:protocol=kj-esp-unified", mdns.calls[1].c_str());
    TEST_ASSERT_EQUAL_STRING("kj-esp:tcp:version=1", mdns.calls[2].c_str());
    TEST_ASSERT_EQUAL_STRING("kj-esp:tcp:path=/unified/manifest", mdns.calls[3].c_str());
}
JsonObjectConst findNode(JsonArrayConst nodes, const char *key)
{
    for (JsonObjectConst n : nodes) {
        if (n["key"] == key) return n;
        auto found = findNode(n["children"].as<JsonArrayConst>(), key);
        if (!found.isNull()) return found;
    }
    return JsonObjectConst();
}
void test_relay_snapshot_preserves_absence_and_refreshes_state()
{
    RelayManifestSnapshot s;
    s.hardwareId = "esp32-10003bba8d4c";
    s.legacyDeviceId = "esp32-water-10003bba8d4c";
    s.name = "water";
    s.firmwareVersion = "4.2.0";
    s.firmwareReleaseDate = "unreleased";
    s.temperatureC = 0; // cached value cannot make an absent probe available
    auto m = buildRelayManifest(s);
    auto roots = m.document["capabilities"].as<JsonArrayConst>();
    TEST_ASSERT_FALSE(findNode(roots, "temperature")["available"].as<bool>());
    TEST_ASSERT_TRUE(findNode(roots, "temperature")["state"].isNull());
    TEST_ASSERT_TRUE(findNode(roots, "led-strip").isNull());
    const std::string revision = m.document["manifest_revision"].as<std::string>();
    s.probePresent = true;
    s.monitoringEnabled = true;
    s.calibrationReady = true;
    s.lowValid = true;
    s.highValid = true;
    s.lowRaw = 1000;
    s.highRaw = 3000;
    s.lowC = 0;
    s.highC = 100;
    s.rawTemperature = 1430;
    s.temperatureC = 21.5;
    s.relayOn = true;
    m = buildRelayManifest(s);
    roots = m.document["capabilities"].as<JsonArrayConst>();
    TEST_ASSERT_TRUE(findNode(roots, "temperature")["available"].as<bool>());
    TEST_ASSERT_EQUAL_FLOAT(21.5, findNode(roots, "temperature")["state"].as<float>());
    TEST_ASSERT_TRUE(findNode(roots, "relay")["state"].as<bool>());
    TEST_ASSERT_EQUAL(10080, findNode(roots, "relay-auto-off")["max"].as<int>());
    TEST_ASSERT_EQUAL_STRING(revision.c_str(), m.document["manifest_revision"]);
    std::string body, error;
    TEST_ASSERT_TRUE_MESSAGE(m.serialize(body, error), error.c_str());
    std::ofstream output(".pio/relay-manifest-output.json");
    output << body;
    s.calibrationReady = false;
    m = buildRelayManifest(s);
    TEST_ASSERT_TRUE(findNode(m.document["capabilities"], "temperature")["state"].isNull());
    s.calibrationReady = true;
    s.temperatureC = NAN;
    m = buildRelayManifest(s);
    TEST_ASSERT_TRUE(findNode(m.document["capabilities"], "temperature")["state"].isNull());
    TEST_ASSERT_TRUE(m.serialize(body, error));
}
void test_metadata_nesting_counts_containers_and_rejects_nonfinite()
{
    std::string body, error;
    auto m = fixture();
    auto metadata = m.document["metadata"].to<JsonObject>(); // root level 1, metadata level 2
    for (int level = 3; level <= 32; ++level) metadata = metadata["nested"].to<JsonObject>();
    metadata["value"] = 1;
    TEST_ASSERT_TRUE(m.serialize(body, error));
    metadata["nested"].to<JsonObject>();
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m = fixture();
    m.document["metadata"]["number"] = NAN;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m = fixture();
    m.document["protocol_version"] = true;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m.document["protocol_version"] = 2;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m.document["protocol_version"] = 1;
    node(m)["commands"][0] = 7;
    TEST_ASSERT_FALSE(m.serialize(body, error));
}

void test_rejects_malformed_hardware_identity()
{
    std::string body, error;
    auto m = fixture();
    m.document["hardware"]["id"] = "esp32-10003BBA8D4C";
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m.document["hardware"]["id"] = "mutable-hostname";
    TEST_ASSERT_FALSE(m.serialize(body, error));
}

void test_rejects_oversized_capability_type()
{
    std::string body, error;
    auto m = fixture();
    node(m)["type"] = std::string(129, 't');
    TEST_ASSERT_FALSE(m.serialize(body, error));
    node(m)["type"] = std::string(128, 't');
    TEST_ASSERT_TRUE(m.serialize(body, error));
}
void test_capability_metadata_must_be_object()
{
    std::string body, error;
    auto m = fixture();
    node(m)["metadata"] = 42;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    node(m)["metadata"].to<JsonObject>();
    TEST_ASSERT_TRUE(m.serialize(body, error));
}
void test_semantic_version_one_accepts_integer_valued_json_number()
{
    std::string body, error;
    auto m = fixture();
    m.document["protocol_version"] = 1.0;
    m.document["schema_version"] = 1.0;
    TEST_ASSERT_TRUE(m.serialize(body, error));
    m.document["protocol_version"] = true;
    TEST_ASSERT_FALSE(m.serialize(body, error));
    m.document["protocol_version"] = 1.5;
    TEST_ASSERT_FALSE(m.serialize(body, error));
}

struct ResponseRecorder {
    int status = 0;
    std::string body, contentType, cacheControl;
    void sendHeader(const char *name, const char *value) {
        if (std::string(name) == "Cache-Control") cacheControl = value;
    }
    void send(int code, const char *type, const std::string &value) {
        status = code; contentType = type; body = value;
    }
};
void test_http_refreshes_snapshot_without_command_or_enrollment_access()
{
    ResponseRecorder server;
    int reads = 0;
    auto getter = [&reads]() { return std::string(++reads == 1 ? "{\"state\":false}" : "{\"state\":true}"); };
    serveManifest(server, getter);
    TEST_ASSERT_EQUAL(200, server.status);
    TEST_ASSERT_EQUAL_STRING("application/json", server.contentType.c_str());
    TEST_ASSERT_EQUAL_STRING("no-store", server.cacheControl.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"state\":false}", server.body.c_str());
    serveManifest(server, getter);
    TEST_ASSERT_EQUAL(2, reads);
    TEST_ASSERT_EQUAL_STRING("{\"state\":true}", server.body.c_str());
    serveManifest(server, []() { return std::string(); });
    TEST_ASSERT_EQUAL(503, server.status);
}

void test_retired_features_are_absent_without_rekeying_survivors()
{
    RelayManifestSnapshot snapshot;
    snapshot.hardwareId = "esp32-10003bba8d4c";
    snapshot.legacyDeviceId = "esp32-water-10003bba8d4c";
    auto manifest = buildRelayManifest(snapshot);
    auto &decoded = manifest.document;
    TEST_ASSERT_EQUAL_STRING("relay-3", decoded["manifest_revision"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("esp32-10003bba8d4c", decoded["hardware"]["id"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("esp32-water-10003bba8d4c", decoded["legacy_device_id"].as<const char *>());
    auto roots = decoded["capabilities"].as<JsonArrayConst>();
    for (const char *key : {"indicators", "led-relay", "led-wifi", "led-strip", "mqtt-connected"})
        TEST_ASSERT_TRUE_MESSAGE(findNode(roots, key).isNull(), key);
    for (const char *key : {"relay", "temperature", "temperature-calibration", "relay-auto-off",
                           "temperature-monitoring", "temperature-trim", "wifi-connected", "time-valid", "restart"})
        TEST_ASSERT_FALSE_MESSAGE(findNode(roots, key).isNull(), key);
    TEST_ASSERT_EQUAL_STRING("relay", roots[0]["children"][0]["key"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("temperature-calibration", findNode(roots, "temperature")["children"][0]["key"].as<const char *>());
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_fixture_roundtrip_and_recursive_identity);
    RUN_TEST(test_rejects_duplicate_keys_bad_bounds_and_nonfinite_state);
    RUN_TEST(test_enforces_tree_and_document_limits);
    RUN_TEST(test_factory_identity_ignores_hostname);
    RUN_TEST(test_mdns_advertises_contract_on_http_port);
    RUN_TEST(test_retired_features_are_absent_without_rekeying_survivors);
    RUN_TEST(test_relay_snapshot_preserves_absence_and_refreshes_state);
    RUN_TEST(test_metadata_nesting_counts_containers_and_rejects_nonfinite);
    RUN_TEST(test_http_refreshes_snapshot_without_command_or_enrollment_access);
    RUN_TEST(test_rejects_malformed_hardware_identity);
    RUN_TEST(test_rejects_oversized_capability_type);
    RUN_TEST(test_capability_metadata_must_be_object);
    RUN_TEST(test_semantic_version_one_accepts_integer_valued_json_number);
    return UNITY_END();
}
