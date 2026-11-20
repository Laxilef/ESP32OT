/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include "Status.h"

namespace ESP32OT::Messages {

  Status Status::request(uint8_t masterFlags, uint8_t lowByte) {
    return Status(masterFlags, lowByte);
  }

  Status Status::reply(uint8_t slaveFlags) const {
    return reply(Protocol::MessageType::READ_ACK, slaveFlags);
  }

  Status Status::reply(Protocol::MessageType type, uint8_t slaveFlags) const {
    // READ_ACK echoes the master flags, not the boiler's actual active flags.
    return Status(createReply(type, (getData() & 0xff00) | slaveFlags));
  }

  Status& Status::setMasterFlags(uint8_t flags) {
    data.frame = Protocol::Frame::build(
      getType(), Protocol::MessageID::Status,
      (static_cast<uint16_t>(flags) << 8) | getSlaveFlags()
    );
    if (data.status == Protocol::OperationStatus::NONE && data.sequence == 0) {
      data.request = data.frame;
    }

    return *this;
  }

  Status& Status::setSlaveFlags(uint8_t flags) {
    data.frame = Protocol::Frame::build(getType(), Protocol::MessageID::Status, (getData() & 0xff00) | flags);
    if (data.status == Protocol::OperationStatus::NONE && data.sequence == 0) {
      data.request = data.frame;
    }

    return *this;
  }

  Status& Status::setMasterFlag(uint8_t mask, bool value) {
    const uint8_t flags = value ? getMasterFlags() | mask : getMasterFlags() & ~mask;
    return setMasterFlags(flags);
  }

  Status& Status::setSlaveFlag(uint8_t mask, bool value) {
    const uint8_t flags = value ? getSlaveFlags() | mask : getSlaveFlags() & ~mask;
    return setSlaveFlags(flags);
  }

}
