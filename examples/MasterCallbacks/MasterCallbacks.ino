/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>

// Synchronous requests with results handled in callbacks.
// Master is the default role; select the GPIOs of your OpenTherm interface below.
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

// The worker invokes this callback; event.message is a reference valid only during the call.
static void onStatusChange(const ESP32OT::Transport::StatusEvent& event) {
  const auto& result = event.message;

  // Handle the completion of each attempt, skipping intermediate statuses.
  if (!result.isComplete()) {
    return;
  }

  // Completion is also reported without a full response: timeout, error or a partial frame.
  // hasFrame() distinguishes a received full frame from a result without a response frame.
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

  // Log errors, but do not interpret their payload as a measurement.
  if (!result.isSuccess()) {
    return;
  }

  if (result.getId() == ESP32OT::MessageID::Status) {

    // as<T>() creates a value copy in the selected format; the application checks the actual ID.
    auto status = result.as<ESP32OT::Messages::Status>();
    Serial.printf(
      "Flame: %u, heating active: %u\n",
      static_cast<unsigned int>(status.isFlameOn()),
      static_cast<unsigned int>(status.isCentralHeatingActive())
    );

  } else if (result.getId() == ESP32OT::MessageID::Tboiler) {
    auto temperature = result.as<ESP32OT::Messages::Float>();
    Serial.printf("Flow temperature: %.2f C\n", temperature.get());
  }
}

void setup() {
  Serial.begin(115200);
  boiler.onRequest([count = uint32_t{0}](const ESP32OT::Messages::Message& request) mutable {
    count++;
    Serial.printf(
      "Request attempt %lu: %08lX\n",
      static_cast<unsigned long>(count),
      static_cast<unsigned long>(request.getRaw())
    );
  });
  boiler.onStatusChange(onStatusChange);
}

void loop() {

  // begin() starts the driver; responses confirm that a boiler is present.
  if (!boiler.isInitialized()) {
    if (!boiler.begin()) {
      Serial.printf("OpenTherm initialization failed: %ld\n", static_cast<long>(boiler.getLastError()));
      delay(1000);
      return;
    }
  }

  ESP32OT::Messages::Status request;
  request.setCentralHeatingEnabled(HEATING_ENABLED);

  // sendRequest() waits for a response and handles the gaps; Config::maxAttempts defaults to 5.
  // The callback already handles the response; this code uses only the request outcome.
  auto status = boiler.sendRequest(request);

  if (status.isSuccess()) {

    // With a value, Float builds WRITE_DATA f8.8; without a value, it builds READ_DATA.
    boiler.sendRequest(
      ESP32OT::Messages::Float(ESP32OT::MessageID::TSet, HEATING_SETPOINT_C)
    );
    boiler.sendRequest(ESP32OT::Messages::Float(ESP32OT::MessageID::Tboiler));
  }

  delay(250);
}
