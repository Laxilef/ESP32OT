# ESP32OT examples

[Русский](README.ru.md)

Each directory contains a separate Arduino sketch using the `ESP32OT` library
in this repository.

| Example | Purpose |
| --- | --- |
| [Master](Master/Master.ino) | Synchronous Status, TSet write and Tboiler read; handles returned results in `loop()` |
| [MasterHeap](MasterHeap/MasterHeap.ino) | Master heap allocation, release and restart scenarios; [procedure and coverage](MasterHeap/README.md) |
| [ReadAll](ReadAll/ReadAll.ino) | Sequential reads of all 87 readable IDs and all reported TSP/fault-history entries in `loop()`; prints response status and successful payloads; [usage](ReadAll/README.md) |
| [TestAllIDs](TestAllIDs/TestAllIDs.ino) | Sequential ID tests in `loop()`, indexed readings, an observed access map and status totals; writes disabled by default; [usage](TestAllIDs/README.md) |
| [MasterCallbacks](MasterCallbacks/MasterCallbacks.ino) | The same synchronous requests; handles and logs attempt completions in a callback |
| [MasterAsync](MasterAsync/MasterAsync.ino) | Asynchronous Status requests at a minimum interval of 750 ms when the bus is ready; receives results in a callback |
| [Slave](Slave/Slave.ino) | Simulated slave supporting Status, TSet, Tboiler and RelModLevel |
| [TransparentProxy](TransparentProxy/TransparentProxy.ino) | Two buses: forwards thermostat requests to a boiler and returns its responses |

## Selecting an example

Open this repository as a PlatformIO project. In the included
[platformio.ini](../platformio.ini), select one sketch with `src_dir`:

```ini
[platformio]
src_dir = examples/Master
```

The alternative directories are listed in the same file. Select the board
environment matching your hardware: `esp32-s3` is the default; `esp32-s2`,
`esp32-c3`, `esp32` and `esp32-c6` are also provided. The `ci` environment reads
its board ID from `PIO_BOARD`. The configuration uses pioarduino's
`prep_IDF6` branch because the library requires ESP-IDF 6 RMT APIs. No parent
firmware project is required.

All master examples, including `MasterHeap` and `ReadAll`, accept
`MASTER_RX_GPIO` / `MASTER_TX_GPIO`. `Slave` accepts `SLAVE_RX_GPIO` /
`SLAVE_TX_GPIO`; the proxy accepts both pairs. The names describe the ESP32's
role: its master interface connects to a boiler and its slave interface to a
thermostat. The S3 profile sets master RX=35 / TX=36; sketch fallbacks are 4/5.
`Slave` defaults to 4/5. The proxy's slave interface defaults to GPIO6/7, or
GPIO18/19 on the original ESP32 because GPIO6/7 normally connect to flash on
those modules. Select GPIOs for your actual board and circuit, including USB,
flash and other peripherals. GPIOs must not connect directly to an OpenTherm
line: use suitable electrical interfaces.

In `TransparentProxy`, each GPIO can be overridden without editing the sketch.
For example, append these definitions to the selected environment's existing
`build_flags`:

```ini
  -DMASTER_RX_GPIO=4
  -DMASTER_TX_GPIO=5
  -DSLAVE_RX_GPIO=6
  -DSLAVE_TX_GPIO=7
```

Unspecified definitions keep their defaults. The sketch explicitly converts
the numeric values to `gpio_num_t`.

## Master and slave behavior

`Master` and `MasterCallbacks` write TSet = 55 C while heating defaults to
`HEATING_ENABLED = false`: the setpoint and enable flag are separate values.
All three master examples retry unsuccessful initial startup from `loop()`.
A successful `begin()` starts the driver and worker; responses confirm that a
boiler is present. After a fatal driver error, an explicit `end()` may be
required before restarting. The examples do not implement automatic recovery
from such an error.

Start with `Master`: `sendRequest()` waits for completion and returns an
independent message value. `loop()` logs the raw request, any full response and
the final status, then reads successful results. Synchronous requests use
`Config::maxAttempts`, which defaults to 5. `INVALID`, timeouts and recoverable
errors can be retried; successful ACKs and valid `DATA_INVALID` /
`UNKNOWN_DATA_ID` replies stop retries. Reception, inter-frame delays and
yielding while waiting are handled internally, so these sketches need no
delay callback.

`MasterCallbacks` sends the same synchronous requests, but `onStatusChange()`
handles each attempt's completion. Its `onRequest()` lambda keeps an attempt
counter in its capture and logs the outgoing frame before TX. Retries can
produce several log entries
for one `sendRequest()`. Callbacks run in the internal worker; references to
callback messages are valid only during the call. Do not make a synchronous
send from the same instance's worker callback.

