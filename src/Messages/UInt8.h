/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // Low-byte u8, e.g. ID71/77/78. New frames set the unused high byte to zero;
  // parsing keeps the raw frame intact and ignores that byte for get().
  class UInt8 final : public Message {
  public:
    UInt8() = default;
    explicit UInt8(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit UInt8(const Message& message) : Message(message.getMessageData()) {}
    explicit UInt8(Protocol::MessageID id) : Message(createData(Protocol::MessageType::READ_DATA, id, 0)) {}
    UInt8(
      Protocol::MessageID id, uint8_t value,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, id, value)) {}

    uint8_t get() const {
      return getLowByte();
    }

    static UInt8 read(Protocol::MessageID id) {
      return UInt8(id);
    }

    static UInt8 write(Protocol::MessageID id, uint8_t value) {
      return UInt8(id, value);
    }

    UInt8 reply(Protocol::MessageType type, uint8_t value) const {
      return UInt8(createReply(type, value));
    }
  };

}
