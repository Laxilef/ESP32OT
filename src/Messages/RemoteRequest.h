/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID4: command in HB; slave completion code in LB (128..255 = accepted).
  class RemoteRequest final : public Message {
  public:
    RemoteRequest() : Message(createData(Protocol::MessageType::WRITE_DATA, Protocol::MessageID::RemoteRequest, 0)) {}
    explicit RemoteRequest(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit RemoteRequest(const Message& message) : Message(message.getMessageData()) {}
    explicit RemoteRequest(
      uint8_t requestCode, uint8_t responseCode = 0,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(type, Protocol::MessageID::RemoteRequest, encode(requestCode, responseCode))) {}

    uint8_t getRequestCode() const {
      return getHighByte();
    }

    uint8_t getResponseCode() const {
      return getLowByte();
    }

    bool isRequestAccepted() const {
      return getType() == Protocol::MessageType::WRITE_ACK && getResponseCode() >= 128;
    }

    RemoteRequest reply(Protocol::MessageType type, uint8_t responseCode) const {
      return RemoteRequest(createReply(type, encode(getRequestCode(), responseCode)));
    }

  private:
    static uint16_t encode(uint8_t requestCode, uint8_t responseCode) {
      return (static_cast<uint16_t>(requestCode) << 8) | responseCode;
    }
  };

}
