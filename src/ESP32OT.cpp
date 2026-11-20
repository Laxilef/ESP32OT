/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include "ESP32OT.h"
#include <esp_timer.h>
#include <utility>

namespace ESP32OT {

  ESP32OT::ESP32OT() : ESP32OT(Config{}) {
  }

  ESP32OT::ESP32OT(const Config& config)
    : transport(config.isSlave), config(config) {
  }

  ESP32OT::ESP32OT(
    gpio_num_t rxGpio, gpio_num_t txGpio, bool isSlave, bool alwaysReceive
  ) : ESP32OT(Config{rxGpio, txGpio, isSlave, alwaysReceive}) {
  }

  ESP32OT::~ESP32OT() {

    // Stop callbacks before destroying any of the facade's members.
    if (!transport.end()) {
      ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE);
    }

    // Release owned captures while the facade's configuration still exists.
    transport.onStatusChange();
    transport.onRequest();
    transport.onResponse();
  }

  bool ESP32OT::begin() {
    return transport.begin(config);
  }

  bool ESP32OT::begin(gpio_num_t rxGpio, gpio_num_t txGpio) {
    Config requestedConfig = config;
    requestedConfig.rxGpio = rxGpio;
    requestedConfig.txGpio = txGpio;
    if (!transport.begin(requestedConfig)) {
      return false;
    }

    config.rxGpio = rxGpio;
    config.txGpio = txGpio;
    return true;
  }

  bool ESP32OT::end() {
    return transport.end();
  }

  bool ESP32OT::setConfig(const Config& config) {
    std::shared_ptr<const MessageCallback> retiredRequest;
    std::shared_ptr<const MessageCallback> retiredResponse;
    if (!transport.takeSession()) {
      return false;
    }
    const bool configured = transport.configureRole(config.isSlave, retiredRequest, retiredResponse);
    if (configured) {
      this->config = config;
    }
    transport.releaseSession();
    return configured;
  }

  ESP32OT& ESP32OT::setDelayCallback(DelayCallback callback, void* context) {
    if (transport.takeSession()) {
      if (!transport.isRunning()) {
        delayCallback = callback;
        delayContext = context;
      }
      transport.releaseSession();
    }
    return *this;
  }

  ESP32OT& ESP32OT::onStatusChange(StatusCallback callback) {
    transport.onStatusChange(std::move(callback));
    return *this;
  }

  ESP32OT& ESP32OT::onStatusChange(LegacyStatusCallback callback, void* context) {
    transport.onStatusChange(callback, context);
    return *this;
  }

  bool ESP32OT::onResponse(MessageCallback callback) {
    return transport.onResponse(std::move(callback));
  }

  bool ESP32OT::onResponse(LegacyMessageCallback callback, void* context) {
    return transport.onResponse(callback, context);
  }

  bool ESP32OT::onRequest(MessageCallback callback) {
    return transport.onRequest(std::move(callback));
  }

  bool ESP32OT::onRequest(LegacyMessageCallback callback, void* context) {
    return transport.onRequest(callback, context);
  }

  void ESP32OT::wait(unsigned int milliseconds) {
    if (delayCallback) {
      delayCallback(milliseconds, delayContext);

    } else {

      // The transport progresses independently; the calling task must yield.
      const TickType_t ticks = pdMS_TO_TICKS(milliseconds);
      vTaskDelay(ticks > 0 ? ticks : 1);
    }
  }

  ::ESP32OT::Protocol::MessageData ESP32OT::sendMessage(const Message& message, bool response) {
    ::ESP32OT::Protocol::MessageData result = message.getMessageData();
    result.status = OperationStatus::ERROR;
    result.error = ESP_ERR_INVALID_STATE;
    result.hasFrame = false;
    result.operation = 0;
    if (xPortInIsrContext() || transport.isWorkerTask() || !transport.takeSession()) {
      return result;
    }

    const int64_t startedAt = esp_timer_get_time();

    // One caller owns a synchronous operation including its retries. An earlier
    // accepted async operation is allowed to finish before we send our message.
    const uint8_t maxAttempts = response || config.maxAttempts == 0 ? 1 : config.maxAttempts;
    for (uint16_t attempt = 1; attempt <= maxAttempts; attempt++) {
      while (transport.isBusy() && isInitialized()) {
        wait(10);
      }

      if (!transport.sendMessage(message, response, static_cast<uint8_t>(attempt), true)) {
        result.status = OperationStatus::ERROR;
        result.hasFrame = false;
        result.error = getLastError();
        if (result.error == ESP_OK) {
          result.error = ESP_ERR_INVALID_STATE;
        }
        break;
      }

      do {
        wait(10);
        result = transport.getSynchronousResult();
      } while (!Message(result).isComplete());

      // A damaged response can be retried after the driver's wait time. A valid
      // negative reply finishes the conversation without repeating it.
      if (result.status == OperationStatus::SUCCESS
        || result.status == OperationStatus::DATA_INVALID || result.status == OperationStatus::UNKNOWN_DATA_ID
        || result.status == OperationStatus::CANCELLED || !isInitialized()) {
        break;
      }
    }
    result.totalElapsedUs = static_cast<uint32_t>(esp_timer_get_time() - startedAt);
    transport.releaseSession();
    return result;
  }

  ESP32OT::Message ESP32OT::sendRequest(const Message& request) {
    Message result(request);
    result.parse(sendMessage(request, false));
    return result;
  }

  ESP32OT::Message ESP32OT::sendResponse(const Message& response) {
    Message result(response);
    result.parse(sendMessage(response, true));
    return result;
  }

  ESP32OT::Message ESP32OT::sendRawRequest(uint32_t frame) {
    return sendRequest(Message(frame));
  }

  ESP32OT::Message ESP32OT::sendRawResponse(uint32_t frame, uint32_t requestSequence) {
    ::ESP32OT::Protocol::MessageData data;
    data.frame = frame;
    data.sequence = requestSequence;
    return sendResponse(Message(data));
  }

  bool ESP32OT::sendRawRequestAsync(uint32_t frame) {
    return sendRequestAsync(Message(frame));
  }

  bool ESP32OT::sendRawResponseAsync(uint32_t frame, uint32_t requestSequence) {
    ::ESP32OT::Protocol::MessageData data;
    data.frame = frame;
    data.sequence = requestSequence;
    return sendResponseAsync(Message(data));
  }

}
