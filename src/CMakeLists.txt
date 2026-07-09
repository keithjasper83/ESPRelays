# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Keith Jasper
# Contact: https://github.com/keithjasper83/ESPRelays/issues

# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
	SRCS ${app_sources}
	REQUIRES esp_wifi esp_netif esp_event nvs_flash mdns
)
