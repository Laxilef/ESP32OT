/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID15: maximum boiler capacity in kW and minimum modulation in percent.
  class Capacity final : public Message {
  public:
    Capacity() : Message(createData(Protocol::MessageType::READ_DATA, Protocol::MessageID::MaxCapacityMinModLevel, 0)) {}
    explicit Capacity(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit Capacity(const Message& message) : Message(message.getMessageData()) {}
    Capacity(
      uint8_t maxCapacity, uint8_t minModulation,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(type, Protocol::MessageID::MaxCapacityMinModLevel, encode(maxCapacity, minModulation))) {}

    uint8_t getMaxCapacity() const {
      return getHighByte();
    }

    uint8_t getMinModulation() const {
      return getLowByte();
    }

    Capacity reply(Protocol::MessageType type, uint8_t maxCapacity, uint8_t minModulation) const {
      return Capacity(createReply(type, encode(maxCapacity, minModulation)));
    }

  private:
    static uint16_t encode(uint8_t maxCapacity, uint8_t minModulation) {
      return (static_cast<uint16_t>(maxCapacity) << 8) | minModulation;
    }
  };

}
