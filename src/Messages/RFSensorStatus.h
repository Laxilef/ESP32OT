/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#pragma once

#include "Message.h"

namespace ESP32OT::Messages {

  // ID98: sensor type/index in HB, battery/signal strength in LB (spec p.40).
  class RFSensorStatus final : public Message {
  public:
    enum class SensorType : uint8_t {
      ROOM_CONTROLLER = 0,
      ROOM_SENSOR = 1,
      OUTSIDE_SENSOR = 2,
      UNDEFINED = 15
    };

    enum class BatteryStatus : uint8_t {
      NO_INDICATION = 0,
      LOW_BATTERY = 1,
      NEARLY_LOW = 2,
      NORMAL = 3
    };

    RFSensorStatus() : Message(createData(
      Protocol::MessageType::WRITE_DATA, Protocol::MessageID::RFsensorStatusInformation, 0
    )) {}

    explicit RFSensorStatus(const Protocol::MessageData& snapshot) : Message(snapshot) {}
    explicit RFSensorStatus(const Message& message) : Message(message.getMessageData()) {}

    RFSensorStatus(
      SensorType sensorType, uint8_t sensorIndex, BatteryStatus batteryStatus, uint8_t signalStrength,
      Protocol::MessageType type = Protocol::MessageType::WRITE_DATA
    ) : Message(createData(
      type, Protocol::MessageID::RFsensorStatusInformation,
      encode(sensorType, sensorIndex, batteryStatus, signalStrength)
    )) {}

    SensorType getSensorType() const {
      return static_cast<SensorType>(getHighByte() >> 4);
    }

    uint8_t getSensorIndex() const {
      return getHighByte() & 0x0f;
    }

    BatteryStatus getBatteryStatus() const {
      return static_cast<BatteryStatus>(getLowByte() & 0x03);
    }

    // 0 = unavailable; 1 = weak/lost signal, 5 = perfect signal.
    uint8_t getSignalStrength() const {
      return (getLowByte() >> 2) & 0x07;
    }

    RFSensorStatus reply(
      Protocol::MessageType type,
      SensorType sensorType, uint8_t sensorIndex, BatteryStatus batteryStatus, uint8_t signalStrength
    ) const {
      return RFSensorStatus(createReply(type, encode(sensorType, sensorIndex, batteryStatus, signalStrength)));
    }

    static uint16_t encode(
      SensorType sensorType, uint8_t sensorIndex, BatteryStatus batteryStatus, uint8_t signalStrength
    ) {
      // Mask field widths only; newly built frames leave reserved LB bits 7..5 clear.
      return ((static_cast<uint16_t>(sensorType) & 0x0f) << 12)
        | ((static_cast<uint16_t>(sensorIndex) & 0x0f) << 8)
        | ((static_cast<uint16_t>(signalStrength) & 0x07) << 2)
        | (static_cast<uint16_t>(batteryStatus) & 0x03);
    }
  };

}
