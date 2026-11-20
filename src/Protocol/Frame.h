/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Types.h"

namespace ESP32OT::Protocol {

  class Frame {
  public:
    static uint32_t build(MessageType type, MessageID id, uint16_t data = 0);
    static bool parity(uint32_t frame);
    static bool isValidRequest(uint32_t frame);
    static bool isValidResponse(uint32_t frame);

    static MessageType getType(uint32_t frame) {
      return static_cast<MessageType>((frame >> 28) & 7);
    }

    static MessageID getId(uint32_t frame) {
      return static_cast<MessageID>((frame >> 16) & 0xff);
    }

    static uint16_t getData(uint32_t frame) {
      return static_cast<uint16_t>(frame);
    }

    static const char* typeToString(MessageType type);
    static const char* statusToString(OperationStatus status);
  };

}
