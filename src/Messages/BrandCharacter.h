/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID93/94/95: request HB selects a character; reply HB carries the maximum index.
  class BrandCharacter final : public Message {
  public:
    BrandCharacter() = default;
    explicit BrandCharacter(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit BrandCharacter(const Message& message) : Message(message.getMessageData()) {}
    explicit BrandCharacter(Protocol::MessageID id, uint8_t index = 0) : Message(createData(
      Protocol::MessageType::READ_DATA, id, static_cast<uint16_t>(index) << 8
    )) {}

    uint8_t getRequestIndex() const {
      return Protocol::Frame::getData(getRequest()) >> 8;
    }

    uint8_t getMaxIndex() const {
      return getHighByte();
    }

    char getCharacter() const {
      return static_cast<char>(getLowByte());
    }

    BrandCharacter reply(Protocol::MessageType type, uint8_t maxIndex, char character) const {
      return BrandCharacter(createReply(
        type, (static_cast<uint16_t>(maxIndex) << 8) | static_cast<uint8_t>(character)
      ));
    }
  };

}
