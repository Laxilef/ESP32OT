/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "../Protocol/Frame.h"
#include <type_traits>

namespace ESP32OT::Messages {

  // Owns a raw frame and its operation metadata. Typed messages use the same
  // snapshot and interpret their payload on demand, without a second value cache.
  class Message {
  public:
    explicit Message(uint32_t frame = 0) {
      data.frame = frame;
      data.request = frame;
    }

    explicit Message(const Protocol::MessageData& snapshot) : data(snapshot) {}
    virtual ~Message() = default;

    // Raw bits, including vendor fields or incorrect parity, are never rewritten.
    virtual uint32_t build() const {
      return data.frame;
    }

    virtual void parse(const Protocol::MessageData& snapshot) {
      data = snapshot;
    }

    // Explicit value conversion; the caller chooses the payload format.
    template <class T>
    T as() const {
      static_assert(std::is_base_of<Message, T>::value, "T must derive from Message");
      return T(*this);
    }

    const Protocol::MessageData& getMessageData() const {
      return data;
    }

    uint32_t getRaw() const {
      return data.frame;
    }

    uint32_t getRequest() const {
      return data.request;
    }

    uint16_t getData() const {
      return Protocol::Frame::getData(data.frame);
    }

    uint8_t getHighByte() const {
      return getData() >> 8;
    }

    uint8_t getLowByte() const {
      return getData() & 0xff;
    }

    Protocol::MessageID getId() const {
      return Protocol::Frame::getId(data.frame);
    }

    Protocol::MessageType getType() const {
      return Protocol::Frame::getType(data.frame);
    }

    Protocol::OperationStatus getStatus() const {
      return data.status;
    }

    uint32_t getOperation() const {
      return data.operation;
    }

    uint32_t getSequence() const {
      return data.sequence;
    }

    uint32_t getElapsedUs() const {
      return data.elapsedUs;
    }

    uint32_t getTotalElapsedUs() const {
      return data.totalElapsedUs;
    }

    uint8_t getAttempt() const {
      return data.attempt;
    }

    esp_err_t getError() const {
      return data.error;
    }

    bool hasFrame() const {
      return data.hasFrame;
    }

    // Successful data acknowledgment or slave TX. Valid negative replies have
    // their own terminal status and must not be used as acknowledged data.
    bool isSuccess() const {
      return data.status == Protocol::OperationStatus::SUCCESS;
    }

    bool isComplete() const {
      return data.status == Protocol::OperationStatus::SUCCESS || data.status == Protocol::OperationStatus::INVALID
        || data.status == Protocol::OperationStatus::TIMEOUT || data.status == Protocol::OperationStatus::ERROR
        || data.status == Protocol::OperationStatus::CANCELLED || data.status == Protocol::OperationStatus::DATA_INVALID
        || data.status == Protocol::OperationStatus::UNKNOWN_DATA_ID;
    }

    static Message read(Protocol::MessageID id, uint16_t payload = 0) {
      return Message(createData(Protocol::MessageType::READ_DATA, id, payload));
    }

    static Message write(Protocol::MessageID id, uint16_t payload) {
      return Message(createData(Protocol::MessageType::WRITE_DATA, id, payload));
    }

    Message reply(Protocol::MessageType type, uint16_t payload) const {
      return Message(createReply(type, payload));
    }

  protected:
    Protocol::MessageData data;

    // Preserve received metadata; only a newly built request follows payload edits.
    void setData(uint16_t payload) {
      data.frame = Protocol::Frame::build(getType(), getId(), payload);
      if (data.status == Protocol::OperationStatus::NONE && data.sequence == 0) {
        data.request = data.frame;
      }
    }

    void setFlag(uint16_t mask, bool value) {
      setData(value ? getData() | mask : getData() & ~mask);
    }

    static Protocol::MessageData createData(Protocol::MessageType type, Protocol::MessageID id, uint16_t payload) {
      Protocol::MessageData result;
      result.frame = Protocol::Frame::build(type, id, payload);
      result.request = result.frame;
      return result;
    }

    Protocol::MessageData createReply(Protocol::MessageType type, uint16_t payload) const {
      Protocol::MessageData result = createData(type, getId(), payload);
      result.sequence = data.sequence;
      result.request = data.frame;
      return result;
    }
  };

}
