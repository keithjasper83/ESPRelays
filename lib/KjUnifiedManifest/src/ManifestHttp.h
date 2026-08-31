// SPDX-License-Identifier: Apache-2.0
#pragma once
namespace kjunified {
// Only a snapshot getter is accepted. No command dispatcher or enrollment callback.
template<typename Server, typename Getter>
void serveManifest(Server &server, Getter getSnapshot) {
    const auto body = getSnapshot();
    server.sendHeader("Cache-Control", "no-store");
    if (body.length() == 0) {
        server.send(503, "application/json", "{\"error\":\"Capability manifest is unavailable\"}");
        return;
    }
    server.send(200, "application/json", body);
}
}
