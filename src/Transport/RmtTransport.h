/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "RmtDriver.h"
#include "../Messages/Message.h"
#include <freertos/semphr.h>
#include <functional>
#include <memory>

namespace ESP32OT {
  class ESP32OT;
}

namespace ESP32OT::Transport {

  struct Config {
    gpio_num_t rxGpio = GPIO_NUM_NC;
    gpio_num_t txGpio = GPIO_NUM_NC;
    bool isSlave = false;
    bool alwaysReceive = false;

    // Total synchronous master attempts, including the first; zero means one.
    uint8_t maxAttempts = 5;

    // A starting value; callbacks are part of the worker's stack budget.
    uint32_t stackSize = 3072;
    UBaseType_t priority = 5;
    BaseType_t core = 0;

    // Transparent forwarding may carry complete frames with bad parity/type.
    // Partial captures are never accepted as requests.
    bool acceptInvalidRequests = false;
  };

  struct StatusEvent {
    Protocol::OperationStatus previous;
    Protocol::OperationStatus current;
    const Messages::Message& message;
  };

  using StatusCallback = std::function<void(const StatusEvent&)>;
  using MessageCallback = std::function<void(const Messages::Message&)>;
  using LegacyStatusCallback = void (*)(const StatusEvent&, void*);
  using LegacyMessageCallback = void (*)(const Messages::Message&, void*);

  // The worker owns the driver. This class only shares commands and snapshots
  // with callers; it never shares a live RMT buffer or parser with them.
  class RmtTransport {
  public:
    explicit RmtTransport(bool isSlave = false);
    ~RmtTransport();
    RmtTransport(const RmtTransport&) = delete;
    RmtTransport& operator=(const RmtTransport&) = delete;

    bool begin(const Config& config);
    bool end();
    bool configureRole(bool isSlave);
    bool sendMessage(
      const Messages::Message& message,
      bool response,
      uint8_t attempt = 1,
      bool synchronous = false
    );
    Protocol::MessageData getResult() const;
    Protocol::MessageData getSynchronousResult() const;
    Protocol::MessageData getRequest() const;
    bool isInitialized() const;
    bool isReady() const;
    bool isBusy() const;
    bool isRunning() const;
    bool isWorkerTask() const;
    esp_err_t getLastError() const;
    uint32_t getStackHighWaterMark() const;
    bool takeSession();
    void releaseSession();

    void onStatusChange(StatusCallback callback = nullptr);
    void onStatusChange(LegacyStatusCallback callback, void* context = nullptr);
    bool onResponse(MessageCallback callback = nullptr);
    bool onResponse(LegacyMessageCallback callback, void* context = nullptr);
    bool onRequest(MessageCallback callback = nullptr);
    bool onRequest(LegacyMessageCallback callback, void* context = nullptr);

  private:
    friend class ::ESP32OT::ESP32OT;
    struct ActiveOperation;

    bool slave;
    mutable StaticSemaphore_t mutexStorage;
    StaticSemaphore_t lifecycleStorage;
    StaticSemaphore_t acknowledgmentStorage;
    StaticSemaphore_t sessionStorage;
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t lifecycle;
    SemaphoreHandle_t acknowledgment;
    SemaphoreHandle_t session;
    TaskHandle_t worker = nullptr;
    RmtDriver driver;
    gpio_num_t rxGpio = GPIO_NUM_NC;
    gpio_num_t txGpio = GPIO_NUM_NC;
    bool alwaysReceive = false;
    bool acceptInvalidRequests = false;
    bool initialized = false;
    bool ready = false;
    bool stopping = false;
    bool busy = false;
    bool pending = false;
    bool pendingResponse = false;
    Protocol::MessageData result;
    Protocol::MessageData pendingCommand;

    // A sync caller can wake after the next slave request has already arrived.
    // Keep its completion until that caller starts its next attempt.
    Protocol::MessageData synchronousResult;
    uint32_t synchronousOperation = 0;
    Protocol::MessageData request;
    Protocol::OperationStatus queuedPrevious = Protocol::OperationStatus::NONE;
    uint32_t nextOperation = 0;
    uint32_t requestSequence = 0;
    uint64_t acceptedAt = 0;
    esp_err_t lastError = ESP_OK;
    // Snapshots retain the registered closure without copying its captures.
    std::shared_ptr<const StatusCallback> statusCallback;
    std::shared_ptr<const MessageCallback> requestCallback;
    std::shared_ptr<const MessageCallback> responseCallback;

    static void runTask(void* context);
    // The facade retains old closures until its configuration/session update ends.
    bool configureRole(
      bool isSlave,
      std::shared_ptr<const MessageCallback>& retiredRequest,
      std::shared_ptr<const MessageCallback>& retiredResponse
    );
    void run();
    bool stopWorker(ActiveOperation& operation);
    void processDriverResult(ActiveOperation& operation, uint32_t& lastResultSequence);
    void processSlaveRequest(Protocol::MessageData& received, ActiveOperation& operation);
    void takePendingOperation(ActiveOperation& operation);
    void publish(Protocol::MessageData data, bool terminal, bool incoming = false);
    void notifyStatus(Protocol::OperationStatus previous, const Protocol::MessageData& data);
    void notifyMessage(const Protocol::MessageData& data, bool response);
    void updateDriverState();
  };

}
