# llama.cpp — Xtensa HiFi5s Bare-Metal Port

![llama](https://user-images.githubusercontent.com/1991296/230134379-7181e485-c521-4d23-a0d6-f7b3b61ba524.png)

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

Bare-metal port of [llama.cpp](https://github.com/ggerganov/llama.cpp) (commit: `3d98b4`) enabling LLM inference directly on the **Cadence Xtensa HiFi5s DSP** — without an OS, using the Xtensa ISS for development and cycle-accurate profiling.

----

## Description

This port brings `llama.cpp` to embedded DSP targets. It provides a standalone bare-metal build for the Cadence Xtensa HiFi5s DSP with:

- Plain C/C++ — no OS or standard runtime required.
- Cross-compilation via Xtensa CMake toolchain (`xt-clang`/`xt-clang++`).
- Supports running a modified `simple.cpp` application, adapted to compile on HiFi5s, with inclusion for support of chat template detection and application from GGUF model metadata.
- Cycle-accurate per-token DSP profiling — TTFT, TG avg, TG max, last token.

----

## Toolchain Requirements

**HiFi5s 2 GB System Memory** (required for models up to 1.5 GB):
- Tools : RJ.5

Set the following environment variables before running any build or cmake command:

```bash
export XTENSA_CORE=<your_core_name>
export XTENSA_TOOLCHAIN=/path/to/XtDevTools/install/tools
export TOOLCHAIN_VER=RJ-2025.5-linux
export XTENSA_SYSTEM=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/config
export PATH=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/bin:$PATH
```
----

## Build

### Download Release

```bash
git clone https://github.com/foss-xtensa/llama.cpp-hifi.git
cd llama.cpp-hifi
git checkout llama-cpp-hifi-rel
```

### CMake — Xtensa HiFi5s

```bash
# Always remove stale cache before reconfiguring
rm -rf build_xtensa

cmake -DCMAKE_TOOLCHAIN_FILE=cmake/xtensa-hifi5s-toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBARE_METAL_TEST=ON \
      -DLLAMA_BUILD_TESTS=OFF \
      -B build_xtensa
cmake --build build_xtensa --target llama-simple -j
# Output: build_xtensa/bin/llama-simple
```

### Makefile — Xtensa HiFi5s

```bash
make -f Makefile.xtensa BUILD_DIR=bin_xtensa
# Output: bin_xtensa/xa_simple_llama_test
```

----

## Models

Models must be in [GGUF](https://github.com/ggerganov/ggml/blob/master/docs/gguf.md) format. Download GGUF models directly from [Hugging Face](https://huggingface.co/models?library=gguf&sort=trending).

**HiFi5s DSP optimization is implemented for Q8_0 only.** GGUF models with other quantization will use unoptimized plain C code.

----

## Run

### Xtensa ISS

| Argument | Description |
|---|---|
| `-m <path>` | GGUF model file (required) |
| `-n <int>` | Number of tokens to generate (default: 32) |
| `<prompt>` | User input prompt |

**Example 1 — SmolLM2-360M-Instruct Q8_0 (370 MB)**

**Model:** [SmolLM2-360M-Instruct-GGUF](https://huggingface.co/HuggingFaceTB/SmolLM2-360M-Instruct-GGUF/tree/main)

```bash
xt-run --memlimit=4096 --mem_model build_xtensa/bin/llama-simple \
  -m <path>/SmolLM2-360M-Instruct-Q8_0.gguf \
  -n 32 "Answer in one sentence: What is the capital of France?"
```

**Example 2 — TinyLlama-1.1B-chat-v0.3 Q8_0 (1.1 GB)**

**Model:** [TinyLlama-1.1B-Chat-v0.3-GGUF](https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v0.3-GGUF/tree/main)

```bash
xt-run --memlimit=4096 --mem_model build_xtensa/bin/llama-simple \
  -m <path>/TinyLlama-1.1B-chat-v0.3-Q8_0.gguf \
  -n 32 "Who is Harrison Ford?"
```

**Example 3 — Qwen2.5-0.5B-Instruct Q8_0 (509 MB)**

**Model:** [Qwen2.5-0.5B-Instruct-GGUF](https://huggingface.co/bartowski/Qwen2.5-0.5B-Instruct-GGUF/tree/main)

```bash
xt-run --memlimit=4096 --mem_model build_xtensa/bin/llama-simple \
  -m <path>/Qwen2.5-0.5B-Instruct-Q8_0.gguf \
  -n 32 "Answer in one sentence: What is water made of?"
```

> **Note:** Output quality depends on the model used.

> For Makefile builds, replace `build_xtensa/bin/llama-simple` with `bin_xtensa/xa_simple_llama_test`.

---

## Profiling Output

The following is an example of the profiling output printed after inference on the DSP:

> **Note:** Actual performance numbers are redacted. The example below illustrates the output format only.

```text
main: decoded N tokens

============================================================
                DETAILED CYCLES REPORT
============================================================
cycles to first output token                    : ********** (aka prefill cycles: includes prompt processing for N prompt tokens)
cycles to decode subsequent N output tokens     : **********
prefill cycles/token                            : **********
avg decode cycles/token                         : **********

============================================================
                PERFORMANCE SUMMARY
============================================================
TTFT @ 1 GHz DSP                                : **********
Decode tokens/sec @ 1 GHz DSP                   : **********

1 threads used
```

| Field | Description |
|---|---|
| `cycles to first output token` | TTFT (Time To First Token) — includes prompt processing (prefill) and generation of the first output token |
| `cycles to decode subsequent N output tokens` | Total cycles consumed to generate all remaining output tokens after the first token |
| `prefill cycles/token` | Average number of cycles per prompt token during the prefill stage |
| `avg decode cycles/token` | Average number of cycles per generated token during token generation (TG), excludes prefill |
| `TTFT @ 1 GHz DSP` | Estimated Time To First Token assuming a DSP frequency of 1 GHz |
| `Decode tokens/sec @ 1 GHz DSP` | Estimated token generation throughput assuming a DSP frequency of 1 GHz |

---

## Chat Template

Chat/instruction models require the user question to be wrapped in a role-based format before tokenization. `simple.cpp` uses `llama_chat_apply_template()` to auto-detect and apply the template from GGUF metadata.

**Example:** `"Who is Harrison Ford?"` → after template:

```
<s> <|im_start|>user
Who is Harrison Ford?<|im_end|>
<|im_start|>assistant

```

| Condition | Behavior |
|---|---|
| Template in GGUF and supported | Applied directly |
| Template in GGUF but not recognised | ChatML format used as fallback |
| No template in GGUF (base model) | Raw prompt used as-is |

----

