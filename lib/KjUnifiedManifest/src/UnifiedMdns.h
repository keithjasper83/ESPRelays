// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "UnifiedManifest.h"
#ifdef ARDUINO_ARCH_ESP32
#include <esp_mac.h>
#endif
namespace kjunified {
// Called after the application's MDNS.begin; never owns or restarts its responder.
template<typename Mdns> void advertise(Mdns &mdns, uint16_t port) {
    mdns.addService("kj-esp", "tcp", port);
    mdns.addServiceTxt("kj-esp", "tcp", "protocol", Protocol);
    mdns.addServiceTxt("kj-esp", "tcp", "version", "1");
    mdns.addServiceTxt("kj-esp", "tcp", "path", ManifestPath);
}
#ifdef ARDUINO_ARCH_ESP32
inline Text factoryHardwareId() {
    uint8_t mac[6];
    if (esp_efuse_mac_get_default(mac) != ESP_OK) return "";
    return hardwareId(mac);
}
#endif
}
