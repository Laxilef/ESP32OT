/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include "RmtTransport.h"
#include <esp_timer.h>
#include <utility>

namespace ESP32OT::Transport {

  RmtTransport::RmtTransport(bool isSlave)
    : slave(isSlave) {
    mutex = xSemaphoreCreateMutexStatic(&mutexStorage);
    lifecycle = xSemaphoreCreateMutexStatic(&lifecycleStorage);
    acknowledgment = xSemaphoreCreateBinaryStatic(&acknowledgmentStorage);
    session = xSemaphoreCreateRecursiveMutexStatic(&sessionStorage);
  }

  RmtTransport::~RmtTransport() {
    // Destroying a live worker after a failed cleanup would leave it using freed
    // storage. Explicit end() lets the application handle/retry that error first.
    if (!end()) {
      ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE);
    }

    // Captured objects can use transport getters while being released.
    statusCallback.reset();
    requestCallback.reset();
    responseCallback.reset();

    vSemaphoreDelete(session);
    vSemaphoreDelete(acknowledgment);
    vSemaphoreDelete(lifecycle);
    vSemaphoreDelete(mutex);
  }

  bool RmtTransport::takeSession() {
    return !xPortInIsrContext() && xSemaphoreTakeRecursive(session, 0) == pdTRUE;
  }

  void RmtTransport::releaseSession() {
    xSemaphoreGiveRecursive(session);
  }

  bool RmtTransport::configureRole(bool isSlave) {
    std::shared_ptr<const MessageCallback> retiredRequest;
    std::shared_ptr<const MessageCallback> retiredResponse;
    return configureRole(isSlave, retiredRequest, retiredResponse);
  }

  bool RmtTransport::configureRole(
    bool isSlave,
    std::shared_ptr<const MessageCallback>& retiredRequest,
    std::shared_ptr<const MessageCallback>& retiredResponse
  ) {
    if (xPortInIsrContext()) {
      return false;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    if (worker != nullptr) {
      lastError = ESP_ERR_INVALID_STATE;
      xSemaphoreGive(mutex);
      return false;
    }

    if (slave != isSlave) {
      slave = isSlave;
      retiredRequest.swap(requestCallback);
      retiredResponse.swap(responseCallback);
    }

    xSemaphoreGive(mutex);
    return true;
  }

  bool RmtTransport::begin(const Config& config) {
    if (xPortInIsrContext() || isWorkerTask() || !takeSession()) {
      return false;
    }

    std::shared_ptr<const MessageCallback> retiredRequest;
    std::shared_ptr<const MessageCallback> retiredResponse;
    xSemaphoreTake(lifecycle, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (worker != nullptr) {
      lastError = ESP_ERR_INVALID_STATE;
      xSemaphoreGive(mutex);
      xSemaphoreGive(lifecycle);
      releaseSession();
      return false;
    }

    if (!GPIO_IS_VALID_GPIO(config.rxGpio) || !GPIO_IS_VALID_OUTPUT_GPIO(config.txGpio)
      || config.core < 0 || config.core >= portNUM_PROCESSORS
      || config.priority >= configMAX_PRIORITIES || config.stackSize < configMINIMAL_STACK_SIZE) {
      lastError = ESP_ERR_INVALID_ARG;
      xSemaphoreGive(mutex);
      xSemaphoreGive(lifecycle);
      releaseSession();
      return false;
    }

    rxGpio = config.rxGpio;
    txGpio = config.txGpio;
    alwaysReceive = config.alwaysReceive;
    if (slave != config.isSlave) {
      slave = config.isSlave;
      retiredRequest.swap(requestCallback);
      retiredResponse.swap(responseCallback);
    }

    acceptInvalidRequests = config.acceptInvalidRequests;
    stopping = false;
    lastError = ESP_OK;
    busy = false;
    pending = false;
    result = {};
    request = {};
    xSemaphoreTake(acknowledgment, 0);
    const BaseType_t created = xTaskCreatePinnedToCore(
      runTask,
      "OpenTherm Transport",
      config.stackSize,
      this,
      config.priority,
      &worker,
      config.core
    );
    if (created != pdPASS) {
      worker = nullptr;
      lastError = ESP_ERR_NO_MEM;
      xSemaphoreGive(mutex);
      xSemaphoreGive(lifecycle);
      releaseSession();
      return false;
    }

    xSemaphoreGive(mutex);

    xSemaphoreTake(acknowledgment, portMAX_DELAY);
    const bool started = isInitialized();
    xSemaphoreGive(lifecycle);
    releaseSession();
    return started;
  }

  bool RmtTransport::end() {
    if (xPortInIsrContext() || isWorkerTask()) {
      return false;
    }

    xSemaphoreTake(lifecycle, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (worker == nullptr) {
      xSemaphoreGive(mutex);
      xSemaphoreGive(lifecycle);
      return true;
    }

    xSemaphoreTake(acknowledgment, 0);
    stopping = true;
    ready = false;
    xTaskNotifyGive(worker);
    xSemaphoreGive(mutex);
    xSemaphoreTake(acknowledgment, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool stopped = worker == nullptr;
    xSemaphoreGive(mutex);
    xSemaphoreGive(lifecycle);
    return stopped;
  }

  bool RmtTransport::sendMessage(
    const Messages::Message& message,
    bool response,
    uint8_t attempt,
    bool synchronous
  ) {
    if (xPortInIsrContext()) {
      return false;
    }

    const bool bypassSession = isWorkerTask() && slave && response;
    if (!bypassSession && !takeSession()) {
      return false;
    }

    // Messages::Message subclasses own their encoding. Do not invoke user code with the
    // shared-state mutex held, and copy the frame before waking the worker.
    Protocol::MessageData command = message.getMessageData();
    command.frame = message.build();

    xSemaphoreTake(mutex, portMAX_DELAY);
    const uint32_t sequence = message.getSequence();

    // The counter survives restarts, but the saved request is cleared.
    // Match both so a reply from before end() cannot be sent after begin().
    if (!initialized || stopping || busy || response != slave
      || (response && (sequence == 0 || sequence != requestSequence || sequence != request.sequence))) {
      lastError = ESP_ERR_INVALID_STATE;
      xSemaphoreGive(mutex);
      if (!bypassSession) {
        releaseSession();
      }

      return false;
    }

    queuedPrevious = result.status;
    result = command;
    if (!response) {
      result.request = command.frame;

    } else {
      result.request = request.frame;
    }

    nextOperation++;
    if (nextOperation == 0) {
      nextOperation++;
    }

    result.operation = nextOperation;
    result.status = Protocol::OperationStatus::QUEUED;
    result.attempt = attempt;
    result.hasFrame = false;
    result.elapsedUs = 0;
    result.totalElapsedUs = 0;
    result.error = ESP_OK;
    pendingCommand = result;
    if (synchronous) {
      synchronousOperation = result.operation;
      synchronousResult = result;
    }

    acceptedAt = esp_timer_get_time();
    busy = true;
    pending = true;
    pendingResponse = response;
    ready = false;
    lastError = ESP_OK;
    xTaskNotifyGive(worker);
    xSemaphoreGive(mutex);
    if (!bypassSession) {
      releaseSession();
    }

    return true;
  }

  Protocol::MessageData RmtTransport::getResult() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const Protocol::MessageData snapshot = result;
    xSemaphoreGive(mutex);
    return snapshot;
  }

  Protocol::MessageData RmtTransport::getSynchronousResult() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const Protocol::MessageData snapshot = synchronousResult;
    xSemaphoreGive(mutex);
    return snapshot;
  }

  Protocol::MessageData RmtTransport::getRequest() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const Protocol::MessageData snapshot = request;
    xSemaphoreGive(mutex);
    return snapshot;
  }

  bool RmtTransport::isInitialized() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool value = initialized;
    xSemaphoreGive(mutex);
    return value;
  }

  bool RmtTransport::isReady() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool value = initialized && !stopping && !busy && ready;
    xSemaphoreGive(mutex);
    return value;
  }

  bool RmtTransport::isBusy() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool value = busy;
    xSemaphoreGive(mutex);
    return value;
  }

  bool RmtTransport::isRunning() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool value = worker != nullptr;
    xSemaphoreGive(mutex);
    return value;
  }

  bool RmtTransport::isWorkerTask() const {
    if (xPortInIsrContext()) {
      return false;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool value = worker != nullptr && xTaskGetCurrentTaskHandle() == worker;
    xSemaphoreGive(mutex);
    return value;
  }

  esp_err_t RmtTransport::getLastError() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const esp_err_t error = lastError;
    xSemaphoreGive(mutex);
    return error;
  }

  uint32_t RmtTransport::getStackHighWaterMark() const {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const uint32_t bytes = worker != nullptr ? uxTaskGetStackHighWaterMark(worker) : 0;
    xSemaphoreGive(mutex);
    return bytes;
  }

  void RmtTransport::onStatusChange(StatusCallback callback) {
    std::shared_ptr<const StatusCallback> replacement;
    if (callback) {
      replacement = std::make_shared<const StatusCallback>(std::move(callback));
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    replacement.swap(statusCallback);
    xSemaphoreGive(mutex);
  }

  void RmtTransport::onStatusChange(LegacyStatusCallback callback, void* context) {
    if (callback == nullptr) {
      onStatusChange(StatusCallback{});

    } else {
      onStatusChange([callback, context](const StatusEvent& event) {
        callback(event, context);
      });
    }
  }

  bool RmtTransport::onResponse(MessageCallback callback) {
    std::shared_ptr<const MessageCallback> replacement;
    if (callback) {
      replacement = std::make_shared<const MessageCallback>(std::move(callback));
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    replacement.swap(responseCallback);
    xSemaphoreGive(mutex);
    return true;
  }

  bool RmtTransport::onResponse(LegacyMessageCallback callback, void* context) {
    if (callback == nullptr) {
      return onResponse(MessageCallback{});
    }

    return onResponse([callback, context](const Messages::Message& message) {
      callback(message, context);
    });
  }

  bool RmtTransport::onRequest(MessageCallback callback) {
    std::shared_ptr<const MessageCallback> replacement;
    if (callback) {
      replacement = std::make_shared<const MessageCallback>(std::move(callback));
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    replacement.swap(requestCallback);
    xSemaphoreGive(mutex);
    return true;
  }

  bool RmtTransport::onRequest(LegacyMessageCallback callback, void* context) {
    if (callback == nullptr) {
      return onRequest(MessageCallback{});
    }

    return onRequest([callback, context](const Messages::Message& message) {
      callback(message, context);
    });
  }

  void RmtTransport::notifyStatus(Protocol::OperationStatus previous, const Protocol::MessageData& data) {
    if (previous == data.status) {
      return;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    const auto callback = statusCallback;
    xSemaphoreGive(mutex);
    if (callback) {
      const Messages::Message message(data);
      const StatusEvent event = {previous, data.status, message};
      (*callback)(event);
    }
  }

  void RmtTransport::notifyMessage(const Protocol::MessageData& data, bool response) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const auto callback = response ? responseCallback : requestCallback;
    xSemaphoreGive(mutex);
    if (callback) {
      const Messages::Message message(data);
      (*callback)(message);
    }
  }

  void RmtTransport::publish(Protocol::MessageData data, bool terminal, bool incoming) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (incoming && data.operation == 0 && busy) {
      xSemaphoreGive(mutex);
      if (data.hasFrame) {
        notifyMessage(data, !slave);
      }

      return;
    }

    const Protocol::OperationStatus previous = result.status;
    result = data;
    if (data.operation != 0 && data.operation == synchronousOperation) {
      synchronousResult = data;
    }

    lastError = data.error;
    if (terminal) {
      busy = false;
    }

    if (incoming && slave && data.sequence != 0) {
      request = data;
    }

    xSemaphoreGive(mutex);
    notifyStatus(previous, data);
    if (incoming && data.hasFrame) {
      notifyMessage(data, !slave);
    }
  }

  void RmtTransport::updateDriverState() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    initialized = driver.isInitialized() && !stopping;
    ready = driver.isReady();
    if (!driver.isInitialized()) {
      lastError = driver.getLastError();
    }

    xSemaphoreGive(mutex);
  }

  void RmtTransport::runTask(void* context) {
    static_cast<RmtTransport*>(context)->run();
    vTaskDelete(nullptr);
  }

  struct RmtTransport::ActiveOperation {
    Protocol::MessageData message;
    bool active = false;
    bool sent = false;
    bool txNotified = false;
    bool isResponse = false;
    uint64_t acceptedAt = 0;
  };

  void RmtTransport::run() {
    const bool started = driver.begin(rxGpio, txGpio, slave, alwaysReceive);
    updateDriverState();
    if (!started && !driver.hasResources()) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      worker = nullptr;
      xSemaphoreGive(mutex);
      xSemaphoreGive(acknowledgment);
      return;
    }

    xSemaphoreGive(acknowledgment);

    ActiveOperation operation;
    uint32_t lastResultSequence = driver.getResultSequence();
    while (true) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      const bool stop = stopping;
      xSemaphoreGive(mutex);
      if (stop && stopWorker(operation)) {
        return;
      }

      // Consume received frames before advancing the next accepted command.
      driver.poll();
      updateDriverState();
      processDriverResult(operation, lastResultSequence);
      takePendingOperation(operation);

      if (operation.active && !driver.isInitialized()) {
        operation.message.status = Protocol::OperationStatus::ERROR;
        operation.message.error = driver.getLastError();
        operation.message.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - operation.acceptedAt);
        operation.message.totalElapsedUs = operation.message.elapsedUs;
        operation.active = false;
        publish(operation.message, true);

      } else if (operation.active && !operation.sent && driver.isReady()) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        const bool current = !operation.isResponse || operation.message.sequence == requestSequence;
        xSemaphoreGive(mutex);
        if (!current) {
          operation.message.status = Protocol::OperationStatus::CANCELLED;
          operation.message.error = ESP_ERR_INVALID_STATE;
          operation.active = false;
          publish(operation.message, true);

        } else {
          if (!operation.txNotified) {
            operation.message.status = Protocol::OperationStatus::TRANSMITTING;
            operation.message.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - operation.acceptedAt);
            operation.message.totalElapsedUs = operation.message.elapsedUs;
            publish(operation.message, false);
            xSemaphoreTake(mutex, portMAX_DELAY);
            const bool stopBeforeNotify = stopping;
            xSemaphoreGive(mutex);
            if (stopBeforeNotify) {
              continue;
            }

            operation.txNotified = true;
            notifyMessage(operation.message, operation.isResponse);
          }

          // A callback can give RX time to finish. Consume it before starting TX,
          // so a newly received request can invalidate an older pending reply.
          if (driver.waitTicks() == 0) {
            continue;
          }

          xSemaphoreTake(mutex, portMAX_DELAY);
          const bool stopBeforeTransmit = stopping;
          const bool stillCurrent = !operation.isResponse || operation.message.sequence == requestSequence;
          xSemaphoreGive(mutex);
          if (stopBeforeTransmit) {
            continue;
          }

          if (!stillCurrent) {
            operation.message.status = Protocol::OperationStatus::CANCELLED;
            operation.message.error = ESP_ERR_INVALID_STATE;
            operation.active = false;
            publish(operation.message, true);
            continue;
          }

          if (driver.send(operation.message.frame)) {
            operation.sent = true;

          } else {
            operation.message.status = Protocol::OperationStatus::ERROR;
            operation.message.error = driver.getLastError();
            operation.message.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - operation.acceptedAt);
            operation.message.totalElapsedUs = operation.message.elapsedUs;
            operation.active = false;
            publish(operation.message, true);
          }
        }
      }

      if (operation.active && operation.sent && !operation.isResponse
        && driver.getState() != RmtDriver::State::REQUEST_SENDING
        && operation.message.status == Protocol::OperationStatus::TRANSMITTING) {
        operation.message.status = Protocol::OperationStatus::WAITING;
        operation.message.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - operation.acceptedAt);
        operation.message.totalElapsedUs = operation.message.elapsedUs;
        publish(operation.message, false);
      }

      // ISR events and new commands use the same task notification. The queue
      // retains every RMT event; a notification is only a wake-up hint.
      ulTaskNotifyTake(pdTRUE, started ? driver.waitTicks() : portMAX_DELAY);
    }
  }

  bool RmtTransport::stopWorker(ActiveOperation& operation) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const bool cancelPending = pending;
    Protocol::MessageData cancelledPending = pendingCommand;
    pending = false;
    xSemaphoreGive(mutex);
    if (operation.active) {
      Protocol::MessageData cancelled = operation.message;
      cancelled.status = Protocol::OperationStatus::CANCELLED;
      cancelled.error = ESP_ERR_INVALID_STATE;
      cancelled.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - operation.acceptedAt);
      cancelled.totalElapsedUs = cancelled.elapsedUs;
      publish(cancelled, true);
      operation.active = false;
    }

    if (cancelPending) {
      notifyStatus(queuedPrevious, cancelledPending);
      cancelledPending.status = Protocol::OperationStatus::CANCELLED;
      cancelledPending.error = ESP_ERR_INVALID_STATE;
      cancelledPending.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - acceptedAt);
      cancelledPending.totalElapsedUs = cancelledPending.elapsedUs;
      publish(cancelledPending, true);
    }

    const bool stopped = driver.end();
    xSemaphoreTake(mutex, portMAX_DELAY);
    initialized = false;
    ready = false;
    stopping = false;
    lastError = driver.getLastError();
    if (stopped) {
      worker = nullptr;
    }

    xSemaphoreGive(mutex);
    xSemaphoreGive(acknowledgment);
    return stopped;
  }

  void RmtTransport::processDriverResult(ActiveOperation& operation, uint32_t& lastResultSequence) {
    if (lastResultSequence == driver.getResultSequence()) {
      return;
    }

    lastResultSequence = driver.getResultSequence();
    const bool masterRequestFinished = !slave && operation.sent;

    // A slave reply also finishes on a fatal error or a TX watchdog timeout.
    const bool slaveResponseFinished = slave && (driver.resultIsTransmit() || !driver.isInitialized()
      || (operation.sent && driver.getResultStatus() == RmtDriver::ResultStatus::NONE));
    const bool outgoingResult = operation.active && (masterRequestFinished || slaveResponseFinished);
    Protocol::MessageData completed = outgoingResult ? operation.message : Protocol::MessageData{};
    completed.frame = slave && outgoingResult ? operation.message.frame : driver.getFrame();
    completed.hasFrame = driver.hasFrame();
    completed.error = driver.getLastError();
    completed.elapsedUs = outgoingResult
      ? static_cast<uint32_t>(esp_timer_get_time() - operation.acceptedAt) : 0;
    completed.totalElapsedUs = completed.elapsedUs;
    const RmtDriver::ResultStatus status = driver.getResultStatus();
    if (status == RmtDriver::ResultStatus::SUCCESS) {
      completed.status = Protocol::OperationStatus::SUCCESS;

    } else if (status == RmtDriver::ResultStatus::INVALID) {
      completed.status = Protocol::OperationStatus::INVALID;

    } else if (status == RmtDriver::ResultStatus::TIMEOUT) {
      completed.status = Protocol::OperationStatus::TIMEOUT;

    } else {
      completed.status = Protocol::OperationStatus::ERROR;
    }

    if (!slave && completed.status == Protocol::OperationStatus::SUCCESS) {

      // Every valid reply, including a negative reply, must match an active
      // request's ID. Passive frames are not compared to an old request.
      if (outgoingResult
        && Protocol::Frame::getId(completed.frame) != Protocol::Frame::getId(operation.message.frame)) {
        completed.status = Protocol::OperationStatus::INVALID;

      } else if (Protocol::Frame::getType(completed.frame) == Protocol::MessageType::DATA_INVALID) {
        completed.status = Protocol::OperationStatus::DATA_INVALID;

      } else if (Protocol::Frame::getType(completed.frame) == Protocol::MessageType::UNKNOWN_DATA_ID) {
        completed.status = Protocol::OperationStatus::UNKNOWN_DATA_ID;
      }
    }

    if (slave && !driver.isInitialized() && !outgoingResult) {
      // This is a transport failure, not a newly received master request.
      // A command accepted just before the failure is completed below.
      xSemaphoreTake(mutex, portMAX_DELAY);
      const bool reserved = busy;
      xSemaphoreGive(mutex);
      if (!reserved) {
        publish(completed, true);
      }

    } else if (slave && !outgoingResult && !driver.resultIsTransmit()) {
      processSlaveRequest(completed, operation);

    } else if (outgoingResult) {
      operation.active = false;
      publish(completed, true, !slave);

    } else {
      // An unsolicited frame must not replace an accepted outgoing command.
      // It remains observable via onResponse while that command waits for MWT.
      publish(completed, true, true);
    }
  }

  void RmtTransport::processSlaveRequest(Protocol::MessageData& received, ActiveOperation& operation) {
    if (received.status == Protocol::OperationStatus::SUCCESS
      || (acceptInvalidRequests && received.hasFrame)) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      requestSequence++;
      if (requestSequence == 0) {
        requestSequence++;
      }

      received.sequence = requestSequence;
      const bool superseded = pending;
      Protocol::MessageData cancelled = pendingCommand;
      pending = false;
      xSemaphoreGive(mutex);

      // By default only valid requests supersede a reply. A transparent
      // proxy may opt in to complete invalid frames, retaining every bit.
      if (operation.active) {
        operation.message.status = Protocol::OperationStatus::CANCELLED;
        operation.message.error = ESP_ERR_INVALID_STATE;
        operation.message.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - operation.acceptedAt);
        operation.message.totalElapsedUs = operation.message.elapsedUs;
        publish(operation.message, true);
        operation.active = false;
      }

      if (superseded) {
        notifyStatus(queuedPrevious, cancelled);
        cancelled.status = Protocol::OperationStatus::CANCELLED;
        cancelled.error = ESP_ERR_INVALID_STATE;
        cancelled.elapsedUs = static_cast<uint32_t>(esp_timer_get_time() - acceptedAt);
        cancelled.totalElapsedUs = cancelled.elapsedUs;
        publish(cancelled, true);
      }

      received.request = received.frame;
      if (received.status == Protocol::OperationStatus::SUCCESS) {
        received.status = Protocol::OperationStatus::REQUEST_RECEIVED;
      }
    }

    publish(received, false, true);
  }

  void RmtTransport::takePendingOperation(ActiveOperation& operation) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (!operation.active && pending) {
      operation.message = pendingCommand;
      operation.isResponse = pendingResponse;
      operation.acceptedAt = acceptedAt;
      pending = false;
      operation.active = true;
      operation.sent = false;
      operation.txNotified = false;
      const Protocol::OperationStatus previous = queuedPrevious;
      xSemaphoreGive(mutex);
      notifyStatus(previous, operation.message);

    } else {
      xSemaphoreGive(mutex);
    }
  }

}
