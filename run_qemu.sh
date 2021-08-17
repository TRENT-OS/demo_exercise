#!/bin/bash -eu

#-------------------------------------------------------------------------------
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#
#-------------------------------------------------------------------------------

if [[ -z "${1:-}" ]]; then
    echo "ERROR: missing system image parameter"
    exit 1
fi
SYSTEM_IMAGE=${1}
shift
if [[ ! -e "${SYSTEM_IMAGE}" ]]; then
    echo "system image not found: ${SYSTEM_IMAGE}"
    exit 1
fi

BUILD_PLATFORM=${BUILD_PLATFORM:-"imx6"}

declare -A QEMU_MACHINE_MAPPING=(
    [zynq7000]=xilinx-zynq-a9
    [imx6]=sabrelite
)

QEMU_MACHINE=${QEMU_MACHINE_MAPPING[${BUILD_PLATFORM}]:-}
if [ -z "${QEMU_MACHINE}" ]; then
    echo "ERROR: missing QEMU machine mapping for ${BUILD_PLATFORM}"
    exit 1
fi

QEMU_PARAMS=(
    -machine ${QEMU_MACHINE}
    -m size=512M
    -nographic
    -serial /dev/null
    -serial mon:stdio      # serial port 1 is used for console
    -kernel ${SYSTEM_IMAGE}
    -drive file="sdk/demos/demo_exercise/yoursd.img",format=raw,id=mycard
    -device sd-card,drive=mycard
)

# run QEMU showing command line
set -x
qemu-system-arm ${QEMU_PARAMS[@]}