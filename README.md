# ESP32OT

[Russian](README.ru.md)

[Complete API reference](docs/API.md)

OpenTherm communication for ESP32 using hardware RMT transmit/receive and an
internal FreeRTOS worker. ESP32OT supports master and slave roles, typed
messages, asynchronous and synchronous calls, and a two-interface transparent proxy.

## Acknowledgements

**ESP32OT exists thanks to the work of Ihor Melnyk and his original
[OpenTherm library](https://github.com/ihormelnyk/opentherm_library).**
His implementation provided the foundation from which this project evolved.

ESP32OT was developed as part of the work on
[OTGateway](https://github.com/Laxilef/OTGateway). The standalone project's
repository is [Laxilef/ESP32OT](https://github.com/Laxilef/ESP32OT).
It can be used independently of OTGateway.

## Requirements

- Arduino-ESP32 and C++17 or newer.
- ESP-IDF 6 RMT APIs. The development configuration uses the pioarduino
  `prep_IDF6` branch; older toolchains are not a supported fallback.
- ESP32, ESP32-S2, ESP32-S3, ESP32-C3 or ESP32-C6 with available RMT resources.
- An electrical OpenTherm interface for each bus. Do not connect an ESP32 GPIO
  directly to the OpenTherm line.

## Installation and example project

The public header is `ESP32OT.h`; the class is `ESP32OT::ESP32OT`.
A local copy can be placed under your
PlatformIO project's `lib/ESP32OT` directory, or installed as an Arduino library.
An Arduino-aware ESP-IDF project can also use the included [CMakeLists.txt](CMakeLists.txt).

The repository's [platformio.ini](platformio.ini) builds a selected example as
an application and discovers ESP32OT itself as a manifest-based library.
Run these commands from the library root:

```sh
pio run
pio run -e esp32-c3
pio run -e esp32-c6
```

The default environment is `esp32-s3`; the default sketch is `examples/Master`.
Other environments are `esp32`, `esp32-s2`, `esp32-c3`, `esp32-c6` and `ci`.
Select one `src_dir` entry in `platformio.ini`, or override it with an absolute
`PLATFORMIO_SRC_DIR` environment variable. For example, in PowerShell:

```powershell
$env:PLATFORMIO_SRC_DIR = (Resolve-Path examples/Slave).Path
pio run -e esp32-s3
Remove-Item Env:PLATFORMIO_SRC_DIR
```

For CI, set `PIO_BOARD` to a supported board ID and use `pio run -e ci`.
`lib_deps = ESP32OT=symlink://.` connects this checkout through `library.json`
and compiles all nested `src/` files. `lib_dir = .pio/lib` keeps discovery local
to this project. Selecting a sketch does not change library sources.
The S3 profile uses USB-Serial/JTAG and a watchdog reset after upload.
Master examples, including `MasterHeap` and `ReadAll`, use `MASTER_RX_GPIO` /
`MASTER_TX_GPIO`; the S3 profile sets RX=35 / TX=36, with sketch fallbacks of 4 / 5.
`MASTER_*_GPIO` selects the ESP32's master interface connected to a boiler;
`SLAVE_RX_GPIO` / `SLAVE_TX_GPIO` selects its slave interface connected to a
thermostat. `Slave` defaults to GPIO4/5; the proxy's slave interface defaults
to GPIO6/7, or GPIO18/19 on the original ESP32.
The configuration keeps the framework's C++ standard and uses no OTGateway
settings, secrets or build scripts. Its development platform branch is not a
stable SDK release.

## Quick start

```cpp
#include <ESP32OT.h>

static ESP32OT::ESP32OT boiler(GPIO_NUM_4, GPIO_NUM_5);

void setup() {
  Serial.begin(115200);
}

void loop() {
  // ESP32OT initialization does not prove that a boiler is connected.
  if (!boiler.isInitialized()) {
    if (!boiler.begin()) {
      delay(1000);
      return;
    }
  }

  // Status uses ID0. Default master flags do not request heating.
  auto status = boiler.sendRequest(ESP32OT::Messages::Status());
  if (status.isSuccess()) {
    Serial.printf("Flame: %u\n", static_cast<unsigned int>(status.isFlameOn()));
  }

  delay(750);
}
```

Choose `gpio_num_t` values for your board and interface. `begin()` starts the
worker and RMT; a valid peer response is separate from initialization.
Calling `begin()` on an active instance returns `false` until a successful `end()`.
`isReady()` reports bus availability, including the inter-frame delay, rather
than a new response waiting to be read.

## Configuration and lifetime

```cpp
ESP32OT::ESP32OT::Config config;
config.rxGpio = GPIO_NUM_4;
config.txGpio = GPIO_NUM_5;
config.isSlave = false;
config.alwaysReceive = false;
config.maxAttempts = 5;

ESP32OT::ESP32OT boiler(config);
```

The positional constructor is also available:

```cpp
ESP32OT::ESP32OT boiler(GPIO_NUM_4, GPIO_NUM_5, false, false);
```

| Field | Default | Meaning |
| --- | --- | --- |
| `rxGpio`, `txGpio` | `GPIO_NUM_NC` | OpenTherm interface GPIOs |
| `isSlave` | `false` | Master by default; `true` selects slave |
| `alwaysReceive` | `false` | Master also listens between requests when enabled |
| `maxAttempts` | `5` | Total synchronous master attempts; zero means one |
| `stackSize` | `3072` bytes | Initial worker stack allocation |
| `priority` | `5` | FreeRTOS worker priority |
| `core` | `0` | Worker and RMT owner core |
| `acceptInvalidRequests` | `false` | Give complete invalid slave requests a routing sequence |

A slave always listens between replies, regardless of `alwaysReceive`.
A master with `alwaysReceive=true` can also report unsolicited full frames
through `onResponse()`.

`getConfig()` returns `const Config&`. To modify settings, copy the current
configuration and call `setConfig()` while the instance is stopped:

```cpp
auto config = boiler.getConfig();
config.maxAttempts = 3;
boiler.setConfig(config);
```

`setConfig()` replaces the complete configuration. A role change clears both
request and response hooks; register them after selecting the role. The status
callback remains registered.
`begin(rxGpio, txGpio)` updates saved GPIOs only after successful startup.
Serialize lifecycle and configuration calls for each instance.

`end()` cooperatively stops the worker and deletes TX before RX. If cleanup
fails, resources remain owned so `end()` can be retried. After a fatal driver
error, `isInitialized()` may be false while resources still require `end()`;
calling `begin()` alone does not recover that state. A destructor that cannot
stop its worker fails fast instead of allowing access to destroyed storage.

`getLastError()` returns the SDK error. `getWorkerStackHighWaterMark()` reports
the minimum remaining worker stack in bytes. The default stack size is a
starting point, not a measured safe minimum for every callback.

## Messages and payloads

`ESP32OT::Messages::Message` owns a raw 32-bit frame and operation metadata.
Its `build()` returns the frame; `parse(snapshot)` replaces the snapshot.
Typed wrappers inherit it and decode fields on demand, without another payload
cache or a heap allocation per message.

All types are available through `ESP32OT.h`. Protocol enums have short names:
`ESP32OT::MessageID`, `ESP32OT::MessageType` and `ESP32OT::OperationStatus`.

```cpp
// A value selects WRITE_DATA; the ID-only constructor selects READ_DATA.
auto setpoint = boiler.sendRequest(
  ESP32OT::Messages::Float(ESP32OT::MessageID::TSet, 55.0f)
);
auto temperature = boiler.sendRequest(
  ESP32OT::Messages::Float(ESP32OT::MessageID::Tboiler)
);
if (temperature.isSuccess()) {
  float value = temperature.get();
}
```

A typed synchronous call returns the same wrapper type. A call through
`const Message&` returns `Message`. For a received raw message, explicitly
select a payload view:

```cpp
auto config = request.as<ESP32OT::Messages::SlaveConfig>();
bool dhwPresent = config.isDhwPresent();
```

`as<T>()` creates an independent value copy, without RTTI or heap allocation.
The caller chooses the format and checks the operation status before reading
fields. A response to an active master request must have the same ID; a different
ID produces `INVALID`. Unsolicited passive-RX frames have no current request to
correlate with, so choose their wrapper from the received ID.

| Wrappers in `ESP32OT::Messages` | Fields / IDs |
| --- | --- |
| `Float`, `UInt8`, `UInt16`, `Int16` | Signed f8.8, low-byte u8, u16, s16 |
| `Status` | Requested master and actual slave flags, ID0 |
| `MasterConfig`, `SlaveConfig` | Capabilities and member ID, IDs2/3 |
| `RemoteRequest`, `Fault`, `RemoteParameters` | Command/result, faults, support/write flags, IDs4/5/6 |
| `Capacity`, `FanSpeed`, `TemperatureBounds` | kW and modulation %, fan Hz, signed upper/lower bounds, IDs15/35/48/49 |
| `ParameterCount`, `IndexedValue` | TSP/fault-history counts and entries |
| `ProductVersion`, `BrandCharacter` | Product type/version and indexed ASCII characters |
| `Date`, `DayTime` | Month/day and day-of-week/hour/minute, IDs21/20 |
| `VentilationStatus`, `VentilationFault`, `VentilationConfig` | Ventilation flags and fields, IDs70/72/74 |
| `VentilationRemoteParameters`, `NominalVentilation` | Support/write flags and nominal ventilation in HB, IDs86/87 |
| `RFSensorStatus`, `RemoteOverrideMode` | RF sensor/battery/signal and separate CH/DHW modes, IDs98/99 |
| `RemoteOverrideFunction` | Manual/program change priority, ID100 |
| `SolarStorageStatus`, `SolarStorageFault`, `SolarStorageConfig` | Solar modes/state, OEM code and configuration/member ID, IDs101/102/103 |

Numeric accessors are `Float::get()`, `UInt8::get()`, `UInt16::getUInt()` and
`Int16::getInt()`. `Float` supports signed f8.8 from -128 to 127.99609375 with
truncation when encoding. `Float::writeTemperature()` explicitly retains the
legacy 0..100 clamp; it does not apply to ordinary `Float` messages.

Structured wrappers expose named fields and flag setters, like `Status`.
For example, `SlaveConfig::isDhwPresent()` reports capability, whereas
`RemoteParameters::isDhwSetpointWritable()` reports permission. Support and
write permissions remain separate. Flag setters retain unrelated bits.

`reply()` builds a typed response while preserving its ID and request context:

```cpp
// In the ID3 READ_DATA branch of a slave request handler.
auto requestConfig = request.as<ESP32OT::Messages::SlaveConfig>();
auto response = requestConfig.reply(ESP32OT::MessageType::READ_ACK, 0, 42);
response.setDhwPresent(true).setCh2Present(true);
slave.sendResponseAsync(response);
```

`IndexedValue(id, index)` builds an indexed read; a third value builds a write.
`BrandCharacter` handles its different ACK format: the request contains a
character index, while the response HB contains a maximum index.
`getRequestIndex()` reads saved request metadata; `getMaxIndex()` reads the response.
It does not allocate or assemble a full string.

### Compatibility choices

Payload layouts follow the supplied OpenTherm v4.2 specification. Existing
vendor tolerance is deliberately retained. Wrappers encode field widths and
preserve received raw bits; application-specific ranges are caller policy.

- ID30: the detailed table says s16, while the overview says f8.8. The existing
  OTGateway interpretation uses f8.8; use `Float` to retain that behavior.
- ID100: `RemoteOverrideFunction` follows the detailed table's LB bits0/1;
  the overview's `flag8/-` description differs.
- ID103: the configuration/member-ID table contradicts its system-type-bit
  description. `SolarStorageConfig` exposes the configuration byte and member
  ID without guessing that bit's semantic meaning.
- ID99: CH mode 2 means comfort, DHW mode 2 means anti-legionella. Separate
  enums preserve those meanings. Mode zero means no override; OFF is 6.
- Zero is valid for several fields, including ID56 and day-of-week zero
  (day unknown). The library does not use a blanket zero-temperature rejection.

## Synchronous and asynchronous operations

`sendRequest()` waits for completion and retries master `INVALID` results,
timeouts and recoverable errors up to `Config::maxAttempts`, including the
first attempt. `SUCCESS`, valid `DATA_INVALID` or `UNKNOWN_DATA_ID` replies,
cancellation or shutdown stop retries. Failed attempts remain observable in
callbacks even when a later attempt succeeds. `sendResponse()` transmits a
slave reply once and waits for TX completion, not a peer ACK.

The caller yields in 10 ms steps. A custom wait hook uses a function pointer
and a context pointer:

```cpp
using DelayCallback = void (*)(unsigned int milliseconds, void* context);
ESP32OT& setDelayCallback(DelayCallback callback = nullptr, void* context = nullptr);
```

Set or clear it only while the instance is stopped, before `begin()` or after
a successful `end()`; changes while running are ignored. The facade stores
the two pointers without allocating or owning context storage. The caller
must keep that storage valid during callback calls. `setDelayCallback()` with
no arguments restores the default `vTaskDelay()` with at least one tick.
RMT timing runs independently. Use the delay callback only for waiting/yielding;
do not start another synchronous operation on the same ESP32OT from it.

`sendRequestAsync()` and `sendResponseAsync()` copy the submitted message and
return immediately. `true` means accepted, not delivered or completed. Async
performs one attempt. One operation can be pending/running per instance;
a new command is rejected while that slot is occupied.

Poll an independent `getResult()` / `getResult<T>()` snapshot, fill an existing
wrapper with `getResult(destination)`, or receive callbacks. `getRequest()`
provides the last slave request. No external `process()` call is needed.

| Callback | Master role | Slave role |
| --- | --- | --- |
| `onStatusChange()` | Public status changes, including failures without a full frame | Public status changes, including failures without a full frame |
| `onRequest()` | Outgoing request before the local TX attempt | Full received request; normal valid requests use `REQUEST_RECEIVED` |
| `onResponse()` | Full received response, including invalid/negative replies | Outgoing response before the local TX attempt |

`MessageCallback` and `StatusCallback` are single-argument `std::function`
handlers and support capturing lambdas. For example, on a master:

```cpp
const char* interfaceName = "boiler";
boiler.onRequest([interfaceName](const ESP32OT::Messages::Message& request) {
  Serial.printf(
    "%s TX=%08lX\n",
    interfaceName,
    static_cast<unsigned long>(request.getRaw())
  );
});
boiler.onResponse([interfaceName](const ESP32OT::Messages::Message& response) {
  Serial.printf(
    "%s RX=%08lX\n",
    interfaceName,
    static_cast<unsigned long>(response.getRaw())
  );
});
```

Outgoing notifications use `TRANSMITTING` with `hasFrame() == false`, once per
attempt before the transport attempts to hand the frame to the driver. Later
cancellation or failure can still prevent TX. They confirm neither TX completion
nor delivery. Incoming notifications
still guarantee a complete received frame; success remains a separate check.

Existing function-pointer/context registrations remain supported through
`LegacyMessageCallback` and `LegacyStatusCallback` overloads. Call a setter
without arguments or with `nullptr` to clear its hook. `DelayCallback` retains
its function-pointer/context signature and stopped-only configuration.

Callbacks execute in the instance's worker, outside ISR and internal locks.
The supplied message reference is valid only during the callback; retain a
value copy for later use. Synchronous send and lifecycle calls are rejected
from the same worker. Forwarding to another instance asynchronously is supported.
Installing a nonempty observer allocates shared callable storage; captures may
allocate additional memory. Dispatch retains shared ownership instead of copying
the callable. The closure is owned, while objects referenced through captures,
`this` or legacy context pointers remain caller-owned.
Replacing or clearing a hook is not a callback-completion barrier; retained
captures or context must remain valid until any running or snapshotted call ends.
For deterministic replacement, stop the instance successfully, then replace
the hook before restarting it. Keep TX observers short: their duration delays
the start of transmission.

Use `isComplete()` to identify terminal operation status and `hasFrame()` to
identify a complete received frame. A timeout or partial reception can finish
without one; do not present `getRaw()` as a complete response in that case.
A full frame can still be invalid: `isSuccess()` is a separate check.
Slave TX success means transmission completed; `hasFrame()` remains false.

The terminal statuses are `SUCCESS`, `DATA_INVALID`, `UNKNOWN_DATA_ID`,
`INVALID`, `TIMEOUT`, `ERROR` and `CANCELLED`. A properly received negative
reply has its own status: `DATA_INVALID` means the requested data is unavailable
or the operation was not accepted; `UNKNOWN_DATA_ID` means the ID is unsupported.
These are valid protocol responses, so `isComplete()` and `hasFrame()` are true,
while `isSuccess()` remains false. Check `isSuccess()` before interpreting the
payload as requested data. Bad framing, parity, response type or a mismatched
active-request ID produces `INVALID` instead.
`INVALID` completes an individual attempt; a synchronous master call can retry
it while attempts remain. Async operations make only one attempt.

Result metadata includes status, SDK error, attempt number, operation and
request sequence, raw request/response, `elapsedUs` for an attempt and
`totalElapsedUs` for the synchronous call. Slave replies must match the current
request sequence; replies from an earlier request or session are rejected.

## Examples

See [the example guide](examples/README.md):

- [Master](examples/Master/Master.ino): synchronous responses handled in `loop()`.
- [MasterCallbacks](examples/MasterCallbacks/MasterCallbacks.ino): synchronous
  sends with per-attempt completion callbacks.
- [MasterAsync](examples/MasterAsync/MasterAsync.ino): asynchronous Status polling,
  using native readiness and a minimum 750 ms application interval.
- [MasterHeap](examples/MasterHeap/README.md): heap/stack and free-block layout
  measurements, fragmentation checks, master lifecycle and all message wrappers.
- [ReadAll](examples/ReadAll/README.md): sequential reads of all 87 readable IDs,
  including all reported TSP and fault-history entries, with response status and
  successful payloads printed from `loop()`.
- [TestAllIDs](examples/TestAllIDs/README.md): sequential ID tests, indexed
  readings, an observed access map and status statistics; write tests require
  explicit configuration and are disabled by default.
- [Slave](examples/Slave/Slave.ino): a small simulated slave with explicit replies.
- [TransparentProxy](examples/TransparentProxy/TransparentProxy.ino): thermostat
  and boiler interfaces forwarding raw frames in both directions.

The proxy preserves raw bits, including complete invalid frames, and request
routing metadata. `acceptInvalidRequests=true` is enabled on its slave side.
A new thermostat request can supersede an old one; a stale boiler reply is
rejected. No synthetic reply or application-level retry is added.

Example callbacks print directly to Serial for clarity. This does not establish
that a production gateway meets the protocol's 7 ms forwarding requirement.

## RMT resources and timing

Each instance owns a pinned worker, a TX channel, an RX channel and two fixed
64-symbol RX buffers. RMT interrupts are allocated on the owner core. RX is
armed from TX_DONE so task scheduling does not determine the start of capture.
There is no per-edge application GPIO ISR or application-wide interrupt mask.

| SoC | TX / RX hardware symbols per instance | Two interfaces |
| --- | --- | --- |
| ESP32 | 64 / 64 | Automatic allocation |
| ESP32-S2 | 64 / 64 | Automatic allocation |
| ESP32-S3 | 48 / 96 | Two RX blocks per interface |
| ESP32-C3 | 48 / 48 | SDK RX ping-pong assembles a full capture |
| ESP32-C6 | 48 / 48 | SDK RX ping-pong assembles a full capture |

No per-operation allocation is required. Worker/driver storage is allocated
at startup; callback stack demand must be measured in the actual application.

TX sends start + 32 data bits MSB-first + stop, with 500 us half-bits. Existing
RX start/sample/stop tolerance and partial-capture state are retained.
RX supports asymmetric half-bit pulses. When a complete capture requires phase
recovery, it accepts only an unambiguous Manchester interpretation derived from
pulse timing. Parity, response-type and active-request ID checks follow that
selection; the expected ID or parity is not used to choose a decoding, and parity
is not repaired. Ambiguous recovery remains `INVALID`; a synchronous master
call can retry it while attempts remain.

The receive inactivity deadline is **1,042,500 us**: 1 s + 40 ms frame grace
plus 2.5 ms capture idle. MWT is 100 ms for master and 20 ms for slave, measured
from RX_DONE; the TX watchdog is 1 s. A completed capture is processed before
timeout. Timestamp resolution and worker wakeup latency are different quantities.

Gateway forwarding must begin within 7 ms under OpenTherm v4.2 section 4.3.2.
RMT's 2.5 ms idle already consumes part of that budget. Hardware testing must
include both interfaces, scheduling and cache effects before claiming compliance.

## Development and verification

[AGENTS.md](AGENTS.md) records project rules and the protocol/architecture
choices that must survive future work. Keep this README and [README.ru.md](README.ru.md)
in sync, and keep code comments in English.

## License

ESP32OT is distributed under [GPL-3.0-only](LICENSE).
