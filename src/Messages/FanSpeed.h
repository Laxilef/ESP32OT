/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID35 reports both fan speeds in Hz (RPM/60), without unit conversion.
  class FanSpeed final : public Message {
  public:
    FanSpeed() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::BoilerFanSpeedSetpointAndActual, 0
    )) {}
    explicit FanSpeed(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit FanSpeed(const Message& message) : Message(message.getMessageData()) {}
    FanSpeed(
      uint8_t setpointHz, uint8_t actualHz,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(type, Protocol::MessageID::BoilerFanSpeedSetpointAndActual, encode(setpointHz, actualHz))) {}

    uint8_t getSetpointHz() const {
      return getHighByte();
    }

    uint8_t getActualHz() const {
      return getLowByte();
    }

    FanSpeed reply(Protocol::MessageType type, uint8_t setpointHz, uint8_t actualHz) const {
      return FanSpeed(createReply(type, encode(setpointHz, actualHz)));
    }

  private:
    static uint16_t encode(uint8_t setpointHz, uint8_t actualHz) {
      return (static_cast<uint16_t>(setpointHz) << 8) | actualHz;
    }
  };

}
