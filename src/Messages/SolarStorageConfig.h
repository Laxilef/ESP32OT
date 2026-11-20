/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID103: HB is the configuration byte, LB is the member ID. The p.36 table
  // contradicts this layout by calling its system-type bit "LB: bit 0";
  // expose the bytes without assigning that ambiguous bit a semantic meaning.
  class SolarStorageConfig final : public Message {
  public:
    SolarStorageConfig() : Message(createData(
      Protocol::MessageType::READ_DATA,
      Protocol::MessageID::SConfigSMemberIDcodeSolarStorage,
      0
    )) {
    }

    explicit SolarStorageConfig(const Protocol::MessageData& snapshot) : Message(snapshot) {
    }

    explicit SolarStorageConfig(const Message& message) : Message(message.getMessageData()) {
    }

    uint8_t getConfigurationByte() const {
      return getHighByte();
    }

    uint8_t getMemberId() const {
      return getLowByte();
    }

    SolarStorageConfig& setConfigurationByte(uint8_t configuration) {
      setData((getData() & 0x00ff) | (static_cast<uint16_t>(configuration) << 8));
      return *this;
    }

    SolarStorageConfig& setMemberId(uint8_t memberId) {
      setData((getData() & 0xff00) | memberId);
      return *this;
    }

    SolarStorageConfig reply(uint8_t configuration, uint8_t memberId) const {
      return reply(Protocol::MessageType::READ_ACK, configuration, memberId);
    }

    SolarStorageConfig reply(Protocol::MessageType type, uint8_t configuration, uint8_t memberId) const {
      const uint16_t payload = (static_cast<uint16_t>(configuration) << 8) | memberId;
      return SolarStorageConfig(createReply(type, payload));
    }
  };

}
