/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  class Int16 final : public Message {
  public:
    Int16() = default;
    explicit Int16(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit Int16(const Message& message) : Message(message.getMessageData()) {}
    explicit Int16(Protocol::MessageID id) : Message(createData(Protocol::MessageType::READ_DATA, id, 0)) {}
    Int16(
      Protocol::MessageID id, int16_t value,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, id, static_cast<uint16_t>(value))) {}

    int16_t getInt() const {
      const uint16_t value = getData();
      return value & 0x8000 ? static_cast<int16_t>(static_cast<int32_t>(value) - 0x10000) : value;
    }

    static Int16 read(Protocol::MessageID id) {
      return Int16(id);
    }

    static Int16 write(Protocol::MessageID id, int16_t value) {
      return Int16(id, value);
    }

    Int16 reply(Protocol::MessageType type, int16_t value) const {
      return Int16(createReply(type, static_cast<uint16_t>(value)));
    }
  };

}
