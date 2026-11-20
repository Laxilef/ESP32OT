/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID5: application-specific fault flags in HB, OEM fault code in LB (p.35).
  class Fault final : public Message {
  public:
    Fault() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::ASFflags, 0
    )) {}

    explicit Fault(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit Fault(const Message& message) : Message(message.getMessageData()) {}
    Fault(
      uint8_t flags, uint8_t oemCode,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(type, Protocol::MessageID::ASFflags, encode(flags, oemCode))) {}

    bool isServiceRequested() const {
      return getData() & 0x0100;
    }

    // This permits a remote reset; it is not a lockout occurrence flag.
    bool isLockoutResetEnabled() const {
      return getData() & 0x0200;
    }

    bool isLowWaterPressureFault() const {
      return getData() & 0x0400;
    }

    bool isGasFlameFault() const {
      return getData() & 0x0800;
    }

    bool isAirPressureFault() const {
      return getData() & 0x1000;
    }

    bool isWaterOvertemperatureFault() const {
      return getData() & 0x2000;
    }

    uint8_t getOemFaultCode() const {
      return getLowByte();
    }

    Fault& setServiceRequested(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    Fault& setLockoutResetEnabled(bool value) {
      setFlag(0x0200, value);
      return *this;
    }

    Fault& setLowWaterPressureFault(bool value) {
      setFlag(0x0400, value);
      return *this;
    }

    Fault& setGasFlameFault(bool value) {
      setFlag(0x0800, value);
      return *this;
    }

    Fault& setAirPressureFault(bool value) {
      setFlag(0x1000, value);
      return *this;
    }

    Fault& setWaterOvertemperatureFault(bool value) {
      setFlag(0x2000, value);
      return *this;
    }

    Fault& setOemFaultCode(uint8_t oemCode) {
      setData((getData() & 0xff00) | oemCode);
      return *this;
    }

    Fault reply(Protocol::MessageType type, uint8_t flags, uint8_t oemCode) const {
      return Fault(createReply(type, encode(flags, oemCode)));
    }

    Fault reply(uint8_t flags, uint8_t oemCode) const {
      return reply(Protocol::MessageType::READ_ACK, flags, oemCode);
    }

  private:
    static uint16_t encode(uint8_t flags, uint8_t oemCode) {
      return static_cast<uint16_t>((static_cast<uint32_t>(flags) << 8) | oemCode);
    }
  };

}
