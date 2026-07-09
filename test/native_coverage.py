# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Keith Jasper
# Contact: https://github.com/keithjasper83/ESPRelays/issues

Import("env")

if env["PIOENV"] == "native":
    env.Append(
        LINKFLAGS=["--coverage"],
        LIBS=["gcov"],
    )