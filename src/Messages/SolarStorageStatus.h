/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID101 has its own modes/status fields; these are not the ID0 boiler flags.
  class SolarStorageStatus final : public Message {
  public:
    enum class Mode : uint8_t {
      OFF = 0,
      DHW_ECO = 1,
      DHW_COMFORT = 2,
      DHW_SINGLE_BOOST = 3,
      DHW_CONTINUOUS_BOOST = 4
    };

    enum class State : uint8_t {
      STANDBY = 0,
      SOLAR_LOADING = 1,
      BOILER_LOADING = 2,
      ANTI_LEGIONELLA = 3
    };

    SolarStorageStatus() : SolarStorageStatus(Mode::OFF) {
    }

    explicit SolarStorageStatus(const Protocol::MessageData& snapshot) : Message(snapshot) {
    }

    explicit SolarStorageStatus(const Message& message) : Message(message.getMessageData()) {
    }

    explicit SolarStorageStatus(Mode masterMode)
      : Message(createData(
        Protocol::MessageType::READ_DATA,
        Protocol::MessageID::StatusSolarStorage,
        (static_cast<uint16_t>(masterMode) & 0x07) << 8
      )) {
    }

    Mode getMasterMode() const {
      return static_cast<Mode>(getHighByte() & 0x07);
    }

    Mode getSlaveMode() const {
      return static_cast<Mode>((getLowByte() >> 1) & 0x07);
    }

    State getState() const {
      return static_cast<State>((getLowByte() >> 4) & 0x03);
    }

    bool isFault() const {
      return getLowByte() & 0x01;
    }

    SolarStorageStatus& setMasterMode(Mode mode) {
      setData((getData() & 0xf8ff) | ((static_cast<uint16_t>(mode) & 0x07) << 8));
      return *this;
    }

    SolarStorageStatus& setSlaveMode(Mode mode) {
      setData((getData() & 0xfff1) | ((static_cast<uint16_t>(mode) & 0x07) << 1));
      return *this;
    }

    SolarStorageStatus& setState(State state) {
      setData((getData() & 0xffcf) | ((static_cast<uint16_t>(state) & 0x03) << 4));
      return *this;
    }

    SolarStorageStatus& setFault(bool value) {
      setFlag(0x0001, value);
      return *this;
    }

    SolarStorageStatus reply(Mode slaveMode, State state, bool fault = false) const {
      return reply(Protocol::MessageType::READ_ACK, slaveMode, state, fault);
    }

    SolarStorageStatus reply(
      Protocol::MessageType type,
      Mode slaveMode,
      State state,
      bool fault = false
    ) const {
      // Echo the whole master byte and retain unspecified LB bits 7..6.
      const uint16_t payload = (getData() & 0xffc0)
        | ((static_cast<uint16_t>(slaveMode) & 0x07) << 1)
        | ((static_cast<uint16_t>(state) & 0x03) << 4)
        | (fault ? 0x0001 : 0);
      return SolarStorageStatus(createReply(type, payload));
    }
  };

}
