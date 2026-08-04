// =============================================================================
// simple.cpp — Minimal llama.cpp inference example with full auto-detection
//
// Supports chat (instruct) models and base (text completion) models.
// NO template flags needed — model type is fully auto-detected from GGUF.
//
// Usage:
//   ./simple_llama_test -n 64  -m model.gguf "Your question"   (chat or base)
//
// Template auto-detection (fully automatic, no flags needed):
//   1. Checks GGUF for tokenizer.chat_template key
//      - Missing → base/pretrain model → raw text completion (no wrapping)
//      - Present → instruct/chat model → continue to step 2
//   2. Checks general.name for "danube"
//      - Contains "danube" → h2o-danube3 → apply <|prompt|>...<|answer|>
//      - Otherwise → library applies the correct template from GGUF
//
// =============================================================================

#include "llama.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "xtensa_hifi.h"

#if defined(HIFI5S_OPT)
#include <xtensa/tie/xt_hifi2.h>
#include <sys/times.h>
#include <xtensa/hal.h>
#include <xtensa/sim.h>
#endif
#define TOKENS_DECODE 16

static void print_usage(int, char ** argv) {
    printf("\nexample usage:\n");
    printf("\n    %s -m model.gguf [-n n_predict] [-ngl n_gpu_layers] [prompt]\n", argv[0]);
	printf("\nExamples:\n");
    printf("  # Chat model (SmolLM2-Instruct, Qwen2.5, TinyLlama, granite, etc.)\n");
    printf("  %s -n 64 -m model.gguf \"What is the capital of France?\"\n", argv[0]);
    printf("\n  # Base model (SmolLM2-360M, pythia, gpt2) — same command, auto-detected\n");
    printf("  %s -n 120 -m model.gguf \"The history of the internet began\"\n", argv[0]);
    printf("\n");
}

