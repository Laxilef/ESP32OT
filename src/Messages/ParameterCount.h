/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // TSP count or fault-history size: ID10/12/88/90/105/107, HB=count, LB=reserved.
  class ParameterCount final : public Message {
  public:
    ParameterCount() = default;
    explicit ParameterCount(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit ParameterCount(const Message& message) : Message(message.getMessageData()) {}
    explicit ParameterCount(Protocol::MessageID id) : Message(createData(Protocol::MessageType::READ_DATA, id, 0)) {}
    ParameterCount(
      Protocol::MessageID id, uint8_t count,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(type, id, static_cast<uint16_t>(count) << 8)) {}

    uint8_t getCount() const {
      return getHighByte();
    }

    ParameterCount reply(Protocol::MessageType type, uint8_t count) const {
      return ParameterCount(createReply(type, static_cast<uint16_t>(count) << 8));
    }
  };

}
