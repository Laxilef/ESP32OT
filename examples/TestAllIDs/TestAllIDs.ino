/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>
#include <atomic>

#ifndef MASTER_RX_GPIO
  #define MASTER_RX_GPIO 4
#endif

#ifndef MASTER_TX_GPIO
  #define MASTER_TX_GPIO 5
#endif

#ifndef TEST_ALL_IDS_ENABLE_WRITES
  #define TEST_ALL_IDS_ENABLE_WRITES 0
#endif

#if TEST_ALL_IDS_ENABLE_WRITES != 0 && TEST_ALL_IDS_ENABLE_WRITES != 1
  #error "TEST_ALL_IDS_ENABLE_WRITES must be 0 or 1"
#endif

namespace Messages = ESP32OT::Messages;

using ID = ESP32OT::MessageID;
using Type = ESP32OT::MessageType;
using Status = ESP32OT::OperationStatus;
using Message = Messages::Message;

enum class Access : uint8_t { ReadOnly, WriteOnly, ReadWrite };
enum class ReadMode : uint8_t { Single, Count, Indexed };
enum class Phase : uint8_t { Read, Write, Report };

struct IDSpec {
  ID id;
  const char* name;
  Access access;
  void (*printData)(const Message&);
  ReadMode readMode = ReadMode::Single;
};

struct IDResult {
  Message response{};
  bool valid = false;
  bool readKnown = false;
  bool writeKnown = false;
  Status readStatus = Status::NONE;
  Status writeStatus = Status::NONE;
};

struct WriteValue {
  bool enabled;
  Message first;
  Message second;
};

static bool isAck(const Message& message, Type type);
static Message sendRequest(const char* action, const IDSpec& item, const Message& request);
static void countStatus(const ESP32OT::ESP32OT::StatusEvent& event);
static bool writeForbidden(ID id);
static const IDSpec* findIDSpec(ID id);
static bool prepareWriteValues(const WriteValue& configured, Message& first, Message& second);
static const char* accessName(Access access);
static const char* observedMode(const IDSpec& item);
static void printSummary();
static void printBrandCharacter(const Messages::Message& result);
static void printCapacity(const Messages::Message& result);
static void printDate(const Messages::Message& result);
static void printDayTime(const Messages::Message& result);
static void printFanSpeed(const Messages::Message& result);
static void printFault(const Messages::Message& result);
static void printFloat(const Messages::Message& result);
static void printIndexedValue(const Messages::Message& result);
static void printInt16(const Messages::Message& result);
static void printNominalVentilation(const Messages::Message& result);
static void printParameterCount(const Messages::Message& result);
static void printProductVersion(const Messages::Message& result);
static void printRemoteOverrideFunction(const Messages::Message& result);
static void printRemoteOverrideMode(const Messages::Message& result);
static void printRemoteParameters(const Messages::Message& result);
static void printSlaveConfig(const Messages::Message& result);
static void printSolarStorageConfig(const Messages::Message& result);
static void printSolarStorageFault(const Messages::Message& result);
static void printSolarStorageStatus(const Messages::Message& result);
static void printStatus(const Messages::Message& result);
static void printTemperatureBounds(const Messages::Message& result);
static void printUInt16(const Messages::Message& result);
static void printUInt8(const Messages::Message& result);
static void printVentilationConfig(const Messages::Message& result);
static void printVentilationFault(const Messages::Message& result);
static void printVentilationRemoteParameters(const Messages::Message& result);
static void printVentilationStatus(const Messages::Message& result);
static void printMasterConfig(const Message& result);
static void printRemoteRequest(const Message& result);
static void printRFSensorStatus(const Message& result);

