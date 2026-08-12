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

load("@rules_python//python:defs.bzl", "py_library")
load("@score_itf//bazel:py_itf_plugin.bzl", "py_itf_plugin")

py_library(
    name = "qemu_compat",
    srcs = ["qemu_compat.py"],
    visibility = ["//visibility:public"],
    deps = [
        "@score_itf//score/itf/plugins/qemu",
    ],
)

py_itf_plugin(
    name = "qemu_compat_plugin",
    enabled_plugins = [
        "quality.integration_testing.plugins.qemu_compat",
    ],
    py_library = ":qemu_compat",
    visibility = ["//visibility:public"],
)
