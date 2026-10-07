/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef NEMU_STORE_LOG_HASH_H
#define NEMU_STORE_LOG_HASH_H

#include <utils.h>

// Runtime collection and constant-size boundary digest; record normalization
// is independent in store_record.h. STORE_LOG separately enables rollback.
#ifdef CONFIG_STORE_LOG_HASH
void store_log_hash_reset(void);
void store_log_hash_set_enabled(bool enabled);
bool store_log_hash_enabled(void);
void store_log_hash_update(uint64_t addr, uint64_t data, uint8_t mask);
void store_log_hash(uint64_t *lo, uint64_t *hi, uint64_t *count);
#ifdef CONFIG_STORE_LOG
void store_log_hash_checkpoint(void);
void store_log_hash_restore(void);
#endif
#endif

#endif // NEMU_STORE_LOG_HASH_H
