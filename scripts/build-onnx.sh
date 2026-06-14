#!/bin/bash
set -eu

GIT_REPOSITORY=https://github.com/onnx/onnx-mlir.git
GIT_COMMIT=b17f83e508c49db0262439ee32f7e3c064f6ee09

# Get the absolute path to this script and set default build and install paths
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
JLM_ROOT_DIR="$(realpath "${SCRIPT_DIR}/..")"
ONNX_BUILD=${JLM_ROOT_DIR}/build-onnx
ONNX_INSTALL=${JLM_ROOT_DIR}/usr

LLVM_VERSION=19
LLVM_CONFIG_BIN=llvm-config-${LLVM_VERSION}

function commit()
{
	echo ${GIT_COMMIT}
}

function usage()
{
	echo "Usage: ./build-onnx.sh [OPTION] [VAR=VALUE]"
	echo ""
	echo "  --llvm-config PATH    The llvm-config script used to determine up llvm"
	echo "                        build dependencies. [${LLVM_CONFIG_BIN}]"
	echo "  --build-path PATH     The path where to build ONNX."
	echo "                        [${ONNX_BUILD}]"
	echo "  --install-path PATH   The path where to install ONNX."
	echo "                        [${ONNX_INSTALL}]"
	echo "  --get-commit-hash     Prints the commit hash used for the build."
	echo "  --help                Prints this message and stops."
}

while [[ "$#" -ge 1 ]] ; do
	case "$1" in
		--llvm-config)
			shift
			LLVM_CONFIG_BIN="$1"
			shift
			;;
		--build-path)
			shift
			ONNX_BUILD=$(readlink -m "$1")
			shift
			;;
		--install-path)
			shift
			ONNX_INSTALL=$(readlink -m "$1")
			shift
			;;
		--get-commit-hash)
			commit >&1
			exit 0
			;;
		--help|*)
			usage >&2
			exit 1
			;;
	esac
done

LLVM_BINDIR=$(${LLVM_CONFIG_BIN} --bindir)
LLVM_CMAKEDIR=$(${LLVM_CONFIG_BIN} --cmakedir)

ONNX_GIT_DIR=${ONNX_BUILD}/onnx-mlir.git
ONNX_BUILD_DIR=${ONNX_BUILD}/build

if [ ! -d "$ONNX_GIT_DIR" ] ;
then
	git clone --recursive ${GIT_REPOSITORY} ${ONNX_GIT_DIR}
fi

#git -C ${ONNX_GIT_DIR} checkout ${GIT_COMMIT}
cmake -G Ninja \
	${ONNX_GIT_DIR} \
	-B ${ONNX_BUILD_DIR} \
	-DLLVM_DIR=${LLVM_CMAKEDIR} \
	-DMLIR_DIR=${LLVM_CMAKEDIR}/../mlir \
	-DCMAKE_PREFIX_PATH=${LLVM_CMAKEDIR}/../mlir \
	-DCMAKE_INSTALL_PREFIX=${ONNX_INSTALL} \
	-DCMAKE_CXX_COMPILER=/usr/bin/c++ \
	-DCMAKE_BUILD_TYPE=Release \
	-DLLVM_ENABLE_ASSERTIONS=ON
cmake --build ${ONNX_BUILD_DIR}
