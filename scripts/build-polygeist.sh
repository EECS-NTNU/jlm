#!/bin/bash
set -eu

GIT_REPOSITORY=https://github.com/llvm/Polygeist.git
GIT_COMMIT=77c04bb2a7a2406ca9480bcc9e729b07d2c8d077

# Get the absolute path to this script and set default build and install paths
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
JLM_ROOT_DIR="$(realpath "${SCRIPT_DIR}/..")"
POLYGEIST_BUILD=${JLM_ROOT_DIR}/build-polygeist

LLVM_VERSION=18
LLVM_CONFIG_BIN=llvm-config-${LLVM_VERSION}

function commit()
{
	echo ${GIT_COMMIT}
}

function usage()
{
	echo "Usage: ./build-polygeist.sh [OPTION] [VAR=VALUE]"
	echo ""
	echo "  --llvm-config PATH    The llvm-config script used to determine up llvm"
	echo "                        build dependencies. [${LLVM_CONFIG_BIN}]"
	echo "  --build-path PATH     The path where to build MLIR."
	echo "                        [${MLIR_BUILD}]"
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
			MLIR_BUILD=$(readlink -m "$1")
			shift
			;;
#		--install-path)
#			shift
#			MLIR_INSTALL=$(readlink -m "$1")
#			shift
#			;;
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

POLYGEIST_GIT_DIR=${POLYGEIST_BUILD}/polygeist.git
POLYGEIST_BUILD_DIR=${POLYGEIST_BUILD}/build
LLVM_BUILD_DIR=${POLYGEIST_BUILD_DIR}/llvm-project/build

if [ ! -d "$POLYGEIST_GIT_DIR" ] ;
then
	git clone --recursive ${GIT_REPOSITORY} ${POLYGEIST_GIT_DIR}
fi

git -C ${POLYGEIST_GIT_DIR} checkout ${GIT_COMMIT}

# Compile LLVM, MLIR, and Clang
mkdir -p ${LLVM_BUILD_DIR}
cmake -G Ninja \
	${POLYGEIST_GIT_DIR}/llvm-project/llvm \
	-B ${LLVM_BUILD_DIR} \
	-DCMAKE_C_COMPILER=${LLVM_BINDIR}/clang \
	-DCMAKE_CXX_COMPILER=${LLVM_BINDIR}/clang++ \
	-DLLVM_ENABLE_PROJECTS="mlir;clang" \
	-DLLVM_TARGETS_TO_BUILD="host" \
	-DLLVM_ENABLE_ASSERTIONS=ON \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DLLVM_USE_LINKER=lld
ninja -C ${LLVM_BUILD_DIR}
ninja -C ${LLVM_BUILD_DIR} check-mlir

# Make sure that FileCheck is in the PATH
export PATH=${LLVM_BINDIR}:${PATH}

# Compile Polygeist
cmake -G Ninja \
	${POLYGEIST_GIT_DIR} \
	-B ${POLYGEIST_BUILD_DIR} \
	-DCMAKE_C_COMPILER=${LLVM_BINDIR}/clang \
	-DCMAKE_CXX_COMPILER=${LLVM_BINDIR}/clang++ \
	-DMLIR_DIR=${LLVM_BUILD_DIR}/lib/cmake/mlir \
	-DCLANG_DIR=${LLVM_BUILD_DIR}/lib/cmake/clang \
	-DLLVM_TARGETS_TO_BUILD="host" \
	-DLLVM_ENABLE_ASSERTIONS=ON \
	-DPOLYGEIST_POLYMER_ENABLE_PLUTO=1 \
	-DPOLYGEIST_ENABLE_POLYMER=1 \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DPOLYGEIST_USE_LINKER=lld
ninja -C ${POLYGEIST_BUILD_DIR}
ninja -C ${POLYGEIST_BUILD_DIR} check-polygeist-opt
ninja -C ${POLYGEIST_BUILD_DIR} check-cgeist
