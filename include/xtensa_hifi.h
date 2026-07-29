#ifndef _XTENSA_HIFI_H_
#define _XTENSA_HIFI_H_

// xtensa_hifi.h — Xtensa HiFi DSP convenience header for llama.cpp bare-metal port
//
// Include this instead of sprinkling individual Xtensa headers across source files.
// All definitions are gated on HIFI5_OPT so the file is safe to include unconditionally.

#undef HIFI5S_OPT

#if XCHAL_HAVE_HIFI5S
#define HIFI5S_OPT
#endif

#endif // _XTENSA_HIFI_H_