// Protocol access; observed device support is printed separately.
static const IDSpec idSpecs[] = {
  {ID::Status, "Status", Access::ReadOnly, printStatus, ReadMode::Single},
  {ID::TSet, "TSet", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::MConfigMMemberIDcode, "MConfigMMemberIDcode", Access::WriteOnly, printMasterConfig, ReadMode::Single},
  {ID::SConfigSMemberIDcode, "SConfigSMemberIDcode", Access::ReadOnly, printSlaveConfig, ReadMode::Single},
  {ID::RemoteRequest, "RemoteRequest", Access::WriteOnly, printRemoteRequest, ReadMode::Single},
  {ID::ASFflags, "ASFflags", Access::ReadOnly, printFault, ReadMode::Single},
  {ID::RBPflags, "RBPflags", Access::ReadOnly, printRemoteParameters, ReadMode::Single},
  {ID::CoolingControl, "CoolingControl", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::TsetCH2, "TsetCH2", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::TrOverride, "TrOverride", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::TSP, "TSP", Access::ReadOnly, printParameterCount, ReadMode::Count},
  {ID::TSPindexTSPvalue, "TSPindexTSPvalue", Access::ReadWrite, printIndexedValue, ReadMode::Indexed},
  {ID::FHBsize, "FHBsize", Access::ReadOnly, printParameterCount, ReadMode::Count},
  {ID::FHBindexFHBvalue, "FHBindexFHBvalue", Access::ReadOnly, printIndexedValue, ReadMode::Indexed},
  {ID::MaxRelModLevelSetting, "MaxRelModLevelSetting", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::MaxCapacityMinModLevel, "MaxCapacityMinModLevel", Access::ReadOnly, printCapacity, ReadMode::Single},
  {ID::TrSet, "TrSet", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::RelModLevel, "RelModLevel", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::CHPressure, "CHPressure", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::DHWFlowRate, "DHWFlowRate", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::DayTime, "DayTime", Access::ReadWrite, printDayTime, ReadMode::Single},
  {ID::Date, "Date", Access::ReadWrite, printDate, ReadMode::Single},
  {ID::Year, "Year", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::TrSetCH2, "TrSetCH2", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::Tr, "Tr", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::Tboiler, "Tboiler", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Tdhw, "Tdhw", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Toutside, "Toutside", Access::ReadWrite, printFloat, ReadMode::Single},
  {ID::Tret, "Tret", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Tstorage, "Tstorage", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Tcollector, "Tcollector", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::TflowCH2, "TflowCH2", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Tdhw2, "Tdhw2", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Texhaust, "Texhaust", Access::ReadOnly, printInt16, ReadMode::Single},
  {ID::TboilerHeatExchanger, "TboilerHeatExchanger", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::BoilerFanSpeedSetpointAndActual, "BoilerFanSpeedSetpointAndActual", Access::ReadOnly, printFanSpeed, ReadMode::Single},
  {ID::FlameCurrent, "FlameCurrent", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::TrCH2, "TrCH2", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::RelativeHumidity, "RelativeHumidity", Access::ReadWrite, printFloat, ReadMode::Single},
  {ID::TrOverride2, "TrOverride2", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::TdhwSetUBTdhwSetLB, "TdhwSetUBTdhwSetLB", Access::ReadOnly, printTemperatureBounds, ReadMode::Single},
  {ID::MaxTSetUBMaxTSetLB, "MaxTSetUBMaxTSetLB", Access::ReadOnly, printTemperatureBounds, ReadMode::Single},
  {ID::TdhwSet, "TdhwSet", Access::ReadWrite, printFloat, ReadMode::Single},
  {ID::MaxTSet, "MaxTSet", Access::ReadWrite, printFloat, ReadMode::Single},
  {ID::StatusVentilationHeatRecovery, "StatusVentilationHeatRecovery", Access::ReadOnly, printVentilationStatus, ReadMode::Single},
  {ID::Vset, "Vset", Access::WriteOnly, printUInt8, ReadMode::Single},
  {ID::ASFflagsOEMfaultCodeVentilationHeatRecovery, "VentilationFaults", Access::ReadOnly, printVentilationFault, ReadMode::Single},
  {ID::OEMDiagnosticCodeVentilationHeatRecovery, "VentilationDiagnosticCode", Access::ReadOnly, printUInt16, ReadMode::Single},
  {ID::SConfigSMemberIDCodeVentilationHeatRecovery, "VentilationConfig", Access::ReadOnly, printVentilationConfig, ReadMode::Single},
  {ID::OpenThermVersionVentilationHeatRecovery, "VentilationOpenThermVersion", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::VentilationHeatRecoveryVersion, "VentilationHeatRecoveryVersion", Access::ReadOnly, printProductVersion, ReadMode::Single},
  {ID::RelVentLevel, "RelVentLevel", Access::ReadOnly, printUInt8, ReadMode::Single},
  {ID::RHexhaust, "RHexhaust", Access::ReadWrite, printUInt8, ReadMode::Single},
  {ID::CO2exhaust, "CO2exhaust", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::Tsi, "Tsi", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Tso, "Tso", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Tei, "Tei", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::Teo, "Teo", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::RPMexhaust, "RPMexhaust", Access::ReadOnly, printUInt16, ReadMode::Single},
  {ID::RPMsupply, "RPMsupply", Access::ReadOnly, printUInt16, ReadMode::Single},
  {ID::RBPflagsVentilationHeatRecovery, "VentilationRBPflags", Access::ReadOnly, printVentilationRemoteParameters, ReadMode::Single},
  {ID::NominalVentilationValue, "NominalVentilationValue", Access::ReadWrite, printNominalVentilation, ReadMode::Single},
  {ID::TSPventilationHeatRecovery, "TSPventilationHeatRecovery", Access::ReadOnly, printParameterCount, ReadMode::Count},
  {ID::TSPindexTSPvalueVentilationHeatRecovery, "VentilationTSPindexTSPvalue", Access::ReadWrite, printIndexedValue, ReadMode::Indexed},
  {ID::FHBsizeVentilationHeatRecovery, "FHBsizeVentilationHeatRecovery", Access::ReadOnly, printParameterCount, ReadMode::Count},
  {ID::FHBindexFHBvalueVentilationHeatRecovery, "VentilationFHBindexFHBvalue", Access::ReadOnly, printIndexedValue, ReadMode::Indexed},
  {ID::Brand, "Brand", Access::ReadOnly, printBrandCharacter, ReadMode::Single},
  {ID::BrandVersion, "BrandVersion", Access::ReadOnly, printBrandCharacter, ReadMode::Single},
  {ID::BrandSerialNumber, "BrandSerialNumber", Access::ReadOnly, printBrandCharacter, ReadMode::Single},
  {ID::CoolingOperationHours, "CoolingOperationHours", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::PowerCycles, "PowerCycles", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::RFsensorStatusInformation, "RFsensorStatusInformation", Access::WriteOnly, printRFSensorStatus, ReadMode::Single},
  {ID::RemoteOverrideOperatingModeHeatingDHW, "RemoteOverrideOperatingModeHeatingDHW", Access::ReadWrite, printRemoteOverrideMode, ReadMode::Single},
  {ID::RemoteOverrideFunction, "RemoteOverrideFunction", Access::ReadOnly, printRemoteOverrideFunction, ReadMode::Single},
  {ID::StatusSolarStorage, "StatusSolarStorage", Access::ReadOnly, printSolarStorageStatus, ReadMode::Single},
  {ID::ASFflagsOEMfaultCodeSolarStorage, "SolarStorageFaults", Access::ReadOnly, printSolarStorageFault, ReadMode::Single},
  {ID::SConfigSMemberIDcodeSolarStorage, "SolarStorageConfig", Access::ReadOnly, printSolarStorageConfig, ReadMode::Single},
  {ID::SolarStorageVersion, "SolarStorageVersion", Access::ReadOnly, printProductVersion, ReadMode::Single},
  {ID::TSPSolarStorage, "TSPSolarStorage", Access::ReadOnly, printParameterCount, ReadMode::Count},
  {ID::TSPindexTSPvalueSolarStorage, "TSPindexTSPvalueSolarStorage", Access::ReadWrite, printIndexedValue, ReadMode::Indexed},
  {ID::FHBsizeSolarStorage, "FHBsizeSolarStorage", Access::ReadOnly, printParameterCount, ReadMode::Count},
  {ID::FHBindexFHBvalueSolarStorage, "FHBindexFHBvalueSolarStorage", Access::ReadOnly, printIndexedValue, ReadMode::Indexed},
  {ID::ElectricityProducerStarts, "ElectricityProducerStarts", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::ElectricityProducerHours, "ElectricityProducerHours", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::ElectricityProduction, "ElectricityProduction", Access::ReadOnly, printUInt16, ReadMode::Single},
  {ID::CumulativElectricityProduction, "CumulativElectricityProduction", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::UnsuccessfulBurnerStarts, "UnsuccessfulBurnerStarts", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::FlameSignalTooLowNumber, "FlameSignalTooLowNumber", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::OEMDiagnosticCode, "OEMDiagnosticCode", Access::ReadOnly, printUInt16, ReadMode::Single},
  {ID::SuccessfulBurnerStarts, "SuccessfulBurnerStarts", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::CHPumpStarts, "CHPumpStarts", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::DHWPumpValveStarts, "DHWPumpValveStarts", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::DHWBurnerStarts, "DHWBurnerStarts", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::BurnerOperationHours, "BurnerOperationHours", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::CHPumpOperationHours, "CHPumpOperationHours", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::DHWPumpValveOperationHours, "DHWPumpValveOperationHours", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::DHWBurnerOperationHours, "DHWBurnerOperationHours", Access::ReadWrite, printUInt16, ReadMode::Single},
  {ID::OpenThermVersionMaster, "OpenThermVersionMaster", Access::WriteOnly, printFloat, ReadMode::Single},
  {ID::OpenThermVersionSlave, "OpenThermVersionSlave", Access::ReadOnly, printFloat, ReadMode::Single},
  {ID::MasterVersion, "MasterVersion", Access::WriteOnly, printProductVersion, ReadMode::Single},
  {ID::SlaveVersion, "SlaveVersion", Access::ReadOnly, printProductVersion, ReadMode::Single}
};

