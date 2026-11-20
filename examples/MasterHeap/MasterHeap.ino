/*
ESP32OT
Copyright (c) 2026 Laxilef
SPDX-License-Identifier: GPL-3.0-only
https://github.com/Laxilef/ESP32OT
*/

#include <ESP32OT.h>
#include <atomic>
#include <esp_heap_caps.h>
#include <esp_timer.h>

// Bench diagnostic: use a test board, not a live heating installation.
// Only Status and temperature/version reads go onto the wire. All other
// wrapper formats are exercised locally using synthetic frame snapshots.
#ifndef MASTER_RX_GPIO
  #define MASTER_RX_GPIO 4
#endif
#ifndef MASTER_TX_GPIO
  #define MASTER_TX_GPIO 5
#endif
#ifndef HEAP_REQUESTS_PER_BATCH
  #define HEAP_REQUESTS_PER_BATCH 6
#endif
#ifndef HEAP_RESTART_CYCLES
  #define HEAP_RESTART_CYCLES 8
#endif
#ifndef HEAP_SUITE_ROUNDS
  #define HEAP_SUITE_ROUNDS 2
#endif
#ifndef HEAP_SAMPLE_PERIOD_MS
  #define HEAP_SAMPLE_PERIOD_MS 1
#endif
#if HEAP_REQUESTS_PER_BATCH < 1 || HEAP_RESTART_CYCLES < 1 || HEAP_SUITE_ROUNDS < 1
  #error "MasterHeap test counts must be positive"
#endif
#if HEAP_SAMPLE_PERIOD_MS < 1
  #error "MasterHeap sampling period must be positive"
#endif

using Controller = ESP32OT::ESP32OT;
using Message = ESP32OT::Messages::Message;
using Status = ESP32OT::Messages::Status;
using Frame = ESP32OT::Protocol::Frame;
using OperationStatus = ESP32OT::OperationStatus;

static const gpio_num_t rxGpio = static_cast<gpio_num_t>(MASTER_RX_GPIO);
static const gpio_num_t txGpio = static_cast<gpio_num_t>(MASTER_TX_GPIO);
static const uint32_t heapCaps[] = {
  MALLOC_CAP_8BIT,
  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT,
  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
};
static const char* const heapNames[] = {"all8", "internal8", "psram8"};
static uint32_t failures = 0;
static const char* firstFailure = nullptr;

struct HeapMinimums {
  size_t freeMinimum = 0;
  size_t largestMinimum = 0;
  uint32_t samples = 0;
  uint32_t ratioSamples = 0;
  uint32_t fragmentationMaximumBp = 0;
  uint64_t lastSampleUs = 0;
  uint64_t maximumGapUs = 0;
};

// The diagnostic observer has static storage, so its stack/TCB do not add
// dynamic allocations to individual operation windows. It samples in a task,
// never in an ISR or a library callback. SDK allocation hooks are not required.
static StaticTask_t heapSamplerState;
static StackType_t heapSamplerStack[3072 / sizeof(StackType_t)];
static StaticSemaphore_t heapSampleMutexStorage;
static StaticSemaphore_t heapSamplerDoneStorage;
static SemaphoreHandle_t heapSampleMutex = nullptr;
static SemaphoreHandle_t heapSamplerDone = nullptr;
static TaskHandle_t heapSamplerTask = nullptr;
static HeapMinimums heapMinimums[3];
static HeapMinimums operationHeapMinimums[3];
static bool heapProbeActive = false;
static uint64_t maximumHeapScanUs = 0;
static uint32_t heapSamplerUnusedStack = 0;

// Raw allocator block sizes include SDK bookkeeping/alignment. They are not
// requested payload sizes or guaranteed malloc capacities. Scan at endpoints
// only: heap_caps_walk holds one region's allocator lock during its callback.
struct HeapBlockStats {
  size_t smallestFree = 0;
  size_t smallestUsed = 0;
  size_t largestFree = 0;
  size_t freeBytes = 0;
  size_t freeBlocks = 0;
  size_t usedBlocks = 0;
  size_t regions = 0;
  size_t largestPerRegionSum = 0;
  uint32_t worstRegionPermille = 0;
};

struct HeapBlockWalk {
  HeapBlockStats stats;
  intptr_t regionStart = 0;
  size_t regionFree = 0;
  size_t regionLargest = 0;
  bool haveRegion = false;
};


struct HeapSnapshot {
  multi_heap_info_t pools[3] = {};
};

struct HeapWindow {
  const char* name;
  HeapSnapshot before;
  HeapSnapshot instrumented;
  bool monitoring;
  HeapBlockStats beforeBlocks[3] = {};
};

// Worker callbacks and loop() use these counters concurrently on different
// cores. Atomics protect only the counters; no callback prints or allocates.
struct CallbackCounters {
  std::atomic<uint32_t> changes{0};
  std::atomic<uint32_t> success{0};
  std::atomic<uint32_t> invalid{0};
  std::atomic<uint32_t> dataInvalid{0};
  std::atomic<uint32_t> unknownDataId{0};
  std::atomic<uint32_t> timeout{0};
  std::atomic<uint32_t> error{0};
  std::atomic<uint32_t> cancelled{0};
  std::atomic<uint32_t> frames{0};
  std::atomic<uint32_t> requests{0};
  std::atomic<uint32_t> lastFrame{0};
  uint32_t waits = 0;
};

static void finishHeapRegion(HeapBlockWalk& walk) {
  if (!walk.haveRegion) {
    return;
  }

  walk.stats.regions++;
  walk.stats.largestPerRegionSum += walk.regionLargest;
  if (walk.regionFree != 0) {
    const uint32_t permille = static_cast<uint32_t>(
      1000ULL * (walk.regionFree - walk.regionLargest) / walk.regionFree
    );
    if (permille > walk.stats.worstRegionPermille) {
      walk.stats.worstRegionPermille = permille;
    }
  }
}

static bool countHeapBlock(walker_heap_into_t heap, walker_block_info_t block, void* context) {
  auto& walk = *static_cast<HeapBlockWalk*>(context);
  if (!walk.haveRegion || heap.start != walk.regionStart) {
    finishHeapRegion(walk);
    walk.haveRegion = true;
    walk.regionStart = heap.start;
    walk.regionFree = 0;
    walk.regionLargest = 0;
  }

  if (block.used) {
    walk.stats.usedBlocks++;
    if (walk.stats.smallestUsed == 0 || block.size < walk.stats.smallestUsed) {
      walk.stats.smallestUsed = block.size;
    }

  } else {
    walk.stats.freeBlocks++;
    walk.stats.freeBytes += block.size;
    walk.regionFree += block.size;
    if (walk.stats.smallestFree == 0 || block.size < walk.stats.smallestFree) {
      walk.stats.smallestFree = block.size;
    }

    if (block.size > walk.stats.largestFree) {
      walk.stats.largestFree = block.size;
    }

    if (block.size > walk.regionLargest) {
      walk.regionLargest = block.size;
    }
  }
  return true;
}

