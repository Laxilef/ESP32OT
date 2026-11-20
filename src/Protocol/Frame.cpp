/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include "Frame.h"

namespace ESP32OT::Protocol {

  bool Frame::parity(uint32_t frame) {
    uint8_t count = 0;
    while (frame != 0) {
      count += frame & 1;
      frame >>= 1;
    }

    return count & 1;
  }

  uint32_t Frame::build(MessageType type, MessageID id, uint16_t data) {
    uint32_t frame = data | (static_cast<uint32_t>(id) << 16)
      | (static_cast<uint32_t>(type) << 28);
    if (parity(frame)) {
      frame |= 1UL << 31;
    }

    return frame;
  }

  bool Frame::isValidRequest(uint32_t frame) {
    const MessageType type = getType(frame);
    return !parity(frame) && (type == MessageType::READ_DATA || type == MessageType::WRITE_DATA);
  }

  bool Frame::isValidResponse(uint32_t frame) {
    // Negative replies are valid protocol frames too; they do not contain a
    // successful data value. The transport exposes their own terminal status.
    const MessageType type = getType(frame);
    return !parity(frame) && (type == MessageType::READ_ACK || type == MessageType::WRITE_ACK
      || type == MessageType::DATA_INVALID || type == MessageType::UNKNOWN_DATA_ID);
  }

  const char* Frame::typeToString(MessageType type) {
    static const char* const names[] = {
      "READ_DATA", "WRITE_DATA", "INVALID_DATA", "RESERVED",
      "READ_ACK", "WRITE_ACK", "DATA_INVALID", "UNKNOWN_DATA_ID"
    };
    const uint8_t index = static_cast<uint8_t>(type);
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "UNKNOWN";
  }

  const char* Frame::statusToString(OperationStatus status) {
    static const char* const names[] = {
      "NONE", "QUEUED", "TRANSMITTING", "WAITING", "REQUEST_RECEIVED",
      "SUCCESS", "INVALID", "TIMEOUT", "ERROR", "CANCELLED", "DATA_INVALID", "UNKNOWN_DATA_ID"
    };
    const uint8_t index = static_cast<uint8_t>(status);
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "UNKNOWN";
  }

}
