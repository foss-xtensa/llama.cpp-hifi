# llama.cpp — Xtensa HiFi5s Bare-Metal Port

![llama](https://user-images.githubusercontent.com/1991296/230134379-7181e485-c521-4d23-a0d6-f7b3b61ba524.png)

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

Bare-metal port of [llama.cpp](https://github.com/ggerganov/llama.cpp) (commit: `3d98b4`) enabling LLM inference directly on the **Cadence Xtensa HiFi5s DSP** — without an OS, using the Xtensa ISS for development and cycle-accurate profiling.

----

## Description

This port brings `llama.cpp` to embedded DSP targets. It provides a standalone bare-metal build for the Cadence Xtensa HiFi5s DSP with:

- Plain C/C++ — no OS or standard runtime required
- Cross-compilation via Xtensa CMake toolchain (`xt-clang`/`xt-clang++`)
- Automatic chat template detection and application from GGUF model metadata
- Cycle-accurate per-token DSP profiling — TTFT, TG avg, TG max, last token

----

## Toolchain Requirements

**HiFi5s 2 GB SRAM config** (required for models up to 1.5 GB):
- Tools : RJ-2025.5-linux or later
- Config : `hifi5s_ao_7_2GSram_L2_Def`

Set the following environment variables before running any build or cmake command:

```bash
export XTENSA_CORE=<your_core_name>
export XTENSA_TOOLCHAIN=/path/to/XtDevTools/install/tools
export TOOLCHAIN_VER=RJ-2025.5-linux
export XTENSA_SYSTEM=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/config
export PATH=$XTENSA_TOOLCHAIN/$TOOLCHAIN_VER/XtensaTools/bin:$PATH
```

Example:

```bash
export PATH=/path/to/XtDevTools/install/tools/RJ-2025.5-linux/XtensaTools/bin:$PATH
export XTENSA_SYSTEM=/path/to/RJ-2025.5-linux/hifi5s_ao_7_2GSram_L2_Def/config
export XTENSA_CORE=hifi5s_ao_7_2GSram_L2_Def
```

**HiFi5s L2 1MB config:**

```bash
export XTENSA_CORE=hifi5s_ao_7_2GSram_L2_1M
```

----

## Build

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

**HiFi5s DSP optimization is implemented for Q8_0 only.** Other quantizations fall back to the reference path.

----

## Run

### Xtensa ISS

| Argument | Description |
|---|---|
| `-m <path>` | GGUF model file (required) |
| `-n <int>` | Number of tokens to generate (default: 32) |
| `<prompt>` | User input prompt |

**Example 1 — SmolLM2-360M-Instruct Q8_0 (370 MB)**

```bash
xt-run --memlimit=4096 --mem_model build_xtensa/bin/llama-simple \
  -m <path>/SmolLM2-360M-Instruct-Q8_0.gguf \
  -n 32 "Answer in one sentence: What is the capital of France?"
```

**Example 2 — TinyLlama-1.1B-chat-v0.3 Q8_0 (1.1 GB)**

```bash
xt-run --memlimit=4096 --mem_model build_xtensa/bin/llama-simple \
  -m <path>/TinyLlama-1.1B-chat-v0.3-Q8_0.gguf \
  -n 32 "Who is Harrison Ford?"
```

**Example 3 — Qwen2.5-0.5B-Instruct Q8_0 (509 MB)**

```bash
xt-run --memlimit=4096 --mem_model build_xtensa/bin/llama-simple \
  -m <path>/Qwen2.5-0.5B-Instruct-Q8_0.gguf \
  -n 32 "Answer in one sentence: What is water made of?"
```

**Example 4 — old-biggie-smollm-twitter Q8_0 (186 MB)**

```bash
xt-run --memlimit=4096 --mem_model build_xtensa/bin/llama-simple \
  -m <path>/old-biggie-smollm-twitter-Q8_0.gguf \
  -n 32 "The future of AI in healthcare is"
```

> **Note:** Output quality depends on the model's training data and quantization.

> For Makefile builds, replace `build_xtensa/bin/llama-simple` with `bin_xtensa/xa_simple_llama_test`.

----

## Profiling Output

The following is an example of the profiling output printed after inference on the DSP:

> **Note:** Numbers shown below are for illustration purposes only — actual values depend on the model and hardware configuration.

```
main: decoded 16 tokens
2199212309 cycles to first output token (includes prompt processing for 33 prompt tokens)
3303244731 cycles to decode subsequent 15 output tokens
220216315 avg decode cycles per token
220364288 max decode cycles at token 15
220364288 cycles to decode last token

1 threads used
```

| Field | Description |
|---|---|
| `cycles to first output token` | TTFT — prompt processing (PP) + first generated token |
| `avg decode cycles per token` | Average TG cycles per token, excludes PP |
| `max decode cycles at token N` | Maximum cycles for a single TG token out of all tokens generated |

----

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