static HeapBlockStats readHeapBlocks(uint32_t caps) {
  HeapBlockWalk walk;
  heap_caps_walk(caps, countHeapBlock, &walk);
  finishHeapRegion(walk);
  return walk.stats;
}

static size_t meanFreeBlock(const HeapBlockStats& stats) {
  return stats.freeBlocks != 0 ? stats.freeBytes / stats.freeBlocks : 0;
}

// This is a within-region concentration indicator, weighted by free raw bytes.
// Values are rounded down to whole per mille; zero can hide tiny fragments.
// Inspect freeBytes - largestPerRegionSum for the exact scattered-byte count.
// This is not an application-independent fragmentation/health percentage.
// Empty capability pools report 0 and freeBlocks=0; their ratio is undefined.
static uint32_t fragmentedFreePermille(const HeapBlockStats& stats) {
  if (stats.freeBytes == 0) {
    return 0;
  }

  return static_cast<uint32_t>(
    1000ULL * (stats.freeBytes - stats.largestPerRegionSum) / stats.freeBytes
  );
}

static void expect(bool condition, const char* description) {
  if (!condition) {
    failures++;
    if (firstFailure == nullptr) {
      firstFailure = description;
    }
  }
}

static HeapSnapshot readHeap() {
  HeapSnapshot result;
  for (size_t index = 0; index < 3; index++) {
    heap_caps_get_info(&result.pools[index], heapCaps[index]);
  }
  return result;
}

static void observeHeap(HeapMinimums& minimums, const multi_heap_info_t& info, uint64_t sampledAt) {
  if (minimums.samples != 0 && sampledAt - minimums.lastSampleUs > minimums.maximumGapUs) {
    minimums.maximumGapUs = sampledAt - minimums.lastSampleUs;
  }
  minimums.lastSampleUs = sampledAt;
  if (minimums.samples == 0 || info.total_free_bytes < minimums.freeMinimum) {
    minimums.freeMinimum = info.total_free_bytes;
  }
  if (minimums.samples == 0 || info.largest_free_block < minimums.largestMinimum) {
    minimums.largestMinimum = info.largest_free_block;
  }
  minimums.samples++;
  if (info.total_free_bytes != 0) {
    // Both operands belong to the same pool snapshot, not separate minima.
    const uint32_t fragmentationBp = static_cast<uint32_t>(
      10000ULL * (info.total_free_bytes - info.largest_free_block) / info.total_free_bytes
    );
    if (fragmentationBp > minimums.fragmentationMaximumBp) {
      minimums.fragmentationMaximumBp = fragmentationBp;
    }
    minimums.ratioSamples++;
  }
}

static void sampleHeapNow() {
  if (heapSampleMutex == nullptr) {
    return;
  }

  // Only this diagnostic mutex is held while querying the SDK. No worker or
  // allocator callback needs it; there is no reverse lock dependency.
  xSemaphoreTake(heapSampleMutex, portMAX_DELAY);
  const uint64_t startedAt = esp_timer_get_time();
  for (size_t index = 0; index < 3; index++) {
    multi_heap_info_t info = {};
    heap_caps_get_info(&info, heapCaps[index]);
    const uint64_t sampledAt = esp_timer_get_time();
    observeHeap(heapMinimums[index], info, sampledAt);
    if (!heapProbeActive) {
      observeHeap(operationHeapMinimums[index], info, sampledAt);
    }
  }
  const uint64_t elapsedUs = esp_timer_get_time() - startedAt;
  if (elapsedUs > maximumHeapScanUs) {
    maximumHeapScanUs = elapsedUs;
  }
  xSemaphoreGive(heapSampleMutex);
}

static void runHeapSampler(void*) {
  const TickType_t ticks = pdMS_TO_TICKS(HEAP_SAMPLE_PERIOD_MS);
  sampleHeapNow();
  while (ulTaskNotifyTake(pdTRUE, ticks > 0 ? ticks : 1) == 0) {
    sampleHeapNow();
  }
  sampleHeapNow();
  heapSamplerUnusedStack = uxTaskGetStackHighWaterMark(nullptr);
  xSemaphoreGive(heapSamplerDone);
  vTaskDelete(nullptr);
}

static void startHeapSampler() {
  heapSampleMutex = xSemaphoreCreateMutexStatic(&heapSampleMutexStorage);
  heapSamplerDone = xSemaphoreCreateBinaryStatic(&heapSamplerDoneStorage);
  expect(heapSampleMutex != nullptr && heapSamplerDone != nullptr, "heap sampler synchronization");
  const BaseType_t core = portNUM_PROCESSORS > 1 ? 1 : 0;
  heapSamplerTask = xTaskCreateStaticPinnedToCore(
    runHeapSampler,
    "HeapSampler",
    sizeof(heapSamplerStack),
    nullptr,
    tskIDLE_PRIORITY + 2,
    heapSamplerStack,
    &heapSamplerState,
    core
  );
  expect(heapSamplerTask != nullptr, "start heap sampler");
  sampleHeapNow();
}

static void logHeapMinimums(const char* point) {
  HeapMinimums minimums[2][3];
  uint64_t scanUs;
  xSemaphoreTake(heapSampleMutex, portMAX_DELAY);
  for (size_t index = 0; index < 3; index++) {
    minimums[0][index] = heapMinimums[index];
    minimums[1][index] = operationHeapMinimums[index];
  }
  scanUs = maximumHeapScanUs;
  xSemaphoreGive(heapSampleMutex);
  for (size_t scope = 0; scope < 2; scope++) {
    for (size_t index = 0; index < 3; index++) {
      const auto& observed = minimums[scope][index];
      Serial.printf(
        "MINIMA point=%s scope=%s pool=%s free_min_observed=%u largest_min_observed=%u "
        "sdk_min_free=%u fragmentation_max_bp=%lu samples=%lu ratio_samples=%lu period_ms=%u max_gap_us=%llu max_scan_us=%llu\n",
        point, scope == 0 ? "run" : "operations", heapNames[index],
        static_cast<unsigned int>(observed.freeMinimum), static_cast<unsigned int>(observed.largestMinimum),
        static_cast<unsigned int>(heap_caps_get_minimum_free_size(heapCaps[index])),
        static_cast<unsigned long>(observed.fragmentationMaximumBp),
        static_cast<unsigned long>(observed.samples), static_cast<unsigned long>(observed.ratioSamples),
        static_cast<unsigned int>(HEAP_SAMPLE_PERIOD_MS),
        static_cast<unsigned long long>(observed.maximumGapUs), static_cast<unsigned long long>(scanUs)
      );
    }
  }
}

