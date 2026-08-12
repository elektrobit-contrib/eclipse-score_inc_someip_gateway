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

"""
Integration Tests
"""

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files")
load("//quality/integration_testing:integration_testing.bzl", "integration_test")

genrule(
    name = "host_libatomic_so_1",
    outs = ["libatomic.so.1"],
    cmd = "cp /usr/lib/x86_64-linux-gnu/libatomic.so.1 $@",
)

genrule(
    name = "host_ping_bin",
    outs = ["ping"],
    cmd = "cp /usr/bin/ping $@",
)

pkg_files(
    name = "vsomeip-runtime-pkg",
    srcs = [
        "@vsomeip",
    ],
    attributes = pkg_attributes(
        mode = "0777",
    ),
    prefix = "/usr",
)

pkg_files(
    name = "example-app-pkg",
    srcs = [
        ":host_libatomic_so_1",
        ":host_ping_bin",
        ":vsomeip.json",
        ":vsomeip-runtime-pkg",
        "//score/config:config_file",
        "//score/gatewayd",
        "//score/gatewayd:etc/mw_com_config.json",
        "//score/serializer:null_serializer",
        "//score/someipd",
        "//tests/integration/sample_client",
    ],
    attributes = pkg_attributes(
        mode = "0777",
    ),
    prefix = "/",
    renames = {
        ":host_libatomic_so_1": "lib/x86_64-linux-gnu/libatomic.so.1",
        ":host_ping_bin": "usr/bin/ping",
        "//score/config:config_file": "mw_someip_config.bin",
        "//score/gatewayd:etc/mw_com_config.json": "gatewayd_mw_com_config.json",
    },
)

integration_test(
    name = "test_tcpdump",
    timeout = "moderate",
    srcs = [
        "tcpdump.py",
        "util.py",
    ],
    filesystem = ":example-app-pkg",
    deps = [
        "@score_someip_gateway_pip//pytest",
    ],
)

integration_test(
    name = "test_gateway_startup",
    timeout = "moderate",
    srcs = [
        "conftest.py",
        "gatewayd_startup.py",
        "util.py",
    ],
    filesystem = ":example-app-pkg",
    deps = [
        "@score_someip_gateway_pip//pytest",
        "@score_someip_gateway_pip//someip",
    ],
)

integration_test(
    name = "test_someipd_startup",
    timeout = "moderate",
    srcs = [
        "conftest.py",
        "someipd_startup.py",
        "util.py",
    ],
    filesystem = ":example-app-pkg",
    deps = [
        "@score_someip_gateway_pip//pytest",
        "@score_someip_gateway_pip//someip",
    ],
)

integration_test(
    name = "test_someipd_sends_someip-sd_message",
    timeout = "moderate",
    srcs = [
        "conftest.py",
        "someipd_sends_someip-sd_message.py",
        "util.py",
    ],
    filesystem = ":example-app-pkg",
    deps = [
        "@score_someip_gateway_pip//pytest",
        "@score_someip_gateway_pip//someip",
    ],
)

integration_test(
    name = "test_someipd_started_by_test",
    timeout = "moderate",
    srcs = [
        "conftest.py",
        "someipd_started_by_test.py",
        "util.py",
    ],
    filesystem = ":example-app-pkg",
    deps = [
        "@score_someip_gateway_pip//pytest",
        "@score_someip_gateway_pip//someip",
    ],
)
