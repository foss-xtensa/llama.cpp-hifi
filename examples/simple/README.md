# llama.cpp-hifi/example/simple

The purpose of this example is to demonstrate a minimal usage of llama.cpp for generating text using a GGUF model.

The example supports both chat/instruct models and base/text-completion models. Prompt formatting is selected using GGUF model metadata.

### Usage

```bash
xt-run [runtime_options] build_xtensa/bin/llama-simple \
    -m model.gguf \
    [-n n_predict] \
    [prompt]
```

### Notes

- Supports both chat/instruct models and base/text-completion models.
- For detailed usage examples, profiling information, and performance metrics, refer to the repository top-level **[README.md](https://github.com/foss-xtensa/llama.cpp-hifi/blob/llama-cpp-hifi-rel/README.md)**.
