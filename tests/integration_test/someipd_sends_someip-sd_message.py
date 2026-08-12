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

import logging
from util import (
    wait_until_output_contains,
)
from conftest import GatewaydWithSomeipdSession


def test_start_someipd_and_gatewayd(
    gatewayd_with_someipd: GatewaydWithSomeipdSession,
) -> None:
    """Verify that someipd schedules and offers the SOME/IP service-discovery advertisement."""
    console_output = wait_until_output_contains(
        gatewayd_with_someipd.someipd_process,
        [
            "rmi::offer_service: added service 0x1234 to pending_sd_offers_.size = 2",
            "OFFER(0100): [1234.5678:0.0] (true)",
        ],
        timeout=10.0,
    )
    logging.info("someipd output confirming SOME/IP-SD offer...\n" + console_output)
