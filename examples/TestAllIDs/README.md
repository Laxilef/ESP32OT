# TestAllIDs

[Русский](README.ru.md)

Select `src_dir = examples/TestAllIDs` in the repository's `platformio.ini`,
choose your board environment and open the serial monitor at 115200 baud.
Set the adapter GPIOs with `MASTER_RX_GPIO` and `MASTER_TX_GPIO`. The included
ESP32-S3 environment uses RX=35/TX=36; sketch defaults are 4/5.

The example's fixed table contains 101 known standard IDs: 87 readable IDs and
14 write-only IDs. It reads the readable IDs sequentially from `loop()`, can
perform configured write tests, and prints an access map and statistics.
The phases are handled directly in `loop()`: `Read` → optional `Write` → `Report`.
`Report` prints the summary, clears the cycle's ID results and starts the next
reading cycle after a 10-second pause.
`ReadAll` remains a separate reading example.

Each synchronous call allows up to ten total attempts. Success and valid
negative replies finish the call; `INVALID`, timeouts and recoverable errors
can be retried. The application pauses 1 ms after each scan step and 10000 ms
after `Report`. The library also enforces bus readiness and inter-frame waits;
1 ms is not the physical interval between transmitted frames.

## Reading and default configuration

`TEST_ALL_IDS_ENABLE_WRITES` defaults to `0`: no write requests are sent.
Status reads carry clear master enable flags, including CH and DHW; use this
example where those flags are appropriate.

Operation lines include the action, ID, name, status, raw request and attempt
count. A complete response adds its raw frame and type, including invalid and
negative replies. Successful payloads are decoded through their message wrapper,
including all named flags and fields. ID30 keeps the library's f8.8 interpretation.

TSP and fault-history groups are read at indices `0..count-1` for the boiler,
ventilation/heat recovery and solar storage. A zero count skips the group; an
unavailable count leaves one index-0 probe. Each indexed result prints its
requested index. Brand strings read character 0.

`DATA_INVALID` and `UNKNOWN_DATA_ID` are valid negative replies. They are distinct
from malformed/mismatched `INVALID` results and from `TIMEOUT` or `ERROR` without
a complete response. An unavailable value is not a successful reading.

## Optional write tests

A successful write pair leaves the second value in the device. Review
`writeValues` before adding this flag to the selected environment's existing
`build_flags`:

```ini
-D TEST_ALL_IDS_ENABLE_WRITES=1
```

`writeValues` contains one row for each of the 43 writable IDs.
Each row is `{enabled, first, second}`; both values are `Message` objects built
through typed wrappers. For example:

```cpp
{
  true,
  Messages::Float::write(ID::TSet, 40.0f),
  Messages::Float::write(ID::TSet, 41.0f)
},
```

The global flag defaults to `0`, even though 24 rows are enabled. The other
19 rows are disabled: three OEM TSP writes, fifteen counter resets and RF sensor
status. Their device-specific meaning must be known before use; the RF row must
represent a sensor actually provided by this master. Both requests must be
WRITE_DATA for the same ID and have different payloads.

Each prepared pair is sent in table order: first value, applicable verification,
second value, applicable verification. Values are explicit test settings;
there is no separate original-value read or restoration step.

| Setting | Configured first → second |
|---|---|
| TSet/TsetCH2 and DHW setpoint | 40 → 41 °C |
| Maximum CH setpoint | 50 → 51 °C |
| Room setpoints, reported room temperatures and outside temperature | 20 → 21 °C |
| Maximum relative modulation / cooling control | 50 → 51% / 0 → 1% |
| Relative humidity and exhaust humidity | 50 → 51% |
| Ventilation setpoint and nominal ventilation | 30 → 31% |
| Exhaust CO₂ | 600 → 650 ppm |
| Day/time, date, year | Monday 12:00 → 12:01; January 1 → 2; 2026 → 2027 |
| Master configuration | Flags 0; member ID 0 → 1 |
| Remote override modes | CH OFF → NO_OVERRIDE; CH2/DHW remain NO_OVERRIDE, DHW push is false |
| Master OpenTherm version | 4.1 → 4.2 |
| Master product information | Product type 0; version 0 → 1 |

All supplied temperatures are whole degrees Celsius. OpenTherm 4.1/4.2 are
protocol versions. The calendar fields are illustrative, not the current clock.
RemoteRequest is an enabled pair: BLOR code 1, then code 0 for normal operation,
in every write-enabled cycle. The disabled RF example uses room sensor index 0,
normal battery status and signal strength 4 → 5.

### Permissions and temperature limits