// The second value remains in the device after the test. Temperatures are whole degrees C.
// OEM parameters and counter resets stay disabled until their meaning is known.
static const WriteValue writeValues[] = {
  // Test remote limits before the CH control setpoints that depend on them.
  {
    true,
    Messages::Float::write(ID::TdhwSet, 40.0f),
    Messages::Float::write(ID::TdhwSet, 41.0f)
  },
  {
    true,
    Messages::Float::write(ID::MaxTSet, 50.0f),
    Messages::Float::write(ID::MaxTSet, 51.0f)
  },
  {
    true,
    Messages::Float::write(ID::TSet, 40.0f),
    Messages::Float::write(ID::TSet, 41.0f)
  },
  {
    true,
    Messages::MasterConfig(0, 0),
    Messages::MasterConfig(0, 1)
  },
  {
    true,
    Messages::RemoteRequest(1),
    Messages::RemoteRequest(0)
  },
  {
    true,
    Messages::Float::write(ID::CoolingControl, 0.0f),
    Messages::Float::write(ID::CoolingControl, 1.0f)
  },
  {
    true,
    Messages::Float::write(ID::TsetCH2, 40.0f),
    Messages::Float::write(ID::TsetCH2, 41.0f)
  },
  {
    false,
    Messages::IndexedValue(ID::TSPindexTSPvalue, 0, 0),
    Messages::IndexedValue(ID::TSPindexTSPvalue, 0, 1)
  },
  {
    true,
    Messages::Float::write(ID::MaxRelModLevelSetting, 50.0f),
    Messages::Float::write(ID::MaxRelModLevelSetting, 51.0f)
  },
  {
    true,
    Messages::Float::write(ID::TrSet, 20.0f),
    Messages::Float::write(ID::TrSet, 21.0f)
  },
  {
    true,
    Messages::DayTime(1, 12, 0),
    Messages::DayTime(1, 12, 1)
  },
  {
    true,
    Messages::Date(1, 1),
    Messages::Date(1, 2)
  },
  {
    true,
    Messages::UInt16::write(ID::Year, 2026),
    Messages::UInt16::write(ID::Year, 2027)
  },
  {
    true,
    Messages::Float::write(ID::TrSetCH2, 20.0f),
    Messages::Float::write(ID::TrSetCH2, 21.0f)
  },
  {
    true,
    Messages::Float::write(ID::Tr, 20.0f),
    Messages::Float::write(ID::Tr, 21.0f)
  },
  {
    true,
    Messages::Float::write(ID::Toutside, 20.0f),
    Messages::Float::write(ID::Toutside, 21.0f)
  },
  {
    true,
    Messages::Float::write(ID::TrCH2, 20.0f),
    Messages::Float::write(ID::TrCH2, 21.0f)
  },
  {
    true,
    Messages::Float::write(ID::RelativeHumidity, 50.0f),
    Messages::Float::write(ID::RelativeHumidity, 51.0f)
  },
  {
    true,
    Messages::UInt8::write(ID::Vset, 30),
    Messages::UInt8::write(ID::Vset, 31)
  },
  {
    true,
    Messages::UInt8::write(ID::RHexhaust, 50),
    Messages::UInt8::write(ID::RHexhaust, 51)
  },
  {
    true,
    Messages::UInt16::write(ID::CO2exhaust, 600),
    Messages::UInt16::write(ID::CO2exhaust, 650)
  },
  {
    true,
    Messages::NominalVentilation(30),
    Messages::NominalVentilation(31)
  },
  {
    false,
    Messages::IndexedValue(ID::TSPindexTSPvalueVentilationHeatRecovery, 0, 0),
    Messages::IndexedValue(ID::TSPindexTSPvalueVentilationHeatRecovery, 0, 1)
  },
  {
    false,
    Messages::UInt16::write(ID::CoolingOperationHours, 0),
    Messages::UInt16::write(ID::CoolingOperationHours, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::PowerCycles, 0),
    Messages::UInt16::write(ID::PowerCycles, 0)
  },
  {
    false,
    Messages::RFSensorStatus(
      Messages::RFSensorStatus::SensorType::ROOM_SENSOR,
      0,
      Messages::RFSensorStatus::BatteryStatus::NORMAL,
      4
    ),
    Messages::RFSensorStatus(
      Messages::RFSensorStatus::SensorType::ROOM_SENSOR,
      0,
      Messages::RFSensorStatus::BatteryStatus::NORMAL,
      5
    )
  },
  {
    true,
    Messages::RemoteOverrideMode(
      Messages::RemoteOverrideMode::HeatingMode::OFF,
      Messages::RemoteOverrideMode::HeatingMode::NO_OVERRIDE,
      Messages::RemoteOverrideMode::DhwMode::NO_OVERRIDE
    ),
    Messages::RemoteOverrideMode(
      Messages::RemoteOverrideMode::HeatingMode::NO_OVERRIDE,
      Messages::RemoteOverrideMode::HeatingMode::NO_OVERRIDE,
      Messages::RemoteOverrideMode::DhwMode::NO_OVERRIDE
    )
  },
  {
    false,
    Messages::IndexedValue(ID::TSPindexTSPvalueSolarStorage, 0, 0),
    Messages::IndexedValue(ID::TSPindexTSPvalueSolarStorage, 0, 1)
  },
  {
    false,
    Messages::UInt16::write(ID::ElectricityProducerStarts, 0),
    Messages::UInt16::write(ID::ElectricityProducerStarts, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::ElectricityProducerHours, 0),
    Messages::UInt16::write(ID::ElectricityProducerHours, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::CumulativElectricityProduction, 0),
    Messages::UInt16::write(ID::CumulativElectricityProduction, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::UnsuccessfulBurnerStarts, 0),
    Messages::UInt16::write(ID::UnsuccessfulBurnerStarts, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::FlameSignalTooLowNumber, 0),
    Messages::UInt16::write(ID::FlameSignalTooLowNumber, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::SuccessfulBurnerStarts, 0),
    Messages::UInt16::write(ID::SuccessfulBurnerStarts, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::CHPumpStarts, 0),
    Messages::UInt16::write(ID::CHPumpStarts, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::DHWPumpValveStarts, 0),
    Messages::UInt16::write(ID::DHWPumpValveStarts, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::DHWBurnerStarts, 0),
    Messages::UInt16::write(ID::DHWBurnerStarts, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::BurnerOperationHours, 0),
    Messages::UInt16::write(ID::BurnerOperationHours, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::CHPumpOperationHours, 0),
    Messages::UInt16::write(ID::CHPumpOperationHours, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::DHWPumpValveOperationHours, 0),
    Messages::UInt16::write(ID::DHWPumpValveOperationHours, 0)
  },
  {
    false,
    Messages::UInt16::write(ID::DHWBurnerOperationHours, 0),
    Messages::UInt16::write(ID::DHWBurnerOperationHours, 0)
  },
  {
    true,
    Messages::Float::write(ID::OpenThermVersionMaster, 4.1f),
    Messages::Float::write(ID::OpenThermVersionMaster, 4.2f)
  },
  {
    true,
    Messages::ProductVersion(ID::MasterVersion, 0, 0),
    Messages::ProductVersion(ID::MasterVersion, 0, 1)
  }
};

