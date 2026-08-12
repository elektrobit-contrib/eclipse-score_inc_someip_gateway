# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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

load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@score_itf//bazel:copy_files_onto_image.bzl", "copy_files_onto_image")

exports_files(
    [
        "qemu_config.json",
        "user-data",
        "meta-data",
        "build_image.sh",
        "render_user_data.py",
    ],
    visibility = ["//:__subpackages__"],
)

filegroup(
    name = "qemu_config",
    srcs = ["qemu_config.json"],
    visibility = ["//:__subpackages__"],
)

# Build a persistent Ubuntu base image with tools needed by integration tests.
# It boots once with internet access and applies the cloud-init customization.
genrule(
    name = "prepared_image",
    srcs = [
        "@ubuntu_24_04_cloudimg//file",
        "build_image.sh",
        "render_user_data.py",
        "user-data",
        "meta-data",
        "qemu_config.json",
    ],
    outs = ["ubuntu24_04_prepared.qcow2"],
    cmd = """
        bash $(location build_image.sh) \
            $(RULEDIR) \
            $(location @ubuntu_24_04_cloudimg//file) \
            $@ \
            $(location user-data) \
            $(location meta-data) \
            $(location qemu_config.json) \
            $(location render_user_data.py)
    """,
    tags = [
        "manual",
        "requires-network",
    ],
    tools = [
        "build_image.sh",
        "render_user_data.py",
    ],
    visibility = ["//:__subpackages__"],
)

pkg_tar(
    name = "overlay_files",
    srcs = ["overlay/score_itf_overlay_marker"],
    mode = "0644",
    package_dir = "/etc/score_itf",
)

copy_files_onto_image(
    name = "image_with_overlay",
    srcs = [":overlay_files"],
    out = "ubuntu24_04_prepared.overlay.qcow2",
    image = ":prepared_image",
    tags = ["manual"],
    visibility = ["//visibility:private"],
)

filegroup(
    name = "image_overlay",
    srcs = [":image_with_overlay"],
    tags = ["manual"],
    visibility = ["//:__subpackages__"],
)
