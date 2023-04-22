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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>


/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10000
#ifdef CONFIG_FTRACE
  int call_num = 0;
  void ftrace(Decode *s);
  extern Elf64_Sym* sym_table;
  extern char* str_table;
  extern int sym_entries;
#endif

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();
int scan_watchpoint(void);
void print_space(int n);

#ifdef CONFIG_ITRACE
Decode Iringbuf[16]; //当前指令的附近环形指令缓存
int Iringbuf_point = 0;

static void exec_Iringbuf(Decode *s){
  Iringbuf[Iringbuf_point] = *s;
  if(Iringbuf_point == 15){
    Iringbuf_point = 0;
  }else{
    Iringbuf_point++;
  }
}

void print_Iringbuf(void){
  printf("Instructions before the wrong one:\n");
  int n = Iringbuf_point;
  while (n!=Iringbuf_point-1)
  {
    printf("   %s\n",Iringbuf[n].logbuf);
    n = (n+1)%16;
  }
  printf("-->%s\n",Iringbuf[n].logbuf);
}
#endif

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); } //在这里记录指令信息
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

#ifdef CONFIG_FTRACE
  ftrace(_this);
#endif

  //watchpoint part
  int wp_NO = scan_watchpoint();
  if(wp_NO != -1){
    printf("The NO.%d watchpoint expr changed, process stops.\n",wp_NO);
    nemu_state.state = NEMU_STOP;
  }
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;  //把PC保存到decode结构s的pc和static next pc中
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf; //填充logbuf
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc); //snprintf返回值表示实际写入的字符数,此处为其填充了PC的信息
  int ilen = s->snpc - s->pc; //ilen = 4
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]); //此处为logbuf填充了指令的具体16进制值
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4); //riscv64 ilen_max = 4
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
  exec_Iringbuf(s); //在附近的环形指令缓存中添加当前指令
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif

#endif
}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++; //用于记录客户指令的计数器
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break; //检查此时nemu的状态是否为NEMU_RUNNING
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      #ifdef CONFIG_ITRACE
      if(nemu_state.state == NEMU_ABORT || nemu_state.halt_ret != 0){
        print_Iringbuf();
      }
      #endif
      // fall through
    case NEMU_QUIT: statistic();
  }
}

#ifdef CONFIG_FTRACE
void ftrace(Decode *s){

  bool JAL = BITS(s->isa.inst.val,6,0) == 0b1101111;
  bool JALR= BITS(s->isa.inst.val,6,0) == 0b1100111;
  bool RET = s->isa.inst.val == 0x00008067;
  bool CALL = false; //表示JALR指令是否是函数调用指令

  if( JAL|JALR ){ //调用函数一般采用JAL指令，opcode对应1101111,有时也会使用JALR; ret指令，JALR指令且rs1为$ra，其余全部为0，

    char *name = (char *)malloc(20*sizeof(char));
    memset(name,'\0',20);

    for(int i=0;i<sym_entries;i++){
      if(ELF64_ST_TYPE(sym_table[i].st_info) == STT_FUNC){

        if(!RET && JALR && ((s->pc >= sym_table[i].st_value) && (s->pc < (sym_table[i].st_value+sym_table[i].st_size)) ) 
        && !((s->dnpc >= sym_table[i].st_value) && (s->dnpc < (sym_table[i].st_value+sym_table[i].st_size) )) ){ //此处是判断JALR是否是函数调用，若是，则pc和dnpc不会在同一个函数范围内
          CALL = true;
        }

        if( (s->dnpc >= sym_table[i].st_value) && (s->dnpc < (sym_table[i].st_value+sym_table[i].st_size) ) ){ //此处是根据dnpc的值找到所处的函数名
          for(int j=0;j<20 && str_table[sym_table[i].st_name+j] != '\0';j++){
            name[j] = str_table[sym_table[i].st_name+j];
          }
          break;
        }

      }
    }

    if(JAL|CALL){
      call_num++;
      printf("0x%lx:",s->pc);
      print_space(call_num);
      printf("call [%s@0x%lx]\n",name,s->dnpc);
    }else if(RET){
      call_num--;
      printf("0x%lx:",s->pc);
      print_space(call_num);
      printf("ret [%s]\n",name);
    }

    free(name);
  }
}
#endif

void print_space(int n){ //此函数用于打印n个空格，只是为了ftrace的格式
  for(int i=0;i<n;i++){
    printf(" ");
  }
}