static const size_t idCount = sizeof(idSpecs) / sizeof(idSpecs[0]);
static const size_t writeCount = sizeof(writeValues) / sizeof(writeValues[0]);
static constexpr size_t statusCount = static_cast<size_t>(Status::UNKNOWN_DATA_ID) + 1;
static ESP32OT::ESP32OT boiler;
static IDResult values[128];
static Phase phase = Phase::Read;
static size_t nextID = 0;
static uint8_t entryCount = 1;
static uint8_t entryIndex = 0;
static uint32_t cycleNumber = 1;
static uint32_t testedWrites = 0;
static uint32_t acknowledgedWrites = 0;
static uint32_t verifiedWrites = 0;
static uint32_t skippedWrites = 0;
static uint32_t finalStatuses[statusCount]{};

// The worker updates attempt counters; loop() prints them after the final callback.
static std::atomic<uint32_t> requestStatuses[2][statusCount]{};
static std::atomic<uint32_t> responseStatuses[statusCount]{};
static std::atomic<uint32_t> responseTypes[8]{};
static std::atomic<uint32_t> lastTerminalOperation{0};

void setup() {
  Serial.begin(115200);

  ESP32OT::ESP32OT::Config config;
  config.rxGpio = static_cast<gpio_num_t>(MASTER_RX_GPIO);
  config.txGpio = static_cast<gpio_num_t>(MASTER_TX_GPIO);
  config.maxAttempts = 10;

  boiler.setConfig(config);
  boiler.onStatusChange(countStatus);
  Serial.printf("TEST_ALL_IDS writes=%u maxAttempts=10\n", TEST_ALL_IDS_ENABLE_WRITES);
}

void loop() {
  if (phase == Phase::Report) {
    printSummary();

    for (auto& value : values) {
      value = {};
    }

    nextID = 0;
    entryCount = 1;
    entryIndex = 0;
    phase = Phase::Read;
    cycleNumber++;
    delay(10000);
    return;
  }

  if (!boiler.isInitialized()) {
    if (!boiler.end() || !boiler.begin()) {
      Serial.printf(
        "OpenTherm initialization failed: %ld\n",
        static_cast<long>(boiler.getLastError())
      );
      delay(1000);
      return;
    }
  }

  if (phase == Phase::Read) {
    if (nextID == idCount) {
      nextID = 0;

      if (TEST_ALL_IDS_ENABLE_WRITES) {
        phase = Phase::Write;

      } else {
        phase = Phase::Report;
      }

      return;
    }

    const auto& item = idSpecs[nextID];
    auto& saved = values[static_cast<uint8_t>(item.id)];

    if (item.access == Access::WriteOnly || (item.readMode == ReadMode::Indexed && entryCount == 0)) {
      nextID++;
      return;
    }

    auto request = Message::read(item.id);

    if (item.id == ID::Status) {
      request = Messages::Status::request();

    } else if (item.readMode == ReadMode::Indexed) {
      request = Messages::IndexedValue(item.id, entryIndex);
    }

    const auto result = sendRequest("READ", item, request);
    saved.readStatus = result.getStatus();
    saved.readKnown |= isAck(result, Type::READ_ACK) || result.getStatus() == Status::DATA_INVALID;
    saved.valid = isAck(result, Type::READ_ACK);

    if (saved.valid) {
      saved.response = result;
    }

    if (item.readMode == ReadMode::Count) {
      entryCount = saved.valid ? result.as<Messages::ParameterCount>().getCount() : 1;
    }

    if (item.readMode == ReadMode::Indexed && entryIndex + 1 < entryCount) {
      entryIndex++;

    } else {
      entryIndex = 0;
      nextID++;
    }

  } else if (phase == Phase::Write) {
    if (nextID == writeCount) {
      phase = Phase::Report;
      return;
    }

    const auto& configured = writeValues[nextID++];
    const auto* item = findIDSpec(configured.first.getId());
    Message requests[2];

    if (!item || item->access == Access::ReadOnly
      || !prepareWriteValues(configured, requests[0], requests[1])) {
      Serial.printf(
        "WRITE_SKIP     ID %3u (%s): disabled or no permitted pair within known limits\n",
        static_cast<unsigned>(configured.first.getId()),
        item ? item->name : "unknown"
      );
      skippedWrites++;
      return;
    }

    auto& saved = values[static_cast<uint8_t>(item->id)];

    for (const auto& request : requests) {
      if (item->id == ID::MaxTSet) {
        saved.valid = false; // A lost ACK can leave the applied maximum unknown.
      }

      const auto result = sendRequest("WRITE", *item, request);
      saved.writeStatus = result.getStatus();
      saved.writeKnown |= isAck(result, Type::WRITE_ACK);
      testedWrites++;

      if (isAck(result, Type::WRITE_ACK)) {
        acknowledgedWrites++;

        if (item->id == ID::MaxTSet) {
          saved.response = result;
          saved.valid = true;
        }

        if (item->access == Access::ReadWrite) {
          auto readRequest = Message::read(item->id);

          if (item->readMode == ReadMode::Indexed) {
            readRequest = Messages::IndexedValue(item->id, request.as<Messages::IndexedValue>().getIndex());
          }

          const auto check = sendRequest("VERIFY_READ", *item, readRequest);
          const bool matched = isAck(check, Type::READ_ACK) && check.getData() == request.getData();
          verifiedWrites += matched;

          if (isAck(check, Type::READ_ACK)) {
            saved.response = check;
            saved.valid = true;
          }

          Serial.printf(
            "WRITE_VALUE id=%u expected=%04X matched=%u\n",
            static_cast<unsigned>(item->id),
            static_cast<unsigned>(request.getData()),
            matched
          );
        }
      }
    }
  }

  // The transport enforces bus readiness and the inter-frame waiting time.
  delay(1);
}

static bool isAck(const Message& message, Type type) {
  return message.isSuccess() && message.hasFrame() && message.getType() == type;
}

