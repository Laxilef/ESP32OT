/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID86, specification page 42: transfer-enable flags in HB, read/write flags in LB.
  class VentilationRemoteParameters final : public Message {
  public:
    VentilationRemoteParameters() : Message(createData(
      Protocol::MessageType::READ_DATA, Protocol::MessageID::RBPflagsVentilationHeatRecovery, 0
    )) {}
    explicit VentilationRemoteParameters(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit VentilationRemoteParameters(const Message& message) : Message(message.getMessageData()) {}
    VentilationRemoteParameters(
      uint8_t transferFlags, uint8_t readWriteFlags,
      Protocol::MessageType type = Protocol::MessageType::READ_ACK
    ) : Message(createData(
      type, Protocol::MessageID::RBPflagsVentilationHeatRecovery,
      (static_cast<uint16_t>(transferFlags) << 8) | readWriteFlags
    )) {}

    uint8_t getTransferFlags() const {
      return getHighByte();
    }

    uint8_t getReadWriteFlags() const {
      return getLowByte();
    }

    bool isNominalVentilationTransferEnabled() const {
      return getData() & 0x0100;
    }

    bool isNominalVentilationWritable() const {
      return getData() & 0x0001;
    }

    VentilationRemoteParameters& setTransferFlags(uint8_t flags) {
      setData((static_cast<uint16_t>(flags) << 8) | getReadWriteFlags());
      return *this;
    }

    VentilationRemoteParameters& setReadWriteFlags(uint8_t flags) {
      setData((getData() & 0xff00) | flags);
      return *this;
    }

    VentilationRemoteParameters& setNominalVentilationTransferEnabled(bool value) {
      setFlag(0x0100, value);
      return *this;
    }

    VentilationRemoteParameters& setNominalVentilationWritable(bool value) {
      setFlag(0x0001, value);
      return *this;
    }

    VentilationRemoteParameters reply(uint8_t transferFlags, uint8_t readWriteFlags) const {
      return reply(Protocol::MessageType::READ_ACK, transferFlags, readWriteFlags);
    }

    VentilationRemoteParameters reply(
      Protocol::MessageType type, uint8_t transferFlags, uint8_t readWriteFlags
    ) const {
      return VentilationRemoteParameters(createReply(
        type, (static_cast<uint16_t>(transferFlags) << 8) | readWriteFlags
      ));
    }
  };

}
