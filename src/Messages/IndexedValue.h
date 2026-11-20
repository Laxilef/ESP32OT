/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // TSP/fault-history entries: ID11/13/89/91/106/108; replies echo the requested index.
  class IndexedValue final : public Message {
  public:
    IndexedValue() = default;
    explicit IndexedValue(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit IndexedValue(const Message& message) : Message(message.getMessageData()) {}
    IndexedValue(Protocol::MessageID id, uint8_t index) : Message(createData(
      Protocol::MessageType::READ_DATA, id, encode(index, 0)
    )) {}
    IndexedValue(
      Protocol::MessageID id, uint8_t index, uint8_t value,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, id, encode(index, value))) {}

    uint8_t getIndex() const {
      return getHighByte();
    }

    uint8_t getValue() const {
      return getLowByte();
    }

    IndexedValue reply(Protocol::MessageType type, uint8_t value) const {
      return IndexedValue(createReply(type, encode(getIndex(), value)));
    }

  private:
    static uint16_t encode(uint8_t index, uint8_t value) {
      return (static_cast<uint16_t>(index) << 8) | value;
    }
  };

}
