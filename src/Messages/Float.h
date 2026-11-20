/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // OpenTherm signed f8.8 payload exposed as a float value.
  class Float final : public Message {
  public:
    Float() = default;
    explicit Float(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit Float(const Message& message) : Message(message.getMessageData()) {}
    explicit Float(Protocol::MessageID id) : Message(createData(Protocol::MessageType::READ_DATA, id, 0)) {}
    Float(
      Protocol::MessageID id, float value,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, id, encode(value))) {}

    float get() const;

    static Float read(Protocol::MessageID id);
    static Float write(Protocol::MessageID id, float value);
    Float reply(Protocol::MessageType type, float value) const;

    // Preserve the old boiler/DHW helper's 0..100 clamp only when requested.
    static Float writeTemperature(Protocol::MessageID id, float temperature);
    static uint16_t encode(float value);
  };

}
