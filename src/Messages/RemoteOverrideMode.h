/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID99: HC1/HC2 modes in LB, DHW mode and manual push flag in HB (spec p.45-46).
  class RemoteOverrideMode final : public Message {
  public:

    // Heating and DHW assign different meanings to mode 2 and mode 3.
    enum class HeatingMode : uint8_t {
      NO_OVERRIDE = 0,
      AUTO = 1,
      COMFORT = 2,
      PRECOMFORT = 3,
      REDUCED = 4,
      PROTECTION = 5,
      OFF = 6
    };

    enum class DhwMode : uint8_t {
      NO_OVERRIDE = 0,
      AUTO = 1,
      ANTI_LEGIONELLA = 2,
      COMFORT = 3,
      REDUCED = 4,
      PROTECTION = 5,
      OFF = 6
    };

    RemoteOverrideMode() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::RemoteOverrideOperatingModeHeatingDHW, 0
    )) {}

    explicit RemoteOverrideMode(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit RemoteOverrideMode(const Message& message) : Message(message.getMessageData()) {}

    RemoteOverrideMode(
      HeatingMode heatingMode, HeatingMode ch2Mode, DhwMode dhwMode, bool dhwPush = false,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(
      type, Protocol::MessageID::RemoteOverrideOperatingModeHeatingDHW,
      encode(heatingMode, ch2Mode, dhwMode, dhwPush)
    )) {}

    HeatingMode getHeatingMode() const {
      return static_cast<HeatingMode>(getLowByte() & 0x0f);
    }

    HeatingMode getCh2Mode() const {
      return static_cast<HeatingMode>(getLowByte() >> 4);
    }

    DhwMode getDhwMode() const {
      return static_cast<DhwMode>(getHighByte() & 0x0f);
    }

    bool isDhwPushRequested() const {
      return getHighByte() & 0x10;
    }

    RemoteOverrideMode reply(
      Protocol::MessageType type,
      HeatingMode heatingMode, HeatingMode ch2Mode, DhwMode dhwMode, bool dhwPush = false
    ) const {
      return RemoteOverrideMode(createReply(type, encode(heatingMode, ch2Mode, dhwMode, dhwPush)));
    }

    static uint16_t encode(HeatingMode heatingMode, HeatingMode ch2Mode, DhwMode dhwMode, bool dhwPush = false) {
      // Reserved HB bits 7..5 are zero in new messages; parsing keeps the raw frame.
      return (static_cast<uint16_t>(heatingMode) & 0x0f)
        | ((static_cast<uint16_t>(ch2Mode) & 0x0f) << 4)
        | ((static_cast<uint16_t>(dhwMode) & 0x0f) << 8)
        | (dhwPush ? 0x1000 : 0);
    }
  };

}
