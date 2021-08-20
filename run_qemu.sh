#!/bin/bash -eum

#-------------------------------------------------------------------------------
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#
#-------------------------------------------------------------------------------
CURRENT_SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"

SDK_PATH=${CURRENT_SCRIPT_DIR}/../..

DIR_BIN_SDK=${SDK_PATH}/bin

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
    -serial tcp:localhost:4444,server # serial port 0 is used for Proxy connection
    -serial mon:stdio      # serial port 1 is used for console
    -kernel ${SYSTEM_IMAGE}
    -drive file="sdk/demos/demo_exercise/yoursd.img",format=raw,id=mycard
    -device sd-card,drive=mycard
)

# run QEMU showing command line
qemu-system-arm ${QEMU_PARAMS[@]} 2> qemu_stderr.txt &
sleep 1

# start proxy app
${DIR_BIN_SDK}/proxy_app -c TCP:4444 -t 1  > proxy_app.out &
sleep 1

fg