static Message sendRequest(const char* action, const IDSpec& item, const Message& request) {
  Message result;

  // RBPflags are permission inputs only. Never write them, even if a table is edited.
  if (request.getType() == Type::WRITE_DATA
    && (!TEST_ALL_IDS_ENABLE_WRITES
      || request.getId() == ID::RBPflags
      || request.getId() == ID::RBPflagsVentilationHeatRecovery)) {
    auto denied = request.getMessageData();
    denied.status = Status::ERROR;
    denied.error = ESP_ERR_INVALID_STATE;
    denied.hasFrame = false;
    result.parse(denied);

  } else {
    result = boiler.sendRequest(request);

    // The synchronous result can arrive before its worker callback has finished.
    while (result.getOperation() != 0
      && lastTerminalOperation.load(std::memory_order_acquire) != result.getOperation()) {
      delay(1);
    }
  }

  finalStatuses[static_cast<size_t>(result.getStatus())]++;
  Serial.printf(
    "%-14s ID %3u (%s): %*s%-15s request=%08lX attempts=%u",
    action,
    static_cast<unsigned>(item.id),
    item.name,
    37 - static_cast<int>(strlen(item.name)),
    "",
    ESP32OT::Protocol::Frame::statusToString(result.getStatus()),
    static_cast<unsigned long>(request.getRaw()),
    static_cast<unsigned>(result.getAttempt())
  );

  if (result.hasFrame()) {
    Serial.printf(
      " response=%08lX type=%s",
      static_cast<unsigned long>(result.getRaw()),
      ESP32OT::Protocol::Frame::typeToString(result.getType())
    );
  }

  if (item.readMode == ReadMode::Indexed) {
    Serial.printf(
      " requestIndex=%u",
      static_cast<unsigned>(request.as<Messages::IndexedValue>().getIndex())
    );
  }

  if (result.isSuccess() && result.hasFrame()) {
    Serial.printf(" data=0x%04X", static_cast<unsigned>(result.getData()));
    item.printData(result);
  }

  Serial.println();
  return result;
}

static void countStatus(const ESP32OT::ESP32OT::StatusEvent& event) {
  const auto& message = event.message;

  if (message.getOperation() == 0) {
    return;
  }

  const size_t direction = ESP32OT::Protocol::Frame::getType(message.getRequest()) == Type::WRITE_DATA
    ? 1
    : 0;
  const size_t status = static_cast<size_t>(event.current);
  requestStatuses[direction][status].fetch_add(1, std::memory_order_relaxed);

  if (message.isComplete()) {
    if (message.hasFrame()) {
      responseStatuses[status].fetch_add(1, std::memory_order_relaxed);
      responseTypes[static_cast<size_t>(message.getType())].fetch_add(1, std::memory_order_relaxed);
    }

    lastTerminalOperation.store(message.getOperation(), std::memory_order_release);
  }
}

static bool writeForbidden(ID id) {
  if (id == ID::RBPflags || id == ID::RBPflagsVentilationHeatRecovery) {
    return true;
  }

  if (id == ID::TdhwSet || id == ID::MaxTSet) {
    const auto& saved = values[static_cast<uint8_t>(ID::RBPflags)];

    if (!saved.valid) {
      return true;
    }

    const auto flags = saved.response.as<Messages::RemoteParameters>();

    if (id == ID::TdhwSet) {
      return !flags.isDhwSetpointSupported() || !flags.isDhwSetpointWritable();
    }

    return !flags.isMaxHeatingSetpointSupported() || !flags.isMaxHeatingSetpointWritable();
  }

  if (id == ID::NominalVentilationValue) {
    const auto& saved = values[static_cast<uint8_t>(ID::RBPflagsVentilationHeatRecovery)];

    if (!saved.valid) {
      return true;
    }

    const auto flags = saved.response.as<Messages::VentilationRemoteParameters>();
    return !flags.isNominalVentilationTransferEnabled() || !flags.isNominalVentilationWritable();
  }

  return false;
}

static const IDSpec* findIDSpec(ID id) {
  for (const auto& item : idSpecs) {
    if (item.id == id) {
      return &item;
    }
  }

  return nullptr;
}

static bool prepareWriteValues(const WriteValue& configured, Message& first, Message& second) {
  first = configured.first;
  second = configured.second;
  const ID id = first.getId();

  if (!configured.enabled || first.getType() != Type::WRITE_DATA
    || second.getType() != Type::WRITE_DATA || second.getId() != id || writeForbidden(id)) {
    return false;
  }

  if (id == ID::MConfigMMemberIDcode
    && (first.as<Messages::MasterConfig>().isSmartPowerImplemented()
      || second.as<Messages::MasterConfig>().isSmartPowerImplemented())) {
    return false; // This example does not implement Smart Power.
  }

  float lower = 0.0f;
  float upper = 127.0f;

  if (id == ID::TdhwSet || id == ID::MaxTSet) {
    const ID boundsID = id == ID::TdhwSet ? ID::TdhwSetUBTdhwSetLB : ID::MaxTSetUBMaxTSetLB;
    const auto& saved = values[static_cast<uint8_t>(boundsID)];

    if (!saved.valid) {
      return false;
    }

    const auto bounds = saved.response.as<Messages::TemperatureBounds>();
    lower = fmaxf(lower, bounds.getLower());
    upper = fminf(upper, bounds.getUpper());

  } else if (id == ID::TSet || id == ID::TsetCH2) {
    const auto& maximum = values[static_cast<uint8_t>(ID::MaxTSet)];

    if (!maximum.valid) {
      return false;
    }

    // ID49's lower bound limits MaxTSet, not the current CH control setpoint.
    upper = floorf(fminf(100.0f, maximum.response.as<Messages::Float>().get()));

  } else {
    return first.getData() != second.getData();
  }

  if (lower >= upper) {
    return false; // A pair needs at least two permitted whole-degree values.
  }

  const float configuredFirst = first.as<Messages::Float>().get();
  const float configuredSecond = second.as<Messages::Float>().get();
  const float firstValue = fminf(upper, fmaxf(lower, configuredFirst));
  float secondValue = fminf(upper, fmaxf(lower, configuredSecond));

  if (firstValue == secondValue) {
    secondValue = firstValue + 1.0f <= upper ? firstValue + 1.0f : firstValue - 1.0f;
  }

  first = Messages::Float::write(id, firstValue);
  second = Messages::Float::write(id, secondValue);

  if (firstValue != configuredFirst || secondValue != configuredSecond) {
    Serial.printf(
      "WRITE_ADJUST   ID %3u: first %.0f -> %.0f C, second %.0f -> %.0f C (allowed %.0f..%.0f)\n",
      static_cast<unsigned>(id),
      configuredFirst,
      firstValue,
      configuredSecond,
      secondValue,
      lower,
      upper
    );
  }

  return true;
}

static const char* accessName(Access access) {
  return access == Access::ReadOnly
    ? "read only"
    : access == Access::WriteOnly ? "write only" : "read/write";
}

