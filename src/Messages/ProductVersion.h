/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID76/104/126/127: HB is product type, LB is version (spec p.37 and OTGateway).
  class ProductVersion final : public Message {
  public:
    ProductVersion() = default;
    explicit ProductVersion(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit ProductVersion(const Message& message) : Message(message.getMessageData()) {}
    explicit ProductVersion(Protocol::MessageID id) : Message(createData(Protocol::MessageType::READ_DATA, id, 0)) {}
    ProductVersion(
      Protocol::MessageID id, uint8_t productType, uint8_t version,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, id, encode(productType, version))) {}

    uint8_t getProductType() const {
      return getHighByte();
    }

    uint8_t getVersion() const {
      return getLowByte();
    }

    ProductVersion reply(Protocol::MessageType type, uint8_t productType, uint8_t version) const {
      return ProductVersion(createReply(type, encode(productType, version)));
    }

  private:
    static uint16_t encode(uint8_t productType, uint8_t version) {
      return (static_cast<uint16_t>(productType) << 8) | version;
    }
  };

}
