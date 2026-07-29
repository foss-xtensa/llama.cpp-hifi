#!/bin/bash
# cmake_build_xtensa.sh — Configure and build llama-simple for Xtensa HiFi5s bare-metal
# Run from $llama.cpp-hifi/
# Usage: bash cmake_build_xtensa.sh

# export XTENSA_CORE=<your_core_name>
# export XTENSA_TOOLCHAIN=/path/to/XtDevTools/install/tools
# export TOOLCHAIN_VER=RJ-2025.5-linux
# export XTENSA_SYSTEM=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/config
# export PATH=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/bin:$PATH

# Verify cmake is available
CMAKE_BIN=$(which cmake 2>/dev/null)
if [ -z "${CMAKE_BIN}" ]; then
    echo "ERROR: cmake not found in PATH."
    echo "Add cmake to PATH before running this script."
    exit 1
fi
echo "Using cmake: ${CMAKE_BIN}"
echo "Using XTENSA_CORE: ${XTENSA_CORE}"
echo "Using XTENSA_SYSTEM: ${XTENSA_SYSTEM}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# Clean previous build (re-configure ensures CMAKE_COMMAND uses the cmake found above)
rm -rf build_xtensa

cmake -DCMAKE_TOOLCHAIN_FILE=cmake/xtensa-hifi5s-toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBARE_METAL_TEST=ON \
      -DLLAMA_BUILD_TESTS=OFF \
      -DLLAMA_BUILD_EXAMPLES=ON \
      -B build_xtensa \
      2>&1 | tee _log_build_cmake_setup

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "cmake configure FAILED — see _log_build_cmake_setup"
    exit 1
fi

# Use -j2 (not -j$(nproc)) — NFS filesystems can stale-handle under heavy parallel writes.
# ggml.c alone generates 50K+ asm lines; many parallel large files can saturate NFS.
cmake --build build_xtensa --target llama-simple -j2 \
      2>&1 | tee _log_build_cmake

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "cmake build FAILED — see _log_build_cmake"
    exit 1
fi

echo ""
echo "Build successful: build_xtensa/bin/llama-simple"
echo "Run: xt-run --memlimit=4096 --turbo build_xtensa/bin/llama-simple -n 16 -m <model.gguf> \"Once upon a time\""
