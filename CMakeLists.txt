# ESP32OT
# Copyright (c) 2026 Laxilef
# SPDX-License-Identifier: GPL-3.0-only
# https://github.com/Laxilef/ESP32OT

idf_component_register(
  SRCS
    "src/ESP32OT.cpp"
    "src/Transport/RmtTransport.cpp"
    "src/Transport/RmtDriver.cpp"
    "src/Protocol/Frame.cpp"
    "src/Messages/Float.cpp"
    "src/Messages/Status.cpp"
  INCLUDE_DIRS "src"
  REQUIRES arduino esp_driver_gpio esp_driver_rmt esp_timer freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
