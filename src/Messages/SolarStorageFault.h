/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID102: the specification reserves all HB flags; only the OEM code is defined.
  class SolarStorageFault final : public Message {
  public:
    SolarStorageFault() : Message(createData(
      Protocol::MessageType::READ_DATA,
      Protocol::MessageID::ASFflagsOEMfaultCodeSolarStorage,
      0
    )) {
    }

    explicit SolarStorageFault(const Protocol::MessageData& snapshot) : Message(snapshot) {
    }

    explicit SolarStorageFault(const Message& message) : Message(message.getMessageData()) {
    }

    uint8_t getOemFaultCode() const {
      return getLowByte();
    }

    SolarStorageFault& setOemFaultCode(uint8_t code) {
      setData((getData() & 0xff00) | code);
      return *this;
    }

    SolarStorageFault reply(uint8_t code) const {
      return reply(Protocol::MessageType::READ_ACK, code);
    }

    SolarStorageFault reply(Protocol::MessageType type, uint8_t code) const {
      return SolarStorageFault(createReply(type, (getData() & 0xff00) | code));
    }
  };

}
