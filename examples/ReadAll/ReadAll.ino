/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>

#ifndef MASTER_RX_GPIO
  #define MASTER_RX_GPIO 4
#endif
#ifndef MASTER_TX_GPIO
  #define MASTER_TX_GPIO 5
#endif

static void printBrandCharacter(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::BrandCharacter>();
  const auto character = static_cast<uint8_t>(data.getCharacter());
  Serial.printf(
    " requestIndex=%u maxIndex=%u character='%c' characterCode=0x%02X",
    static_cast<unsigned>(data.getRequestIndex()),
    static_cast<unsigned>(data.getMaxIndex()),
    character >= 32 && character <= 126 ? character : '.',
    static_cast<unsigned>(character)
  );
}

static void printCapacity(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::Capacity>();
  Serial.printf(
    " maxCapacity=%u minModulation=%u",
    static_cast<unsigned>(data.getMaxCapacity()),
    static_cast<unsigned>(data.getMinModulation())
  );
}

static void printDate(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::Date>();
  Serial.printf(
    " month=%u day=%u",
    static_cast<unsigned>(data.getMonth()),
    static_cast<unsigned>(data.getDay())
  );
}

static void printDayTime(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::DayTime>();
  Serial.printf(
    " dayOfWeek=%u hours=%u minutes=%u",
    static_cast<unsigned>(data.getDayOfWeek()),
    static_cast<unsigned>(data.getHours()),
    static_cast<unsigned>(data.getMinutes())
  );
}

static void printFanSpeed(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::FanSpeed>();
  Serial.printf(
    " setpointHz=%u actualHz=%u",
    static_cast<unsigned>(data.getSetpointHz()),
    static_cast<unsigned>(data.getActualHz())
  );
}

