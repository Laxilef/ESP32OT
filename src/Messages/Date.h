/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID21: month in HB, day of month in LB (specification, page 39).
  class Date final : public Message {
  public:
    Date() : Message(createData(Protocol::MessageType::READ_DATA, Protocol::MessageID::Date, 0)) {}
    explicit Date(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit Date(const Message& message) : Message(message.getMessageData()) {}
    Date(
      uint8_t month, uint8_t day,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, Protocol::MessageID::Date, encode(month, day))) {}

    uint8_t getMonth() const {
      return getHighByte();
    }

    uint8_t getDay() const {
      return getLowByte();
    }

    Date reply(Protocol::MessageType type, uint8_t month, uint8_t day) const {
      return Date(createReply(type, encode(month, day)));
    }

  private:
    static uint16_t encode(uint8_t month, uint8_t day) {
      return (static_cast<uint16_t>(month) << 8) | day;
    }
  };

}
