# Xtensa HiFi5s DSP Toolchain File
# Usage:
# export XTENSA_CORE=<your_core_name>
# export XTENSA_TOOLCHAIN=/path/to/XtDevTools/install/tools
# export TOOLCHAIN_VER=RJ-2025.5-linux
# export XTENSA_SYSTEM=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/config
# export PATH=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/bin:$PATH
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/xtensa-hifi5s-toolchain.cmake \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DBARE_METAL_TEST=ON \
#         -DLLAMA_BUILD_TESTS=OFF \
#         -B build_xtensa
#   cmake --build build_xtensa --target llama-simple

set(CMAKE_SYSTEM_NAME Generic)        # bare-metal — no OS
set(CMAKE_SYSTEM_PROCESSOR xtensa)

# Xtensa compiler — must be on PATH (set XTENSA_TOOLS and export PATH first)
set(CMAKE_C_COMPILER   xt-clang)
set(CMAKE_CXX_COMPILER xt-clang++)
set(CMAKE_AR           xt-ar)

# Skip compiler test (bare-metal cross-compiler can't run test binaries on host)
set(CMAKE_C_COMPILER_WORKS   1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# C11 / C++17 — cmake adds -std=c11 / -std=c++17 (not gnu* variants) via these vars
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)   # -std=c11, not -std=gnu11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # -std=c++17, not -std=gnu++17

# Bare-metal has no pthreads/OS — satisfy find_package(Threads REQUIRED) without failing
set(CMAKE_THREAD_LIBS_INIT   "")
set(CMAKE_USE_PTHREADS_INIT  0)
set(Threads_FOUND            TRUE)
set(CMAKE_THREAD_PREFER_PTHREAD FALSE)

# Read Xtensa core settings from environment (must be set before running cmake)
set(XTENSA_SYSTEM $ENV{XTENSA_SYSTEM})
set(XTENSA_CORE   $ENV{XTENSA_CORE})

if(NOT XTENSA_SYSTEM OR NOT XTENSA_CORE)
    message(FATAL_ERROR
        "XTENSA_SYSTEM and XTENSA_CORE must be set before running cmake.\n"
        "  export XTENSA_SYSTEM=\$XTENSA_CONFIGS/\$XTENSA_CORE/config\n"
        "  export XTENSA_CORE=hifi5s_ao_7_2GSram_L2_Def")
endif()

# Pass --xtensa-system and --xtensa-core to every compile and link invocation
set(XT_FLAGS "--xtensa-system=${XTENSA_SYSTEM} --xtensa-core=${XTENSA_CORE}")

# Bare-metal defines — match Makefile CFLAGS/CXXFLAGS exactly
# NOTE: -DHIFI5_OPT is NOT here — Makefile only adds it for simple.cpp (SIMPLE_CXXFLAGS)
#       It is set in examples/simple/CMakeLists.txt for that target only
set(BARE_METAL_FLAGS "-DBARE_METAL_TEST -DM_PI=3.14159265358979323846 -DGGML_USE_CPU -DNDEBUG -DOPT_ISSUE_CHANGES")

# Xtensa hardware/ISA flags.
# NOTE: -g, -O3, -DNDEBUG intentionally omitted — cmake adds them via CMAKE_BUILD_TYPE
#       (Release → -O3 -DNDEBUG, RelWithDebInfo → -O2 -g -DNDEBUG).
#       Including them here would duplicate them and bloat ggml.c to 176K+ asm lines,
#       causing NFS stale-file-handle failures during the long assembler write.
# NOTE: -std=c11/-std=c++17 intentionally omitted — cmake adds them via
#       CMAKE_C_STANDARD/CMAKE_CXX_STANDARD (set above). Putting them here AND
#       letting cmake also add -std=gnu11 created duplicate conflicting -std= flags.
set(XT_CFLAGS "-LNO:simd -mcoproc -mtext-section-literals -mlongcalls -ffunction-sections -stdlib=libc++")

set(CMAKE_C_FLAGS_INIT   "${BARE_METAL_FLAGS} ${XT_CFLAGS} ${XT_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${BARE_METAL_FLAGS} ${XT_CFLAGS} ${XT_FLAGS}")

# Linker — bare-metal LSP (sim memory layout), matches Makefile LDFLAGS
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mlsp=sim -std=c++17 -stdlib=libc++ ${XT_FLAGS}")

# Shared libraries not possible in bare-metal — force static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static for Xtensa bare-metal" FORCE)

# Disable extra warning flags (they differ from Makefile and add -Werror= variants
# that can fail for ISA intrinsic code; Makefile uses plain -Werror instead)
set(GGML_ALL_WARNINGS  OFF CACHE BOOL "Disabled for Xtensa bare-metal" FORCE)
set(LLAMA_ALL_WARNINGS OFF CACHE BOOL "Disabled for Xtensa bare-metal" FORCE)

# Disable AArch64 runtime weight conversion (adds GGML_USE_CPU_AARCH64 define;
# not applicable to Xtensa and the corresponding .cpp was removed from sources)
set(GGML_CPU_AARCH64 OFF CACHE BOOL "Disabled for Xtensa bare-metal" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
