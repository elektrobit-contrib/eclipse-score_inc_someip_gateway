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

# benchmark_event_transmission_client_to_server profiling analysis

## Scope
- Benchmark target: `//src/gateway_ipc_binding/benchmark:gateway_ipc_binding_benchmark`
- Benchmark case profiled: `benchmark_event_transmission_client_to_server/1024/manual_time`
- Build mode used for final analysis: `-c opt --features=-tsan`
- Profiling command: `perf record -F 999 -g --call-graph dwarf,16384`

## Artifacts
- Flamegraph (final, non-TSan): `profiling/event_transmission_client_to_server_1024_notsan_flamegraph.svg`
- Perf data (final, non-TSan): `profiling/perf_event_c2s_1024_notsan_ok.data`
- Folded stacks: `profiling/perf_event_c2s_1024_notsan_ok.folded`
- Percent summary: `profiling/perf_event_c2s_1024_notsan_ok_percent_summary.txt`

## Benchmark result used for profile
- `benchmark_event_transmission_client_to_server/1024/manual_time`: about 5228 ns manual time (single run with perf attached)

## Important caveat
An initial profile was collected while ThreadSanitizer instrumentation was active due `user.bazelrc` containing `common --features=tsan`. That profile was dominated by `__tsan_*` internals and is not representative for release bottlenecks. Final analysis below uses the non-TSan profile.

## Bottlenecks (non-TSan)
Percentages below are cumulative stack presence from folded stacks and indicate where execution time is spent across call paths.

1. Syscall and scheduler heavy path
- `entry_SYSCALL_64_after_hwframe`: ~72.31%
- `do_syscall_64`: ~71.75%
- `x64_sys_call`: ~67.71%
- `schedule` + `__schedule`: ~32.01% combined cumulative presence
- Interpretation: event transmission path is dominated by kernel transitions, waiting/wakeup, and scheduling overhead rather than payload processing.

2. Poll/recv/send loop overhead
- `score::os::SysPollImpl::poll`: ~25.76%
- `__GI___poll`: ~25.71%
- `score::os::SocketImpl::recvmsg`: ~24.77%
- `score::os::SocketImpl::sendmsg`: ~16.79%
- `do_sys_poll`: ~21.16%, `__x64_sys_recvmsg`: ~21.50%, `__x64_sys_sendmsg`: ~14.94%
- Interpretation: Unix domain IPC calls and readiness polling are central costs.

3. Message handling path
- `score::message_passing::detail::UnixDomainServer::ServerConnection::ProcessInput`: ~28.35%
- `score::message_passing::UnixDomainEngine::ReceiveProtocolMessage`: ~25.08%
- `score::gateway_ipc_binding::Gateway_ipc_binding_base::handle_event_update_message`: ~11.92%
- Interpretation: framework dispatch and protocol processing is significant but still below kernel communication overhead.

4. Synchronization/queueing micro-costs (leaf signals)
- `fdget`: ~2.07%
- `fput`: ~1.94%
- `_copy_to_iter`: ~1.54%
- `_copy_from_user`: ~1.25%
- `_raw_spin_lock` + `__raw_spin_lock_irqsave`: ~3.25% combined leaf
- Interpretation: many small per-message kernel-side bookkeeping costs accumulate due high message rate.

## Improvement suggestions
Prioritized by expected impact and implementation risk.

1. Reduce syscall frequency per event (highest impact)
- Batch event updates where semantics allow.
- Coalesce notifications and transmit multiple updates in one message.
- Prefer fewer, larger transfers over many small transfer operations.
- Why: the profile is syscall-bound; fewer transitions directly lower end-to-end cost.

2. Replace or reduce poll wakeups
- If feasible in the transport layer, move from poll loop patterns toward readiness/eventfd integration with less wake-sleep churn.
- Tune poll timeout and wake strategy to avoid frequent short sleeps.
- Why: `poll` and scheduler frames are consistently dominant.

3. Reuse buffers and avoid per-update allocation/metadata churn
- Reuse event payload objects and avoid repeated setup where API allows.
- Validate that shared memory slot lookup and map operations are amortized/cached.
- Why: while not top-most alone, small per-event fixed costs become material at sub-10 us event times.

4. Optimize synchronization strategy in hot callbacks
- Review mutex/condition variable use in benchmark context for wait/notify overhead.
- Consider lock-free sequence publication in the benchmark harness to avoid benchmark-measurement interference.
- Why: lock and scheduler costs appear often in leaf frames.

5. Ensure production profiling setup in CI/dev
- Keep sanitizer builds for correctness, but profile performance with sanitizer features disabled.
- Add a documented perf profile target that enforces `--features=-tsan`.
- Why: sanitizer noise can fully mask true bottlenecks.

## Stability note
- During one non-TSan run with longer profiling duration, the benchmark reported `Failed to send or receive benchmark event`. The final analyzed run completed successfully.
- Suggestion: add retry logic around profiling runs and optionally increase shared-memory slot count or timeout handling in the benchmark harness to avoid intermittent aborts under profiler overhead.