`MasterAsync` sends Status with zero master flags without waiting. `true` from
`sendRequestAsync()` means a command was accepted; its result arrives later in
the callback. Each asynchronous command makes one attempt. `isReady()` accounts
for the exchange and inter-frame gap, so no separate waiting flag or
`getResult()` polling is needed. The 750 ms interval is the minimum between
accepted requests; a busy bus can increase it.

Completion can occur without a full response, for example on timeout or driver
error. `hasFrame()` distinguishes these results from full received frames; it
does not indicate success. For an active master request, an otherwise valid
response with a different ID is reported as `INVALID`, not `SUCCESS`. Its full
raw frame remains available to `onResponse` and the status callback. This ID
check does not apply to incoming slave requests or passive unsolicited frames.

Properly received `DATA_INVALID` and `UNKNOWN_DATA_ID` replies have their own
terminal operation statuses. They are valid negative responses: `hasFrame()`
and `isComplete()` are true, while `isSuccess()` is false. Their payload must
not be interpreted as a successful reading. `INVALID` identifies malformed
or mismatched replies.

`as<Float>()` creates a value copy with f8.8 getters, without heap allocation,
RTTI or a runtime ID check of its own. Choose the wrapper for the message's
actual format; `get()` decodes the value while preserving raw data and metadata.

`Slave` is a simulation with no heater control: actual status flags and
modulation are zero, flow temperature is fixed at 35.5 C, and a written TSet
is stored only in RAM. A valid request for an unknown ID receives
`UNKNOWN_DATA_ID`; an unsupported operation on a known ID receives
`DATA_INVALID`. `sendResponseAsync()` submits the response without blocking;
the library handles TX timing. The master does not acknowledge a slave response.

## Transparent proxy

```text
Room thermostat (master)
  <-> slave OpenTherm interface
ESP32: thermostat -- callbacks -- boiler
  <-> master OpenTherm interface
Boiler (slave)
```

Two electrical interfaces and two independent driver instances/workers are
required. The boiler side starts first. Each receive callback immediately
submits a **copy of the raw frame** to the other driver. IDs, vendor bits,
parity and negative responses are not rebuilt. Full frames with invalid parity
are forwarded too; a partial capture cannot be forwarded as a message.
`acceptInvalidRequests` is enabled only on the thermostat-facing input so that
full invalid requests also receive routing context.

A boiler reply with an ID different from the active request is reported as
`INVALID`, and the proxy still forwards the full raw frame unchanged. The
diagnostic does not filter or repair the received response.

The `Message` already contains the raw frame and request context. Callbacks
pass it directly to `boiler.sendRequestAsync(request)` and
`thermostat.sendResponseAsync(response)`. The library carries the request
sequence through the exchange and rejects old responses after the thermostat
replaces its request. If the boiler bus is busy, the command is not queued.
The proxy does not fabricate responses, retry requests itself or substitute
IDs. The room thermostat decides when to retry after a timeout.

RMT RX memory is selected automatically: one hardware block per driver on
C3/C6, two on S3, and one on ESP32/S2. Two buses fit without an additional
memory setting. Each driver also keeps its two software RX buffers.

OpenTherm v4.2 section 4.3.2 requires a gateway to start forwarding within 7 ms
of receiving a message on either side. RMT reports completion after 2.5 ms of
idle, leaving part of the budget for waking and running the worker.
**The 7 ms limit has not been verified on hardware.** Measure both interfaces,
including scheduler and cache delays. The specification describes ordinary
slave response timing of 20-400 ms and the gateway exception separately.

## Logging

The proxy logs raw frames in all four directions: `Thermostat -> ESP`,
`ESP -> Boiler`, `Boiler -> ESP` and `ESP -> Thermostat`, with operation statuses.
Status callbacks also log each new bus status, including timeouts and errors.
`onRequest` logs received thermostat requests and outgoing boiler requests;
`onResponse` logs received boiler replies and outgoing thermostat replies.
Outgoing hooks run before TX with `TRANSMITTING` and `hasFrame() == false`;
incoming hooks contain complete frames, including invalid ones. `SUCCESS` after
a response to the thermostat means TX completed; the thermostat sends no ACK.

Direct `Serial.printf` in callbacks keeps these examples simple. Console
output can delay a worker; consider moving production gateway logging out of
forwarding callbacks when measuring or meeting timing limits. `Master` logs
from `loop()` after `sendRequest()` returns; `MasterCallbacks`, `MasterAsync`
and `Slave` log from callbacks. The slave and proxy loops contain only
`delay(1000)`; their workers continue communicating.

## Attribution and licenses

ESP32OT is an independent library informed by studying
[Ihor Melnyk's OpenTherm library](https://github.com/ihormelnyk/opentherm_library)
and other OpenTherm implementations.
See [LICENSE](../LICENSE) for ESP32OT's GPL-3.0-only terms.
