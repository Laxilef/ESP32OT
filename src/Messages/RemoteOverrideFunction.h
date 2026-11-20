/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID100 uses LB bits 0..1 per the detailed p.46 table; the overview's
  // flag8/- description differs. HB and the other LB bits are retained.
  class RemoteOverrideFunction final : public Message {
  public:
    RemoteOverrideFunction() : Message(createData(
      Protocol::MessageType::READ_DATA,
      Protocol::MessageID::RemoteOverrideFunction,
      0
    )) {
    }

    explicit RemoteOverrideFunction(const Protocol::MessageData& snapshot) : Message(snapshot) {
    }

    explicit RemoteOverrideFunction(const Message& message) : Message(message.getMessageData()) {
    }

    bool isManualChangePriorityEnabled() const {
      return getLowByte() & 0x01;
    }

    bool isProgramChangePriorityEnabled() const {
      return getLowByte() & 0x02;
    }

    RemoteOverrideFunction& setManualChangePriorityEnabled(bool value) {
      setFlag(0x0001, value);
      return *this;
    }

    RemoteOverrideFunction& setProgramChangePriorityEnabled(bool value) {
      setFlag(0x0002, value);
      return *this;
    }

    RemoteOverrideFunction reply(bool manualChangePriority, bool programChangePriority) const {
      return reply(Protocol::MessageType::READ_ACK, manualChangePriority, programChangePriority);
    }

    RemoteOverrideFunction reply(
      Protocol::MessageType type,
      bool manualChangePriority,
      bool programChangePriority
    ) const {
      const uint16_t payload = (getData() & 0xfffc)
        | (manualChangePriority ? 0x0001 : 0)
        | (programChangePriority ? 0x0002 : 0);
      return RemoteOverrideFunction(createReply(type, payload));
    }
  };

}
