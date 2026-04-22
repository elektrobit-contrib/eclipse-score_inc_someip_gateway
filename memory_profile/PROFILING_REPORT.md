<!--
*******************************************************************************
Copyright (c) 2026 Contributors to the Eclipse Foundation

See the NOTICE file(s) distributed with this work for additional
information regarding copyright ownership.

This program and the accompanying materials are made available under the
terms of the Apache License Version 2.0 which is available at
https://www.apache.org/licenses/LICENSE-2.0

SPDX-License-Identifier: Apache-2.0
*******************************************************************************
-->

# Memory Profiling Report: gateway_ipc_binding_memory

## Build Configuration
- **Build Type**: Release (-c opt)
- **Compilation Optimizations**: Enabled
- **Thread Sanitizer**: Disabled (removed -tsan feature)
- **Binary**: `gateway_ipc_binding_memory` (dedicated memory profiling application)
- **Binary Path**: `bazel-bin/src/gateway_ipc_binding/benchmark/gateway_ipc_binding_memory`
- **Binary Size**: ~18 MB
- **Build Date**: 2026-04-22

## Profiling Setup
- **Tools**: GNU memusage + Valgrind Massif
- **Workload**: 1,000,000 iterations, 1 MB payload
- **Test Duration**: ~6.2 seconds (memusage), ~52 seconds (Massif)
- **Profiling Data**: `memusage.data`, `memusage.png`, `massif.out.1`

## Memory Usage Summary

### GNU memusage Statistics
- **Total Heap Allocated**: 297,104,581 bytes (~283 MB cumulative over 1M iterations)
- **Peak Heap Usage**: 172,314 bytes (~168 KB)
- **Peak Stack Usage**: 10,384 bytes (~10 KB)

### Allocation Operations
- **Total malloc calls**: 6,021,543
- **Total realloc calls**: 0
- **Total calloc calls**: 3
- **Total free calls**: 6,021,547
- **Failed allocations**: 0

### Valgrind Massif Statistics
- **Peak Heap Usage (useful)**: 175,594 bytes (~172 KB)
- **Peak Heap Usage (total with overhead)**: 180,664 bytes (~176 KB)
- **Number of Snapshots**: 59
- **Heap Profile Shape**: Flat/stable across entire run (no growth)

### Allocation Pattern (Histogram)
Dominant allocation sizes:
- **32-47 bytes**: 3,002,521 blocks (49%) - Most prevalent
- **16-31 bytes**: 1,009,023 blocks (16%)
- **48-63 bytes**: 1,000,055 blocks (16%)
- **112-127 bytes**: 1,000,017 blocks (16%)

## Execution Performance
(Dedicated memory profiling application, 1 MB payload, 1M iterations)

### Event Transmission (1 MB payload)
- **Total benchmark time**: 3,878 ms
- **Total execution time**: 6,222 ms (including setup/teardown)
- **Latency per iteration**: ~3.9 µs
- **Iterations**: 1,000,000

### Per-Iteration Allocation Cost
- **malloc calls per iteration**: ~6.0
- **Bytes allocated per iteration**: ~297 bytes
- **Net heap growth per iteration**: 0 (all freed)

## Massif Top Allocators (at peak snapshot)

| Allocator | Size | % of Peak |
|-----------|------|-----------|
| `ClientConnection::ClientConnection` (vector reserve) | 81,920 B | 49.1% |
| `ConsoleRecorderFactory::CreateConsoleLoggingBackend` (logging) | 24,792 B | 14.8% |
| `LogRecord` circular allocator (logging buffer) | 16,384 B | 9.8% |
| `Shared_memory_managers::insert_allocation` (hash table) | 3,072 B | 1.7% |
| `Gateway_ipc_binding_client_impl` | 2,912 B | 1.6% |
| `Connection_metadata::add_mapping` | 2,128 B | 1.2% |
| `Writable_payload` (shared_ptr) | 2,048 B | 1.1% |
| `Shared_memory_slot_manager_impl` | 2,048 B | 1.1% |
| `ClientConnection` send command buffer | 1,920 B | 1.1% |
| Other (159 places below threshold) | 23,914 B | 14.3% |

## Key Findings

1. **Stable Memory Profile**: Peak heap is ~168-172 KB useful bytes, perfectly flat across 1M iterations — no growth
2. **Peak Memory**: 172 KB peak (Massif useful-heap), 176 KB total with overhead
3. **Allocation Pattern**: Small allocations dominate (32-47 bytes account for 49% of 6M allocations)
4. **No Leaks**: Perfect balance — 6,021,547 free calls vs 6,021,543 malloc + 3 calloc calls
5. **Dominant Allocator**: `ClientConnection` buffer reserve accounts for 49% of peak heap
6. **Logging Overhead**: Console logging backend accounts for ~25% of peak heap (24.8 KB + 16.4 KB)

## Recommendations

- **Light Memory Footprint**: Peak at 172 KB useful heap — excellent for embedded/real-time systems
- **Flat Allocation Profile**: Zero heap growth over 1M iterations confirms no leaks and stable memory
- **Small Allocation Optimization**: 97% of allocations are < 128 bytes; object pooling for 32-47 byte blocks could reduce allocator overhead
- **Logging Overhead**: ~25% of peak heap is logging infrastructure; consider disabling in production or using a lighter backend
- **ClientConnection Buffer**: 80 KB reserve (49% of peak) is one-time setup cost, acceptable for long-running services

## Output Files

- `memusage.data`: Raw profiling data file (276 MB)
- `memusage.png`: Memory usage timeline graph
- `massif.out.1`: Valgrind Massif heap profile (324 KB)
- `benchmark_run.log`: memusage execution log
- `massif_run.log`: Massif execution log
- `PROFILING_REPORT.md`: This report
