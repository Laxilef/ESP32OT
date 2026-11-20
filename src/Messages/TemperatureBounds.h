/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID48/49 share the same upper/lower signed-byte temperature bounds in degrees C.
  class TemperatureBounds final : public Message {
  public:
    TemperatureBounds() = default;
    explicit TemperatureBounds(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit TemperatureBounds(const Message& message) : Message(message.getMessageData()) {}
    explicit TemperatureBounds(Protocol::MessageID id) : Message(createData(Protocol::MessageType::READ_DATA, id, 0)) {}
    TemperatureBounds(
      Protocol::MessageID id, int8_t upper, int8_t lower,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(type, id, encode(upper, lower))) {}

    int8_t getUpper() const {
      return decode(getHighByte());
    }

    int8_t getLower() const {
      return decode(getLowByte());
    }

    TemperatureBounds reply(Protocol::MessageType type, int8_t upper, int8_t lower) const {
      return TemperatureBounds(createReply(type, encode(upper, lower)));
    }

  private:
    static uint16_t encode(int8_t upper, int8_t lower) {
      return (static_cast<uint16_t>(static_cast<uint8_t>(upper)) << 8) | static_cast<uint8_t>(lower);
    }

    static int8_t decode(uint8_t value) {
      return value < 0x80 ? value : static_cast<int16_t>(value) - 0x100;
    }
  };

}
