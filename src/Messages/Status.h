/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID0 only: flags used by other Status IDs have different meanings.
  class Status final : public Message {
  public:
    Status() : Status(0) {}
    explicit Status(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit Status(const Message& message) : Message(message.getMessageData()) {}
    explicit Status(uint8_t masterFlags, uint8_t lowByte = 0) : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::Status,
      (static_cast<uint16_t>(masterFlags) << 8) | lowByte
    )) {}

    static Status request(uint8_t masterFlags = 0, uint8_t lowByte = 0);
    Status reply(uint8_t slaveFlags) const;
    Status reply(Protocol::MessageType type, uint8_t slaveFlags) const;

    uint8_t getMasterFlags() const {
      return getData() >> 8;
    }

    uint8_t getSlaveFlags() const {
      return getData() & 0xff;
    }

    bool isCentralHeatingEnabled() const {
      return getMasterFlags() & 0x01;
    }

    bool isHotWaterEnabled() const {
      return getMasterFlags() & 0x02;
    }

    bool isCoolingEnabled() const {
      return getMasterFlags() & 0x04;
    }

    bool isOutsideTemperatureCompensationEnabled() const {
      return getMasterFlags() & 0x08;
    }

    bool isCh2Enabled() const {
      return getMasterFlags() & 0x10;
    }

    bool isSummerWinterMode() const {
      return getMasterFlags() & 0x20;
    }

    bool isDhwBlocking() const {
      return getMasterFlags() & 0x40;
    }

    bool isFault() const {
      return getSlaveFlags() & 0x01;
    }

    bool isCentralHeatingActive() const {
      return getSlaveFlags() & 0x02;
    }

    bool isHotWaterActive() const {
      return getSlaveFlags() & 0x04;
    }

    bool isFlameOn() const {
      return getSlaveFlags() & 0x08;
    }

    bool isCoolingActive() const {
      return getSlaveFlags() & 0x10;
    }

    bool isCh2Active() const {
      return getSlaveFlags() & 0x20;
    }

    bool isDiagnostic() const {
      return getSlaveFlags() & 0x40;
    }

    bool isElectricityProductionActive() const {
      return getSlaveFlags() & 0x80;
    }

    Status& setMasterFlags(uint8_t flags);
    Status& setSlaveFlags(uint8_t flags);
    Status& setCentralHeatingEnabled(bool value) {
      return setMasterFlag(0x01, value);
    }

    Status& setHotWaterEnabled(bool value) {
      return setMasterFlag(0x02, value);
    }

    Status& setCoolingEnabled(bool value) {
      return setMasterFlag(0x04, value);
    }

    Status& setOutsideTemperatureCompensationEnabled(bool value) {
      return setMasterFlag(0x08, value);
    }

    Status& setCh2Enabled(bool value) {
      return setMasterFlag(0x10, value);
    }

    Status& setSummerWinterMode(bool value) {
      return setMasterFlag(0x20, value);
    }

    Status& setDhwBlocking(bool value) {
      return setMasterFlag(0x40, value);
    }

    Status& setFault(bool value) {
      return setSlaveFlag(0x01, value);
    }

    Status& setCentralHeatingActive(bool value) {
      return setSlaveFlag(0x02, value);
    }

    Status& setHotWaterActive(bool value) {
      return setSlaveFlag(0x04, value);
    }

    Status& setFlameOn(bool value) {
      return setSlaveFlag(0x08, value);
    }

    Status& setCoolingActive(bool value) {
      return setSlaveFlag(0x10, value);
    }

    Status& setCh2Active(bool value) {
      return setSlaveFlag(0x20, value);
    }

    Status& setDiagnostic(bool value) {
      return setSlaveFlag(0x40, value);
    }

    Status& setElectricityProductionActive(bool value) {
      return setSlaveFlag(0x80, value);
    }

  private:
    Status& setMasterFlag(uint8_t mask, bool value);
    Status& setSlaveFlag(uint8_t mask, bool value);
  };

}