static void printFault(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::Fault>();
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

static void printFloat(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::Float>();
  Serial.printf(
    " value=%.4f",
    data.get()
  );
}

static void printIndexedValue(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::IndexedValue>();
  Serial.printf(
    " index=%u value=%u",
    static_cast<unsigned>(data.getIndex()),
    static_cast<unsigned>(data.getValue())
  );
}

static void printInt16(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::Int16>();
  Serial.printf(
    " value=%d",
    static_cast<int>(data.getInt())
  );
}

static void printNominalVentilation(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::NominalVentilation>();
  Serial.printf(
    " value=%u",
    static_cast<unsigned>(data.get())
  );
}

static void printParameterCount(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::ParameterCount>();
  Serial.printf(
    " count=%u",
    static_cast<unsigned>(data.getCount())
  );
}

static void printProductVersion(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::ProductVersion>();
  Serial.printf(
    " productType=%u version=%u",
    static_cast<unsigned>(data.getProductType()),
    static_cast<unsigned>(data.getVersion())
  );
}

static void printRemoteOverrideFunction(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::RemoteOverrideFunction>();
  Serial.printf(
    " manualChangePriorityEnabled=%u programChangePriorityEnabled=%u",
    static_cast<unsigned>(data.isManualChangePriorityEnabled()),
    static_cast<unsigned>(data.isProgramChangePriorityEnabled())
  );
}

static void printRemoteOverrideMode(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::RemoteOverrideMode>();
  Serial.printf(
    " heatingMode=%u ch2Mode=%u dhwMode=%u dhwPushRequested=%u",
    static_cast<unsigned>(data.getHeatingMode()),
    static_cast<unsigned>(data.getCh2Mode()),
    static_cast<unsigned>(data.getDhwMode()),
    static_cast<unsigned>(data.isDhwPushRequested())
  );
}

static void printRemoteParameters(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::RemoteParameters>();
  Serial.printf(
    " dhwSetpointSupported=%u dhwSetpointWritable=%u maxHeatingSetpointSupported=%u"
    " maxHeatingSetpointWritable=%u",
    static_cast<unsigned>(data.isDhwSetpointSupported()),
    static_cast<unsigned>(data.isDhwSetpointWritable()),
    static_cast<unsigned>(data.isMaxHeatingSetpointSupported()),
    static_cast<unsigned>(data.isMaxHeatingSetpointWritable())
  );
}

static void printSlaveConfig(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::SlaveConfig>();
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

static void printSolarStorageConfig(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::SolarStorageConfig>();
  Serial.printf(
    " configurationByte=%u memberId=%u",
    static_cast<unsigned>(data.getConfigurationByte()),
    static_cast<unsigned>(data.getMemberId())
  );
}

static void printSolarStorageFault(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::SolarStorageFault>();
  Serial.printf(
    " oemFaultCode=%u",
    static_cast<unsigned>(data.getOemFaultCode())
  );
}

static void printSolarStorageStatus(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::SolarStorageStatus>();
  Serial.printf(
    " masterMode=%u slaveMode=%u state=%u fault=%u",
    static_cast<unsigned>(data.getMasterMode()),
    static_cast<unsigned>(data.getSlaveMode()),
    static_cast<unsigned>(data.getState()),
    static_cast<unsigned>(data.isFault())
  );
}

static void printStatus(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::Status>();
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

static void printTemperatureBounds(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::TemperatureBounds>();
  Serial.printf(
    " upper=%d lower=%d",
    static_cast<int>(data.getUpper()),
    static_cast<int>(data.getLower())
  );
}

static void printUInt16(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::UInt16>();
  Serial.printf(
    " value=%u",
    static_cast<unsigned>(data.getUInt())
  );
}

static void printUInt8(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::UInt8>();
  Serial.printf(
    " value=%u",
    static_cast<unsigned>(data.get())
  );
}

static void printVentilationConfig(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::VentilationConfig>();
  Serial.printf(
    " configFlags=%u memberId=%u heatRecoverySystem=%u bypass=%u variableSpeed=%u",
    static_cast<unsigned>(data.getConfigFlags()),
    static_cast<unsigned>(data.getMemberId()),
    static_cast<unsigned>(data.isHeatRecoverySystem()),
    static_cast<unsigned>(data.hasBypass()),
    static_cast<unsigned>(data.isVariableSpeed())
  );
}

static void printVentilationFault(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::VentilationFault>();
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

static void printVentilationRemoteParameters(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::VentilationRemoteParameters>();
  Serial.printf(
    " transferFlags=%u readWriteFlags=%u nominalVentilationTransferEnabled=%u"
    " nominalVentilationWritable=%u",
    static_cast<unsigned>(data.getTransferFlags()),
    static_cast<unsigned>(data.getReadWriteFlags()),
    static_cast<unsigned>(data.isNominalVentilationTransferEnabled()),
    static_cast<unsigned>(data.isNominalVentilationWritable())
  );
}

static void printVentilationStatus(const ESP32OT::Messages::Message& result) {
  const auto data = result.as<ESP32OT::Messages::VentilationStatus>();
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

enum class ReadMode : uint8_t { Single, Count, Indexed };

struct ReadableID {
  ESP32OT::MessageID id;
  const char* name;
  void (*printData)(const ESP32OT::Messages::Message& result);
  ReadMode mode = ReadMode::Single;
};

// Readable IDs from the OpenTherm v4.2.1 overview map; write-only IDs are omitted.
// Each printer converts the received snapshot to its existing Message wrapper.
// Each Count row immediately precedes its Indexed row for TSP or fault history.
static const ReadableID readableIDs[] = {
  {ESP32OT::MessageID::Status, "Status", printStatus},
  {ESP32OT::MessageID::SConfigSMemberIDcode, "SConfigSMemberIDcode", printSlaveConfig},
  {ESP32OT::MessageID::ASFflags, "ASFflags", printFault},
  {ESP32OT::MessageID::RBPflags, "RBPflags", printRemoteParameters},
  {ESP32OT::MessageID::TrOverride, "TrOverride", printFloat},
  {ESP32OT::MessageID::TSP, "TSP", printParameterCount, ReadMode::Count},
  {ESP32OT::MessageID::TSPindexTSPvalue, "TSPindexTSPvalue", printIndexedValue, ReadMode::Indexed},
  {ESP32OT::MessageID::FHBsize, "FHBsize", printParameterCount, ReadMode::Count},
  {ESP32OT::MessageID::FHBindexFHBvalue, "FHBindexFHBvalue", printIndexedValue, ReadMode::Indexed},
  {ESP32OT::MessageID::MaxCapacityMinModLevel, "MaxCapacityMinModLevel", printCapacity},
  {ESP32OT::MessageID::RelModLevel, "RelModLevel", printFloat},
  {ESP32OT::MessageID::CHPressure, "CHPressure", printFloat},
  {ESP32OT::MessageID::DHWFlowRate, "DHWFlowRate", printFloat},
  {ESP32OT::MessageID::DayTime, "DayTime", printDayTime},
  {ESP32OT::MessageID::Date, "Date", printDate},
  {ESP32OT::MessageID::Year, "Year", printUInt16},
  {ESP32OT::MessageID::Tboiler, "Tboiler", printFloat},
  {ESP32OT::MessageID::Tdhw, "Tdhw", printFloat},
  {ESP32OT::MessageID::Toutside, "Toutside", printFloat},
  {ESP32OT::MessageID::Tret, "Tret", printFloat},
  {ESP32OT::MessageID::Tstorage, "Tstorage", printFloat},
  {ESP32OT::MessageID::Tcollector, "Tcollector", printFloat},
  {ESP32OT::MessageID::TflowCH2, "TflowCH2", printFloat},
  {ESP32OT::MessageID::Tdhw2, "Tdhw2", printFloat},
  {ESP32OT::MessageID::Texhaust, "Texhaust", printInt16},
  {ESP32OT::MessageID::TboilerHeatExchanger, "TboilerHeatExchanger", printFloat},
  {ESP32OT::MessageID::BoilerFanSpeedSetpointAndActual, "BoilerFanSpeedSetpointAndActual", printFanSpeed},
  {ESP32OT::MessageID::FlameCurrent, "FlameCurrent", printFloat},
  {ESP32OT::MessageID::RelativeHumidity, "RelativeHumidity", printFloat},
  {ESP32OT::MessageID::TrOverride2, "TrOverride2", printFloat},
  {ESP32OT::MessageID::TdhwSetUBTdhwSetLB, "TdhwSetUBTdhwSetLB", printTemperatureBounds},
  {ESP32OT::MessageID::MaxTSetUBMaxTSetLB, "MaxTSetUBMaxTSetLB", printTemperatureBounds},
  {ESP32OT::MessageID::TdhwSet, "TdhwSet", printFloat},
  {ESP32OT::MessageID::MaxTSet, "MaxTSet", printFloat},
  {ESP32OT::MessageID::StatusVentilationHeatRecovery, "StatusVentilationHeatRecovery", printVentilationStatus},
  {ESP32OT::MessageID::ASFflagsOEMfaultCodeVentilationHeatRecovery, "VentilationFaults", printVentilationFault},
  {ESP32OT::MessageID::OEMDiagnosticCodeVentilationHeatRecovery, "VentilationDiagnosticCode", printUInt16},
  {ESP32OT::MessageID::SConfigSMemberIDCodeVentilationHeatRecovery, "VentilationConfig", printVentilationConfig},
  {ESP32OT::MessageID::OpenThermVersionVentilationHeatRecovery, "VentilationOpenThermVersion", printFloat},
  {ESP32OT::MessageID::VentilationHeatRecoveryVersion, "VentilationHeatRecoveryVersion", printProductVersion},
  {ESP32OT::MessageID::RelVentLevel, "RelVentLevel", printUInt8},
  {ESP32OT::MessageID::RHexhaust, "RHexhaust", printUInt8},
  {ESP32OT::MessageID::CO2exhaust, "CO2exhaust", printUInt16},
  {ESP32OT::MessageID::Tsi, "Tsi", printFloat},
  {ESP32OT::MessageID::Tso, "Tso", printFloat},
  {ESP32OT::MessageID::Tei, "Tei", printFloat},
  {ESP32OT::MessageID::Teo, "Teo", printFloat},
  {ESP32OT::MessageID::RPMexhaust, "RPMexhaust", printUInt16},
  {ESP32OT::MessageID::RPMsupply, "RPMsupply", printUInt16},
  {ESP32OT::MessageID::RBPflagsVentilationHeatRecovery, "VentilationRBPflags", printVentilationRemoteParameters},
  {ESP32OT::MessageID::NominalVentilationValue, "NominalVentilationValue", printNominalVentilation},
  {ESP32OT::MessageID::TSPventilationHeatRecovery, "TSPventilationHeatRecovery", printParameterCount, ReadMode::Count},
  {ESP32OT::MessageID::TSPindexTSPvalueVentilationHeatRecovery, "VentilationTSPindexTSPvalue", printIndexedValue, ReadMode::Indexed},
  {ESP32OT::MessageID::FHBsizeVentilationHeatRecovery, "FHBsizeVentilationHeatRecovery", printParameterCount, ReadMode::Count},
  {ESP32OT::MessageID::FHBindexFHBvalueVentilationHeatRecovery, "VentilationFHBindexFHBvalue", printIndexedValue, ReadMode::Indexed},
  {ESP32OT::MessageID::Brand, "Brand", printBrandCharacter},
  {ESP32OT::MessageID::BrandVersion, "BrandVersion", printBrandCharacter},
  {ESP32OT::MessageID::BrandSerialNumber, "BrandSerialNumber", printBrandCharacter},
  {ESP32OT::MessageID::CoolingOperationHours, "CoolingOperationHours", printUInt16},
  {ESP32OT::MessageID::PowerCycles, "PowerCycles", printUInt16},
  {ESP32OT::MessageID::RemoteOverrideOperatingModeHeatingDHW, "RemoteOverrideOperatingModeHeatingDHW", printRemoteOverrideMode},
  {ESP32OT::MessageID::RemoteOverrideFunction, "RemoteOverrideFunction", printRemoteOverrideFunction},
  {ESP32OT::MessageID::StatusSolarStorage, "StatusSolarStorage", printSolarStorageStatus},
  {ESP32OT::MessageID::ASFflagsOEMfaultCodeSolarStorage, "SolarStorageFaults", printSolarStorageFault},
  {ESP32OT::MessageID::SConfigSMemberIDcodeSolarStorage, "SolarStorageConfig", printSolarStorageConfig},
  {ESP32OT::MessageID::SolarStorageVersion, "SolarStorageVersion", printProductVersion},
  {ESP32OT::MessageID::TSPSolarStorage, "TSPSolarStorage", printParameterCount, ReadMode::Count},
  {ESP32OT::MessageID::TSPindexTSPvalueSolarStorage, "TSPindexTSPvalueSolarStorage", printIndexedValue, ReadMode::Indexed},
  {ESP32OT::MessageID::FHBsizeSolarStorage, "FHBsizeSolarStorage", printParameterCount, ReadMode::Count},
  {ESP32OT::MessageID::FHBindexFHBvalueSolarStorage, "FHBindexFHBvalueSolarStorage", printIndexedValue, ReadMode::Indexed},
  {ESP32OT::MessageID::ElectricityProducerStarts, "ElectricityProducerStarts", printUInt16},
  {ESP32OT::MessageID::ElectricityProducerHours, "ElectricityProducerHours", printUInt16},
  {ESP32OT::MessageID::ElectricityProduction, "ElectricityProduction", printUInt16},
  {ESP32OT::MessageID::CumulativElectricityProduction, "CumulativElectricityProduction", printUInt16},
  {ESP32OT::MessageID::UnsuccessfulBurnerStarts, "UnsuccessfulBurnerStarts", printUInt16},
  {ESP32OT::MessageID::FlameSignalTooLowNumber, "FlameSignalTooLowNumber", printUInt16},
  {ESP32OT::MessageID::OEMDiagnosticCode, "OEMDiagnosticCode", printUInt16},
  {ESP32OT::MessageID::SuccessfulBurnerStarts, "SuccessfulBurnerStarts", printUInt16},
  {ESP32OT::MessageID::CHPumpStarts, "CHPumpStarts", printUInt16},
  {ESP32OT::MessageID::DHWPumpValveStarts, "DHWPumpValveStarts", printUInt16},
  {ESP32OT::MessageID::DHWBurnerStarts, "DHWBurnerStarts", printUInt16},
  {ESP32OT::MessageID::BurnerOperationHours, "BurnerOperationHours", printUInt16},
  {ESP32OT::MessageID::CHPumpOperationHours, "CHPumpOperationHours", printUInt16},
  {ESP32OT::MessageID::DHWPumpValveOperationHours, "DHWPumpValveOperationHours", printUInt16},
  {ESP32OT::MessageID::DHWBurnerOperationHours, "DHWBurnerOperationHours", printUInt16},
  {ESP32OT::MessageID::OpenThermVersionSlave, "OpenThermVersionSlave", printFloat},
  {ESP32OT::MessageID::SlaveVersion, "SlaveVersion", printProductVersion}
};
static ESP32OT::ESP32OT boiler;
static size_t nextID = 0;
static uint8_t entryCount = 1;
static uint8_t entryIndex = 0;

void setup() {
  Serial.begin(115200);
  ESP32OT::ESP32OT::Config config;
  config.rxGpio = static_cast<gpio_num_t>(MASTER_RX_GPIO);
  config.txGpio = static_cast<gpio_num_t>(MASTER_TX_GPIO);
  config.maxAttempts = 10;
  boiler.setConfig(config);
}

void loop() {
  if (!boiler.isInitialized()) {
    if (!boiler.begin()) {
      Serial.printf("OpenTherm initialization failed: %ld\n", static_cast<long>(boiler.getLastError()));
      delay(1000);
      return;
    }
  }

  if (readableIDs[nextID].mode == ReadMode::Indexed && entryCount == 0) {
    nextID = (nextID + 1) % (sizeof(readableIDs) / sizeof(readableIDs[0]));
  }

  const auto& item = readableIDs[nextID];

  // Indexed reads put the index in HB and zero in LB. Status keeps all master flags clear.
  auto request = ESP32OT::Messages::Message::read(item.id);
  if (item.mode == ReadMode::Indexed) {
    request = ESP32OT::Messages::IndexedValue(item.id, entryIndex);
  }

  auto result = boiler.sendRequest(request);
  Serial.printf(
    "ID %3u (%s): %*s%-15s", static_cast<unsigned>(item.id), item.name,
    37 - static_cast<int>(strlen(item.name)), "",
    ESP32OT::Protocol::Frame::statusToString(result.getStatus())
  );

  // Terminal statuses can have no frame; a full invalid frame is still useful to log.
  if (result.hasFrame()) {
    Serial.printf(
      " response=%08lX type=%s", static_cast<unsigned long>(result.getRaw()),
      ESP32OT::Protocol::Frame::typeToString(result.getType())
    );
  }

  if (item.mode == ReadMode::Indexed) {
    Serial.printf(" requestIndex=%u", static_cast<unsigned>(entryIndex));
  }

  if (result.isSuccess()) {
    Serial.printf(" data=0x%04X", static_cast<unsigned>(result.getData()));
    item.printData(result);
  }

  Serial.println();

  if (item.mode == ReadMode::Count) {
    // An unavailable count keeps a single index-0 probe; a known empty group is skipped.
    entryCount = result.isSuccess() ? result.as<ESP32OT::Messages::ParameterCount>().getCount() : 1;
  }

  if (item.mode == ReadMode::Indexed && entryIndex + 1 < entryCount) {
    entryIndex++;

  } else {
    entryIndex = 0;
    nextID = (nextID + 1) % (sizeof(readableIDs) / sizeof(readableIDs[0]));
  }

  delay(nextID == 0 ? 10000 : 1);
}
