#ifndef NEMU_CMO_WRITE_WATCH_H
#define NEMU_CMO_WRITE_WATCH_H

#include <common.h>

#if defined(CONFIG_SHARE_REF) && defined(CONFIG_ISA_riscv64) && !defined(CONFIG_RV_AME) && \
    !defined(CONFIG_LIGHTQS) && !defined(CONFIG_PERF_OPT_SHARE) && !defined(CONFIG_FAST_REF)
#define CMO_WRITE_WATCH
#endif

#ifdef CMO_WRITE_WATCH
#ifdef __cplusplus
extern "C" {
#endif

extern bool cmo_write_watch_active __attribute__((visibility("hidden")));
void cmo_write_watch_record(paddr_t addr, int len) __attribute__((visibility("hidden")));
void cmo_write_watch_reset(void) __attribute__((visibility("hidden")));

/* Begin tracking an aligned 64-byte line. End returns its byte mask and removes it. */
void difftest_cmo_inval_begin(paddr_t line);
uint64_t difftest_cmo_inval_end(paddr_t line);

#ifdef __cplusplus
}
#endif
#endif

#endif
