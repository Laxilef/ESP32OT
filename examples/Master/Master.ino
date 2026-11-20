/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>

// Master is the default role: the ESP32 sends requests to the boiler.
// Select the GPIOs of your OpenTherm interface with the build flags below.
#ifndef MASTER_RX_GPIO
  #define MASTER_RX_GPIO 4
#endif
#ifndef MASTER_TX_GPIO
  #define MASTER_TX_GPIO 5
#endif
static ESP32OT::ESP32OT boiler(
  static_cast<gpio_num_t>(MASTER_RX_GPIO), static_cast<gpio_num_t>(MASTER_TX_GPIO)
);
static const bool HEATING_ENABLED = false;
static const float HEATING_SETPOINT_C = 55.0f;

static void logResult(const ESP32OT::Messages::Message& result) {

  // An operation can finish without a complete response: timeout, error or a partial frame.
  // hasFrame() allows getRaw() to be logged as a response only when a full frame was received.
  // A complete frame is not necessarily successful; check isSuccess() before using its payload.
  if (result.hasFrame()) {
    Serial.printf(
      "Request: %08lX, response: %08lX, status: %s\n",
      static_cast<unsigned long>(result.getRequest()),
      static_cast<unsigned long>(result.getRaw()),
      ESP32OT::Protocol::Frame::statusToString(result.getStatus())
    );

  } else {
    Serial.printf(
      "Request: %08lX, response: none, status: %s\n",
      static_cast<unsigned long>(result.getRequest()),
      ESP32OT::Protocol::Frame::statusToString(result.getStatus())
    );
  }
}

void setup() {
  Serial.begin(115200);
}

void loop() {

  // ESP32OT initialization does not yet mean that the boiler responds to requests.
  if (!boiler.isInitialized()) {
    if (!boiler.begin()) {
      Serial.printf("OpenTherm initialization failed: %ld\n", static_cast<long>(boiler.getLastError()));
      delay(1000);
      return;
    }
  }

  ESP32OT::Messages::Status request;
  request.setCentralHeatingEnabled(HEATING_ENABLED);

  // The synchronous call waits for a response; Config::maxAttempts sets the attempt limit (default: 5).
  // The library handles reception and the gaps between frames.
  auto status = boiler.sendRequest(request);
  logResult(status);

  // This example reads the payload after a successful exchange.
  if (status.isSuccess()) {
    Serial.printf(
      "Flame: %u, heating active: %u\n",
      static_cast<unsigned int>(status.isFlameOn()),
      static_cast<unsigned int>(status.isCentralHeatingActive())
    );

    // Float(ID, value) builds a signed f8.8 write request, here TSet = 55 C.
    auto setpoint = boiler.sendRequest(
      ESP32OT::Messages::Float(ESP32OT::MessageID::TSet, HEATING_SETPOINT_C)
    );
    logResult(setpoint);

    // Float(ID) builds a read request; get() decodes the received f8.8 value as float.
    auto temperature = boiler.sendRequest(ESP32OT::Messages::Float(ESP32OT::MessageID::Tboiler));
    logResult(temperature);
    if (temperature.isSuccess()) {
      Serial.printf("Flow temperature: %.2f C\n", temperature.get());
    }
  }

  delay(250);
}
