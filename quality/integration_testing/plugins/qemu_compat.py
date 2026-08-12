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

"""Compatibility wrapper around upstream ITF QEMU plugin checks.

The upstream plugin currently uses strict readiness timing in
``pre_tests_phase``. On virtualized CI hosts (especially without KVM), first
boot can be significantly slower. Keep upstream behavior intact, but allow
tuning the ping window via SCORE_ITF_QEMU_PING_TIMEOUT and use more forgiving
defaults for SSH/SFTP readiness checks.
"""

import os
import time

import score.itf.plugins.qemu as _upstream_qemu_plugin
import score.itf.plugins.qemu.checks as _upstream_checks
from score.itf.plugins.qemu import *  # pylint: disable=wildcard-import,unused-wildcard-import


_DEFAULT_PING_TIMEOUT_S = 180
_SFTP_READINESS_ATTEMPTS = 10
_SFTP_READINESS_DELAY_S = 2


def _pre_tests_phase_with_compat_timeout(target):
    ping_timeout = int(
        os.getenv("SCORE_ITF_QEMU_PING_TIMEOUT", str(_DEFAULT_PING_TIMEOUT_S))
    )
    _upstream_checks._check_ping(target, check_timeout=ping_timeout)

    _upstream_checks._check_ssh_is_up(target, check_timeout=20, check_n_retries=180)
    for attempt in range(_SFTP_READINESS_ATTEMPTS):
        try:
            _upstream_checks._check_sftp_is_up(target)
            return
        except Exception:  # noqa: BLE001
            if attempt == _SFTP_READINESS_ATTEMPTS - 1:
                raise
            time.sleep(_SFTP_READINESS_DELAY_S)


# Rebind the symbol used by score.itf.plugins.qemu.target_init.
_upstream_qemu_plugin.pre_tests_phase = _pre_tests_phase_with_compat_timeout
