/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID87 uses HB for the nominal ventilation percentage; LB is reserved.
  class NominalVentilation final : public Message {
  public:
    NominalVentilation() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::NominalVentilationValue, 0
    )) {}
    explicit NominalVentilation(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit NominalVentilation(const Message& message) : Message(message.getMessageData()) {}
    explicit NominalVentilation(
      uint8_t value, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, Protocol::MessageID::NominalVentilationValue, static_cast<uint16_t>(value) << 8)) {}

    uint8_t get() const {
      return getHighByte();
    }

    NominalVentilation reply(Protocol::MessageType type, uint8_t value) const {
      return NominalVentilation(createReply(type, static_cast<uint16_t>(value) << 8));
    }
  };

}