static const char* observedMode(const IDSpec& item) {
  const auto& saved = values[static_cast<uint8_t>(item.id)];
  const ID flagsID = item.id == ID::NominalVentilationValue
    ? ID::RBPflagsVentilationHeatRecovery
    : ID::RBPflags;
  const bool flagsKnown = values[static_cast<uint8_t>(flagsID)].valid;

  if (saved.readKnown && saved.writeKnown) {
    return "read/write";
  }

  if (saved.readKnown && (item.access == Access::ReadOnly || (flagsKnown && writeForbidden(item.id)))) {
    return "read only";
  }

  if (saved.writeKnown && item.access == Access::WriteOnly) {
    return "write only";
  }

  if (!saved.readKnown && !saved.writeKnown
    && (item.access == Access::WriteOnly || saved.readStatus == Status::UNKNOWN_DATA_ID)
    && (item.access == Access::ReadOnly || saved.writeStatus == Status::UNKNOWN_DATA_ID)) {
    return "not supported";
  }

  return "unverified";
}

static void printSummary() {
  uint32_t completedCalls = 0;
  uint32_t receivedFrames = 0;

  for (size_t index = 0; index < statusCount; index++) {
    completedCalls += finalStatuses[index];
    receivedFrames += responseStatuses[index].load();
  }

  const uint32_t readAttempts = requestStatuses[0][static_cast<size_t>(Status::QUEUED)].load();
  const uint32_t writeAttempts = requestStatuses[1][static_cast<size_t>(Status::QUEUED)].load();
  Serial.printf(
    "\n==================== TestAllIDs: cycle %lu ====================\n",
    static_cast<unsigned long>(cycleNumber)
  );
  Serial.printf("Write tests: %s\n", TEST_ALL_IDS_ENABLE_WRITES ? "enabled" : "disabled");
  Serial.println("Totals since boot:");
  Serial.printf("  Completed calls : %lu\n", static_cast<unsigned long>(completedCalls));
  Serial.printf(
    "  Queued attempts : %lu (READ: %lu, WRITE: %lu)\n",
    static_cast<unsigned long>(readAttempts + writeAttempts),
    static_cast<unsigned long>(readAttempts),
    static_cast<unsigned long>(writeAttempts)
  );
  Serial.printf("  Response frames : %lu\n", static_cast<unsigned long>(receivedFrames));
  Serial.printf("\nID_MAP cycle=%lu\n", static_cast<unsigned long>(cycleNumber));
  Serial.println("Protocol = declared access; Observed = device evidence; '-' = not tested.");
  Serial.println("Unverified = insufficient evidence, including disabled or skipped writes.");
  Serial.println("+-----+---------------------------------------+-------------+---------------+-----------------+-----------------+");
  Serial.println("| ID  | Name                                  | Protocol    | Observed      | Read result     | Write result    |");
  Serial.println("+-----+---------------------------------------+-------------+---------------+-----------------+-----------------+");

  for (const auto& item : idSpecs) {
    const auto& saved = values[static_cast<uint8_t>(item.id)];
    Serial.printf(
      "| %3u | %-37s | %-11s | %-13s | %-15s | %-15s |\n",
      static_cast<unsigned>(item.id),
      item.name,
      accessName(item.access),
      observedMode(item),
      saved.readStatus == Status::NONE
        ? "-"
        : ESP32OT::Protocol::Frame::statusToString(saved.readStatus),
      saved.writeStatus == Status::NONE
        ? "-"
        : ESP32OT::Protocol::Frame::statusToString(saved.writeStatus)
    );
    delay(1);
  }

  Serial.println("+-----+---------------------------------------+-------------+---------------+-----------------+-----------------+");
  Serial.println("\nCUMULATIVE_STATUS since boot");
  Serial.println("Request status transitions (all attempts, including retries)");
  Serial.println("Each row counts entries into that status; do not add the rows together.");
  Serial.println("+------------------+------------+------------+");
  Serial.println("| Status           |       READ |      WRITE |");
  Serial.println("+------------------+------------+------------+");

  for (size_t index = 0; index < statusCount; index++) {
    Serial.printf(
      "| %-16s | %10lu | %10lu |\n",
      ESP32OT::Protocol::Frame::statusToString(static_cast<Status>(index)),
      static_cast<unsigned long>(requestStatuses[0][index].load()),
      static_cast<unsigned long>(requestStatuses[1][index].load())
    );
  }

  Serial.println("+------------------+------------+------------+");
  Serial.println("\nReceived frames and final call results (after retries)");
  Serial.println("Frames include complete invalid frames; timeouts without a frame add none.");
  Serial.println("+------------------+------------+-------------+");
  Serial.println("| Status           |     Frames | Final calls |");
  Serial.println("+------------------+------------+-------------+");

  for (size_t index = static_cast<size_t>(Status::SUCCESS); index < statusCount; index++) {
    Serial.printf(
      "| %-16s | %10lu | %11lu |\n",
      ESP32OT::Protocol::Frame::statusToString(static_cast<Status>(index)),
      static_cast<unsigned long>(responseStatuses[index].load()),
      static_cast<unsigned long>(finalStatuses[index])
    );
  }

  Serial.println("+------------------+------------+-------------+");
  Serial.printf(
    "| %-16s | %10lu | %11lu |\n",
    "Total",
    static_cast<unsigned long>(receivedFrames),
    static_cast<unsigned long>(completedCalls)
  );
  Serial.println("+------------------+------------+-------------+");
  Serial.println("\nRESPONSE_TYPES since boot (all complete frames)");
  Serial.println("+------------------+------------+");
  Serial.println("| Frame type       |      Count |");
  Serial.println("+------------------+------------+");

  for (size_t index = 0; index < 8; index++) {
    Serial.printf(
      "| %-16s | %10lu |\n",
      ESP32OT::Protocol::Frame::typeToString(static_cast<Type>(index)),
      static_cast<unsigned long>(responseTypes[index].load())
    );
  }

  Serial.println("+------------------+------------+");
  Serial.println("\nWRITE_TOTALS since boot");
  Serial.println("+----------------------+------------+");
  Serial.println("| Metric               |      Count |");
  Serial.println("+----------------------+------------+");
  Serial.printf("| Write calls          | %10lu |\n", static_cast<unsigned long>(testedWrites));
  Serial.printf("| WRITE_ACK results    | %10lu |\n", static_cast<unsigned long>(acknowledgedWrites));
  Serial.printf("| Matching readbacks   | %10lu |\n", static_cast<unsigned long>(verifiedWrites));
  Serial.printf("| Skipped ID pairs     | %10lu |\n", static_cast<unsigned long>(skippedWrites));
  Serial.println("+----------------------+------------+");
  Serial.printf(
    "\nCYCLE_DONE cycle=%lu writes=%u ids=%u\n\n",
    static_cast<unsigned long>(cycleNumber),
    TEST_ALL_IDS_ENABLE_WRITES,
    static_cast<unsigned>(idCount)
  );
}

