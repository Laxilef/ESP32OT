/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include "RmtDriver.h"
#include "../Protocol/Frame.h"
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <new>

namespace ESP32OT::Transport {

  static constexpr size_t TX_SYMBOL_COUNT = 34;
  static constexpr size_t RX_SYMBOL_COUNT = 64;
  static constexpr uint8_t NO_RX_BUFFER = 2;
  static constexpr uint32_t RX_IDLE_US = 2500;
  static constexpr uint32_t RESPONSE_TIMEOUT_US = 1000000;
  static constexpr uint32_t RESPONSE_FRAME_GRACE_US = 40000;
  static constexpr uint32_t RX_MIN_BIT_PERIOD_US = 900;
  static constexpr uint32_t RX_MAX_BIT_PERIOD_US = 1150;

  enum class RmtEventType : uint8_t {
    TRANSMIT_DONE,
    RECEIVE_DONE
  };

  struct RmtEvent {
    RmtEventType type;
    uint32_t timestamp;
    size_t symbolCount;
    uint8_t buffer;
    esp_err_t error;
  };

  struct RmtDriver::RmtContext {
    rmt_channel_handle_t rx = nullptr;
    rmt_channel_handle_t tx = nullptr;
    rmt_encoder_handle_t encoder = nullptr;
    rmt_receive_config_t receiveConfig = {};
    rmt_symbol_word_t txSymbols[TX_SYMBOL_COUNT] = {};
    rmt_symbol_word_t rxSymbols[2][RX_SYMBOL_COUNT] = {};
    StaticQueue_t queueState;
    uint8_t queueStorage[3 * sizeof(RmtEvent)];
    QueueHandle_t events = nullptr;
    StaticQueue_t availableState;
    uint8_t availableStorage[2];
    QueueHandle_t available = nullptr;

    // Only the buffer index crosses ISR/task contexts; queue operations transfer
    // ownership of the buffers. Protocol state belongs exclusively to the task.
    volatile uint8_t activeRxBuffer = NO_RX_BUFFER;
    TaskHandle_t owner = nullptr;
    BaseType_t core = tskNO_AFFINITY;
    bool rxEnabled = false;
    bool txEnabled = false;
    bool gpioConfigured = false;
  };

  struct RmtDecodedFrame {
    RmtDriver::State state = RmtDriver::State::RESPONSE_WAITING;
    uint32_t data = 0;
    uint32_t elapsedUs = 0;
    uint32_t lastSampleUs = 0;
    uint8_t bits = 0;
    bool timingValid = true;
  };

  static RmtDecodedFrame decodeRmtFrameLegacy(
    const rmt_symbol_word_t* symbols,
    size_t symbolCount,
    RmtDriver::State state,
    uint32_t data,
    uint8_t bits,
    bool timingValid
  ) {
    RmtDecodedFrame frame;
    if (state == RmtDriver::State::RESPONSE_START_BIT || state == RmtDriver::State::RESPONSE_RECEIVING) {
      frame.state = state;
      frame.data = data;
      frame.bits = bits;
      frame.timingValid = timingValid;
    }

    bool started = false;
    for (size_t pulseIndex = 0; pulseIndex < symbolCount * 2; pulseIndex++) {
      const rmt_symbol_word_t& symbol = symbols[pulseIndex / 2];
      const uint16_t durationUs = pulseIndex % 2 == 0 ? symbol.duration0 : symbol.duration1;
      const bool level = pulseIndex % 2 == 0 ? symbol.level0 : symbol.level1;
      if (durationUs == 0) {
        break;
      }

      if (!started) {
        started = true;
        if (frame.state == RmtDriver::State::RESPONSE_START_BIT) {
          // A previous capture ended on idle, so this edge is already too late
          // for the second start edge (<750 us), just as in the GPIO decoder.
          frame.state = RmtDriver::State::RESPONSE_INVALID;
          frame.timingValid = false;
          return frame;

        } else if (frame.state == RmtDriver::State::RESPONSE_RECEIVING) {
          // The gap between captures is >=RX_IDLE_US, hence >750 us. Preserve
          // the old parser state instead of discarding the partially read frame.
          frame.timingValid = false;
          if (frame.bits == 32) {
            frame.state = RmtDriver::State::RESPONSE_READY;
            return frame;
          }

          frame.data = (frame.data << 1) | static_cast<uint32_t>(!level);
          frame.bits++;

        } else {
          frame.state = level
            ? RmtDriver::State::RESPONSE_START_BIT
            : RmtDriver::State::RESPONSE_INVALID;
          if (!level) {
            return frame;
          }
        }
      }

      // The terminal idle interval has no ending edge. Do not invent a stop bit.
      if (durationUs >= RX_IDLE_US) {
        break;
      }

      frame.elapsedUs += durationUs;
      const bool newLevel = !level;
      const uint32_t sampleElapsedUs = frame.elapsedUs - frame.lastSampleUs;
      if (frame.state == RmtDriver::State::RESPONSE_START_BIT) {
        // Preserve the adapter/vendor tolerance of the original GPIO decoder.
        if (sampleElapsedUs >= 750 || newLevel) {
          frame.state = RmtDriver::State::RESPONSE_INVALID;
          frame.timingValid = false;
          frame.lastSampleUs = frame.elapsedUs;
          return frame;
        }

        frame.state = RmtDriver::State::RESPONSE_RECEIVING;
        frame.lastSampleUs = frame.elapsedUs;

      } else if (sampleElapsedUs > 750) {
        // OpenTherm specifies the period between data mid-bit transitions,
        // not separate limits for the two adapter-dependent half-pulses.
        // Keep the raw legacy decode even when timing is invalid so callbacks
        // retain a complete frame; the caller can retry without guessing bits.
        if (sampleElapsedUs < RX_MIN_BIT_PERIOD_US || sampleElapsedUs > RX_MAX_BIT_PERIOD_US) {
          frame.timingValid = false;
        }
        frame.lastSampleUs = frame.elapsedUs;
        if (frame.bits == 32) {
          // Keep the existing stop-edge acceptance for vendor compatibility.
          frame.state = RmtDriver::State::RESPONSE_READY;
          return frame;
        }

        frame.data = (frame.data << 1) | static_cast<uint32_t>(!newLevel);
        frame.bits++;
      }
    }

    return frame;
  }

