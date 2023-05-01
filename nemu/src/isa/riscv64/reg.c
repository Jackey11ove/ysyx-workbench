/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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

#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  printf("\n");
  for(int idx = 0; idx < 32; idx++){
    const char * name = regs[check_reg_idx(idx)];
    printf("%s  0x%16lx   ",name,gpr(idx));
    if (idx % 4 == 3)
    {
      printf("\n");
    }
  }
}

void isa_ref_r_display(CPU_state ref_r)
{
  printf("\nref_regs:\n");
  for (int idx = 0; idx < 32; idx++)
  {
    const char *name = regs[idx];
    printf("%s  0x%16lx   ", name, ref_r.gpr[idx]);
    if (idx % 4 == 3)
    {
      printf("\n");
    }
  }
  printf("ref_pc = 0x%lx\n",ref_r.pc);
}

word_t isa_reg_str2val(const char *s, bool *success) {
  for (int idx = 0; idx < 32; idx++)
  {
    if( !strcmp(s,regs[idx]) ){
      *success = true;
      return (word_t)gpr(idx);
    }
  }
  *success = false;
  return 0;
}