static void stopHeapSampler() {
  if (heapSamplerTask != nullptr) {
    xTaskNotifyGive(heapSamplerTask);
    expect(xSemaphoreTake(heapSamplerDone, pdMS_TO_TICKS(1000)) == pdTRUE, "stop heap sampler");
    heapSamplerTask = nullptr;
  }
  logHeapMinimums("complete");
  Serial.printf("SAMPLER stack_min_bytes=%lu\n", static_cast<unsigned long>(heapSamplerUnusedStack));
}

static HeapWindow startWindow(const char* name) {
  sampleHeapNow();
  HeapWindow window{name, readHeap(), {}, false};
  for (size_t index = 0; index < 3; index++) {
    window.beforeBlocks[index] = readHeapBlocks(heapCaps[index]);
  }

  // The SDK monitor itself allocates. Measure its overhead separately and
  // compare retained heap only after stop() has freed the monitor's storage.
  window.monitoring = heap_caps_monitor_local_minimum_free_size_start() == ESP_OK;
  expect(window.monitoring, "start heap monitor");
  window.instrumented = readHeap();
  return window;
}

static void logHeapBlocks(const char* phase, const char* pool, const char* point, const HeapBlockStats& blocks) {
  Serial.printf(
    "BLOCKS phase=%s pool=%s point=%s free_blocks=%u used_blocks=%u "
    "smallest_free_raw=%u smallest_used_raw=%u mean_free_raw=%u largest_free_raw=%u "
    "free_raw=%u regions=%u region_largest_sum=%u frag_permille=%lu worst_region_permille=%lu\n",
    phase, pool, point, static_cast<unsigned int>(blocks.freeBlocks),
    static_cast<unsigned int>(blocks.usedBlocks), static_cast<unsigned int>(blocks.smallestFree),
    static_cast<unsigned int>(blocks.smallestUsed), static_cast<unsigned int>(meanFreeBlock(blocks)),
    static_cast<unsigned int>(blocks.largestFree), static_cast<unsigned int>(blocks.freeBytes),
    static_cast<unsigned int>(blocks.regions), static_cast<unsigned int>(blocks.largestPerRegionSum),
    static_cast<unsigned long>(fragmentedFreePermille(blocks)),
    static_cast<unsigned long>(blocks.worstRegionPermille)
  );
}

static void finishWindow(HeapWindow& window, Controller* controller = nullptr) {
  sampleHeapNow();
  const HeapSnapshot low = readHeap();
  const uint32_t stack = controller != nullptr ? controller->getWorkerStackHighWaterMark() : 0;
  if (window.monitoring) {
    expect(heap_caps_monitor_local_minimum_free_size_stop() == ESP_OK, "stop heap monitor");
  }

  const HeapSnapshot after = readHeap();
  const bool intact = heap_caps_check_integrity(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, true);
  expect(intact, "internal heap integrity");
  for (size_t index = 0; index < 3; index++) {
    const HeapBlockStats blocksAfter = readHeapBlocks(heapCaps[index]);
    const auto& beforePool = window.before.pools[index];
    const auto& afterPool = after.pools[index];
    Serial.printf(
      "HEAP phase=%s pool=%s before=%u after=%u delta=%ld min_local=%u "
      "largest_before=%u largest_after=%u blocks_before=%u blocks_after=%u monitor_bytes=%ld\n",
      window.name, heapNames[index],
      static_cast<unsigned int>(beforePool.total_free_bytes),
      static_cast<unsigned int>(afterPool.total_free_bytes),
      static_cast<long>(afterPool.total_free_bytes) - static_cast<long>(beforePool.total_free_bytes),
      static_cast<unsigned int>(low.pools[index].minimum_free_bytes),
      static_cast<unsigned int>(beforePool.largest_free_block),
      static_cast<unsigned int>(afterPool.largest_free_block),
      static_cast<unsigned int>(beforePool.allocated_blocks),
      static_cast<unsigned int>(afterPool.allocated_blocks),
      static_cast<long>(beforePool.total_free_bytes)
        - static_cast<long>(window.instrumented.pools[index].total_free_bytes)
    );
    logHeapBlocks(window.name, heapNames[index], "before", window.beforeBlocks[index]);
    logHeapBlocks(window.name, heapNames[index], "after", blocksAfter);
  }
  Serial.printf(
    "CHECK phase=%s integrity=%s worker_stack_min_bytes=%lu caller_stack_min_bytes=%lu failures=%lu first_failure=%s\n",
    window.name, intact ? "ok" : "FAIL", static_cast<unsigned long>(stack),
    static_cast<unsigned long>(uxTaskGetStackHighWaterMark(nullptr)),
    static_cast<unsigned long>(failures), firstFailure != nullptr ? firstFailure : "none"
  );
  logHeapMinimums(window.name);
}

static void exerciseFragmentation() {
  xSemaphoreTake(heapSampleMutex, portMAX_DELAY);
  heapProbeActive = true;
  xSemaphoreGive(heapSampleMutex);
  HeapWindow window = startWindow("fragmentation_probe");
  const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  const HeapBlockStats baseline = readHeapBlocks(caps);
  void* buffers[8] = {};

  // This bounded 8 KiB workload belongs to the diagnostic, not the library.
  // Alternating holes verify that the sampler observes real fragmentation.
  for (size_t index = 0; index < 8; index++) {
    buffers[index] = heap_caps_malloc(1024, caps);
    expect(buffers[index] != nullptr, "fragmentation probe allocation");
    sampleHeapNow();
  }
  for (size_t index = 0; index < 8; index += 2) {
    heap_caps_free(buffers[index]);
    buffers[index] = nullptr;
    sampleHeapNow();
  }
  const HeapBlockStats fragmented = readHeapBlocks(caps);
  for (size_t index = 0; index < 8; index++) {
    heap_caps_free(buffers[index]);
    sampleHeapNow();
  }
  const HeapBlockStats recovered = readHeapBlocks(caps);
  expect(recovered.freeBytes == baseline.freeBytes && recovered.freeBlocks == baseline.freeBlocks
    && recovered.largestPerRegionSum == baseline.largestPerRegionSum, "fragmentation probe recovery");
  finishWindow(window);
  logHeapBlocks("fragmentation_probe", "internal8", "instrumented_baseline", baseline);
  logHeapBlocks("fragmentation_probe", "internal8", "fragmented", fragmented);
  logHeapBlocks("fragmentation_probe", "internal8", "recovered", recovered);
  xSemaphoreTake(heapSampleMutex, portMAX_DELAY);
  heapProbeActive = false;
  xSemaphoreGive(heapSampleMutex);
}

