#include "ggml-threading.h"
#ifndef BARE_METAL_TEST
#include <mutex>

std::mutex ggml_critical_section_mutex;

void ggml_critical_section_start() {
    ggml_critical_section_mutex.lock();
}

void ggml_critical_section_end(void) {
    ggml_critical_section_mutex.unlock();
}
#else
void ggml_critical_section_start() {
}

void ggml_critical_section_end(void) {
}
#endif