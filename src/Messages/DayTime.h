/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID20: day of week 1=Monday..7=Sunday, 0=unknown (specification, page 39).
  class DayTime final : public Message {
  public:
    DayTime() : Message(createData(Protocol::MessageType::READ_DATA, Protocol::MessageID::DayTime, 0)) {}
    explicit DayTime(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit DayTime(const Message& message) : Message(message.getMessageData()) {}
    DayTime(
      uint8_t dayOfWeek, uint8_t hours, uint8_t minutes,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, Protocol::MessageID::DayTime, encode(dayOfWeek, hours, minutes))) {}

    uint8_t getDayOfWeek() const {
      return (getHighByte() >> 5) & 0x07;
    }

    uint8_t getHours() const {
      return getHighByte() & 0x1f;
    }

    uint8_t getMinutes() const {
      return getLowByte();
    }

    DayTime reply(Protocol::MessageType type, uint8_t dayOfWeek, uint8_t hours, uint8_t minutes) const {
      return DayTime(createReply(type, encode(dayOfWeek, hours, minutes)));
    }

  private:
    static uint16_t encode(uint8_t dayOfWeek, uint8_t hours, uint8_t minutes) {
      return (static_cast<uint16_t>(dayOfWeek & 0x07) << 13)
        | (static_cast<uint16_t>(hours & 0x1f) << 8)
        | minutes;
    }
  };

}
