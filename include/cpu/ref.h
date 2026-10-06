/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef NEMU_CPU_REF_H
#define NEMU_CPU_REF_H

#include <common.h>
#include <difftest.h>

static inline bool ref_fast_supported(void) {
  return ISDEF(CONFIG_SHARE_REF) && ISNDEF(CONFIG_LIGHTQS) &&
         ISNDEF(CONFIG_RV_AME);
}

#if defined(CONFIG_SHARE_REF) && !defined(CONFIG_LIGHTQS) && !defined(CONFIG_RV_AME)
extern int difftest_exec_mode;
#endif

static inline bool ref_is_fast(void) {
#if defined(CONFIG_SHARE_REF) && !defined(CONFIG_LIGHTQS) && !defined(CONFIG_RV_AME)
  return difftest_exec_mode == DIFFTEST_EXEC_FAST;
#else
  return false;
#endif
}

void difftest_flush_state(void);

#endif // NEMU_CPU_REF_H