static void exerciseLargestBlockMinimum() {
  xSemaphoreTake(heapSampleMutex, portMAX_DELAY);
  heapProbeActive = true;
  xSemaphoreGive(heapSampleMutex);
  HeapWindow window = startWindow("largest_block_probe");
  const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  multi_heap_info_t before = {};
  multi_heap_info_t during = {};
  multi_heap_info_t after = {};
  heap_caps_get_info(&before, caps);
  void* buffer = heap_caps_malloc(before.largest_free_block / 2, caps);
  expect(buffer != nullptr, "largest-block probe allocation");
  heap_caps_get_info(&during, caps);

  // Keep the transient allocation alive for several sampler periods. The
  // observer must capture its effect even though both window endpoints match.
  delay(HEAP_SAMPLE_PERIOD_MS * 10);
  xSemaphoreTake(heapSampleMutex, portMAX_DELAY);
  const size_t observedMinimum = heapMinimums[1].largestMinimum;
  xSemaphoreGive(heapSampleMutex);
  heap_caps_free(buffer);
  heap_caps_get_info(&after, caps);
  expect(during.largest_free_block < before.largest_free_block, "largest-block probe reduces maximum");
  expect(observedMinimum <= during.largest_free_block, "sampler observes transient maximum drop");
  expect(after.total_free_bytes == before.total_free_bytes
    && after.largest_free_block == before.largest_free_block, "largest-block probe recovery");
  finishWindow(window);
  Serial.printf(
    "LARGEST_PROBE before=%u during=%u after=%u observed_min=%u\n",
    static_cast<unsigned int>(before.largest_free_block), static_cast<unsigned int>(during.largest_free_block),
    static_cast<unsigned int>(after.largest_free_block), static_cast<unsigned int>(observedMinimum)
  );
  xSemaphoreTake(heapSampleMutex, portMAX_DELAY);
  heapProbeActive = false;
  xSemaphoreGive(heapSampleMutex);
}

static void onStatusChange(const ESP32OT::Transport::StatusEvent& event, CallbackCounters& counters) {
  counters.changes.fetch_add(1, std::memory_order_relaxed);
  switch (event.current) {
    case OperationStatus::SUCCESS:
      counters.success.fetch_add(1, std::memory_order_relaxed);
      break;
    case OperationStatus::INVALID:
      counters.invalid.fetch_add(1, std::memory_order_relaxed);
      break;
    case OperationStatus::DATA_INVALID:
      counters.dataInvalid.fetch_add(1, std::memory_order_relaxed);
      break;
    case OperationStatus::UNKNOWN_DATA_ID:
      counters.unknownDataId.fetch_add(1, std::memory_order_relaxed);
      break;
    case OperationStatus::TIMEOUT:
      counters.timeout.fetch_add(1, std::memory_order_relaxed);
      break;
    case OperationStatus::ERROR:
      counters.error.fetch_add(1, std::memory_order_relaxed);
      break;
    case OperationStatus::CANCELLED:
      counters.cancelled.fetch_add(1, std::memory_order_relaxed);
      break;
    default:
      break;
  }
}

static void onResponse(const Message& message, CallbackCounters& counters) {

  // This hook already guarantees a complete frame, including INVALID frames.
  counters.lastFrame.store(message.getRaw(), std::memory_order_relaxed);
  counters.frames.fetch_add(1, std::memory_order_relaxed);
}

static void waitAndYield(unsigned int milliseconds, void* context) {
  auto& counters = *static_cast<CallbackCounters*>(context);
  counters.waits++;
  const TickType_t ticks = pdMS_TO_TICKS(milliseconds);
  vTaskDelay(ticks > 0 ? ticks : 1);
}

static void logResult(const char* phase, const Message& message) {
  char response[9] = "none";

  // A terminal timeout/error need not contain any received frame.
  if (message.hasFrame()) {
    snprintf(response, sizeof(response), "%08lX", static_cast<unsigned long>(message.getRaw()));
  }

  Serial.printf(
    "RESULT phase=%s status=%s has_frame=%u request=%08lX response=%s "
    "operation=%lu sequence=%lu attempt=%u error=%ld elapsed_us=%lu total_us=%lu\n",
    phase, Frame::statusToString(message.getStatus()), static_cast<unsigned int>(message.hasFrame()),
    static_cast<unsigned long>(message.getRequest()), response,
    static_cast<unsigned long>(message.getOperation()), static_cast<unsigned long>(message.getSequence()),
    static_cast<unsigned int>(message.getAttempt()), static_cast<long>(message.getError()),
    static_cast<unsigned long>(message.getElapsedUs()), static_cast<unsigned long>(message.getTotalElapsedUs())
  );
}

static bool isExchangeResult(const Message& message) {
  return message.getStatus() == OperationStatus::SUCCESS || message.getStatus() == OperationStatus::INVALID
    || message.getStatus() == OperationStatus::DATA_INVALID || message.getStatus() == OperationStatus::UNKNOWN_DATA_ID
    || message.getStatus() == OperationStatus::TIMEOUT;
}

static Message waitForResult(Controller& controller) {
  const uint32_t startedAt = millis();
  Message result = controller.getResult();
  while (!result.isComplete() && millis() - startedAt < 5000) {
    delay(5);
    controller.getResult(result);
  }
  expect(result.isComplete(), "async completion deadline");
  expect(isExchangeResult(result), "async exchange status");
  if (!result.isComplete()) {
    expect(controller.end(), "stop after async deadline");
    controller.getResult(result);
  }

  // Readiness includes the inter-frame gap; it does not mean a peer exists.
  while (controller.isInitialized() && !controller.isReady() && millis() - startedAt < 6000) {
    delay(5);
  }
  expect(!controller.isInitialized() || controller.isReady(), "async readiness deadline");
  return result;
}

static void stopController(Controller& controller, const char* phase) {
  HeapWindow window = startWindow(phase);
  expect(controller.end(), "end");

  // end() acknowledges cleanup before the worker deletes itself. Both idle
  // tasks must get CPU time to reclaim its stack and TCB before comparison.
  delay(100);
  expect(!controller.isInitialized() && !controller.isReady(), "stopped state");
  finishWindow(window);
}

