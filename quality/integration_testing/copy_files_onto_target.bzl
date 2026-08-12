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

"""Rule for copying filesystem tar archives onto a QEMU rootfs overlay."""

def _copy_files_onto_target_impl(ctx):
    out = ctx.outputs.out

    args = ctx.actions.args()
    args.add(ctx.file.image.path)
    args.add(out.path)
    args.add_all([src.path for src in ctx.files.srcs])

    ctx.actions.run_shell(
        inputs = [ctx.file.image] + ctx.files.srcs,
        outputs = [out],
        arguments = [args],
        execution_requirements = {
            "no-sandbox": "1",
        },
        command = """
set -euo pipefail

BASE_IMAGE="$1"
OVERLAY_IMAGE="$2"
shift 2

for tool in qemu-img qemu-system-x86_64 cloud-localds; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "Error: ${tool} is required but not installed" >&2
        exit 1
    fi
done

BASE_IMAGE_ABS="$(readlink -f "${BASE_IMAGE}")"
BASE_FORMAT="$(qemu-img info --output=json "${BASE_IMAGE_ABS}" | python3 -c 'import json,sys; print(json.load(sys.stdin)["format"])')"

rm -f "${OVERLAY_IMAGE}" "${OVERLAY_IMAGE}.pid" "${OVERLAY_IMAGE}.seed.img"
qemu-img create -f qcow2 -b "${BASE_IMAGE_ABS}" -F "${BASE_FORMAT}" "${OVERLAY_IMAGE}" >/dev/null

WORKDIR="$(dirname "${OVERLAY_IMAGE}")"
SEED_IMAGE="${OVERLAY_IMAGE}.seed.img"
USER_DATA="${OVERLAY_IMAGE}.user-data"
META_DATA="${OVERLAY_IMAGE}.meta-data"
QEMU_LOG="${OVERLAY_IMAGE}.qemu.log"

cat >"${USER_DATA}" <<'EOF'
#cloud-config
runcmd:
  - - bash
    - -eux
    - -c
    - |
      extracted=0
      for dev in /dev/vd[a-z]; do
          case "${dev}" in
              /dev/vda|/dev/vdb)
                  continue
                  ;;
          esac

          [ -b "${dev}" ] || continue
          if tar -tf "${dev}" >/dev/null 2>&1; then
              tar -xf "${dev}" -C /
              extracted=$((extracted + 1))
          fi
      done

      if [ "${extracted}" -eq 0 ]; then
          echo "score ITF filesystem injection failed: no payload device found" > /dev/console
          exit 1
      fi

      systemctl mask apt-daily.timer apt-daily-upgrade.timer unattended-upgrades || true
      systemctl mask snapd.service snapd.socket snapd.seeded.service || true
      systemctl mask motd-news.timer e2scrub.timer e2scrub_all.timer || true
      systemctl mask boot-efi.mount || true
      awk '{if ($2 == "/boot/efi") print "# disabled by score ITF: " $0; else print $0}' /etc/fstab > /etc/fstab.scoreitf && mv /etc/fstab.scoreitf /etc/fstab || true
      touch /etc/cloud/cloud-init.disabled
      sync
      echo "score ITF filesystem injection finished" > /dev/console

power_state:
  mode: poweroff
  timeout: 60
  condition: true
EOF

cat >"${META_DATA}" <<EOF
instance-id: score-itf-filesystem-inject-$$
local-hostname: score-itf-filesystem-inject
EOF

drive_args=()
drive_index=2

if [[ "$#" -eq 0 ]]; then
    echo "Error: no payload tar archives provided" >&2
    exit 1
fi

for src in "$@"; do
    drive_args+=("-device" "virtio-blk-pci,drive=vd${drive_index}")
    drive_args+=("-drive" "if=none,format=raw,file=${src},id=vd${drive_index},readonly=on")
    drive_index=$((drive_index + 1))
done
cloud-localds "${SEED_IMAGE}" "${USER_DATA}" "${META_DATA}"

timeout 900 qemu-system-x86_64 \
    -accel kvm -accel tcg \
    -machine pc \
    -cpu Cascadelake-Server-v5 \
    -smp 2 \
    -m 2048 \
    -no-reboot \
    -device virtio-blk-pci,drive=vd0 -drive if=none,format=qcow2,file="${OVERLAY_IMAGE}",id=vd0 \
    -device virtio-blk-pci,drive=vd1 -drive if=none,format=raw,file="${SEED_IMAGE}",id=vd1,readonly=on \
    "${drive_args[@]}" \
    -netdev user,id=net0 \
    -device virtio-net-pci,netdev=net0 \
    -nographic \
    -serial mon:stdio </dev/null >"${QEMU_LOG}" 2>&1

cleanup() {
    rm -f "${OVERLAY_IMAGE}.pid" "${SEED_IMAGE}" "${USER_DATA}" "${META_DATA}" "${QEMU_LOG}"
}
trap cleanup EXIT

if ! grep -q "score ITF filesystem injection finished" "${QEMU_LOG}"; then
    echo "Error: cloud-init filesystem injection did not complete" >&2
    echo "----- QEMU LOG -----" >&2
    cat "${QEMU_LOG}" >&2
    exit 1
fi
trap - EXIT
cleanup
""",
        mnemonic = "CopyFilesOntoTarget",
        progress_message = "Creating image overlay %s" % out.short_path,
    )

    return [
        DefaultInfo(
            files = depset([out]),
            runfiles = ctx.runfiles(files = [out]),
        ),
    ]

_copy_files_onto_target = rule(
    implementation = _copy_files_onto_target_impl,
    attrs = {
        "image": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
        "srcs": attr.label_list(
            allow_files = [
                ".tar",
                ".tar.gz",
                ".tgz",
                ".tar.bz2",
                ".tbz2",
                ".tar.xz",
                ".txz",
            ],
            default = [],
        ),
        "out": attr.output(
            mandatory = True,
        ),
    },
)

def copy_files_onto_target(name, image, srcs = [], out = None, **kwargs):
    if out == None:
        out = "%s.qcow2" % name

    _copy_files_onto_target(
        name = name,
        image = image,
        srcs = srcs,
        out = out,
        **kwargs
    )
