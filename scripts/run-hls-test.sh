#!/bin/bash
set -eu

# URL to the benchmark git repository and the commit to be used
GIT_REPOSITORY=https://github.com/phate/hls-test-suite.git
GIT_COMMIT=ffd3191dc3046cc54618fc4a43cc656eda755a69

# Get the absolute path to this script and set default JLM paths
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
JLM_ROOT_DIR=${SCRIPT_DIR}/..
JLM_BIN_DIR=${JLM_ROOT_DIR}/build

# Set default path for where the benchmark will be cloned and make target for running it
BENCHMARK_DIR=${JLM_ROOT_DIR}/usr/hls-test-suite
BENCHMARK_RUN_TARGET=run

# We assume that the firtool is in the PATH
FIRTOOL=firtool

function commit()
{
	echo ${GIT_COMMIT}
}

function usage()
{
	echo "Usage: ./run-hls-test.sh [OPTION] [VAR=VALUE]"
	echo ""
	echo "  --benchmark-path PATH The path where to place the HLS test suite."
	echo "                        [${BENCHMARK_DIR}]"
	echo "  --firtool COMMAND     The command for running firtool, which can include a path."
	echo "                        [${FIRTOOL}]"
	echo "  --get-commit-hash     Prints the commit hash used for the build."
	echo "  --help                Prints this message and stops."
}

while [[ "$#" -ge 1 ]] ; do
	case "$1" in
		--benchmark-path)
			shift
			BENCHMARK_DIR=$(readlink -m "$1")
			shift
			;;
		--firtool)
			shift
			FIRTOOL=$(readlink -m "$1")
			shift
			;;
		--get-commit-hash)
			commit >&2
			exit 1
			;;
		--help)
			usage >&2
			exit 1
			;;
	esac
done

# Check if firtool exists
if ! command -v ${FIRTOOL} &> /dev/null
then
	echo "${FIRTOOL} is not found."
	echo "Make sure to use '--firtool COMMAND' to specify which firtool to use."
	echo "You can use './scripts/build-circt.sh' to build it, if needed."
	exit 1
fi

# Check if verilator exists
if ! command -v verilator &> /dev/null
then
	echo "No verilator in ${PATH}" 
	echo "Consider installing the verilator package for your Linux distro."
	exit 1
fi

if [ ! -d "$BENCHMARK_DIR" ] ;
then
	git clone ${GIT_REPOSITORY} ${BENCHMARK_DIR}
fi

export PATH=${JLM_BIN_DIR}:${PATH}
cd ${BENCHMARK_DIR}
git checkout ${GIT_COMMIT}
make clean
make FIRTOOL=${FIRTOOL} ${BENCHMARK_RUN_TARGET}
