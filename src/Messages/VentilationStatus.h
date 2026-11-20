/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID70, specification page 34: requested modes in HB, actual status in LB.
  class VentilationStatus final : public Message {
  public:
    VentilationStatus() : VentilationStatus(0) {}
    explicit VentilationStatus(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit VentilationStatus(const Message& message) : Message(message.getMessageData()) {}
    explicit VentilationStatus(uint8_t masterFlags, uint8_t lowByte = 0) : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::StatusVentilationHeatRecovery,
      (static_cast<uint16_t>(masterFlags) << 8) | lowByte
    )) {}

    uint8_t getMasterFlags() const {
      return getHighByte();
    }

    uint8_t getSlaveFlags() const {
      return getLowByte();
    }

    bool isVentilationEnabled() const {
      return getData() & 0x0100;
    }

    // The requested position applies only in manual bypass mode.
    bool isBypassOpenRequested() const {
      return getData() & 0x0200;
    }

    bool isAutomaticBypassEnabled() const {
      return getData() & 0x0400;
    }

    bool isFreeVentilationEnabled() const {
      return getData() & 0x0800;
    }

    bool isFault() const {
      return getData() & 0x0001;
    }

    bool isVentilationActive() const {
      return getData() & 0x0002;
    }

    bool isBypassOpen() const {
      return getData() & 0x0004;
    }

    bool isAutomaticBypassActive() const {
      return getData() & 0x0008;
    }

    bool isFreeVentilationActive() const {
      return getData() & 0x0010;
    }

    bool isDiagnostic() const {
      return getData() & 0x0040;
    }

    VentilationStatus& setMasterFlags(uint8_t flags) {
      setData((static_cast<uint16_t>(flags) << 8) | getSlaveFlags());
      return *this;
    }

    VentilationStatus& setSlaveFlags(uint8_t flags) {
      setData((getData() & 0xff00) | flags);
      return *this;
    }

    VentilationStatus& setVentilationEnabled(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    VentilationStatus& setBypassOpenRequested(bool value) {
      setFlag(0x0200, value);
      return *this;
    }

    VentilationStatus& setAutomaticBypassEnabled(bool value) {
      setFlag(0x0400, value);
      return *this;
    }

    VentilationStatus& setFreeVentilationEnabled(bool value) {
      setFlag(0x0800, value);
      return *this;
    }

    VentilationStatus& setFault(bool value) {
      setFlag(0x0001, value);
      return *this;
    }

    VentilationStatus& setVentilationActive(bool value) {
      setFlag(0x0002, value);
      return *this;
    }

    VentilationStatus& setBypassOpen(bool value) {
      setFlag(0x0004, value);
      return *this;
    }

    VentilationStatus& setAutomaticBypassActive(bool value) {
      setFlag(0x0008, value);
      return *this;
    }

    VentilationStatus& setFreeVentilationActive(bool value) {
      setFlag(0x0010, value);
      return *this;
    }

    VentilationStatus& setDiagnostic(bool value) {
      setFlag(0x0040, value);
      return *this;
    }

    VentilationStatus reply(uint8_t slaveFlags) const {
      return reply(Protocol::MessageType::READ_ACK, slaveFlags);
    }

    VentilationStatus reply(Protocol::MessageType type, uint8_t slaveFlags) const {
      return VentilationStatus(createReply(type, (getData() & 0xff00) | slaveFlags));
    }
  };

}
