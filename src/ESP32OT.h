/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include <Arduino.h>
#include <type_traits>
#include <soc/soc_caps.h>
#include "Transport/RmtTransport.h"
#include "Messages/Float.h"
#include "Messages/UInt16.h"
#include "Messages/Int16.h"
#include "Messages/Status.h"
#include "Messages/UInt8.h"
#include "Messages/MasterConfig.h"
#include "Messages/SlaveConfig.h"
#include "Messages/Fault.h"
#include "Messages/RemoteParameters.h"
#include "Messages/RemoteRequest.h"
#include "Messages/Capacity.h"
#include "Messages/FanSpeed.h"
#include "Messages/TemperatureBounds.h"
#include "Messages/ParameterCount.h"
#include "Messages/IndexedValue.h"
#include "Messages/BrandCharacter.h"
#include "Messages/ProductVersion.h"
#include "Messages/NominalVentilation.h"
#include "Messages/VentilationStatus.h"
#include "Messages/VentilationFault.h"
#include "Messages/VentilationConfig.h"
#include "Messages/VentilationRemoteParameters.h"
#include "Messages/SolarStorageStatus.h"
#include "Messages/SolarStorageFault.h"
#include "Messages/SolarStorageConfig.h"
#include "Messages/RemoteOverrideFunction.h"
#include "Messages/Date.h"
#include "Messages/DayTime.h"
#include "Messages/RFSensorStatus.h"
#include "Messages/RemoteOverrideMode.h"

#ifndef ARDUINO_ARCH_ESP32
  #error "ESP32OT requires Arduino-ESP32"
#endif

#if !SOC_RMT_SUPPORTED
  #error "ESP32OT requires RMT support"
#endif

namespace ESP32OT {

  class ESP32OT {
  public:
    using MessageID = ::ESP32OT::MessageID;
    using MessageType = ::ESP32OT::MessageType;
    using OperationStatus = ::ESP32OT::OperationStatus;
    using Message = ::ESP32OT::Messages::Message;
    using Config = ::ESP32OT::Transport::Config;
    using StatusEvent = ::ESP32OT::Transport::StatusEvent;
    using StatusCallback = ::ESP32OT::Transport::StatusCallback;
    using MessageCallback = ::ESP32OT::Transport::MessageCallback;
    using LegacyStatusCallback = ::ESP32OT::Transport::LegacyStatusCallback;
    using LegacyMessageCallback = ::ESP32OT::Transport::LegacyMessageCallback;
    using DelayCallback = void (*)(unsigned int milliseconds, void* context);

    ESP32OT();
    explicit ESP32OT(const Config& config);
    explicit ESP32OT(gpio_num_t rxGpio, gpio_num_t txGpio, bool isSlave = false, bool alwaysReceive = false);
    ~ESP32OT();
    ESP32OT(const ESP32OT&) = delete;
    ESP32OT& operator=(const ESP32OT&) = delete;
    ESP32OT(ESP32OT&&) = delete;
    ESP32OT& operator=(ESP32OT&&) = delete;

    bool begin();
    bool begin(gpio_num_t rxGpio, gpio_num_t txGpio);
    bool end();

    bool isInitialized() const {
      return transport.isInitialized();
    }

    bool isReady() const {
      return transport.isReady();
    }

    esp_err_t getLastError() const {
      return transport.getLastError();
    }

    uint32_t getWorkerStackHighWaterMark() const {
      return transport.getStackHighWaterMark();
    }

    const Config& getConfig() const {
      return config;
    }

    bool setConfig(const Config& config);
    ESP32OT& setDelayCallback(DelayCallback callback = nullptr, void* context = nullptr);
    ESP32OT& onStatusChange(StatusCallback callback = nullptr);
    ESP32OT& onStatusChange(LegacyStatusCallback callback, void* context = nullptr);
    bool onResponse(MessageCallback callback = nullptr);
    bool onResponse(LegacyMessageCallback callback, void* context = nullptr);
    bool onRequest(MessageCallback callback = nullptr);
    bool onRequest(LegacyMessageCallback callback, void* context = nullptr);

    bool sendRequestAsync(const Message& request) {
      return transport.sendMessage(request, false);
    }

    bool sendResponseAsync(const Message& response) {
      return transport.sendMessage(response, true);
    }

    Message sendRequest(const Message& request);
    Message sendResponse(const Message& response);

    template <class T, typename std::enable_if<std::is_base_of<Message, T>::value, int>::type = 0>
    T sendRequest(const T& request) {
      T result(request);
      result.parse(sendMessage(request, false));
      return result;
    }

    template <class T, typename std::enable_if<std::is_base_of<Message, T>::value, int>::type = 0>
    T sendResponse(const T& response) {
      T result(response);
      result.parse(sendMessage(response, true));
      return result;
    }

    Message getResult() const {
      return Message(transport.getResult());
    }

    Message getRequest() const {
      return Message(transport.getRequest());
    }

    void getResult(Message& destination) const {
      destination.parse(transport.getResult());
    }

    void getRequest(Message& destination) const {
      destination.parse(transport.getRequest());
    }

    template <class T>
    T getResult() const {
      T result;
      result.parse(transport.getResult());
      return result;
    }

    Message sendRawRequest(uint32_t frame);
    Message sendRawResponse(uint32_t frame, uint32_t requestSequence);
    bool sendRawRequestAsync(uint32_t frame);
    bool sendRawResponseAsync(uint32_t frame, uint32_t requestSequence);

  private:
    ::ESP32OT::Transport::RmtTransport transport;
    Config config;
    DelayCallback delayCallback = nullptr;
    void* delayContext = nullptr;

    ::ESP32OT::Protocol::MessageData sendMessage(const Message& message, bool response);
    void wait(unsigned int milliseconds);
  };

}
