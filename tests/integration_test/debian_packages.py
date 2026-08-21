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

from score.itf.plugins.core import Target


def _execute_and_assert_success(target: Target, command: str) -> None:
    exit_code, output = target.execute(command)
    assert exit_code == 0, f"{command} failed:\n{output.decode(errors='replace')}"


def test_debian_packages_install_and_run_help(clean_state: Target) -> None:
    _execute_and_assert_success(
        clean_state,
        "apt-get install --yes /packages/s-core-base.deb /packages/someip-gateway.deb",
    )
    _execute_and_assert_success(clean_state, "someipd --help")
    _execute_and_assert_success(clean_state, "gatewayd --help")