`prepareWriteValues` checks enabled rows, matching IDs, WRITE_DATA and applicable
permissions. RBPflags IDs 6 and 86 are read-only permission inputs and are never
written; the send boundary protects their actual request IDs even if a table is
edited. SmartPower-enabled master configuration is skipped because this example
does not implement Smart Power.

| Written ID | Required information and allowed range |
|---|---|
| 56: DHW setpoint | Support/write permission from ID6 and valid bounds from ID48; clamp both values to the intersection of those bounds and 0..127 °C. |
| 57: maximum CH setpoint | Support/write permission from ID6 and valid bounds from ID49; clamp both values to the intersection of those bounds and 0..127 °C. |
| 1 / 8: CH control setpoints | A known MaxTSet from ID57; clamp both values to 0..floor(min(100 °C, known MaxTSet)). ID49's lower bound does not apply to these IDs. |
| 87: nominal ventilation | Transfer/write permission from ID86. |

If clamping makes both temperatures equal, the second becomes a neighbouring
value 1 °C away within the allowed interval. An interval containing only one
whole-degree value is skipped. Missing required flags or bounds, an invalid
interval, or an unavailable pair produces `WRITE_SKIP`. Adjustments are printed
as `WRITE_ADJUST`. The suitability of other configured values remains the
caller's responsibility.

ID56 and ID57 are processed before TSet/TsetCH2. Each actual ID57 write marks
its cached maximum unknown; a WRITE_ACK supplies the accepted maximum and a
successful verification read refines it. If the last ID57 write has no ACK,
IDs 1 and 8 are skipped. If the ID57 pair is skipped, a valid ID57 value from
the normal reading phase remains available.

## Verification

After each WRITE_ACK for a readable/writable ID, one synchronous `VERIFY_READ`
call reads the value and prints its typed fields. For indexed IDs it retains
the written index. `WRITE_VALUE` prints the ID, expected payload in hexadecimal
and `matched`: true requires READ_ACK with the exact requested payload,
including any adjustment. Verification uses the same maximum of ten attempts
as other operations.

Write-only IDs have no verification read. Their successful ACK is decoded with
the ID's wrapper; RemoteRequest prints its request code, response code and
acceptance flag. Acknowledgement and matching readback are separate results.
A failed write does not prevent the second request or the next table row;
reported failures can leave the applied value uncertain.

## Access map and statistics

`Report` prints the cycle number, whether writes are enabled, an overview and
separate bordered tables. Numeric counts are right-aligned. Totals accumulate
since boot, while `ID_MAP` is rebuilt each cycle.

| Overview metric | Meaning |
|---|---|
| `Completed calls` | Final results of synchronous calls after all retries, including verification reads. |
| `Queued attempts` | READ/WRITE entries into QUEUED, including retries; this counts accepted attempts, not proof of completed wire transmissions. |
| `Response frames` | Complete response frames, including invalid and valid negative replies; a timeout without a frame adds none. |

In `ID_MAP`, `Protocol` shows the table's declared access;
`Observed` combines reading-phase replies, write ACKs and applicable
protocol/device permissions. `Read result` and `Write result` show the last
recorded status; `-` means that direction was not tested.

| Observed | Meaning |
|---|---|
| `read/write` | Read evidence and a WRITE_ACK were observed. |
| `read only` | Read evidence was observed and the protocol or device permissions rule out writing. |
| `write only` | A WRITE_ACK was observed for a protocol write-only ID. |
| `not supported` | UNKNOWN_DATA_ID was received for the applicable tested directions, with no read/write evidence. |
| `unverified` | An applicable direction remains untested or inconclusive. |

Read evidence includes READ_ACK or DATA_INVALID in the reading phase; only
READ_ACK supplies a usable value. Disabled writes, skipped pairs and communication failures do not
prove lack of support. Missing required permission flags keep affected IDs
`unverified`, rather than proving read-only access. The map summarizes each ID;
individual indexed results retain their own statuses.

The remaining report sections are:

- `CUMULATIVE_STATUS`: the `Request status transitions` table counts entries into
  each status in separate READ/WRITE columns, including retries. Do not add its
  rows to obtain a request count. The `Received frames and final call results`
  table separately shows complete frames and final calls by terminal status,
  with a total row. READ counts include verification.
- `RESPONSE_TYPES`: types of all complete response frames, including invalid
  frames and valid negative replies.
- `WRITE_TOTALS`: `Write calls`, `WRITE_ACK results`, `Matching readbacks`
  for readable/writable IDs, and `Skipped ID pairs`.
  Each pair has two write calls. Each call can include multiple library attempts;
  those attempts appear in the status totals.

`CYCLE_DONE` marks the end of the report. An acknowledged write and a matching
readback remain separate results.

This diagnostic scan does not define a heating-control polling schedule.