// Local synthetic frames only: this does not exercise the RMT receive path.
static bool exerciseMessages() {
  using namespace ESP32OT::Messages;
  using ID = ESP32OT::MessageID;
  using Type = ESP32OT::MessageType;
  using Frame = ESP32OT::Protocol::Frame;
  const uint8_t sample = static_cast<uint8_t>(micros());
  bool ok = true;
  uint32_t checksum = 0;

  // Raw copies retain invalid parity and metadata; editing preserves routing.
  {
    ESP32OT::Protocol::MessageData snapshot;
    snapshot.frame = Frame::build(Type::READ_ACK, ID::Status, 0xa55a) ^ 0x80000000UL;
    snapshot.request = Message::read(ID::Status, 0x8100).build();
    snapshot.sequence = 17;
    snapshot.operation = 9;
    snapshot.attempt = 2;
    snapshot.elapsedUs = 12000;
    snapshot.totalElapsedUs = 34000;
    snapshot.error = ESP_ERR_INVALID_RESPONSE;
    snapshot.status = ESP32OT::OperationStatus::INVALID;
    snapshot.hasFrame = true;
    Message raw(snapshot);
    Status status = raw.as<Status>();
    ok &= status.build() == snapshot.frame && !Frame::isValidResponse(raw.getRaw());
    ok &= status.hasFrame() && status.isComplete() && !status.isSuccess();
    ok &= status.getOperation() == 9 && status.getAttempt() == 2;
    ok &= status.getElapsedUs() == 12000 && status.getTotalElapsedUs() == 34000;
    status.setCentralHeatingEnabled(false).setFlameOn(true);
    ok &= status.getData() == 0xa45a && status.getRequest() == snapshot.request;
    ok &= status.getSequence() == 17 && status.getError() == ESP_ERR_INVALID_RESPONSE;
    const auto reply = status.reply(Type::READ_ACK, 8);
    ok &= reply.getId() == ID::Status && reply.getSequence() == 17;
    ok &= reply.getRequest() == status.build() && Frame::isValidResponse(reply.build());
    raw.parse(reply.getMessageData());
    ok &= raw.getRaw() == reply.build() && raw.getHighByte() == 0xa4 && raw.getLowByte() == 8;
    checksum ^= raw.build() ^ Message::write(static_cast<ID>(200), sample).build();
  }

  // Exact signed f8.8 and integer boundaries, including the valid ID56 zero.
  {
    const auto number = Float::write(ID::Toutside, -12.25f);
    const auto readback = Float::read(ID::Toutside).reply(Type::READ_ACK, number.get());
    ok &= number.getData() == 0xf3c0 && readback.getData() == number.getData();
    ok &= Float::encode(-200.0f) == 0x8000 && Float::encode(200.0f) == 0x7fff;
    ok &= Float::writeTemperature(ID::TdhwSet, 0.0f).getData() == 0;
    const auto counter = UInt16::read(ID::PowerCycles).reply(Type::READ_ACK, 65535);
    const auto signedValue = Int16::write(ID::Texhaust, -32768).reply(Type::READ_ACK, 32767);
    const auto position = UInt8::write(ID::Vset, sample).reply(Type::WRITE_ACK, sample);
    ok &= counter.getUInt() == 65535 && signedValue.getInt() == 32767 && position.get() == sample;
    checksum ^= readback.build() ^ counter.build() ^ signedValue.build() ^ position.build();
  }

  // Named flag setters must preserve unknown bits and the companion byte.
  {
    MasterConfig master(0x80, sample);
    master.setSmartPowerImplemented(true).setMemberId(sample);
    SlaveConfig slave(0xc0, sample);
    slave.setDhwPresent(true).setCoolingSupported(true).setOnOffControl(false);
    Fault fault(0xc0, sample);
    fault.setServiceRequested(true).setGasFlameFault(true).setOemFaultCode(sample);
    RemoteParameters parameters(0x80, 0x80);
    parameters.setDhwSetpointSupported(true).setDhwSetpointWritable(true);
    ok &= master.isSmartPowerImplemented() && master.getData() == (0x8100 | sample);
    ok &= slave.isDhwPresent() && slave.isCoolingSupported() && slave.getMemberId() == sample;
    ok &= fault.isServiceRequested() && fault.isGasFlameFault() && fault.getHighByte() == 0xc9;
    ok &= parameters.getData() == 0x8181 && parameters.isDhwSetpointWritable();
    checksum ^= master.reply(0x81, sample).build() ^ slave.reply(0xc5, sample).build();
    checksum ^= fault.reply(0xc9, sample).build() ^ parameters.reply(0x81, 0x81).build();
  }

  // Read/write replies retain their requested indexes and distinct byte roles.
  {
    const RemoteRequest command(1);
    const auto rejected = command.reply(Type::WRITE_ACK, 127);
    const auto accepted = command.reply(Type::WRITE_ACK, 128);
    ok &= !rejected.isRequestAccepted() && accepted.isRequestAccepted();
    ok &= accepted.getRequestCode() == 1 && accepted.getResponseCode() == 128;
    const auto capacity = Capacity().reply(Type::READ_ACK, 24, 15);
    const auto fan = FanSpeed().reply(Type::READ_ACK, 60, sample);
    const auto bounds = TemperatureBounds(ID::TdhwSetUBTdhwSetLB).reply(Type::READ_ACK, 100, -40);
    const auto count = ParameterCount(ID::TSP).reply(Type::READ_ACK, sample);
    const auto entry = IndexedValue(ID::TSPindexTSPvalue, 7).reply(Type::READ_ACK, sample);
    ok &= capacity.getMaxCapacity() == 24 && capacity.getMinModulation() == 15;
    ok &= fan.getSetpointHz() == 60 && fan.getActualHz() == sample;
    ok &= bounds.getUpper() == 100 && bounds.getLower() == -40;
    ok &= count.getCount() == sample && count.getLowByte() == 0;
    ok &= entry.getIndex() == 7 && entry.getValue() == sample;
    checksum ^= accepted.build() ^ capacity.build() ^ fan.build() ^ bounds.build();
    checksum ^= count.build() ^ entry.build();
  }

  {
    const auto character = BrandCharacter(ID::Brand, 2).reply(Type::READ_ACK, 9, 'O');
    const auto version = ProductVersion(ID::SlaveVersion).reply(Type::READ_ACK, 4, sample);
    const auto nominal = NominalVentilation().reply(Type::READ_ACK, 100);
    ok &= character.getRequestIndex() == 2 && character.getMaxIndex() == 9 && character.getCharacter() == 'O';
    ok &= version.getProductType() == 4 && version.getVersion() == sample;
    ok &= nominal.get() == 100 && nominal.getData() == 0x6400;
    checksum ^= character.build() ^ version.build() ^ nominal.build();
  }

  // Ventilation flags are independent from boiler Status flags.
  {
    VentilationStatus status(0x80, 0x80);
    status.setVentilationEnabled(true).setBypassOpenRequested(true).setVentilationActive(true);
    VentilationFault fault(0xf0, sample);
    fault.setExhaustFanFault(true).setFrostProtectionActive(true).setOemFaultCode(sample);
    VentilationConfig config(0xf8, sample);
    config.setHeatRecoverySystem(true).setBypassPresent(true).setVariableSpeed(true);
    VentilationRemoteParameters parameters(0x80, 0x80);
    parameters.setNominalVentilationTransferEnabled(true).setNominalVentilationWritable(true);
    ok &= status.isVentilationEnabled() && status.isBypassOpenRequested() && status.getData() == 0x8382;
    ok &= fault.hasExhaustFanFault() && fault.isFrostProtectionActive() && fault.getHighByte() == 0xfa;
    ok &= config.isHeatRecoverySystem() && config.hasBypass() && config.isVariableSpeed();
    ok &= config.getConfigFlags() == 0xff && config.getMemberId() == sample;
    ok &= parameters.getData() == 0x8181 && parameters.isNominalVentilationWritable();
    checksum ^= status.reply(0x82).build() ^ fault.reply(0xfa, sample).build();
    checksum ^= config.reply(0xff, sample).build() ^ parameters.reply(0x81, 0x81).build();
  }

  // Solar reserved bits and raw configuration bytes remain caller-owned.
  {
    const Message source(Frame::build(Type::READ_ACK, ID::StatusSolarStorage, 0xf8c0));
    auto status = source.as<SolarStorageStatus>();
    status.setMasterMode(SolarStorageStatus::Mode::DHW_ECO)
      .setSlaveMode(SolarStorageStatus::Mode::DHW_COMFORT)
      .setState(SolarStorageStatus::State::SOLAR_LOADING).setFault(true);
    const auto reply = status.reply(SolarStorageStatus::Mode::DHW_ECO, SolarStorageStatus::State::BOILER_LOADING);
    SolarStorageConfig config;
    config.setConfigurationByte(0xa5).setMemberId(sample);
    const Message faultSource(Frame::build(Type::READ_ACK, ID::ASFflagsOEMfaultCodeSolarStorage, 0xa500));
    auto fault = faultSource.as<SolarStorageFault>();
    fault.setOemFaultCode(sample);
    ok &= status.getMasterMode() == SolarStorageStatus::Mode::DHW_ECO && status.isFault();
    ok &= (status.getData() & 0xf8c0) == 0xf8c0 && (reply.getData() & 0xffc0) == 0xf9c0;
    ok &= reply.getState() == SolarStorageStatus::State::BOILER_LOADING;
    ok &= config.getConfigurationByte() == 0xa5 && config.getMemberId() == sample;
    ok &= fault.getOemFaultCode() == sample && fault.getHighByte() == 0xa5;
    checksum ^= reply.build() ^ config.reply(0xa5, sample).build() ^ fault.reply(sample).build();
  }

  {
    const Message source(Frame::build(Type::READ_ACK, ID::RemoteOverrideFunction, 0xa5fc));
    auto priorities = source.as<RemoteOverrideFunction>();
    priorities.setManualChangePriorityEnabled(true).setProgramChangePriorityEnabled(false);
    ok &= priorities.isManualChangePriorityEnabled() && !priorities.isProgramChangePriorityEnabled();
    ok &= priorities.getData() == 0xa5fd && priorities.reply(false, true).getData() == 0xa5fe;
    const auto date = Date().reply(Type::READ_ACK, 12, 31);
    const auto time = DayTime().reply(Type::READ_ACK, 7, 23, 59);
    ok &= date.getMonth() == 12 && date.getDay() == 31;
    ok &= time.getDayOfWeek() == 7 && time.getHours() == 23 && time.getMinutes() == 59;
    checksum ^= priorities.build() ^ date.build() ^ time.build();
  }

  {
    RFSensorStatus radio(
      RFSensorStatus::SensorType::ROOM_SENSOR, 15,
      RFSensorStatus::BatteryStatus::LOW_BATTERY, 5
    );
    const auto reply = radio.reply(
      Type::WRITE_ACK, RFSensorStatus::SensorType::ROOM_SENSOR, 15,
      RFSensorStatus::BatteryStatus::NORMAL, 5
    );
    ok &= radio.getSensorType() == RFSensorStatus::SensorType::ROOM_SENSOR;
    ok &= radio.getSensorIndex() == 15 && radio.getSignalStrength() == 5;
    ok &= radio.getBatteryStatus() == RFSensorStatus::BatteryStatus::LOW_BATTERY;
    ok &= reply.getBatteryStatus() == RFSensorStatus::BatteryStatus::NORMAL;
    RemoteOverrideMode mode(
      RemoteOverrideMode::HeatingMode::COMFORT, RemoteOverrideMode::HeatingMode::REDUCED,
      RemoteOverrideMode::DhwMode::ANTI_LEGIONELLA, true
    );
    ok &= mode.getHeatingMode() == RemoteOverrideMode::HeatingMode::COMFORT;
    ok &= mode.getCh2Mode() == RemoteOverrideMode::HeatingMode::REDUCED;
    ok &= mode.getDhwMode() == RemoteOverrideMode::DhwMode::ANTI_LEGIONELLA && mode.isDhwPushRequested();
    checksum ^= radio.build() ^ reply.build() ^ mode.build();
  }

  // Keep repeated wrapper work observable in optimized builds without heap use.
  static volatile uint32_t retainedChecksum = 0;
  retainedChecksum = retainedChecksum ^ checksum;
  return ok;
}


