/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef NEMU_STORE_RECORD_H
#define NEMU_STORE_RECORD_H

#include <common.h>

typedef struct {
  uint64_t addr;
  uint64_t data;
  uint8_t mask;
} store_record_t;

// Normalization only: records live on the caller stack, not in a retained log.
// A scalar write touches at most two aligned 8-byte records. Masked-off data
// bytes are zero, matching the committed-store queue and store-hash protocol.
static inline unsigned store_record_split(uint64_t addr, uint64_t data, int len,
                                          store_record_t records[2]) {
  assert(len > 0 && len <= 8);
  unsigned offset = addr & 7;
  unsigned low_len = MIN_OF((unsigned)len, 8 - offset);
  records[0] = (store_record_t) {
    .addr = addr & ~UINT64_C(7),
    .data = (data & (UINT64_MAX >> ((8 - low_len) * 8))) << (offset * 8),
    .mask = ((1u << low_len) - 1) << offset,
  };
  if (low_len == (unsigned)len) return 1;

  unsigned high_len = len - low_len;
  records[1] = (store_record_t) {
    .addr = records[0].addr + 8,
    .data = (data >> (low_len * 8)) & (UINT64_MAX >> ((8 - high_len) * 8)),
    .mask = (1u << high_len) - 1,
  };
  return 2;
}

#endif // NEMU_STORE_RECORD_H
