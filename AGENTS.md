# Agent instructions for ESP32OT

## Purpose and scope

ESP32OT is an independent OpenTherm master/slave library for ESP32. It was
developed during work on [OTGateway](https://github.com/Laxilef/OTGateway)
and exists thanks to Ihor Melnyk's original
[OpenTherm library](https://github.com/ihormelnyk/opentherm_library).
The repository is https://github.com/Laxilef/ESP32OT.

- Preserve reliable heating control, vendor compatibility and predictable
  timing. A protocol deviation may be intentional. Investigate it and obtain
  approval before changing existing wire behavior, validation or timing.
- Deliver the requested scope. Do not turn a move, rename or packaging task
  into an architecture or control-algorithm rewrite.
- Follow the whole conversation; the latest explicit user instruction wins.
  Treat a request to analyze or propose as analysis until implementation is authorized.
- Continue authorized work without repeatedly asking for permission. Present
  a concrete proposed change when approval of behavior is required.
- Communicate in the user's language. Code comments, the main README and
  metadata are English. Keep README.ru.md and translated example documentation
  synchronized with their English counterparts.
- Preserve the GPL-3.0-only LICENSE and acknowledge the developers whose work
  informed this independent implementation in README and documentation.
  Project headers contain ESP32OT, its copyright, SPDX license and repository URL.
  Do not remove copyrights from third-party code.

## Working tree and verification

- Inspect Git status and relevant existing changes before editing. Preserve
  unrelated and concurrent user changes. Never use a broad reset/checkout
  or discard untracked files to simplify a task.
- Commit only when requested. Use semantic commit messages, normally without
  scope, such as `fix: ...`, `feat: ...`, `refactor: ...`.
- Do not publish, create releases, push or change remote services without
  authorization. Do not commit secrets, private test settings or device logs.
- Use the installed PlatformIO; do not create another package installation
  or modify installed SDK/dependency sources without authorization.
- Respect an active no-build instruction across turns until explicitly lifted.
  Do not substitute preprocessing, example compilation or SDK rebuilding for
  a prohibited build. Reading sources and parsing configuration are separate checks.
- When builds are allowed, start with ESP32-S3 and the affected example.
  Cover ESP32, S2, S3, C3 and C6 when changing shared hardware code.
- Distinguish source inspection, Python models, compilation and hardware tests.
  A passing build does not prove timing, stack margins, electrical behavior
  or boiler compatibility. Avoid repeating successful broad tests without a reason.
- Use proportional tests, including boundary cases for changed arithmetic or
  lifecycle rules. Do not add tests that merely restate a trivial formatting change.
- Before flashing or issuing control commands, identify the device and the
  authorized scope. A test-board approval is not permission to operate a real boiler.
  For read-only logging, preserve device settings and release the COM/WS connection.
- Before further investigation, read agent-notes/README.md and the relevant notes
  when available. Keep reusable findings, sources, checks and limitations in this
  library's agent-notes/ and update its index. Keep task history there rather than
  expanding this file with temporary plans. Never force-add private notes without
  a request.

## Code style

- Use two-space indentation and braces for conditional/loop bodies. Indent
  code inside namespaces and nested preprocessor branches too.
- Separate a new logical comment block from preceding code with a blank line.
  Separate adjacent independent control-flow blocks. Keep useful explanations
  of method purpose, field units, timing constraints and non-obvious decisions.
- Start a fluent chain's first call on the variable's line. Do not split a
  single simple call just to imitate a longer chain. For a multiline call,
  put arguments and the closing parenthesis on separate, properly indented lines.
- Prefer clear `while` loops to `for (; condition; ...)`. Small, straightforward
  switch statements are appropriate for examples; avoid large dispatch switches.
- Use `static` for file-local functions/data instead of an anonymous namespace.
  `this->` is optional. Names must describe data and actions, not implementation guesses.
- Use `gpio_num_t`, `rxGpio` and `txGpio`; do not reintroduce `pin`, `inPin`,
  `outPin`, ESP8266 support or per-edge application GPIO interrupt code.
- Use the ESP32OT namespace, with Messages, Protocol and Transport subnamespaces
  and matching capitalized directories. Short protocol enums are exposed as
  ESP32OT::MessageID, MessageType and OperationStatus.
- Header-only definitions are suitable for simple wrappers. Do not create
  extra .cpp files, objects or helpers solely to impose formal structure.
- Templates, constexpr, mutexes and abstractions must solve an actual problem.
  Do not add redundant state, parsing, readiness flags, allocation or std::move
  of scalar values. Respect flash, heap, IRAM, task stacks and CPU separately.
- Use a physically meaningful tolerance for floating-point comparisons.
  Do not apply an arbitrary global epsilon to all payloads.

## Public API and message model

- The public facade is ESP32OT::ESP32OT and the umbrella header is ESP32OT.h.
  ESP32OT::ESP32OT::sendMessage and Transport::RmtTransport::sendMessage have
  distinct roles. Inside the facade, qualify namespace-owned types with
  ::ESP32OT:: to avoid the injected class name hiding its namespace.
- Config owns GPIOs, role, passive RX and worker/retry settings. getConfig()
  returns const Config&; changes use a copy and stopped-only setConfig().
  setConfig replaces the whole configuration, not selected non-default fields.
- Preserve default, Config and positional constructors. Keep
  ESP32OT(rxGpio, txGpio, isSlave=false, alwaysReceive=false).
- begin() must fail when an instance is already running until end() succeeds.
  begin(rx,tx) saves new GPIOs only on successful startup. A false
  isInitialized() after a fatal error does not itself prove resources are released.
- Role changes clear both request/response hooks; register them after configuring
  the role. Status callbacks remain independent.
- Message is a concrete raw-frame/metadata base. Wrappers build and parse their
  own payloads on demand. Preserve received raw bits and operation metadata.
- Keep simple numeric wrappers for shared formats. Structured IDs expose named
  fields and flags, as Status does; use meaningful ID-specific wrappers instead
  of requiring application masks for generic flag or byte-pair containers.
- as<T>() is an explicit value conversion, with no RTTI, heap allocation or
  automatic ID validation. A typed result keeps the caller-selected wrapper.
- Do not add a runtime ID-to-wrapper factory, registry, variant/vector storage,
  dynamically allocated messages, or virtual logging/visitor machinery.
  Raw request/response frames are sufficient for logging.
- Keep sendRawRequest/sendRawResponse and their async variants for unknown IDs,
  vendor fields and complete raw forwarding. Do not silently rebuild raw parity.
- reply() must retain the ID and request routing context. Named setters change
  their field and preserve unrelated bits; keep original request metadata when
  editing received snapshots. Do not add a second payload cache.

## Worker, callbacks and concurrency

- Each instance owns a small pinned FreeRTOS worker and its RMT resources.
  Callers must not service an external process() loop. Keep instances independent.
- Keep one pending/running operation per instance. Do not introduce application
  polling tasks, priority arbiters or queues of boiler IDs into the library.
  The caller chooses polling intervals and initialization/connection policy.
- isInitialized, isReady, complete result, received frame and peer connectivity
  are different concepts. Do not infer a connected boiler from bus readiness.
- Keep synchronous callers yielding while the worker progresses independently.
  maxAttempts counts all synchronous master attempts, default 5; zero means one.
  The retry counter must terminate at 255 without wrapping. Async and slave TX
  retain one attempt. Synchronous master operations retry INVALID, timeouts and
  recoverable errors while attempts remain. Success, valid DATA_INVALID or
  UNKNOWN_DATA_ID replies, cancellation and shutdown stop retries. Preserve
  invalid raw frames/status in per-attempt callbacks; do not repair parity.
- DelayCallback is void (*)(unsigned int milliseconds, void* context), with
  setDelayCallback(callback=nullptr, context=nullptr). Store only the function
  and context pointers; do not add std::function ownership or heap allocation.
  Context storage belongs to the caller and must remain valid during calls.
  Set or clear this hook only while fully stopped; running changes are ignored.
  A null callback preserves the default vTaskDelay with at least one tick.
  It is a wait/yield hook. Do not reenter a synchronous operation on the same
  ESP32OT from it: nested operations can replace the retained completion snapshot.
- onStatusChange covers public status changes. onRequest/onResponse work in both
  roles: outgoing notifications precede TX attempts and have TRANSMITTING with
  hasFrame=false; incoming notifications contain full frames, including invalid
  frames. hasFrame is not a validity/success check.
- Observer hooks accept std::function captures and retain a shared snapshot
  without copying captures or allocating per event. Replace/retire closures
  outside internal locks. Legacy function/context registration remains supported;
  setDelayCallback keeps its separate function-pointer/context API.
- Callbacks run in the worker outside ISR and internal locks. Keep them short
  and nonblocking. Same-instance synchronous send/begin/end are rejected there.
  Forward between instances asynchronously; avoid cross-worker blocking cycles.
- Callback message references are temporary; retain value copies when needed.
  Replacing/clearing a hook is not a callback-completion barrier. Captured raw
  pointers/references and legacy contexts remain caller-owned until any running
  or snapshotted call has finished.
- Add synchronization only for a demonstrated conflict. Never suspend a task
  that may hold a required lock. Preserve cooperative stop and cleanup ownership.
- Preserve the saved-request and monotonic-counter sequence checks: an old reply
  must not answer a newer request or survive end()/begin() as a current response.
- Ordinary invalid slave noise must not supersede a valid request. Proxy
  acceptInvalidRequests opts in only to complete invalid frames and retains INVALID.

## Protocol, timing and resource invariants

- Read the provided OpenTherm specification and actual SDK sources before
  protocol/hardware changes. Do not infer modern ESP32 APIs from older versions.
- Keep 500 us TX half-bits, start + 32 MSB-first data bits + stop, existing RX
  start/sample/stop tolerance and partial-capture behavior unless a change is approved.
- Preserve asymmetric half-bit support. Complete-capture phase recovery must
  select a unique Manchester interpretation from pulse timing before parity,
  response-type and active-request ID checks. Do not choose a candidate by the
  expected ID or repair parity. Keep captured RMT symbols unchanged and report
  ambiguous recovery as INVALID.
- Preserve the approved receive inactivity budget of 1 s + 40 ms frame grace
  + 2.5 ms capture idle, MWT master 100 ms / slave 20 ms from RX_DONE, and the
  1 s TX watchdog. Process completed capture events before timeout decisions.
- Keep IRQ allocation/ownership on the pinned worker core, arm RX promptly from
  TX_DONE and preserve fixed-buffer ownership. Avoid application-wide interrupt
  masks such as noInterrupts() and application portMUX critical sections.
- Do not assume all ESP32 RMT variants have identical memory. Preserve two-bus
  support on all five chip families, including C3/C6 RX ping-pong and S3 RX blocks.
- Account for timer wrap, tick rounding, scheduler/cache latency and ISR/task
  timestamps separately. A configured timeout is not a measured response latency.
- Do not claim the OpenTherm gateway forwarding limit of 7 ms is met without
  measurements on both interfaces. Capture idle already consumes 2.5 ms.
- Preserve approved ACK/parity/vendor behavior. A full response to an active
  master request must match its ID; a mismatch is INVALID while raw/frame metadata
  remains available. Do not compare unsolicited passive RX to an old request ID,
  and do not treat a new slave-side master request as a reply to the previous ID.
  Do not add request/ACK-type pairing or reserved-bit restrictions incidentally.
- Keep requested master flags distinct from actual slave state. Do not encode
  a zero room setpoint as a generic heating-off command: some devices interpret
  it as cooling. Do not reject zero for every temperature ID; ID56 allows it.
- Preserve documented ambiguities: ID30 has s16/f8.8 descriptions and existing
  OTGateway uses Float; ID100's detailed table uses LB priority flags; ID103's
  system-type-bit location is contradictory and must not be guessed.

## Examples and standalone packaging

- Keep Master responses in loop(), MasterCallbacks as the callback-based sync
  example, MasterAsync with native readiness and no shadow waiting flag, and
  both Slave and TransparentProxy examples understandable to new programmers.
- Explain why hasFrame() is needed for terminal status results. RX hooks already
  guarantee a full frame and do not need that redundant guard.
- Log raw requests, responses and statuses in both proxy directions. Direct
  Serial inside example callbacks is accepted for clarity; keep its timing
  limitation documented rather than adding logging queues/tasks unnecessarily.
- Slave examples should use readable switch/case blocks, if/else operation
  branches and break. Reply to unsupported operations/IDs explicitly; ID0 WRITE
  must receive DATA_INVALID, not a successful Status acknowledgement.
- Preserve MASTER_RX_GPIO, MASTER_TX_GPIO, SLAVE_RX_GPIO and
  SLAVE_TX_GPIO compile-time overrides. MASTER names the ESP32 interface acting
  as master toward a boiler; SLAVE names its interface acting as slave toward
  a thermostat. GPIO choices remain board-dependent.
- Keep platformio.ini self-contained: one selected sketch, S3 default, supported
  board environments, no parent-project secrets/scripts or unrelated libraries.
  The framework must retain a C++ standard of at least 17 and required IDF 6 APIs.
- Keep library.json, library.properties, CMakeLists.txt, SPDX headers and README
  paths/credits consistent. Do not claim a release, successful build or hardware
  validation solely because packaging/configuration checks pass.
