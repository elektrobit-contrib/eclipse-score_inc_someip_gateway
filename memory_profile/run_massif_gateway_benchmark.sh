#!/usr/bin/env bash
# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCHMARK_BIN="${ROOT_DIR}/bazel-bin/src/gateway_ipc_binding/benchmark/gateway_ipc_binding_benchmark"
OUT_FILE="${MASSIF_OUT_FILE:-${ROOT_DIR}/memory_profile/massif.out}"
OUT_PATTERN="${OUT_FILE}.%p"

if [[ ! -x "${BENCHMARK_BIN}" ]]; then
  echo "Benchmark binary not found or not executable: ${BENCHMARK_BIN}" >&2
  echo "Build it first with:" >&2
  echo "  bazel build //src/gateway_ipc_binding/benchmark:gateway_ipc_binding_benchmark" >&2
  exit 1
fi

mkdir -p "$(dirname "${OUT_FILE}")"
rm -f "${OUT_FILE}" "${OUT_FILE}".*

# The benchmark re-execs internally; Massif must follow child execs to emit output.
valgrind \
  --tool=massif \
  --trace-children=yes \
  --massif-out-file="${OUT_PATTERN}" \
  "${BENCHMARK_BIN}" \
  "$@"

# will be fixed later
# shellcheck disable=SC2012
latest_massif_file="$(ls -1t "${OUT_FILE}".* 2>/dev/null | head -n 1 || true)"
if [[ -z "${latest_massif_file}" ]]; then
  echo "Massif did not create an output file. Check benchmark/valgrind output above." >&2
  exit 2
fi

cp "${latest_massif_file}" "${OUT_FILE}"

echo "Massif output written to: ${latest_massif_file}"
echo "Stable copy written to: ${OUT_FILE}"
echo "Render with: ms_print ${OUT_FILE}"