static void runSuite(uint32_t round) {
  Serial.printf("SUITE_BEGIN round=%lu\n", static_cast<unsigned long>(round));
  const HeapSnapshot suiteBefore = readHeap();
  CallbackCounters counters;
  exerciseFragmentation();
  exerciseLargestBlockMinimum();
  {
    HeapWindow window = startWindow("constructors");
    {
      Controller positional(rxGpio, txGpio, false, false);
      expect(positional.getConfig().rxGpio == rxGpio, "positional constructor");
    }
    {
      Controller::Config config;
      config.rxGpio = rxGpio;
      config.txGpio = txGpio;
      Controller configured(config);
      expect(configured.getConfig().txGpio == txGpio, "Config constructor");
    }
    delay(100);
    finishWindow(window);
  }
  {
    HeapWindow window = startWindow("wrappers_500");
    for (uint32_t iteration = 0; iteration < 500; iteration++) {
      expect(exerciseMessages(), "synthetic wrapper checks");
    }
    finishWindow(window);
  }

  // Stack allocation makes the facade's own size separate from dynamic heap.
  // The callback context outlives this controller and its completed shutdown.
  {
    Controller controller;
    {
      HeapWindow window = startWindow("stopped_rejections");
      expect(!controller.begin(), "reject unconfigured GPIOs");
      expect(controller.getLastError() == ESP_ERR_INVALID_ARG, "invalid GPIO error");
      expect(!controller.sendRawRequestAsync(0), "reject async while stopped");
      expect(controller.sendRawRequest(0).getStatus() == OperationStatus::ERROR, "reject sync while stopped");
      expect(controller.end(), "end before startup");
      expect(controller.getWorkerStackHighWaterMark() == 0, "no stopped worker stack");
      finishWindow(window);
    }
    controller.onStatusChange([&counters](const ESP32OT::Transport::StatusEvent& event) {
      onStatusChange(event, counters);
    });
    expect(controller.onResponse([&counters](const Message& response) {
      onResponse(response, counters);
    }), "register response hook");
    expect(controller.onRequest([&counters](const Message&) {
      counters.requests.fetch_add(1, std::memory_order_relaxed);
    }), "register outgoing request hook");
    {
      HeapWindow window = startWindow("begin_default");
      expect(controller.begin(rxGpio, txGpio), "begin with GPIOs");
      expect(controller.getConfig().rxGpio == rxGpio && controller.getConfig().txGpio == txGpio, "save GPIOs");
      finishWindow(window, &controller);
    }
    {
      HeapWindow window = startWindow("idle_running");
      delay(250);
      finishWindow(window, &controller);
    }
    {
      HeapWindow window = startWindow("running_rejections");
      expect(!controller.begin(), "reject double begin");
      auto config = controller.getConfig();
      config.maxAttempts = 1;
      expect(!controller.setConfig(config), "reject running setConfig");
      expect(controller.getConfig().maxAttempts == 5, "retain running config");
      const auto response = Message::read(ESP32OT::MessageID::Status).reply(ESP32OT::MessageType::READ_ACK, 0);
      expect(controller.sendResponse(response).getStatus() == OperationStatus::ERROR, "reject master response");
      expect(!controller.sendResponseAsync(response), "reject master async response");
      expect(controller.sendRawResponse(response.build(), 1).getStatus() == OperationStatus::ERROR, "reject master raw response");
      expect(!controller.sendRawResponseAsync(response.build(), 1), "reject master raw async response");
      Message request;
      controller.getRequest(request);
      const auto typedRequest = controller.getRequest().as<Status>();
      expect(request.getRaw() == typedRequest.getRaw() && controller.getRequest().getSequence() == 0, "empty master request snapshot");
      finishWindow(window, &controller);
    }
    {
      HeapWindow window = startWindow("sync_default_5");
      const auto result = controller.sendRequest(Status());
      expect(isExchangeResult(result) && result.getAttempt() >= 1 && result.getAttempt() <= 5, "default retries complete");
      finishWindow(window, &controller);
      logResult("sync_default_5", result);
    }
    stopController(controller, "end_default");

    auto config = controller.getConfig();
    config.maxAttempts = 0;
    expect(controller.setConfig(config), "configure zero attempts");
    {
      HeapWindow window = startWindow("begin_retry_0");
      expect(controller.begin(), "begin zero attempts");
      finishWindow(window, &controller);
    }
    {
      HeapWindow window = startWindow("sync_retry_0");
      const Message request = Message::read(ESP32OT::MessageID::Tboiler);
      const Message result = controller.sendRequest(request);
      expect(isExchangeResult(result) && result.getAttempt() == 1, "zero means one attempt");
      finishWindow(window, &controller);
      logResult("sync_retry_0", result);
    }
    stopController(controller, "end_retry_0");

    config.maxAttempts = 1;
    expect(controller.setConfig(config), "configure one attempt");
    controller.setDelayCallback(waitAndYield, &counters);
    {
      HeapWindow window = startWindow("begin_custom_wait");
      expect(controller.begin(), "begin custom wait");
      finishWindow(window, &controller);
    }
    {
      HeapWindow window = startWindow("sync_typed_batch");
      ESP32OT::Protocol::MessageData results[HEAP_REQUESTS_PER_BATCH] = {};
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        const auto result = controller.sendRequest(ESP32OT::Messages::Float(ESP32OT::MessageID::Tboiler));
        expect(isExchangeResult(result) && result.getAttempt() == 1, "typed synchronous completion");
        results[index] = result.getMessageData();
      }
      finishWindow(window, &controller);

      // Keep every result on the caller's stack and print after measurement.
      // A final successful request must not hide earlier timeouts or invalid frames.
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        logResult("sync_typed_batch", Message(results[index]));
      }
      expect(controller.getResult<ESP32OT::Messages::Float>().getRaw() == results[HEAP_REQUESTS_PER_BATCH - 1].frame, "typed result snapshot");
    }
    {
      HeapWindow window = startWindow("sync_raw_batch");
      ESP32OT::Protocol::MessageData results[HEAP_REQUESTS_PER_BATCH] = {};
      const uint32_t raw = Frame::build(ESP32OT::MessageType::READ_DATA, ESP32OT::MessageID::SlaveVersion);
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        const auto result = controller.sendRawRequest(raw);
        expect(isExchangeResult(result) && result.getRequest() == raw, "raw synchronous completion");
        results[index] = result.getMessageData();
      }
      finishWindow(window, &controller);
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        logResult("sync_raw_batch", Message(results[index]));
      }
    }
    {
      HeapWindow window = startWindow("async_typed_busy_batch");
      ESP32OT::Protocol::MessageData results[HEAP_REQUESTS_PER_BATCH] = {};
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        const bool accepted = controller.sendRequestAsync(Status());
        expect(accepted, "accept typed async");
        if (accepted) {
          expect(!controller.sendRequestAsync(Status()), "reject concurrent async");
          const auto result = waitForResult(controller);
          expect(result.isComplete(), "typed async completion");
          results[index] = result.getMessageData();
        }
      }
      finishWindow(window, &controller);
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        logResult("async_typed_busy_batch", Message(results[index]));
      }
    }
    {
      HeapWindow window = startWindow("async_raw_batch");
      ESP32OT::Protocol::MessageData results[HEAP_REQUESTS_PER_BATCH] = {};
      const uint32_t raw = Frame::build(ESP32OT::MessageType::READ_DATA, ESP32OT::MessageID::Tboiler);
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        const bool accepted = controller.sendRawRequestAsync(raw);
        expect(accepted, "accept raw async");
        if (accepted) {
          const auto result = waitForResult(controller);
          expect(result.getAttempt() == 1 && result.getRequest() == raw, "raw async completion");
          results[index] = result.getMessageData();
        }
      }
      finishWindow(window, &controller);
      for (uint32_t index = 0; index < HEAP_REQUESTS_PER_BATCH; index++) {
        logResult("async_raw_batch", Message(results[index]));
      }
    }
    stopController(controller, "end_custom_wait");
    expect(counters.waits != 0, "custom delay hook called");
    controller.setDelayCallback();

    config.alwaysReceive = true;
    expect(controller.setConfig(config), "enable passive RX");
    {
      HeapWindow window = startWindow("begin_passive");
      expect(controller.begin(), "begin passive RX");
      finishWindow(window, &controller);
    }
    {
      HeapWindow window = startWindow("passive_idle_and_request");
      delay(500);
      expect(isExchangeResult(controller.sendRequest(Status())), "request with passive RX");
      finishWindow(window, &controller);
      logResult("passive_idle_and_request", controller.getResult());
    }
    {
      HeapWindow window = startWindow("cancel_on_end");
      expect(controller.sendRawRequestAsync(0), "accept request to cancel");
      const uint32_t operation = controller.getResult().getOperation();
      expect(controller.end(), "end during active request");
      delay(100);
      const auto result = controller.getResult();
      expect(result.getOperation() == operation && result.getStatus() == OperationStatus::CANCELLED, "retain cancellation");
      expect(controller.end(), "idempotent end");
      finishWindow(window);
      logResult("cancel_on_end", result);
    }

    config.alwaysReceive = false;
    config.stackSize = 4096;
    expect(controller.setConfig(config), "configure larger worker stack");
    {
      HeapWindow window = startWindow("begin_stack_4096");
      expect(controller.begin(), "begin larger worker stack");
      finishWindow(window, &controller);
    }
    stopController(controller, "end_stack_4096");

    config.stackSize = 3072;
    expect(controller.setConfig(config), "restore worker stack");
    {
      HeapWindow window = startWindow("restart_cycles");
      for (uint32_t index = 0; index < HEAP_RESTART_CYCLES; index++) {
        expect(controller.begin(), "repeated begin");
        expect(controller.end(), "repeated end");
        delay(100);
      }
      finishWindow(window);
    }

    // end() completed all worker callbacks. Only now clear the hooks and allow
    // the stack-owned callback context to be destroyed after the controller.
    controller.onStatusChange();
    expect(controller.onResponse(), "clear response hook");
    expect(controller.onRequest(), "clear request hook");
  }
  delay(100);
  const HeapSnapshot suiteAfter = readHeap();
  for (size_t index = 0; index < 3; index++) {
    Serial.printf(
      "SUITE_END round=%lu pool=%s before=%u after=%u delta=%ld largest_before=%u largest_after=%u\n",
      static_cast<unsigned long>(round), heapNames[index],
      static_cast<unsigned int>(suiteBefore.pools[index].total_free_bytes),
      static_cast<unsigned int>(suiteAfter.pools[index].total_free_bytes),
      static_cast<long>(suiteAfter.pools[index].total_free_bytes)
        - static_cast<long>(suiteBefore.pools[index].total_free_bytes),
      static_cast<unsigned int>(suiteBefore.pools[index].largest_free_block),
      static_cast<unsigned int>(suiteAfter.pools[index].largest_free_block)
    );
  }
  Serial.printf(
    "CALLBACKS round=%lu changes=%lu success=%lu invalid=%lu data_invalid=%lu unknown_data_id=%lu "
    "timeout=%lu error=%lu cancelled=%lu "
    "frames=%lu tx_requests=%lu last_frame=%08lX waits=%lu failures=%lu first_failure=%s\n",
    static_cast<unsigned long>(round), static_cast<unsigned long>(counters.changes.load()),
    static_cast<unsigned long>(counters.success.load()), static_cast<unsigned long>(counters.invalid.load()),
    static_cast<unsigned long>(counters.dataInvalid.load()), static_cast<unsigned long>(counters.unknownDataId.load()),
    static_cast<unsigned long>(counters.timeout.load()), static_cast<unsigned long>(counters.error.load()),
    static_cast<unsigned long>(counters.cancelled.load()), static_cast<unsigned long>(counters.frames.load()),
    static_cast<unsigned long>(counters.requests.load()), static_cast<unsigned long>(counters.lastFrame.load()),
    static_cast<unsigned long>(counters.waits),
    static_cast<unsigned long>(failures), firstFailure != nullptr ? firstFailure : "none"
  );
}

