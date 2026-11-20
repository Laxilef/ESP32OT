/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>

// A simulated boiler with no heater control. The third argument, true, selects
// slave mode: the connected room thermostat is the master.
#ifndef SLAVE_RX_GPIO
  #define SLAVE_RX_GPIO 4
#endif
#ifndef SLAVE_TX_GPIO
  #define SLAVE_TX_GPIO 5
#endif
static ESP32OT::ESP32OT slave(
  static_cast<gpio_num_t>(SLAVE_RX_GPIO), static_cast<gpio_num_t>(SLAVE_TX_GPIO), true
);

// Only the internal OpenTherm worker reads and changes this variable.
static float requestedSetpoint = 55.0f;

static void logFrame(const char* direction, uint32_t frame, ESP32OT::OperationStatus status) {
  Serial.printf(
    "%s: %08lX (%s)\n",
    direction,
    static_cast<unsigned long>(frame),
    ESP32OT::Protocol::Frame::statusToString(status)
  );
}

static void onRequest(const ESP32OT::Messages::Message& request) {

  // onRequest receives a full frame, including frames with invalid parity or type.
  // This slave replies only to valid READ_DATA or WRITE_DATA requests.
  logFrame("Thermostat -> ESP", request.getRaw(), request.getStatus());
  if (request.getStatus() != ESP32OT::OperationStatus::REQUEST_RECEIVED) {
    return;
  }

  const ESP32OT::MessageType type = request.getType();

  // as<T>() copies the data into the selected wrapper; choose its format by ID.
  // sendResponseAsync() submits the reply to the worker without waiting. The library
  // handles the delay before TX; the master does not acknowledge a response.
  switch (request.getId()) {
    case ESP32OT::MessageID::Status: {
      if (type == ESP32OT::MessageType::READ_DATA) {
        auto status = request.as<ESP32OT::Messages::Status>();

        // reply() echoes the master flags in HB; LB = 0 reports no flame or fault.
        slave.sendResponseAsync(status.reply(0));

      } else {

        // The ID is known, but this example does not support writing Status.
        slave.sendResponseAsync(request.reply(ESP32OT::MessageType::DATA_INVALID, 0));
      }
      break;
    }

    case ESP32OT::MessageID::TSet: {
      auto setpoint = request.as<ESP32OT::Messages::Float>();
      if (type == ESP32OT::MessageType::WRITE_DATA) {

        // get() decodes f8.8; reply() encodes the response and preserves its request context.
        requestedSetpoint = setpoint.get();
        slave.sendResponseAsync(setpoint.reply(ESP32OT::MessageType::WRITE_ACK, requestedSetpoint));

      } else {
        slave.sendResponseAsync(setpoint.reply(ESP32OT::MessageType::READ_ACK, requestedSetpoint));
      }
      break;
    }

    case ESP32OT::MessageID::Tboiler: {
      if (type == ESP32OT::MessageType::READ_DATA) {
        auto temperature = request.as<ESP32OT::Messages::Float>();

        // A fixed simulated measurement; Float::reply() builds the f8.8 payload.
        slave.sendResponseAsync(temperature.reply(ESP32OT::MessageType::READ_ACK, 35.5f));

      } else {
        slave.sendResponseAsync(request.reply(ESP32OT::MessageType::DATA_INVALID, 0));
      }
      break;
    }

    case ESP32OT::MessageID::RelModLevel: {
      if (type == ESP32OT::MessageType::READ_DATA) {

        // For 0 percent, the f8.8 payload is also raw uint16_t(0); no wrapper is needed.
        slave.sendResponseAsync(request.reply(ESP32OT::MessageType::READ_ACK, 0));

      } else {
        slave.sendResponseAsync(request.reply(ESP32OT::MessageType::DATA_INVALID, 0));
      }
      break;
    }

    default: {

      // UNKNOWN_DATA_ID means an unsupported ID; DATA_INVALID above means
      // an unsupported operation for a known ID.
      slave.sendResponseAsync(request.reply(ESP32OT::MessageType::UNKNOWN_DATA_ID, 0));
      break;
    }
  }
}

static void onStatusChange(const ESP32OT::Transport::StatusEvent& event) {

  // Status events also report errors without a full frame; onRequest is not called then.
  Serial.printf("Thermostat status: %s\n", ESP32OT::Protocol::Frame::statusToString(event.current));
}

void setup() {
  Serial.begin(115200);

  // The internal worker runs these handlers independently of the frequency of loop().
  slave.onRequest(onRequest);
  slave.onResponse([](const ESP32OT::Messages::Message& response) {
    logFrame("ESP -> Thermostat", response.getRaw(), response.getStatus());
  });
  slave.onStatusChange(onStatusChange);

  // begin() starts the driver; an incoming request confirms that a thermostat is present.
  if (!slave.begin()) {
    Serial.printf("OpenTherm begin failed: %ld\n", static_cast<long>(slave.getLastError()));
  }
}

void loop() {

  // The worker continues receiving requests and sending replies during this delay.
  delay(1000);
}
