/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>

// Master is the default role: requests go from the ESP32 to the boiler.
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
static uint32_t previousRequestAt = 0;

// The internal worker invokes this callback. Do not retain a reference to event.message.
static void onStatusChange(const ESP32OT::Transport::StatusEvent& event) {
  const auto& result = event.message;

  // Read the data after the exchange completes.
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

  if (result.isSuccess()) {

    // Only ID0 is requested here. as<T>() creates a value copy without checking the received ID.
    auto status = result.as<ESP32OT::Messages::Status>();
    Serial.printf("Flame: %u\n", static_cast<unsigned int>(status.isFlameOn()));
  }
}

void setup() {
  Serial.begin(115200);
  boiler.onStatusChange(onStatusChange);
}

void loop() {

  // ESP32OT initialization and a successful boiler response are different states.
  if (!boiler.isInitialized()) {
    if (!boiler.begin()) {
      Serial.printf("OpenTherm initialization failed: %ld\n", static_cast<long>(boiler.getLastError()));
      delay(1000);
      return;
    }
  }

  const uint32_t now = millis();

  // isReady() accounts for the exchange and the gap between frames; no separate flag is needed.
  if (boiler.isReady() && now - previousRequestAt >= 750) {

    // Zero flags do not request heating. Async sends make one attempt;
    // true means that the command was accepted, not that a response was received.
    if (boiler.sendRequestAsync(ESP32OT::Messages::Status())) {
      previousRequestAt = now;
    }
  }

  // The worker communicates independently of loop(); there is no process() call.
  delay(1);
}
