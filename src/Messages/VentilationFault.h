/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID72, specification page 35: ventilation faults in HB, OEM code in LB.
  class VentilationFault final : public Message {
  public:
    VentilationFault() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::ASFflagsOEMfaultCodeVentilationHeatRecovery, 0
    )) {}
    explicit VentilationFault(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit VentilationFault(const Message& message) : Message(message.getMessageData()) {}
    VentilationFault(
      uint8_t faultFlags, uint8_t oemFaultCode,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(
      type, Protocol::MessageID::ASFflagsOEMfaultCodeVentilationHeatRecovery,
      (static_cast<uint16_t>(faultFlags) << 8) | oemFaultCode
    )) {}

    uint8_t getFaultFlags() const {
      return getHighByte();
    }

    uint8_t getOemFaultCode() const {
      return getLowByte();
    }

    bool isServiceRequired() const {
      return getData() & 0x0100;
    }

    bool hasExhaustFanFault() const {
      return getData() & 0x0200;
    }

    bool hasInletFanFault() const {
      return getData() & 0x0400;
    }

    bool isFrostProtectionActive() const {
      return getData() & 0x0800;
    }

    VentilationFault& setFaultFlags(uint8_t flags) {
      setData((static_cast<uint16_t>(flags) << 8) | getOemFaultCode());
      return *this;
    }

    VentilationFault& setOemFaultCode(uint8_t code) {
      setData((getData() & 0xff00) | code);
      return *this;
    }

    VentilationFault& setServiceRequired(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    VentilationFault& setExhaustFanFault(bool value) {
      setFlag(0x0200, value);
      return *this;
    }

    VentilationFault& setInletFanFault(bool value) {
      setFlag(0x0400, value);
      return *this;
    }

    VentilationFault& setFrostProtectionActive(bool value) {
      setFlag(0x0800, value);
      return *this;
    }

    VentilationFault reply(uint8_t faultFlags, uint8_t oemFaultCode) const {
      return reply(Protocol::MessageType::READ_ACK, faultFlags, oemFaultCode);
    }

    VentilationFault reply(Protocol::MessageType type, uint8_t faultFlags, uint8_t oemFaultCode) const {
      return VentilationFault(createReply(type, (static_cast<uint16_t>(faultFlags) << 8) | oemFaultCode));
    }
  };

}
