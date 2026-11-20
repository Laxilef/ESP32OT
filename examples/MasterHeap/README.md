# MasterHeap

[Русский](README.ru.md)

A master-mode heap diagnostic for a test board. It measures allocation and
release across startup, requests, shutdown and repeated restarts. On the wire
it sends Status with no heating request and temperature/version reads. Other
message formats are exercised locally. Use an electrical OpenTherm interface;
GPIOs must not connect directly to the OpenTherm line.

## Running

From the repository root, select this example without editing `platformio.ini`:

```powershell
$env:PLATFORMIO_SRC_DIR = (Resolve-Path examples/MasterHeap).Path
pio run -e esp32-s3
pio run -e esp32-s3 -t upload --upload-port COM3
pio device monitor -p COM3 -b 115200
Remove-Item Env:PLATFORMIO_SRC_DIR
```

Replace COM3 with the identified test board's port. Stop the monitor before
another upload and remove the source override after testing. The S3 profile
uses RX=35 / TX=36, USB-Serial/JTAG and a watchdog reset after upload.
`MASTER_RX_GPIO` / `MASTER_TX_GPIO` select the GPIOs; the sketch's
fallbacks without those definitions are 4 / 5.

Defaults are `HEAP_SUITE_ROUNDS=2`, `HEAP_REQUESTS_PER_BATCH=6`,
`HEAP_RESTART_CYCLES=8` and `HEAP_SAMPLE_PERIOD_MS=1`.
Override them in the selected environment's existing
`build_flags` to change the workload. Wait for `HEAP_DONE`; after the finite
suite completes the loop only yields. The initial four-second delay allows a
serial monitor to attach. Capture the complete log, including both rounds.

## Scenarios and coverage

- Default, Config and positional constructors; stopped-operation rejection.
- `begin()` / `begin(rx, tx)`, idle, double-start and running-configuration guards.
- Synchronous typed/base/raw requests; default five attempts, zero meaning one,
  one-attempt batches and a custom delay/yield callback.
- Asynchronous typed/raw requests, rejection of another pending operation,
  result snapshots, readiness and operation metadata.
- Passive reception enabled while idle and requesting, cancellation through
  `end()`, idempotent shutdown and clearing hooks after completed shutdown.
- Worker stacks of 3072 and 4096 bytes, repeated start/stop cycles and destructor.
- A bounded 8 KiB diagnostic allocation pattern with alternating holes and
  a check that freeing every buffer restores the original block layout.
- A temporary allocation of half the largest internal block, sampled while
  allocated and checked for full recovery after release.
- 500 local passes over all 30 typed wrappers: build, parse, value conversion,
  named fields, replies, raw/metadata preservation and representative boundaries.

Worker callbacks update atomic counters without printing or allocation. The
wait callback yields in the calling task and does not reenter synchronous
operations. Result, heap and summary output comes from the Arduino task.
Synchronous master calls retry `INVALID`, timeouts and recoverable errors up to
their configured attempt limit. Valid `DATA_INVALID` and `UNKNOWN_DATA_ID`
replies stop retries without providing acknowledged data. `CALLBACKS` counts
these replies separately as `data_invalid` and `unknown_data_id`; `invalid`
counts malformed or mismatched replies. Callback counters cover every attempt,
including attempts followed by a successful retry.

The controller and captured counters are stack-owned. Observer closures are
registered once before startup and released after shutdown; dispatch does not
allocate or copy their captures. `tx_requests` counts outgoing request hooks,
while `frames` counts received complete responses. Printed `sizeof` values are
object sizes, separate from dynamic hook, worker and RMT storage.
Every batch retains its result snapshots on the caller's stack and prints all
of them after the measurement window. Each array occupies
`HEAP_REQUESTS_PER_BATCH * sizeof(MessageData)` bytes. Increasing a batch increases application stack
demand; use the caller watermark as well as the separate worker watermark.

## Reading the output

`HEAP` reports `all8`, `internal8` and `psram8` capability pools. These pools
overlap; do not add them together. Heap/stack sizes are bytes; `RESULT` times
are microseconds. `BLOCKS`, `CHECK`, `RESULT`, `CALLBACKS`, `SUITE_END` and `HEAP_DONE`
provide integrity, operation, counter, retained-memory and completion context.

| Field | Meaning |
| --- | --- |
| `before`, `after`, `delta` | Free heap before the window and after stopping its monitor; delta = after − before |
| `min_local` | SDK local low-water reading during the window, including monitor overhead |
| `largest_before`, `largest_after` | Largest available contiguous block at each endpoint |
| `blocks_before`, `blocks_after` | Allocated block count at each endpoint |
| `monitor_bytes` | Free-byte difference immediately after enabling the SDK monitor |
| `worker_stack_min_bytes` | Minimum unused worker stack reported while it is alive; zero when no worker is sampled |
| `caller_stack_min_bytes` | Minimum unused stack of the Arduino task that runs the diagnostic and retains batch snapshots |
| `failures`, `first_failure` | Accumulated scenario checks; completion alone does not mean all passed |

