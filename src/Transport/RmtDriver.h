/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rmt_rx.h>
#include <driver/rmt_tx.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace ESP32OT::Transport {

  class RmtDriver {
  public:
    enum class State : uint8_t {
      NOT_INITIALIZED,
      READY,
      DELAY,
      REQUEST_SENDING,
      RESPONSE_WAITING,
      RESPONSE_START_BIT,
      RESPONSE_RECEIVING,
      RESPONSE_READY,
      RESPONSE_INVALID
    };

    enum class ResultStatus : uint8_t {
      NONE,
      SUCCESS,
      INVALID,
      TIMEOUT
    };

    RmtDriver() = default;
    RmtDriver(const RmtDriver&) = delete;
    RmtDriver& operator=(const RmtDriver&) = delete;

    bool begin(gpio_num_t rxGpio, gpio_num_t txGpio, bool isSlave, bool alwaysReceive = false);
    bool end();
    bool send(uint32_t frame);
    void poll();
    TickType_t waitTicks() const;

    State getState() const {
      return state;
    }

    bool isReady() const {
      return state == State::READY;
    }

    bool isInitialized() const {
      return state != State::NOT_INITIALIZED;
    }

    bool hasResources() const {
      return rmt != nullptr;
    }

    uint32_t getFrame() const {
      return response;
    }

    ResultStatus getResultStatus() const {
      return responseStatus;
    }

    bool hasFrame() const {
      return receivedFrame;
    }

    bool resultIsTransmit() const {
      return transmitResult;
    }

    uint32_t getTimestamp() const {
      return responseTimestamp;
    }

    uint32_t getResultSequence() const {
      return resultSequence;
    }

    esp_err_t getLastError() const {
      return lastError;
    }

  private:
    struct RmtContext;
    RmtContext* rmt = nullptr;
    gpio_num_t rxGpio = GPIO_NUM_NC;
    gpio_num_t txGpio = GPIO_NUM_NC;
    bool isSlave = false;
    bool alwaysReceive = false;
    State state = State::NOT_INITIALIZED;
    uint32_t response = 0;
    ResultStatus responseStatus = ResultStatus::NONE;
    uint32_t responseTimestamp = 0;
    uint32_t transmitTimestamp = 0;
    uint32_t transmittedFrame = 0;
    uint8_t responseBitIndex = 0;
    bool responseTimingValid = true;
    bool receivedFrame = false;
    bool transmitResult = false;
    uint32_t resultSequence = 0;
    esp_err_t lastError = ESP_OK;

    static bool IRAM_ATTR onTransmitDone(
      rmt_channel_handle_t,
      const rmt_tx_done_event_data_t*,
      void*
    );
    static bool IRAM_ATTR onReceiveDone(
      rmt_channel_handle_t,
      const rmt_rx_done_event_data_t*,
      void*
    );
    bool isOwnerTask() const;
    bool startReceive();
    bool stopReceive();
    bool resetTransmitter();
    bool sendFrame(uint32_t frame);
    void processRmtEvents();
    void publishResult();
  };

}
