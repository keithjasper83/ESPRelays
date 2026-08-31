# SPDX-License-Identifier: Apache-2.0
"""Reject an actual OTA image that cannot fit the built partition table."""
import struct
from pathlib import Path


def validate_image(image, partition_table):
    sizes = []
    for offset in range(0, len(partition_table), 32):
        entry = partition_table[offset : offset + 32]
        if len(entry) != 32:
            raise ValueError("Truncated ESP32 partition table")
        magic, kind, subtype, address, size, label, flags = struct.unpack("<HBBII16sI", entry)
        if magic in (0xEBEB, 0xFFFF):  # generated MD5 footer / erased padding
            break
        if magic != 0x50AA:
            raise ValueError("Invalid ESP32 partition entry")
        if kind == 0:
            if size == 0:
                raise ValueError("Empty application partition")
            sizes.append(size)
    if not sizes:
        raise ValueError("No application partitions found")
    maximum = min(sizes)
    if len(image) > maximum:
        raise ValueError(f"Actual firmware image {len(image)} bytes exceeds application partition {maximum} bytes")
    return maximum - len(image)


def check_build_image(source, target, env):
    image_path = Path(env.subst("$BUILD_DIR/${PROGNAME}.bin"))
    table_path = Path(env.subst("$BUILD_DIR/partitions.bin"))
    headroom = validate_image(image_path.read_bytes(), table_path.read_bytes())
    print(f"Actual OTA image: {image_path.stat().st_size} bytes; smallest application slot headroom: {headroom} bytes")


if "Import" in globals():
    Import("env")
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_build_image)
