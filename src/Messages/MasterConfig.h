/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID2: master capabilities in HB, member ID in LB (specification, p.36).
  class MasterConfig final : public Message {
  public:
    MasterConfig() : MasterConfig(0, 0) {}
    explicit MasterConfig(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit MasterConfig(const Message& message) : Message(message.getMessageData()) {}
    MasterConfig(
      uint8_t flags, uint8_t memberId,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, Protocol::MessageID::MConfigMMemberIDcode, encode(flags, memberId))) {}

    bool isSmartPowerImplemented() const {
      return getData() & 0x0100;
    }

    uint8_t getMemberId() const {
      return getLowByte();
    }

    MasterConfig& setSmartPowerImplemented(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    MasterConfig& setMemberId(uint8_t memberId) {
      setData((getData() & 0xff00) | memberId);
      return *this;
    }

    MasterConfig reply(Protocol::MessageType type, uint8_t flags, uint8_t memberId) const {
      return MasterConfig(createReply(type, encode(flags, memberId)));
    }

    MasterConfig reply(uint8_t flags, uint8_t memberId) const {
      return reply(Protocol::MessageType::WRITE_ACK, flags, memberId);
    }

  private:
    static uint16_t encode(uint8_t flags, uint8_t memberId) {
      return static_cast<uint16_t>((static_cast<uint32_t>(flags) << 8) | memberId);
    }
  };

}
