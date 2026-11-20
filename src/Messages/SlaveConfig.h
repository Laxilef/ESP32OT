/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID3: slave configuration in HB, member ID in LB (specification, p.36).
  class SlaveConfig final : public Message {
  public:
    SlaveConfig() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::SConfigSMemberIDcode, 0
    )) {}

    explicit SlaveConfig(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit SlaveConfig(const Message& message) : Message(message.getMessageData()) {}
    SlaveConfig(
      uint8_t flags, uint8_t memberId,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(type, Protocol::MessageID::SConfigSMemberIDcode, encode(flags, memberId))) {}

    bool isDhwPresent() const {
      return getData() & 0x0100;
    }

    // A clear bit means modulating control; a set bit means on/off control.
    bool isOnOffControl() const {
      return getData() & 0x0200;
    }

    bool isCoolingSupported() const {
      return getData() & 0x0400;
    }

    // A clear bit means instantaneous DHW or an unspecified configuration.
    bool hasDhwStorageTank() const {
      return getData() & 0x0800;
    }

    bool isMasterLowOffPumpControlAllowed() const {
      return !(getData() & 0x1000);
    }

    bool isCh2Present() const {
      return getData() & 0x2000;
    }

    // Clear means available OR unknown, including protocol versions <=2.2.
    bool isRemoteWaterFillingUnavailable() const {
      return getData() & 0x4000;
    }

    bool isHeatCoolModeControlledBySlave() const {
      return getData() & 0x8000;
    }

    uint8_t getMemberId() const {
      return getLowByte();
    }

    SlaveConfig& setDhwPresent(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    SlaveConfig& setOnOffControl(bool value) {
      setFlag(0x0200, value);
      return *this;
    }

    SlaveConfig& setCoolingSupported(bool value) {
      setFlag(0x0400, value);
      return *this;
    }

    SlaveConfig& setDhwStorageTank(bool value) {
      setFlag(0x0800, value);
      return *this;
    }

    SlaveConfig& setMasterLowOffPumpControlAllowed(bool value) {
      setFlag(0x1000, !value);
      return *this;
    }

    SlaveConfig& setCh2Present(bool value) {
      setFlag(0x2000, value);
      return *this;
    }

    SlaveConfig& setRemoteWaterFillingUnavailable(bool value) {
      setFlag(0x4000, value);
      return *this;
    }

    SlaveConfig& setHeatCoolModeControlledBySlave(bool value) {
      setFlag(0x8000, value);
      return *this;
    }

    SlaveConfig& setMemberId(uint8_t memberId) {
      setData((getData() & 0xff00) | memberId);
      return *this;
    }

    SlaveConfig reply(Protocol::MessageType type, uint8_t flags, uint8_t memberId) const {
      return SlaveConfig(createReply(type, encode(flags, memberId)));
    }

    SlaveConfig reply(uint8_t flags, uint8_t memberId) const {
      return reply(Protocol::MessageType::READ_ACK, flags, memberId);
    }

  private:
    static uint16_t encode(uint8_t flags, uint8_t memberId) {
      return static_cast<uint16_t>((static_cast<uint32_t>(flags) << 8) | memberId);
    }
  };

}