int main(int argc, char ** argv) {
#ifdef BARE_METAL_TEST
    printf("**** simple main start ****\n"); fflush(stdout);
#endif
    // path to the model gguf file
    std::string model_path;
    // prompt to generate text from
    //std::string prompt = "Hello my name is";
    std::string prompt;
    bool prompt_set = false;
    // number of layers to offload to the GPU

#if defined(HIFI5S_OPT)
struct  tms *full_timestart, *full_timestop;
#endif

#ifndef BARE_METAL_TEST
    int ngl = 99;
    // number of tokens to predict
    int n_predict = 32;
#else
    int ngl = 1;
    int n_predict = TOKENS_DECODE;
#endif

    // parse command line arguments

    {
        int i = 1;
        for (; i < argc; i++) {
            if (strcmp(argv[i], "-m") == 0) {
                if (i + 1 < argc) {
                    model_path = argv[++i];
                } else {
                    print_usage(argc, argv);
                    return 1;
                }
            } else if (strcmp(argv[i], "-n") == 0) {
                if (i + 1 < argc) {
                    try {
                        n_predict = std::stoi(argv[++i]);
                    } catch (...) {
                        print_usage(argc, argv);
                        return 1;
                    }
                } else {
                    print_usage(argc, argv);
                    return 1;
                }
            } else if (strcmp(argv[i], "-ngl") == 0) {
                if (i + 1 < argc) {
                    try {
                        ngl = std::stoi(argv[++i]);
                    } catch (...) {
                        print_usage(argc, argv);
                        return 1;
                    }
                } else {
                    print_usage(argc, argv);
                    return 1;
                }
            } else {
                // prompt starts here
                break;
            }
        }
        if (model_path.empty()) {
            print_usage(argc, argv);
            return 1;
        }
        if (i < argc) {
            prompt = argv[i++];
            for (; i < argc; i++) {
                prompt += " ";
                prompt += argv[i];
            }
			prompt_set = true;
        }
    }

#if defined(HIFI5S_OPT)
    full_timestart = (struct tms *)malloc(sizeof(struct tms) * n_predict);
    full_timestop  = (struct tms *)malloc(sizeof(struct tms) * n_predict);
#endif

    // load dynamic backends

    ggml_backend_load_all();

    // initialize the model

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = ngl;

    llama_model * model = llama_load_model_from_file(model_path.c_str(), model_params);

    if (model == NULL) {
        fprintf(stderr , "%s: error: unable to load model\n" , __func__);
        return 1;
    }

    // ── Apply chat template (fully automatic — no flags needed) ──────────────
    // Step 1: check if GGUF has tokenizer.chat_template
    //   - Missing → base/pretrain model → raw text completion (no wrapping)
    //   - Present → chat/instruct model → step 2
    // Step 2: library applies the template from GGUF
    //   - Success (>=0) → use formatted prompt
    //   - Failure  (<0) → GGUF template not in library's list → chatml fallback
    {
        char tmpl_check[8];
        int has_tmpl = llama_model_meta_val_str(model, "tokenizer.chat_template",
                                                tmpl_check, sizeof(tmpl_check));

		if(!prompt_set)
		{
			if (has_tmpl < 0) {
				prompt = "Once upon a time";                   // base model default
			} else {
				prompt = "Who painted the Mona Lisa?";         // chat model default
			}
			fprintf(stderr, "info: no prompt — using default for %s model: \"%s\"\n",
					has_tmpl < 0 ? "base" : "chat", prompt.c_str());
			prompt_set = true;
		}
        const std::string user_content = prompt;
		{
			if (has_tmpl < 0) {
				// No chat template in GGUF → base/pretrain model → raw text completion
				fprintf(stderr, "info: no chat template in GGUF — base model, raw completion\n");
				prompt = user_content;
			} else {
				// Chat model — let the library apply the template from GGUF
				llama_chat_message chat_msgs[1];
				chat_msgs[0] = {"user", user_content.c_str()};
				int buf_size = llama_chat_apply_template(model, NULL, chat_msgs, 1, true, NULL, 0);
				if (buf_size < 0) {
					// Library can't parse this model's template → chatml fallback
					fprintf(stderr, "info: GGUF template unsupported by library — using chatml fallback\n");
					prompt = "<|im_start|>user\n" + user_content + "<|im_end|>\n<|im_start|>assistant\n";
				} else {
					std::vector<char> buf(buf_size + 1);
					llama_chat_apply_template(model, NULL, chat_msgs, 1, true, buf.data(), buf.size());
					prompt = std::string(buf.data(), buf_size);
				}
			}
		}
    }

    // tokenize the prompt

    // find the number of tokens in the prompt
    const int n_prompt = -llama_tokenize(model, prompt.c_str(), prompt.size(), NULL, 0, true, true);

    // allocate space for the tokens and tokenize the prompt
    std::vector<llama_token> prompt_tokens(n_prompt);
    if (llama_tokenize(model, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true) < 0) {
        fprintf(stderr, "%s: error: failed to tokenize the prompt\n", __func__);
        return 1;
    }

    // initialize the context

    llama_context_params ctx_params = llama_context_default_params();
    // n_ctx is the context size
    ctx_params.n_ctx = n_prompt + n_predict - 1;
    // n_batch is the maximum number of tokens that can be processed in a single call to llama_decode
    ctx_params.n_batch = n_prompt;
    // enable performance counters
    ctx_params.no_perf = false;

    llama_context * ctx = llama_new_context_with_model(model, ctx_params);

    if (ctx == NULL) {
        fprintf(stderr , "%s: error: failed to create the llama_context\n" , __func__);
        return 1;
    }

    // initialize the sampler

    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = false;
    llama_sampler * smpl = llama_sampler_chain_init(sparams);

    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

    // print the prompt token-by-token

    for (auto id : prompt_tokens) {
        char buf[128];
        int n = llama_token_to_piece(model, id, buf, sizeof(buf), 0, true);
        if (n < 0) {
            fprintf(stderr, "%s: error: failed to convert token to piece\n", __func__);
            return 1;
        }
        std::string s(buf, n);
        printf("%s", s.c_str());
    }

    // prepare a batch for the prompt

    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

    // main loop

    const auto t_main_start = ggml_time_us();
    int n_decode = 0;
    llama_token new_token_id;

#if defined(HIFI5S_OPT)
    xt_iss_client_command("all", "enable");
#endif
    for (int n_pos = 0; n_pos + batch.n_tokens < n_prompt + n_predict; ) {
#if defined(HIFI5S_OPT)
        times(&full_timestart[n_decode]);
#endif
        // evaluate the current batch with the transformer model
        if (llama_decode(ctx, batch)) {
            fprintf(stderr, "%s : failed to eval, return code %d\n", __func__, 1);
            return 1;
        }
#if defined(HIFI5S_OPT)
        times(&full_timestop[n_decode]);
#endif

        n_pos += batch.n_tokens;

        // sample the next token
        {
            new_token_id = llama_sampler_sample(smpl, ctx, -1);

            // is it an end of generation?
            if (llama_token_is_eog(model, new_token_id)) {
                break;
            }

            char buf[128];
            int n = llama_token_to_piece(model, new_token_id, buf, sizeof(buf), 0, true);
            if (n < 0) {
                fprintf(stderr, "%s: error: failed to convert token to piece\n", __func__);
                return 1;
            }
            std::string s(buf, n);
            printf("%s", s.c_str());
            fflush(stdout);

            // prepare the next batch with the sampled token
            batch = llama_batch_get_one(&new_token_id, 1);

            n_decode += 1;
        }
    }

#if defined(HIFI5S_OPT)
    xt_iss_client_command("all", "disable");
#endif

    const auto t_main_end = ggml_time_us();
    
    fflush(stdout);
    printf("\n**** decode is completed ****\n");
    fflush(stdout);

#if !defined (HIFI5S_OPT)
    fprintf(stderr, "\n%s: decoded %d tokens in %.2f s, speed: %.2f t/s\n",
            __func__, n_decode, (t_main_end - t_main_start) / 1000000.0f, n_decode / ((t_main_end - t_main_start) / 1000000.0f));

    fprintf(stderr, "\n");
    llama_perf_sampler_print(smpl);
    llama_perf_context_print(ctx);
    fprintf(stderr, "\n");
#else
    unsigned long long avg_cycles = 0, max_cycles = 0, total_cycles = 0;
    unsigned int max_frame = 0;
    unsigned long full_cycles;   // unsigned long avoids signed 32-bit clock_t overflow
    unsigned int cnt;

    for (cnt = 1; cnt < n_decode; cnt++)
    {
        full_cycles = (unsigned long)full_timestop[cnt].tms_utime -
                      (unsigned long)full_timestart[cnt].tms_utime;

        if (max_cycles < full_cycles)
        {
            max_cycles = full_cycles;
            max_frame = cnt;
        }

        total_cycles += (unsigned long long)full_cycles;
    }

    // Guard against divide-by-zero when only 1 or fewer tokens were decoded
    avg_cycles = (n_decode > 1) ? total_cycles / (n_decode - 1) : 0;

    fprintf(stdout, "\n%s: decoded %d tokens\n", __func__, n_decode);
    printf("\n");

    long long int prefill_cycles =
        (long long int)((unsigned long)full_timestop[0].tms_utime -
                        (unsigned long)full_timestart[0].tms_utime);

    long long int prefill_cycles_per_token =
        (n_prompt > 0) ? (prefill_cycles / n_prompt) : 0;

    float ttft_sec =
        (float)prefill_cycles / (float)(1e9);

    float decode_tokens_per_sec =
        (avg_cycles > 0) ? ((float)(1e9) / (float)avg_cycles) : 0.0f;

    const int label_width = 48;

    printf("============================================================\n");
    printf("                DETAILED CYCLES REPORT\n");
    printf("============================================================\n");

    fprintf(stdout,
            "%-*s : %14lld (aka prefill cycles: includes prompt processing for %d prompt tokens)\n",
            label_width,
            "cycles to first output token",
            prefill_cycles,
            n_prompt);

    {
        char label[128];
        snprintf(label, sizeof(label),
                 "cycles to decode subsequent %d output tokens",
                 n_decode - 1);

        fprintf(stdout,
                "%-*s : %14llu\n",
                label_width,
                label,
                total_cycles);
    }

    fprintf(stdout,
            "%-*s : %14lld\n",
            label_width,
            "prefill cycles/token",
            prefill_cycles_per_token);

    fprintf(stdout,
            "%-*s : %14llu\n",
            label_width,
            "avg decode cycles/token",
            avg_cycles);

    printf("\n");
    printf("============================================================\n");
    printf("                PERFORMANCE SUMMARY\n");
    printf("============================================================\n");

    fprintf(stdout,
            "%-*s : %10.4f sec\n",
            label_width,
            "TTFT @ 1 GHz DSP",
            ttft_sec);

    fprintf(stdout,
            "%-*s : %10.2f\n",
            label_width,
            "Decode tokens/sec @ 1 GHz DSP",
            decode_tokens_per_sec);

    fprintf(stdout, "\n%d threads used\n", GGML_DEFAULT_N_THREADS);
#endif

#ifndef BARE_METAL_TEST
    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_free_model(model);
#endif

#if defined(HIFI5S_OPT)
    free(full_timestart);
    free(full_timestop);
#endif

#ifdef BARE_METAL_TEST
    printf("**** simple main end ****\n"); fflush(stdout);
#endif
    return 0;
}
