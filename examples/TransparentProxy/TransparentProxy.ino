/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>

// Each GPIO can be overridden separately, for example with -DMASTER_RX_GPIO=4.
#ifndef MASTER_RX_GPIO
  #define MASTER_RX_GPIO 4
#endif
#ifndef MASTER_TX_GPIO
  #define MASTER_TX_GPIO 5
#endif
#ifndef SLAVE_RX_GPIO
  #if CONFIG_IDF_TARGET_ESP32

    // On typical original ESP32 modules, GPIO6/7 are connected to flash.
    #define SLAVE_RX_GPIO 18
  #else
    #define SLAVE_RX_GPIO 6
  #endif
#endif
#ifndef SLAVE_TX_GPIO
  #if CONFIG_IDF_TARGET_ESP32
    #define SLAVE_TX_GPIO 19
  #else
    #define SLAVE_TX_GPIO 7
  #endif
#endif

// Two electrical OpenTherm interfaces are required. The boiler side defaults to
// master; true as the third argument makes the thermostat side a slave.
static ESP32OT::ESP32OT boiler(
  static_cast<gpio_num_t>(MASTER_RX_GPIO), static_cast<gpio_num_t>(MASTER_TX_GPIO)
);
static ESP32OT::ESP32OT thermostat(
  static_cast<gpio_num_t>(SLAVE_RX_GPIO), static_cast<gpio_num_t>(SLAVE_TX_GPIO), true
);

static void logFrame(const char* direction, uint32_t frame, ESP32OT::OperationStatus status) {
  Serial.printf(
    "%s: %08lX (%s)\n",
    direction,
    static_cast<unsigned long>(frame),
    ESP32OT::Protocol::Frame::statusToString(status)
  );
}

static void onThermostatRequest(const ESP32OT::Messages::Message& request) {
  logFrame("Thermostat -> ESP", request.getRaw(), request.getStatus());

  // Pass the complete raw frame and request context without repairing parity.
  // Async copies the message before returning and makes one attempt.
  if (!boiler.sendRequestAsync(request)) {

    // No queue or retries are added: the thermostat decides whether to retry.
    Serial.println("Boiler request not accepted (busy or stopped)");
  }
}

static void onBoilerResponse(const ESP32OT::Messages::Message& response) {
  logFrame("Boiler -> ESP", response.getRaw(), response.getStatus());

  // Forward complete negative or invalid responses too. The sequence links the
  // response to its original request; the master does not acknowledge a response.
  if (!thermostat.sendResponseAsync(response)) {

    // An old response must not answer a request that has already been replaced.
    Serial.println("Thermostat response not accepted (stale request or unavailable bus)");
  }
}

static void onBoilerStatus(const ESP32OT::ESP32OT::StatusEvent& event) {

  // A TIMEOUT without a frame produces a status event, not a fabricated response.
  Serial.printf("Boiler status: %s\n", ESP32OT::Protocol::Frame::statusToString(event.current));
}

static void onThermostatStatus(const ESP32OT::ESP32OT::StatusEvent& event) {

  // SUCCESS on this side means TX completed, not that the thermostat acknowledged it.
  Serial.printf("Thermostat status: %s\n", ESP32OT::Protocol::Frame::statusToString(event.current));
}

void setup() {
  Serial.begin(115200);

  // getConfig() returns a const reference; auto creates a mutable copy here.
  // Preserve the GPIOs and slave role selected by the constructor.
  auto thermostatConfig = thermostat.getConfig();

  // The proxy also forwards full frames with invalid parity or type. This setting
  // does not turn a partial capture into a complete message.
  thermostatConfig.acceptInvalidRequests = true;
  thermostat.setConfig(thermostatConfig);

  // Forwarding runs immediately in the two workers' callbacks, not through loop().
  boiler.onResponse(onBoilerResponse);
  thermostat.onRequest(onThermostatRequest);
  boiler.onRequest([](const ESP32OT::Messages::Message& request) {
    logFrame("ESP -> Boiler", request.getRaw(), request.getStatus());
  });
  thermostat.onResponse([](const ESP32OT::Messages::Message& response) {
    logFrame("ESP -> Thermostat", response.getRaw(), response.getStatus());
  });
  boiler.onStatusChange(onBoilerStatus);
  thermostat.onStatusChange(onThermostatStatus);

  // Prepare the boiler side first. begin() starts RMT and the worker; only an exchange
  // confirms that the boiler is present. Then start listening to the thermostat.
  if (!boiler.begin()) {
    Serial.printf("Boiler bus begin failed: %ld\n", static_cast<long>(boiler.getLastError()));
    return;
  }

  if (!thermostat.begin()) {
    Serial.printf("Thermostat bus begin failed: %ld\n", static_cast<long>(thermostat.getLastError()));
    boiler.end();
    return;
  }
}

void loop() {

  // Both workers keep receiving and forwarding frames during delay().
  delay(1000);
}