static void printBrandCharacter(const Messages::Message& result) {
  const auto data = result.as<Messages::BrandCharacter>();
  const auto character = static_cast<uint8_t>(data.getCharacter());
  Serial.printf(
    " requestIndex=%u maxIndex=%u character='%c' characterCode=0x%02X",
    static_cast<unsigned>(data.getRequestIndex()),
    static_cast<unsigned>(data.getMaxIndex()),
    character >= 32 && character <= 126 ? character : '.',
    static_cast<unsigned>(character)
  );
}

static void printCapacity(const Messages::Message& result) {
  const auto data = result.as<Messages::Capacity>();
  Serial.printf(
    " maxCapacity=%u minModulation=%u",
    static_cast<unsigned>(data.getMaxCapacity()),
    static_cast<unsigned>(data.getMinModulation())
  );
}

static void printDate(const Messages::Message& result) {
  const auto data = result.as<Messages::Date>();
  Serial.printf(
    " month=%u day=%u",
    static_cast<unsigned>(data.getMonth()),
    static_cast<unsigned>(data.getDay())
  );
}

static void printDayTime(const Messages::Message& result) {
  const auto data = result.as<Messages::DayTime>();
  Serial.printf(
    " dayOfWeek=%u hours=%u minutes=%u",
    static_cast<unsigned>(data.getDayOfWeek()),
    static_cast<unsigned>(data.getHours()),
    static_cast<unsigned>(data.getMinutes())
  );
}

static void printFanSpeed(const Messages::Message& result) {
  const auto data = result.as<Messages::FanSpeed>();
  Serial.printf(
    " setpointHz=%u actualHz=%u",
    static_cast<unsigned>(data.getSetpointHz()),
    static_cast<unsigned>(data.getActualHz())
  );
}

static void printFault(const Messages::Message& result) {
  const auto data = result.as<Messages::Fault>();
  Serial.printf(
    " serviceRequested=%u lockoutResetEnabled=%u lowWaterPressureFault=%u gasFlameFault=%u"
    " airPressureFault=%u waterOvertemperatureFault=%u oemFaultCode=%u",
    static_cast<unsigned>(data.isServiceRequested()),
    static_cast<unsigned>(data.isLockoutResetEnabled()),
    static_cast<unsigned>(data.isLowWaterPressureFault()),
    static_cast<unsigned>(data.isGasFlameFault()),
    static_cast<unsigned>(data.isAirPressureFault()),
    static_cast<unsigned>(data.isWaterOvertemperatureFault()),
    static_cast<unsigned>(data.getOemFaultCode())
  );
}

static void printFloat(const Messages::Message& result) {
  const auto data = result.as<Messages::Float>();
  Serial.printf(" value=%.4f", data.get());
}

static void printIndexedValue(const Messages::Message& result) {
  const auto data = result.as<Messages::IndexedValue>();
  Serial.printf(
    " index=%u value=%u",
    static_cast<unsigned>(data.getIndex()),
    static_cast<unsigned>(data.getValue())
  );
}

static void printInt16(const Messages::Message& result) {
  const auto data = result.as<Messages::Int16>();
  Serial.printf(" value=%d", static_cast<int>(data.getInt()));
}

static void printNominalVentilation(const Messages::Message& result) {
  const auto data = result.as<Messages::NominalVentilation>();
  Serial.printf(" value=%u", static_cast<unsigned>(data.get()));
}

static void printParameterCount(const Messages::Message& result) {
  const auto data = result.as<Messages::ParameterCount>();
  Serial.printf(" count=%u", static_cast<unsigned>(data.getCount()));
}

static void printProductVersion(const Messages::Message& result) {
  const auto data = result.as<Messages::ProductVersion>();
  Serial.printf(
    " productType=%u version=%u",
    static_cast<unsigned>(data.getProductType()),
    static_cast<unsigned>(data.getVersion())
  );
}

static void printRemoteOverrideFunction(const Messages::Message& result) {
  const auto data = result.as<Messages::RemoteOverrideFunction>();
  Serial.printf(
    " manualChangePriorityEnabled=%u programChangePriorityEnabled=%u",
    static_cast<unsigned>(data.isManualChangePriorityEnabled()),
    static_cast<unsigned>(data.isProgramChangePriorityEnabled())
  );
}

static void printRemoteOverrideMode(const Messages::Message& result) {
  const auto data = result.as<Messages::RemoteOverrideMode>();
  Serial.printf(
    " heatingMode=%u ch2Mode=%u dhwMode=%u dhwPushRequested=%u",
    static_cast<unsigned>(data.getHeatingMode()),
    static_cast<unsigned>(data.getCh2Mode()),
    static_cast<unsigned>(data.getDhwMode()),
    static_cast<unsigned>(data.isDhwPushRequested())
  );
}

static void printRemoteParameters(const Messages::Message& result) {
  const auto data = result.as<Messages::RemoteParameters>();
  Serial.printf(
    " dhwSetpointSupported=%u dhwSetpointWritable=%u maxHeatingSetpointSupported=%u"
    " maxHeatingSetpointWritable=%u",
    static_cast<unsigned>(data.isDhwSetpointSupported()),
    static_cast<unsigned>(data.isDhwSetpointWritable()),
    static_cast<unsigned>(data.isMaxHeatingSetpointSupported()),
    static_cast<unsigned>(data.isMaxHeatingSetpointWritable())
  );
}

static void printSlaveConfig(const Messages::Message& result) {
  const auto data = result.as<Messages::SlaveConfig>();
  Serial.printf(
    " dhwPresent=%u onOffControl=%u coolingSupported=%u dhwStorageTank=%u"
    " masterLowOffPumpControlAllowed=%u ch2Present=%u remoteWaterFillingUnavailable=%u"
    " heatCoolModeControlledBySlave=%u memberId=%u",
    static_cast<unsigned>(data.isDhwPresent()),
    static_cast<unsigned>(data.isOnOffControl()),
    static_cast<unsigned>(data.isCoolingSupported()),
    static_cast<unsigned>(data.hasDhwStorageTank()),
    static_cast<unsigned>(data.isMasterLowOffPumpControlAllowed()),
    static_cast<unsigned>(data.isCh2Present()),
    static_cast<unsigned>(data.isRemoteWaterFillingUnavailable()),
    static_cast<unsigned>(data.isHeatCoolModeControlledBySlave()),
    static_cast<unsigned>(data.getMemberId())
  );
}

static void printSolarStorageConfig(const Messages::Message& result) {
  const auto data = result.as<Messages::SolarStorageConfig>();
  Serial.printf(
    " configurationByte=%u memberId=%u",
    static_cast<unsigned>(data.getConfigurationByte()),
    static_cast<unsigned>(data.getMemberId())
  );
}

static void printSolarStorageFault(const Messages::Message& result) {
  const auto data = result.as<Messages::SolarStorageFault>();
  Serial.printf(" oemFaultCode=%u", static_cast<unsigned>(data.getOemFaultCode()));
}

