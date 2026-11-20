/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  class UInt16 final : public Message {
  public:
    UInt16() = default;
    explicit UInt16(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit UInt16(const Message& message) : Message(message.getMessageData()) {}
    explicit UInt16(Protocol::MessageID id) : Message(createData(Protocol::MessageType::READ_DATA, id, 0)) {}
    UInt16(
      Protocol::MessageID id, uint16_t value,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, id, value)) {}

    uint16_t getUInt() const {
      return getData();
    }

    static UInt16 read(Protocol::MessageID id) {
      return UInt16(id);
    }

    static UInt16 write(Protocol::MessageID id, uint16_t value) {
      return UInt16(id, value);
    }

    UInt16 reply(Protocol::MessageType type, uint16_t value) const {
      return UInt16(createReply(type, value));
    }
  };

}