The SDK monitor allocates memory itself. Endpoint deltas are taken after its
storage is released; `monitor_bytes` and `min_local` expose its influence.
Pool minima combine per-region low-water readings reached at potentially
different times: they are not a simultaneous whole-system peak allocation.
Warmup precedes the rounds. After `end()`, a 100 ms delay gives idle tasks time
to reclaim the worker's stack/TCB. Compare matching phases and both round-end
baselines; a one-time SDK allocation or a lower minimum alone is not a leak.

## Sampled minima

`MINIMA` accumulates observations from a separate diagnostic task with a static
3072-byte stack, a static task control block and priority 2. It runs on core 1
on dual-core targets, otherwise core 0. This task belongs to the example and
is separate from the library worker. Its static storage is included in the
application footprint rather than allocated for each measurement window.

| Field | Meaning |
| --- | --- |
| `free_min_observed` | Lowest free heap among the samples, in bytes |
| `largest_min_observed` | Lowest sampled value of the largest allocatable block, in bytes |
| `sdk_min_free` | SDK free-heap low-water value when the line is printed; separate from the sampled minimum |
| `fragmentation_max_bp` | Maximum of `10000 * (free - largest) / free` over nonempty pool samples; 100 bp = 1% |
| `samples`, `ratio_samples` | Number of heap observations and number with nonzero free heap used for the ratio |
| `period_ms` | Requested delay between periodic observations, selected with `HEAP_SAMPLE_PERIOD_MS` |
| `max_gap_us` | Longest interval between observations included in that scope |
| `max_scan_us` | Longest time to query all three capability pools in one sampling pass |

`scope=run` includes the whole diagnostic, including synthetic allocation
probes. `scope=operations` omits samples taken during `fragmentation_probe`
and `largest_block_probe`; its observation gaps can span those excluded
intervals. Both scopes accumulate from observer startup across suite rounds.
`point` identifies when the accumulated values were printed.

These are observed minima, not an exact trace of every allocation. An
allocation freed between observations can be missed; use `max_gap_us` with the
configured period. The ratio uses free bytes and the largest block from the
same pool observation, never their independently accumulated minima. It also
reflects separate physical heap regions, so use the regional `BLOCKS` metrics
when inspecting layout. Capability pools are queried sequentially and overlap.
An absent PSRAM pool has zero free bytes and `ratio_samples=0`; its printed
zero ratio is undefined rather than a fragmentation measurement.

`largest_block_probe` allocates half the largest available `internal8` block,
holds it for ten sampling periods, then frees it and checks free-heap and
largest-block recovery. `LARGEST_PROBE before/during/after` reports the largest
block around that allocation; `observed_min` is the accumulated `internal8`
minimum for `scope=run`. This probe checks that a transient decrease is observed
even when the values before and after match. `SAMPLER stack_min_bytes` reports
the observer task's minimum unused stack.

## Block layout and fragmentation

`BLOCKS point=before/after` traverses matching heaps before enabling the SDK
monitor and after stopping it. `free_blocks` / `used_blocks` count blocks;
`smallest_free_raw`, `smallest_used_raw`, `mean_free_raw` and `largest_free_raw`
describe raw allocator block sizes. These sizes include relevant SDK bookkeeping
and alignment; they are not requested payload sizes or guaranteed allocation
capacities. Keep `HEAP largest_*` for the SDK's largest allocatable block.
`free_raw` is the sum of raw free-block sizes and need not equal SDK free bytes.
`regions` counts physical heaps; `region_largest_sum` sums their largest free blocks.

The weighted within-region indicator is
`frag_permille = 1000 * (free_raw - region_largest_sum) / free_raw`.
`worst_region_permille` gives the largest equivalent indicator for one region.
Separate contiguous regions do not count as fragmentation. Integer division
rounds down, so zero can hide amounts below 0.1%; compare the exact byte
difference and block counts too. For an empty pool, both indicators print zero
but are undefined; `free_blocks=0` identifies that case. This is a layout
comparison aid, not a universal heap-health percentage or a continuous minimum.

The allocator holds a region lock during its walker callback. That callback
only aggregates scalars; it does not allocate, print or call the library.
Regions and capability pools are sampled sequentially, not atomically together.
`fragmentation_probe` allocates eight 1024-byte internal buffers, frees alternate
buffers, samples the holes, then frees the rest and checks recovery. These
allocations belong to the diagnostic, not ESP32OT. Its instrumented baseline,
fragmented and recovered snapshots contain the same SDK-monitor overhead.

No peer is needed for timeout, cancellation and guard scenarios. Use a
responding test peer to exercise successful/invalid hardware RX and response callbacks.
Slave mode, proxy forwarding, electrical timing and boiler compatibility are
outside this diagnostic. Heap integrity is checked for internal 8-bit memory.

ESP32OT is based on [Ihor Melnyk's OpenTherm library](https://github.com/ihormelnyk/opentherm_library).
See [LICENSE](../../LICENSE) for ESP32OT's GPL-3.0-only terms.