static void printSolarStorageStatus(const Messages::Message& result) {
  const auto data = result.as<Messages::SolarStorageStatus>();
  Serial.printf(
    " masterMode=%u slaveMode=%u state=%u fault=%u",
    static_cast<unsigned>(data.getMasterMode()),
    static_cast<unsigned>(data.getSlaveMode()),
    static_cast<unsigned>(data.getState()),
    static_cast<unsigned>(data.isFault())
  );
}

static void printStatus(const Messages::Message& result) {
  const auto data = result.as<Messages::Status>();
  Serial.printf(
    " masterFlags=%u slaveFlags=%u centralHeatingEnabled=%u hotWaterEnabled=%u coolingEnabled=%u"
    " outsideTemperatureCompensationEnabled=%u ch2Enabled=%u summerWinterMode=%u dhwBlocking=%u fault=%u"
    " centralHeatingActive=%u hotWaterActive=%u flameOn=%u coolingActive=%u ch2Active=%u diagnostic=%u"
    " electricityProductionActive=%u",
    static_cast<unsigned>(data.getMasterFlags()),
    static_cast<unsigned>(data.getSlaveFlags()),
    static_cast<unsigned>(data.isCentralHeatingEnabled()),
    static_cast<unsigned>(data.isHotWaterEnabled()),
    static_cast<unsigned>(data.isCoolingEnabled()),
    static_cast<unsigned>(data.isOutsideTemperatureCompensationEnabled()),
    static_cast<unsigned>(data.isCh2Enabled()),
    static_cast<unsigned>(data.isSummerWinterMode()),
    static_cast<unsigned>(data.isDhwBlocking()),
    static_cast<unsigned>(data.isFault()),
    static_cast<unsigned>(data.isCentralHeatingActive()),
    static_cast<unsigned>(data.isHotWaterActive()),
    static_cast<unsigned>(data.isFlameOn()),
    static_cast<unsigned>(data.isCoolingActive()),
    static_cast<unsigned>(data.isCh2Active()),
    static_cast<unsigned>(data.isDiagnostic()),
    static_cast<unsigned>(data.isElectricityProductionActive())
  );
}

static void printTemperatureBounds(const Messages::Message& result) {
  const auto data = result.as<Messages::TemperatureBounds>();
  Serial.printf(
    " upper=%d lower=%d",
    static_cast<int>(data.getUpper()),
    static_cast<int>(data.getLower())
  );
}

static void printUInt16(const Messages::Message& result) {
  const auto data = result.as<Messages::UInt16>();
  Serial.printf(" value=%u", static_cast<unsigned>(data.getUInt()));
}

static void printUInt8(const Messages::Message& result) {
  const auto data = result.as<Messages::UInt8>();
  Serial.printf(" value=%u", static_cast<unsigned>(data.get()));
}

static void printVentilationConfig(const Messages::Message& result) {
  const auto data = result.as<Messages::VentilationConfig>();
  Serial.printf(
    " configFlags=%u memberId=%u heatRecoverySystem=%u bypass=%u variableSpeed=%u",
    static_cast<unsigned>(data.getConfigFlags()),
    static_cast<unsigned>(data.getMemberId()),
    static_cast<unsigned>(data.isHeatRecoverySystem()),
    static_cast<unsigned>(data.hasBypass()),
    static_cast<unsigned>(data.isVariableSpeed())
  );
}

static void printVentilationFault(const Messages::Message& result) {
  const auto data = result.as<Messages::VentilationFault>();
  Serial.printf(
    " faultFlags=%u oemFaultCode=%u serviceRequired=%u exhaustFanFault=%u inletFanFault=%u"
    " frostProtectionActive=%u",
    static_cast<unsigned>(data.getFaultFlags()),
    static_cast<unsigned>(data.getOemFaultCode()),
    static_cast<unsigned>(data.isServiceRequired()),
    static_cast<unsigned>(data.hasExhaustFanFault()),
    static_cast<unsigned>(data.hasInletFanFault()),
    static_cast<unsigned>(data.isFrostProtectionActive())
  );
}

static void printVentilationRemoteParameters(const Messages::Message& result) {
  const auto data = result.as<Messages::VentilationRemoteParameters>();
  Serial.printf(
    " transferFlags=%u readWriteFlags=%u nominalVentilationTransferEnabled=%u"
    " nominalVentilationWritable=%u",
    static_cast<unsigned>(data.getTransferFlags()),
    static_cast<unsigned>(data.getReadWriteFlags()),
    static_cast<unsigned>(data.isNominalVentilationTransferEnabled()),
    static_cast<unsigned>(data.isNominalVentilationWritable())
  );
}

static void printVentilationStatus(const Messages::Message& result) {
  const auto data = result.as<Messages::VentilationStatus>();
  Serial.printf(
    " masterFlags=%u slaveFlags=%u ventilationEnabled=%u bypassOpenRequested=%u automaticBypassEnabled=%u"
    " freeVentilationEnabled=%u fault=%u ventilationActive=%u bypassOpen=%u automaticBypassActive=%u"
    " freeVentilationActive=%u diagnostic=%u",
    static_cast<unsigned>(data.getMasterFlags()),
    static_cast<unsigned>(data.getSlaveFlags()),
    static_cast<unsigned>(data.isVentilationEnabled()),
    static_cast<unsigned>(data.isBypassOpenRequested()),
    static_cast<unsigned>(data.isAutomaticBypassEnabled()),
    static_cast<unsigned>(data.isFreeVentilationEnabled()),
    static_cast<unsigned>(data.isFault()),
    static_cast<unsigned>(data.isVentilationActive()),
    static_cast<unsigned>(data.isBypassOpen()),
    static_cast<unsigned>(data.isAutomaticBypassActive()),
    static_cast<unsigned>(data.isFreeVentilationActive()),
    static_cast<unsigned>(data.isDiagnostic())
  );
}

static void printMasterConfig(const Message& result) {
  const auto value = result.as<Messages::MasterConfig>();
  Serial.printf(
    " smartPowerImplemented=%u memberId=%u",
    value.isSmartPowerImplemented(),
    value.getMemberId()
  );
}

static void printRemoteRequest(const Message& result) {
  const auto value = result.as<Messages::RemoteRequest>();
  Serial.printf(
    " requestCode=%u responseCode=%u accepted=%u",
    value.getRequestCode(),
    value.getResponseCode(),
    value.isRequestAccepted()
  );
}

static void printRFSensorStatus(const Message& result) {
  const auto value = result.as<Messages::RFSensorStatus>();
  Serial.printf(
    " sensorType=%u sensorIndex=%u batteryStatus=%u signalStrength=%u",
    static_cast<unsigned>(value.getSensorType()),
    value.getSensorIndex(),
    static_cast<unsigned>(value.getBatteryStatus()),
    value.getSignalStrength()
  );
}
