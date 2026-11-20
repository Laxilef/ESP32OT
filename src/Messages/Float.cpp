/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include "Float.h"
#include <math.h>

namespace ESP32OT::Messages {

  float Float::get() const {
    const uint16_t value = getData();
    return (value & 0x8000) ? -(0x10000L - value) / 256.0f : value / 256.0f;
  }

  uint16_t Float::encode(float value) {
    // Limit only the representable signed f8.8 range; truncation matches legacy writes.
    if (!isfinite(value)) {
      return 0;
    }

    if (value < -128.0f) {
      value = -128.0f;

    } else if (value > 32767.0f / 256.0f) {
      value = 32767.0f / 256.0f;
    }

    return static_cast<uint16_t>(static_cast<int32_t>(value * 256.0f));
  }

  Float Float::read(Protocol::MessageID id) {
    return Float(id);
  }

  Float Float::write(Protocol::MessageID id, float value) {
    return Float(id, value);
  }

  Float Float::reply(Protocol::MessageType type, float value) const {
    return Float(createReply(type, encode(value)));
  }

  Float Float::writeTemperature(Protocol::MessageID id, float temperature) {
    if (temperature < 0) {
      temperature = 0;

    } else if (temperature > 100) {
      temperature = 100;
    }

    return write(id, temperature);
  }

}
