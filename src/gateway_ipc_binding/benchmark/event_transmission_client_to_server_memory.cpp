/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include <unistd.h>

#include <cstddef>
#include <cstdlib>

#include "event_transmission_benchmark_context.hpp"

int main() {
    score::gateway_ipc_binding::Event_transmission_benchmark_context context(1024 * 1024);

    for (std::size_t i = 0; i < 100000; ++i) {
        auto duration = context.send_and_measure_once();
        benchmark::DoNotOptimize(duration);
    }

    return EXIT_SUCCESS;
}