  struct RmtPulse {
    uint32_t durationUs = 0;
    bool level = false;
  };

  static bool readRmtPulse(
    const rmt_symbol_word_t* symbols,
    size_t symbolCount,
    size_t& pulseIndex,
    RmtPulse& pulse
  ) {
    if (pulseIndex >= symbolCount * 2) {
      return false;
    }

    const auto& symbol = symbols[pulseIndex / 2];
    pulse.durationUs = pulseIndex % 2 == 0 ? symbol.duration0 : symbol.duration1;
    pulse.level = pulseIndex % 2 == 0 ? symbol.level0 : symbol.level1;
    pulseIndex++;
    if (pulse.durationUs == 0 || pulse.durationUs >= RX_IDLE_US) {
      pulseIndex = symbolCount * 2;
      return false;
    }

    return true;
  }

  static bool readRmtFilteredPulse(
    const rmt_symbol_word_t* symbols,
    size_t symbolCount,
    size_t& pulseIndex,
    RmtPulse& pulse
  ) {
    if (!readRmtPulse(symbols, symbolCount, pulseIndex, pulse)) {
      return false;
    }

    while (true) {
      const size_t followingIndex = pulseIndex;
      RmtPulse following;
      if (!readRmtPulse(symbols, symbolCount, pulseIndex, following)) {
        return true;
      }

      RmtPulse after;
      if (following.durationUs <= 2 && pulse.level != following.level
          && readRmtPulse(symbols, symbolCount, pulseIndex, after) && after.level == pulse.level) {
        // Suppress only a bounded interior excursion, preserving elapsed time.
        // The fixed capture remains unchanged for diagnostics.
        pulse.durationUs += following.durationUs + after.durationUs;

      } else {
        pulseIndex = followingIndex;
        return true;
      }
    }
  }

  struct RmtPhaseCandidate {
    uint32_t data = 0;
    uint8_t halfIndex = 0;
    bool ambiguous = false;
  };