void setup() {
  Serial.begin(115200);
  delay(4000);
  Serial.printf(
    "MasterHeap RX=%d TX=%d rounds=%u batch=%u restarts=%u sizeof_controller=%u sizeof_message=%u sizeof_status=%u\n",
    static_cast<int>(rxGpio), static_cast<int>(txGpio), HEAP_SUITE_ROUNDS,
    HEAP_REQUESTS_PER_BATCH, HEAP_RESTART_CYCLES,
    static_cast<unsigned int>(sizeof(Controller)), static_cast<unsigned int>(sizeof(Message)),
    static_cast<unsigned int>(sizeof(Status))
  );
  Serial.println("Heap/stack units are bytes. delta=after-before. Pools overlap; do not add them.");
  Serial.println("Local minima include monitor overhead and combine regional minima, not simultaneous peaks.");
  Serial.println("BLOCKS sizes are raw allocator bytes; fragmentation is weighted within physical heap regions.");
  Serial.println("No peer required for timeout tests; received responses require a responding test peer.");
  Serial.println("MINIMA records repeated samples; max_gap_us reports observation gaps, not an allocation trace.");
  Serial.printf(
    "SAMPLER period_ms=%u static_stack_bytes=%u static_tcb_bytes=%u\n",
    static_cast<unsigned int>(HEAP_SAMPLE_PERIOD_MS), static_cast<unsigned int>(sizeof(heapSamplerStack)),
    static_cast<unsigned int>(sizeof(heapSamplerState))
  );
  startHeapSampler();

  // Warm up stdio, heap inspection and the SDK monitor before taking baselines.
  HeapWindow window = startWindow("measurement_warmup");
  finishWindow(window);
}

void loop() {
  static uint32_t round = 0;
  if (round < HEAP_SUITE_ROUNDS) {
    runSuite(++round);
    if (round == HEAP_SUITE_ROUNDS) {
      stopHeapSampler();
      Serial.printf("HEAP_DONE rounds=%lu failures=%lu\n", static_cast<unsigned long>(round), static_cast<unsigned long>(failures));
    }
  }
  delay(1000);
}
