/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID74, specification page 36: ventilation configuration in HB, member ID in LB.
  class VentilationConfig final : public Message {
  public:
    VentilationConfig() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::SConfigSMemberIDCodeVentilationHeatRecovery, 0
    )) {}
    explicit VentilationConfig(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit VentilationConfig(const Message& message) : Message(message.getMessageData()) {}
    VentilationConfig(
      uint8_t configFlags, uint8_t memberId,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(
      type, Protocol::MessageID::SConfigSMemberIDCodeVentilationHeatRecovery,
      (static_cast<uint16_t>(configFlags) << 8) | memberId
    )) {}

    uint8_t getConfigFlags() const {
      return getHighByte();
    }

    uint8_t getMemberId() const {
      return getLowByte();
    }

    // A clear system-type flag denotes central exhaust ventilation.
    bool isHeatRecoverySystem() const {
      return getData() & 0x0100;
    }

    bool hasBypass() const {
      return getData() & 0x0200;
    }

    // A clear speed-control flag denotes a three-speed system.
    bool isVariableSpeed() const {
      return getData() & 0x0400;
    }

    VentilationConfig& setConfigFlags(uint8_t flags) {
      setData((static_cast<uint16_t>(flags) << 8) | getMemberId());
      return *this;
    }

    VentilationConfig& setMemberId(uint8_t memberId) {
      setData((getData() & 0xff00) | memberId);
      return *this;
    }

    VentilationConfig& setHeatRecoverySystem(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    VentilationConfig& setBypassPresent(bool value) {
      setFlag(0x0200, value);
      return *this;
    }

    VentilationConfig& setVariableSpeed(bool value) {
      setFlag(0x0400, value);
      return *this;
    }

    VentilationConfig reply(uint8_t configFlags, uint8_t memberId) const {
      return reply(Protocol::MessageType::READ_ACK, configFlags, memberId);
    }

    VentilationConfig reply(Protocol::MessageType type, uint8_t configFlags, uint8_t memberId) const {
      return VentilationConfig(createReply(type, (static_cast<uint16_t>(configFlags) << 8) | memberId));
    }
  };

}
