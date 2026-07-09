# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Keith Jasper
# Contact: https://github.com/keithjasper83/ESPRelays/issues

import json
import socket
import struct

MCAST_GRP = "239.255.42.99"
MCAST_PORT = 42424


def main() -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        sock.bind(("", MCAST_PORT))
    except OSError:
        sock.bind((MCAST_GRP, MCAST_PORT))

    mreq = struct.pack("=4sl", socket.inet_aton(MCAST_GRP), socket.INADDR_ANY)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    print(f"Listening for ESP discovery packets on {MCAST_GRP}:{MCAST_PORT}...")

    while True:
        data, addr = sock.recvfrom(8192)
        text = data.decode("utf-8", errors="replace")

        try:
            packet = json.loads(text)
        except json.JSONDecodeError:
            print(f"\nMalformed packet from {addr[0]}:{addr[1]}")
            print(text)
            continue

        device_name = packet.get("device_name", "unknown")
        device_type = packet.get("device_type", "unknown")
        network = packet.get("network", {})
        state = packet.get("state", {})
        capabilities = packet.get("capabilities", [])

        print("\nFound device:")
        print(f"  Name: {device_name}")
        print(f"  Type: {device_type}")
        print(f"  IP: {network.get('ip', addr[0])}")
        print(f"  URL: {network.get('base_url', 'n/a')}")
        print(f"  Capabilities: {', '.join(capabilities) if capabilities else 'none'}")

        if state:
            state_lines = ", ".join(f"{k}={v}" for k, v in state.items())
            print(f"  State: {state_lines}")


if __name__ == "__main__":
    main()
