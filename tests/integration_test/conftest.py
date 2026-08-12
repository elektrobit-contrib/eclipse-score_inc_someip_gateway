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
Pytest configuration and fixtures for integration tests.
"""

import os
import pytest
from dataclasses import dataclass
from typing import Generator
from util import (
    ShellProcess,
    check_environment_and_mark,
    stop_local_process,
    tcpdump_capture,
)
from score.itf.core.process.async_process import AsyncProcess
from score.itf.plugins.core import Target


@dataclass
class GatewaydWithSomeipdSession:
    target: Target
    someipd_process: AsyncProcess


@pytest.fixture(scope="function")
def clean_state(target: Target) -> Generator[Target, None, None]:
    """This fixture can only be used once per QEMU instance / linux-sandbox instance, to ensure a clean environment.

    Otherwise subsequent tests might fail due to leftover state from previous tests.
    Additionally killing tcpdump is problematic (kill errors with permission denied), but terminating linux-sandbox should get rid of it.
    Lastly the output pcap file name should contain the test name, if we want to support multiple tests per run.
    """

    check_environment_and_mark(target)
    yield target


@pytest.fixture(scope="function")
def gatewayd_with_someipd(
    clean_state: Target,
) -> Generator[GatewaydWithSomeipdSession, None, None]:
    """Start someipd and gatewayd before tests and stop them after."""

    # TODO tcpdump cannot be killed when the test is done. Need to figure out why

    # Store test traffic in file for later analysis of failures
    pcap_dir = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR", ".")
    pcap_file = os.path.join(pcap_dir, "test_traffic.pcap")

    # No context manager is used for host tcpdump because Popen.__exit__ waits for the
    # process to terminate, which would otherwise block teardown when the capture stays open.
    tcpdump_process = tcpdump_capture("", output_file=pcap_file)

    try:
        with ShellProcess(
            clean_state,
            "/someipd",
            args=[
                "--configuration",
                "/mw_someip_config.bin",
            ],
            env="VSOMEIP_CONFIGURATION=/vsomeip.json",
        ) as someipd_process:
            assert someipd_process.is_running(), someipd_process.get_output()
            with ShellProcess(
                clean_state,
                "/gatewayd",
                args=[
                    "--configuration",
                    "/mw_someip_config.bin",
                    "--service_instance_manifest",
                    "/gatewayd_mw_com_config.json",
                ],
            ) as gatewayd_process:
                assert gatewayd_process.is_running(), gatewayd_process.get_output()
                assert gatewayd_process.is_running(), (
                    gatewayd_process.get_output(),
                    "exit code: ",
                    gatewayd_process.get_exit_code(),
                )
                assert someipd_process.is_running(), (
                    someipd_process.get_output(),
                    "exit code: ",
                    someipd_process.get_exit_code(),
                )
                yield GatewaydWithSomeipdSession(
                    target=clean_state,
                    someipd_process=someipd_process,
                )
    finally:
        stop_local_process(tcpdump_process)