  static bool decodeRmtFramePhase(
    const rmt_symbol_word_t* symbols,
    size_t symbolCount,
    RmtDecodedFrame& frame
  ) {
    size_t pulseIndex = 0;
    uint32_t totalUs = 0;
    RmtPulse pulse;
    RmtPulse previousPulse;
    bool hasPrevious = false;
    uint16_t periodSamples[RX_SYMBOL_COUNT * 2];
    size_t sampleCount = 0;
    while (readRmtFilteredPulse(symbols, symbolCount, pulseIndex, pulse)) {
      totalUs += pulse.durationUs;
      if (hasPrevious) {
        if (pulse.level == previousPulse.level) {
          return false;
        }

        // Estimate clock independently of frame length. Opposite-level pairs
        // cancel the adapter's different rising/falling edge delays. Runs in
        // the ambiguous short/full overlap do not contribute clock samples.
        const uint8_t previousWidth = previousPulse.durationUs < 750 ? 1 : previousPulse.durationUs > 850 ? 2 : 0;
        const uint8_t currentWidth = pulse.durationUs < 750 ? 1 : pulse.durationUs > 850 ? 2 : 0;
        if (previousWidth != 0 && currentWidth != 0) {
          const uint8_t pairWidth = previousWidth + currentWidth;
          const uint32_t periodUs = (2 * (previousPulse.durationUs + pulse.durationUs) + pairWidth / 2) / pairWidth;
          if (periodUs >= RX_MIN_BIT_PERIOD_US && periodUs <= RX_MAX_BIT_PERIOD_US) {
            if (sampleCount == RX_SYMBOL_COUNT * 2) {
              return false;
            }

            size_t position = sampleCount++;
            while (position != 0 && periodSamples[position - 1] > periodUs) {
              periodSamples[position] = periodSamples[position - 1];
              position--;
            }
            periodSamples[position] = static_cast<uint16_t>(periodUs);
          }
        }
      }

      previousPulse = pulse;
      hasPrevious = true;
    }

    if (sampleCount == 0) {
      return false;
    }

    const uint32_t periodUs = (periodSamples[(sampleCount - 1) / 2] + periodSamples[sampleCount / 2]) / 2;
    // Start through the stop mid-bit edge spans 67 completed half-cells.
    // Do not fit an incorrect bit count by stretching its inferred clock.
    const int32_t frameError = static_cast<int32_t>(2 * totalUs) - static_cast<int32_t>(67 * periodUs);
    const uint32_t frameDistance = static_cast<uint32_t>(frameError < 0 ? -frameError : frameError);
    if (frameDistance * 5 > periodUs * 3) {
      return false;
    }

    RmtPhaseCandidate candidates[2] = {};
    size_t candidateCount = 1;
    uint32_t elapsedUs = 0;
    pulseIndex = 0;
    if (!readRmtFilteredPulse(symbols, symbolCount, pulseIndex, pulse)) {
      return false;
    }

    while (true) {
      elapsedUs += pulse.durationUs;
      RmtPhaseCandidate next[2] = {};
      size_t nextCount = 0;
      for (size_t index = 0; index < candidateCount; index++) {
        const auto& previous = candidates[index];
        const uint8_t maximumWidth = previous.halfIndex % 2 == 0 ? 1 : 2;
        for (uint8_t width = 1; width <= maximumWidth; width++) {
          RmtPhaseCandidate candidate = previous;
          candidate.halfIndex += width;
          if (candidate.halfIndex > 67) {
            continue;
          }

          // Allow measured edge displacement without choosing bits by parity
          // or ID. This window is +/-0.6 half-cell around the frame clock;
          // at most two integer phase positions can lie inside it.
          const int32_t phaseError = static_cast<int32_t>(2 * elapsedUs)
            - static_cast<int32_t>(candidate.halfIndex * periodUs);
          const uint32_t phaseDistance = static_cast<uint32_t>(phaseError < 0 ? -phaseError : phaseError);
          if (phaseDistance * 5 > periodUs * 3) {
            continue;
          }

          if ((candidate.halfIndex == 1 || candidate.halfIndex == 67) && !pulse.level) {
            continue;
          }

          if (candidate.halfIndex % 2 != 0 && candidate.halfIndex != 1 && candidate.halfIndex != 67) {
            candidate.data = (candidate.data << 1) | static_cast<uint32_t>(pulse.level);
          }

          bool merged = false;
          for (size_t prior = 0; prior < nextCount; prior++) {
            if (next[prior].halfIndex == candidate.halfIndex) {
              next[prior].ambiguous = next[prior].ambiguous || candidate.ambiguous
                || next[prior].data != candidate.data;
              merged = true;
              break;
            }
          }

          if (!merged) {
            if (nextCount == 2) {
              return false;
            }

            next[nextCount++] = candidate;
          }
        }
      }

      if (nextCount == 0) {
        return false;
      }

      candidateCount = nextCount;
      for (size_t index = 0; index < nextCount; index++) {
        candidates[index] = next[index];
      }

      if (!readRmtFilteredPulse(symbols, symbolCount, pulseIndex, pulse)) {
        break;
      }
    }

    for (size_t index = 0; index < candidateCount; index++) {
      const auto& candidate = candidates[index];
      if (candidate.halfIndex == 67 && !candidate.ambiguous) {
        frame.state = RmtDriver::State::RESPONSE_READY;
        frame.data = candidate.data;
        frame.bits = 32;
        frame.elapsedUs = totalUs;
        frame.lastSampleUs = totalUs;
        frame.timingValid = true;
        return true;
      }
    }

    return false;
  }

