/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID6: supported parameters in HB, write permissions in LB (specification, p.42).
  class RemoteParameters final : public Message {
  public:
    RemoteParameters() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::RBPflags, 0
    )) {}

    explicit RemoteParameters(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit RemoteParameters(const Message& message) : Message(message.getMessageData()) {}
    RemoteParameters(
      uint8_t transferEnableFlags, uint8_t readWriteFlags,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(type, Protocol::MessageID::RBPflags, encode(transferEnableFlags, readWriteFlags))) {}

    bool isDhwSetpointSupported() const {
      return getData() & 0x0100;
    }

    bool isDhwSetpointWritable() const {
      return getData() & 0x0001;
    }

    bool isMaxHeatingSetpointSupported() const {
      return getData() & 0x0200;
    }

    bool isMaxHeatingSetpointWritable() const {
      return getData() & 0x0002;
    }

    RemoteParameters& setDhwSetpointSupported(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    RemoteParameters& setDhwSetpointWritable(bool value) {
      setFlag(0x0001, value);
      return *this;
    }

    RemoteParameters& setMaxHeatingSetpointSupported(bool value) {
      setFlag(0x0200, value);
      return *this;
    }

    RemoteParameters& setMaxHeatingSetpointWritable(bool value) {
      setFlag(0x0002, value);
      return *this;
    }

    RemoteParameters reply(Protocol::MessageType type, uint8_t transferEnableFlags, uint8_t readWriteFlags) const {
      return RemoteParameters(createReply(type, encode(transferEnableFlags, readWriteFlags)));
    }

    RemoteParameters reply(uint8_t transferEnableFlags, uint8_t readWriteFlags) const {
      return reply(Protocol::MessageType::READ_ACK, transferEnableFlags, readWriteFlags);
    }

  private:
    static uint16_t encode(uint8_t transferEnableFlags, uint8_t readWriteFlags) {
      return static_cast<uint16_t>((static_cast<uint32_t>(transferEnableFlags) << 8) | readWriteFlags);
    }
  };

}
