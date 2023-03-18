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

#include <common.h>

extern uint64_t g_nr_guest_inst;
FILE *log_fp = NULL;

void init_log(const char *log_file) {
  log_fp = stdout; //log_file的流设为终端，也即将log_fp指针指向stdout文件
  if (log_file != NULL) {
    FILE *fp = fopen(log_file, "w"); //打开文字文件只写，fopen函数返回文件指针
    Assert(fp, "Can not open '%s'", log_file); //两者不相等则打开文件失败
    log_fp = fp;
  }
  Log("Log is written to %s", log_file ? log_file : "stdout"); //log_file不空显示写入log_file文件，否则显示写入stdout
} //init_log()函数其实就是把全局变量log_fp设为log_file

bool log_enable() {
  return MUXDEF(CONFIG_TRACE, (g_nr_guest_inst >= CONFIG_TRACE_START) &&
         (g_nr_guest_inst <= CONFIG_TRACE_END), false);
}