  static RmtDecodedFrame decodeRmtFrame(
    const rmt_symbol_word_t* symbols,
    size_t symbolCount,
    RmtDriver::State state,
    uint32_t data,
    uint8_t bits,
    bool timingValid
  ) {
    RmtDecodedFrame frame = decodeRmtFrameLegacy(symbols, symbolCount, state, data, bits, timingValid);
    const bool newFrame = state == RmtDriver::State::RESPONSE_WAITING || state == RmtDriver::State::READY;
    if (newFrame && (!frame.timingValid || frame.state != RmtDriver::State::RESPONSE_READY)) {
      decodeRmtFramePhase(symbols, symbolCount, frame);
    }

    return frame;
  }

  bool RmtDriver::isOwnerTask() const {
    return rmt != nullptr && !xPortInIsrContext()
      && xTaskGetCurrentTaskHandle() == rmt->owner && xPortGetCoreID() == rmt->core;
  }

  bool RmtDriver::begin(gpio_num_t rxGpio, gpio_num_t txGpio, bool isSlave, bool alwaysReceive) {
    if (isInitialized()) {
      lastError = ESP_ERR_INVALID_STATE;
      return false;
    }

    if (xPortInIsrContext() || rmt != nullptr || xTaskGetCoreID(nullptr) == tskNO_AFFINITY) {
      lastError = ESP_ERR_INVALID_STATE;
      return false;
    }

    if (!GPIO_IS_VALID_GPIO(rxGpio) || !GPIO_IS_VALID_OUTPUT_GPIO(txGpio)) {
      lastError = ESP_ERR_INVALID_ARG;
      return false;
    }

    this->rxGpio = rxGpio;
    this->txGpio = txGpio;
    this->isSlave = isSlave;
    this->alwaysReceive = alwaysReceive;

    void* memory = heap_caps_malloc(sizeof(RmtContext), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (memory == nullptr) {
      lastError = ESP_ERR_NO_MEM;
      return false;
    }

    rmt = new (memory) RmtContext;
    rmt->owner = xTaskGetCurrentTaskHandle();
    rmt->core = xPortGetCoreID();
    rmt->events = xQueueCreateStatic(3, sizeof(RmtEvent), rmt->queueStorage, &rmt->queueState);
    rmt->available = xQueueCreateStatic(2, sizeof(uint8_t), rmt->availableStorage, &rmt->availableState);
    esp_err_t error = rmt->events != nullptr && rmt->available != nullptr ? ESP_OK : ESP_ERR_NO_MEM;
    if (error == ESP_OK) {
      for (uint8_t buffer = 0; buffer < 2; buffer++) {
        xQueueSend(rmt->available, &buffer, 0);
      }
    }

    rmt_rx_channel_config_t rxConfig = {};
    rxConfig.gpio_num = rxGpio;
    rxConfig.clk_src = RMT_CLK_SRC_DEFAULT;
    rxConfig.resolution_hz = 1000000;
    rxConfig.mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL;
    #if CONFIG_IDF_TARGET_ESP32S3
      // Four RX blocks allow two receivers with two blocks each. C3/C6 use one
      // block per receiver so that two OpenTherm buses fit without configuration.
      rxConfig.mem_block_symbols *= 2;
    #endif
    rxConfig.intr_priority = 1;
    if (error == ESP_OK) {
      error = rmt_new_rx_channel(&rxConfig, &rmt->rx);
    }

    rmt_rx_event_callbacks_t rxCallbacks = {};
    rxCallbacks.on_recv_done = onReceiveDone;
    if (error == ESP_OK) {
      error = rmt_rx_register_event_callbacks(rmt->rx, &rxCallbacks, rmt);
    }

    if (error == ESP_OK) {
      error = rmt_enable(rmt->rx);
      rmt->rxEnabled = error == ESP_OK;
    }

    // Suppress sub-microsecond glitches without clipping OpenTherm transitions.
    rmt->receiveConfig.signal_range_min_ns = 1000;
    rmt->receiveConfig.signal_range_max_ns = RX_IDLE_US * 1000;

    rmt_tx_channel_config_t txConfig = {};
    txConfig.gpio_num = txGpio;
    txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
    txConfig.resolution_hz = 1000000;
    txConfig.mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL;
    txConfig.trans_queue_depth = 1;
    txConfig.intr_priority = 1;
    txConfig.flags.init_level = 1;
    if (error == ESP_OK) {
      error = rmt_new_tx_channel(&txConfig, &rmt->tx);
      rmt->gpioConfigured = error == ESP_OK;
    }

    rmt_copy_encoder_config_t encoderConfig = {};
    if (error == ESP_OK) {
      error = rmt_new_copy_encoder(&encoderConfig, &rmt->encoder);
    }

    if (error == ESP_OK) {
      error = rmt_enable(rmt->tx);
      rmt->txEnabled = error == ESP_OK;
    }

    rmt_tx_event_callbacks_t txCallbacks = {};
    txCallbacks.on_trans_done = onTransmitDone;
    if (error == ESP_OK) {
      error = rmt_tx_register_event_callbacks(rmt->tx, &txCallbacks, rmt);
    }

    if (error == ESP_OK) {
      gpio_set_level(txGpio, 1);
      delay(1150);
      response = 0;
      responseStatus = ResultStatus::NONE;
      responseBitIndex = 0;
      responseTimingValid = true;
      receivedFrame = false;
      transmitResult = false;
      state = State::READY;
      if ((isSlave || alwaysReceive) && !startReceive()) {
        error = lastError;
      }
    }

    if (error != ESP_OK) {
      end();
      state = State::NOT_INITIALIZED;
      if (rmt == nullptr) {
        lastError = error;
      }

      return false;
    }

    lastError = ESP_OK;
    return true;
  }

  bool IRAM_ATTR RmtDriver::onTransmitDone(
    rmt_channel_handle_t,
    const rmt_tx_done_event_data_t*,
    void* context
  ) {
    RmtContext* transport = static_cast<RmtContext*>(context);
    RmtEvent event = {
      RmtEventType::TRANSMIT_DONE,
      static_cast<uint32_t>(esp_timer_get_time()),
      0,
      NO_RX_BUFFER,
      ESP_OK
    };

    // Both RMT IRQs run on the owner's core at the same priority. No application
    // state is shared with this callback; RX must be ready before the task wakes.
    BaseType_t taskWoken = pdFALSE;
    uint8_t buffer;
    if (xQueueReceiveFromISR(transport->available, &buffer, &taskWoken) == pdPASS) {
      transport->activeRxBuffer = buffer;
      event.error = rmt_receive(
        transport->rx,
        transport->rxSymbols[buffer],
        sizeof(transport->rxSymbols[buffer]),
        &transport->receiveConfig
      );
      if (event.error != ESP_OK) {
        transport->activeRxBuffer = NO_RX_BUFFER;
        xQueueSendFromISR(transport->available, &buffer, &taskWoken);
      }

    } else {
      event.error = ESP_ERR_NO_MEM;
    }

    xQueueSendFromISR(transport->events, &event, &taskWoken);
    vTaskNotifyGiveFromISR(transport->owner, &taskWoken);
    return taskWoken == pdTRUE;
  }

  bool IRAM_ATTR RmtDriver::onReceiveDone(
    rmt_channel_handle_t,
    const rmt_rx_done_event_data_t* data,
    void* context
  ) {
    RmtContext* transport = static_cast<RmtContext*>(context);
    RmtEvent event = {
      RmtEventType::RECEIVE_DONE,
      static_cast<uint32_t>(esp_timer_get_time()),
      data->num_symbols,
      transport->activeRxBuffer,
      ESP_OK
    };
    transport->activeRxBuffer = NO_RX_BUFFER;
    BaseType_t taskWoken = pdFALSE;
    uint8_t buffer;

    // Continue capturing into the other owned buffer while the task is asleep.
    // If both buffers are pending, stop here; the task rearms after consuming them.
    if (xQueueReceiveFromISR(transport->available, &buffer, &taskWoken) == pdPASS) {
      transport->activeRxBuffer = buffer;
      event.error = rmt_receive(
        transport->rx,
        transport->rxSymbols[buffer],
        sizeof(transport->rxSymbols[buffer]),
        &transport->receiveConfig
      );
      if (event.error != ESP_OK) {
        transport->activeRxBuffer = NO_RX_BUFFER;
        xQueueSendFromISR(transport->available, &buffer, &taskWoken);
      }
    }

    xQueueSendFromISR(transport->events, &event, &taskWoken);
    vTaskNotifyGiveFromISR(transport->owner, &taskWoken);
    return taskWoken == pdTRUE;
  }

  bool RmtDriver::startReceive() {
    if (rmt->activeRxBuffer != NO_RX_BUFFER) {
      return true;
    }

    uint8_t buffer;
    if (xQueueReceive(rmt->available, &buffer, 0) != pdPASS) {
      lastError = ESP_ERR_NO_MEM;
      return false;
    }

    rmt->activeRxBuffer = buffer;
    lastError = rmt_receive(
      rmt->rx,
      rmt->rxSymbols[buffer],
      sizeof(rmt->rxSymbols[buffer]),
      &rmt->receiveConfig
    );
    if (lastError != ESP_OK) {
      rmt->activeRxBuffer = NO_RX_BUFFER;
      xQueueSend(rmt->available, &buffer, 0);
      return false;
    }

    return true;
  }

  bool RmtDriver::stopReceive() {
    if (rmt->activeRxBuffer == NO_RX_BUFFER) {
      return true;
    }

    lastError = rmt_disable(rmt->rx);
    if (lastError != ESP_OK) {
      return false;
    }

    rmt->rxEnabled = false;
    const uint8_t buffer = rmt->activeRxBuffer;
    rmt->activeRxBuffer = NO_RX_BUFFER;
    if (buffer != NO_RX_BUFFER) {
      xQueueSend(rmt->available, &buffer, 0);
    }

    lastError = rmt_enable(rmt->rx);
    rmt->rxEnabled = lastError == ESP_OK;
    return rmt->rxEnabled;
  }

  bool RmtDriver::resetTransmitter() {
    lastError = rmt_disable(rmt->tx);
    if (lastError != ESP_OK) {
      return false;
    }

    rmt->txEnabled = false;
    lastError = rmt_enable(rmt->tx);
    rmt->txEnabled = lastError == ESP_OK;
    return rmt->txEnabled;
  }

  bool RmtDriver::sendFrame(const uint32_t request) {
    for (size_t index = 0; index < TX_SYMBOL_COUNT; index++) {
      const bool bit = index == 0 || index == TX_SYMBOL_COUNT - 1
        ? true
        : ((request >> (32 - index)) & 1) != 0;
      rmt_symbol_word_t& symbol = rmt->txSymbols[index];
      symbol.level0 = !bit;
      symbol.duration0 = 500;
      symbol.level1 = bit;
      symbol.duration1 = 500;
    }

    rmt_transmit_config_t config = {};
    config.flags.eot_level = 1;
    config.flags.queue_nonblocking = 1;
    transmitTimestamp = static_cast<uint32_t>(esp_timer_get_time());
    lastError = rmt_transmit(
      rmt->tx,
      rmt->encoder,
      rmt->txSymbols,
      sizeof(rmt->txSymbols),
      &config
    );
    return lastError == ESP_OK;
  }

  bool RmtDriver::send(uint32_t request) {
    if (!isOwnerTask() || !isReady()) {
      lastError = ESP_ERR_INVALID_STATE;
      return false;
    }

    if (!stopReceive()) {
      state = State::NOT_INITIALIZED;
      responseStatus = ResultStatus::NONE;
      return false;
    }

    xQueueReset(rmt->events);
    xQueueReset(rmt->available);
    for (uint8_t buffer = 0; buffer < 2; buffer++) {
      xQueueSend(rmt->available, &buffer, 0);
    }

    state = State::REQUEST_SENDING;
    response = 0;
    responseStatus = ResultStatus::NONE;
    responseBitIndex = 0;
    responseTimingValid = true;
    receivedFrame = false;
    transmitResult = false;
    transmittedFrame = request;

    if (!sendFrame(request)) {
      state = State::READY;
      return false;
    }

    return true;
  }

  void RmtDriver::processRmtEvents() {
    RmtEvent event;
    while (xQueueReceive(rmt->events, &event, 0) == pdPASS) {
      if (event.type == RmtEventType::TRANSMIT_DONE) {
        if (state != State::REQUEST_SENDING) {
          continue;
        }

        lastError = event.error;
        responseTimestamp = event.timestamp;
        if (event.error != ESP_OK) {
          state = State::NOT_INITIALIZED;
          responseStatus = ResultStatus::NONE;
          return;
        }

        if (isSlave) {
          state = State::READY;
          response = transmittedFrame;
          responseStatus = ResultStatus::SUCCESS;
          transmitResult = true;
          receivedFrame = false;
          publishResult();
          return;
        }

        state = State::RESPONSE_WAITING;

      } else {
        if (event.buffer >= 2) {
          state = State::NOT_INITIALIZED;
          responseStatus = ResultStatus::NONE;
          lastError = ESP_ERR_INVALID_STATE;
          return;
        }

        const bool accept = state == State::RESPONSE_WAITING || state == State::RESPONSE_START_BIT
          || state == State::RESPONSE_RECEIVING || (state == State::READY && (isSlave || alwaysReceive));
        if (accept) {
          transmitResult = false;
          receivedFrame = false;
          lastError = event.error;
          if (event.symbolCount >= RX_SYMBOL_COUNT) {
            lastError = ESP_ERR_INVALID_SIZE;
            state = State::RESPONSE_INVALID;

          } else {
            const RmtDecodedFrame frame = decodeRmtFrame(
              rmt->rxSymbols[event.buffer],
              event.symbolCount,
              state,
              response,
              responseBitIndex,
              responseTimingValid
            );
            response = frame.data;
            responseBitIndex = frame.bits;
            responseTimingValid = frame.timingValid;
            state = frame.state;
            receivedFrame = state == State::RESPONSE_READY;
          }

          if (event.symbolCount != 0 && rmt->rxSymbols[event.buffer][0].duration0 != 0) {
            responseTimestamp = event.timestamp;
          }

          if (state == State::RESPONSE_READY || state == State::RESPONSE_INVALID) {
            const esp_err_t captureError = lastError;
            if (!stopReceive()) {
              state = State::NOT_INITIALIZED;
              responseStatus = ResultStatus::NONE;
              return;
            }

            lastError = captureError;

          } else if (event.error != ESP_OK) {
            state = State::NOT_INITIALIZED;
            responseStatus = ResultStatus::NONE;
            return;
          }
        }

        xQueueSend(rmt->available, &event.buffer, 0);
      }
    }
  }

  void RmtDriver::poll() {
    if (state == State::NOT_INITIALIZED) {
      return;
    }

    if (!isOwnerTask()) {
      lastError = ESP_ERR_INVALID_STATE;
      return;
    }

    const uint32_t previousSequence = resultSequence;
    processRmtEvents();
    if (resultSequence != previousSequence) {
      return;
    }

    if (state == State::NOT_INITIALIZED) {
      publishResult();
      return;
    }

    uint32_t now = static_cast<uint32_t>(esp_timer_get_time());
    if (state == State::REQUEST_SENDING) {
      if (now - transmitTimestamp <= RESPONSE_TIMEOUT_US) {
        return;
      }

      // Stop this channel before deciding: its final ISR event may have arrived
      // after the first queue read. The owner's core cannot run its ISR in parallel.
      if (!resetTransmitter()) {
        state = State::NOT_INITIALIZED;
        responseStatus = ResultStatus::NONE;
        publishResult();
        return;
      }

      processRmtEvents();
      if (resultSequence != previousSequence) {
        return;
      }

      if (state == State::NOT_INITIALIZED) {
        publishResult();
        return;
      }

      if (state == State::REQUEST_SENDING) {
        state = State::READY;
        responseStatus = ResultStatus::NONE;
        lastError = ESP_ERR_TIMEOUT;
        publishResult();
        return;
      }
    }

    const bool waiting = state == State::RESPONSE_WAITING
      || state == State::RESPONSE_START_BIT || state == State::RESPONSE_RECEIVING;
    const uint32_t receiveTimeoutUs = RESPONSE_TIMEOUT_US + RESPONSE_FRAME_GRACE_US + RX_IDLE_US;
    if (waiting && now - responseTimestamp > receiveTimeoutUs) {
      if (!stopReceive()) {
        state = State::NOT_INITIALIZED;
        responseStatus = ResultStatus::NONE;
        publishResult();
        return;
      }

      // Claim an already completed capture before declaring the slave silent.
      processRmtEvents();
      if (state == State::NOT_INITIALIZED) {
        publishResult();
        return;
      }

      now = static_cast<uint32_t>(esp_timer_get_time());
      const bool stillWaiting = state == State::RESPONSE_WAITING
        || state == State::RESPONSE_START_BIT || state == State::RESPONSE_RECEIVING;
      if (stillWaiting && now - responseTimestamp > receiveTimeoutUs) {
        state = State::READY;
        responseStatus = ResultStatus::TIMEOUT;
        publishResult();
        return;
      }
    }

    if (state == State::RESPONSE_INVALID) {
      state = State::DELAY;
      responseStatus = ResultStatus::INVALID;
      publishResult();
      return;

    } else if (state == State::RESPONSE_READY) {
      state = State::DELAY;
      responseStatus = responseTimingValid && (isSlave
        ? Protocol::Frame::isValidRequest(response)
        : Protocol::Frame::isValidResponse(response))
        ? ResultStatus::SUCCESS
        : ResultStatus::INVALID;
      publishResult();
      return;

    } else if (state == State::DELAY) {
      if (now - responseTimestamp > (isSlave ? 20000U : 100000U)) {
        state = State::READY;
      }
    }

    const bool continueReceive = state == State::RESPONSE_WAITING || state == State::RESPONSE_START_BIT
      || state == State::RESPONSE_RECEIVING || (state == State::READY && (isSlave || alwaysReceive));
    if (continueReceive) {
      if (!startReceive()) {
        state = State::NOT_INITIALIZED;
        responseStatus = ResultStatus::NONE;
        receivedFrame = false;
        transmitResult = false;
        publishResult();
      }
    }
  }

  bool RmtDriver::end() {
    if (rmt != nullptr) {
      if (!isOwnerTask()) {
        lastError = ESP_ERR_INVALID_STATE;
        return false;
      }

      state = State::NOT_INITIALIZED;

      // TX_DONE can arm RX. Delete TX completely while RX and the ISR context live.
      if (rmt->tx != nullptr) {
        if (rmt->txEnabled) {
          lastError = rmt_disable(rmt->tx);
          if (lastError != ESP_OK) {
            return false;
          }

          rmt->txEnabled = false;
        }

        lastError = rmt_del_channel(rmt->tx);
        if (lastError != ESP_OK) {
          return false;
        }

        rmt->tx = nullptr;
      }

      if (rmt->rx != nullptr) {
        if (rmt->rxEnabled) {
          lastError = rmt_disable(rmt->rx);
          if (lastError != ESP_OK) {
            return false;
          }

          rmt->rxEnabled = false;
        }

        lastError = rmt_del_channel(rmt->rx);
        if (lastError != ESP_OK) {
          return false;
        }

        rmt->rx = nullptr;
      }

      if (rmt->encoder != nullptr) {
        lastError = rmt_del_encoder(rmt->encoder);
        if (lastError != ESP_OK) {
          return false;
        }

        rmt->encoder = nullptr;
      }

      if (rmt->gpioConfigured) {
        gpio_set_direction(txGpio, GPIO_MODE_OUTPUT);
        gpio_set_level(txGpio, 0);
      }

      if (rmt->events != nullptr) {
        vQueueDelete(rmt->events);
      }

      if (rmt->available != nullptr) {
        vQueueDelete(rmt->available);
      }

      rmt->~RmtContext();
      heap_caps_free(rmt);
      rmt = nullptr;
    }

    state = State::NOT_INITIALIZED;
    response = 0;
    responseStatus = ResultStatus::NONE;
    responseTimestamp = 0;
    responseBitIndex = 0;
    responseTimingValid = true;
    transmitTimestamp = 0;
    receivedFrame = false;
    transmitResult = false;
    lastError = ESP_OK;
    return true;
  }

  void RmtDriver::publishResult() {
    resultSequence++;
  }

  TickType_t RmtDriver::waitTicks() const {
    if (state == State::NOT_INITIALIZED) {
      return portMAX_DELAY;
    }

    if (rmt != nullptr && rmt->events != nullptr && uxQueueMessagesWaiting(rmt->events) != 0) {
      return 0;
    }

    // A terminal timeout/error can leave READY with RX stopped. Poll once before
    // sleeping so passive/slave reception resumes without another send command.
    if (state == State::READY && (isSlave || alwaysReceive)
      && rmt != nullptr && rmt->activeRxBuffer == NO_RX_BUFFER) {
      return 0;
    }

    uint32_t timestamp = 0;
    uint32_t timeoutUs = 0;
    if (state == State::REQUEST_SENDING) {
      timestamp = transmitTimestamp;
      timeoutUs = RESPONSE_TIMEOUT_US;

    } else if (state == State::RESPONSE_WAITING || state == State::RESPONSE_START_BIT
      || state == State::RESPONSE_RECEIVING) {
      timestamp = responseTimestamp;
      timeoutUs = RESPONSE_TIMEOUT_US + RESPONSE_FRAME_GRACE_US + RX_IDLE_US;

    } else if (state == State::DELAY) {
      timestamp = responseTimestamp;
      timeoutUs = isSlave ? 20000U : 100000U;

    } else {
      return portMAX_DELAY;
    }

    const uint32_t elapsedUs = static_cast<uint32_t>(esp_timer_get_time()) - timestamp;
    if (elapsedUs > timeoutUs) {
      return 0;
    }

    // Strict > deadlines and tick-phase uncertainty require rounding upward.
    const uint64_t remainingUs = static_cast<uint64_t>(timeoutUs - elapsedUs) + 1;
    return static_cast<TickType_t>((remainingUs * configTICK_RATE_HZ + 999999) / 1000000) + 1;
  }

}
