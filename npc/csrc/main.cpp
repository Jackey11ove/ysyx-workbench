#include "verilated_vcd_c.h"
#include <verilated.h> // 必须包含的头文件
#include "Vtop__Dpi.h"
#include <verilated_dpi.h>
#include "Vtop.h" // Verilator编译生成的头文件
// 引入头文件
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <SDL2/SDL.h>

// 宏定义
#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))
//#define CONFIG_ITRACE
//#define CONFIG_MTRACE
//#define CONFIG_WAVE
#define HAS_GUI
//#define CONFIG_DIFFTEST
#define CONFIG_IRINGBUF
#define CONFIG_MSIZE 0x8000000
#define CONFIG_MBASE 0x80000000
#define DEVICE_BASE  0xa0000000
#define RTC_ADDR (DEVICE_BASE + 0x00000048)
#define SERIAL_PORT (DEVICE_BASE + 0x000003f8)
#define VGACTL_ADDR (DEVICE_BASE + 0x00000100)
#define SYNC_ADDR (VGACTL_ADDR + 4)
#define FB_ADDR (DEVICE_BASE + 0x1000000)
#define RESET_VECTOR (uint32_t) CONFIG_MBASE // 客户程序在内存中的初始位置
#define PG_ALIGN __attribute((aligned(4096)))
#define FMT_WORD "0x%016" PRIx64
#define MAX_INST_TO_PRINT 10000
#define SCREEN_W 400
#define SCREEN_H 300

// 类型定义
typedef uint64_t word_t;
typedef uint32_t paddr_t;
typedef uint64_t vaddr_t;

typedef struct
{
  union
  {
    uint32_t val;
  } inst;
} ISADecodeInfo;

typedef struct Decode
{
  vaddr_t pc;
  vaddr_t snpc; // static next pc
  vaddr_t dnpc; // dynamic next pc
  ISADecodeInfo isa;
#ifdef CONFIG_ITRACE
  char logbuf[128];
#endif
} Decode;

typedef struct
{
  word_t gpr[32];
  vaddr_t pc;
  word_t csr[4];
} CPU_state; // 一个PC寄存器，32个通用寄存器

enum
{
  DIFFTEST_TO_DUT,
  DIFFTEST_TO_REF
};

enum
{
  NPC_RUN,
  NPC_DUMP,
  NPC_STOP
};

// 变量定义
using namespace std;
Vtop *top; // 实例化Verilog模块
VerilatedVcdC *tfp;
uint64_t simtime = 0;
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {}; // 内存指针
static char *img_file = NULL;                    // 装载程序镜像的文件指针
static const char *diff_so_file = "/home/jackey/ysyx-workbench/nemu/build/riscv64-nemu-interpreter-so";
static int difftest_port = 1234;
uint64_t *cpu_gpr = NULL;
uint64_t *cpu_csr = NULL;
uint64_t cpu_pc;
uint32_t cpu_inst;
bool ws_valid;
static bool g_print_step = false;
static int NPC_STATE = NPC_RUN;
static void *vmem = NULL; //显存
static uint32_t *vgactl_port_base = NULL;

// 函数定义
uint8_t *guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; } // paddr是物理地址，此函数由物理地址返回虚拟地址
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; } // 此函数由虚拟地址返回物理地址，haddr为指向pmem数组的指针
void init_npc();
void init_mem();
word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, int len, word_t data);
static word_t pmem_read(paddr_t addr, int len);
static void pmem_write(paddr_t addr, int len, word_t data);
static inline bool in_pmem(paddr_t addr);
static inline word_t host_read(void *addr, int len);
static inline void host_write(void *addr, int len, word_t data);
word_t vaddr_ifetch(vaddr_t addr, int len);
void init_monitor(int argc, char *argv[]);
void init_rand();

static long load_img();
static int parse_args(int argc, char *argv[]);
void cpu_exec(uint64_t n);
static void execute(uint64_t n);
static void exec_once(Decode *s, Vtop *top);
void isa_reg_display();
void sdb_mainloop();
void init_disasm(const char *triple);
#ifdef CONFIG_DIFFTEST
void difftest_step(vaddr_t pc);
void init_difftest(const char *ref_so_file, long img_size, int port);
void difftest_skip_ref(void);
#endif
#ifdef HAS_GUI
void init_vga();
void vga_update_screen();
#endif
#ifdef CONFIG_IRINGBUF
static void exec_Iringbuf(Decode *s);
void print_Iringbuf(void);
#endif

int main(int argc, char **argv)
{
  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(true);
  top = new Vtop;
  // VCD波形设置  start
  tfp = new VerilatedVcdC;
  #ifdef CONFIG_WAVE
  top->trace(tfp, 0);
  tfp->open("wave.vcd");
  #endif
  // 初始化npc中相关信号的值
  init_npc();

  init_monitor(argc, argv);

  sdb_mainloop();

  delete top; // 释放Verilog模块实例
  delete tfp;

  return 0;
}
/***************timer.c***************/
static uint64_t boot_time = 0;

static uint64_t get_time_internal() {
  struct timeval now;
  gettimeofday(&now, NULL);
  uint64_t us = now.tv_sec * 1000000 + now.tv_usec; //转换成微秒,返回现在的时间
  return us;
}

uint64_t get_time() {
  if (boot_time == 0) boot_time = get_time_internal(); //获取一开始的初始时间
  uint64_t now = get_time_internal();
  return now - boot_time;
}


/***************npc***************/
void init_npc()
{
  // 初始化时钟和复位信号
  top->clk = 0;
  top->reset = 1;
  top->eval();
  #ifdef CONFIG_WAVE
  tfp->dump(simtime);
  #endif
  simtime++;
  top->clk = 1;
  top->reset = 1;
  top->eval();
  #ifdef CONFIG_WAVE
  tfp->dump(simtime);
  #endif
  simtime++;
}

extern "C" void ebreak()
{
  if (cpu_gpr[10] == 0)
  {
    printf("\33[1;32mHIT GOOD TRAP\n");
  }
  else
  {
    printf("\33[1;31mHIT BAD TRAP\n");
  }
  NPC_STATE = NPC_STOP;
}

extern "C" void set_gpr_ptr(const svOpenArrayHandle r)
{
  cpu_gpr = (uint64_t *)(((VerilatedDpiOpenVar *)r)->datap());
}

extern "C" void set_csr_ptr(const svOpenArrayHandle r)
{
  cpu_csr = (uint64_t *)(((VerilatedDpiOpenVar *)r)->datap());
}

extern "C" void get_cpu_pc(long long pc)
{
  cpu_pc = (uint64_t)pc;
}

extern "C" void get_cpu_inst(int inst)
{
  cpu_inst = (uint32_t)inst;
}

extern "C" void get_cpu_ws_valid(char valid){
  ws_valid = (bool)valid;
}

extern "C" void inst_fetch(long long raddr, long long *rdata)
{
  *rdata = paddr_read(raddr & ~0x7ull, 8);
}

extern "C" void mem_read(long long raddr, long long *rdata)
{
  // 总是读取地址为`raddr & ~0x7ull`的8字节返回给`rdata`
  if(raddr == RTC_ADDR){
    *rdata = get_time();
    #ifdef CONFIG_DIFFTEST
      difftest_skip_ref();
    #endif
  }else if(raddr == VGACTL_ADDR){
    *(uint32_t*)rdata = vgactl_port_base[0];
    #ifdef CONFIG_DIFFTEST
      difftest_skip_ref();
    #endif
  }else{
    *rdata = paddr_read(raddr & ~0x7ull, 8);
    //printf("MEM_R: read addr = 0x%llx, rdata = 0x%llx\n",raddr, *rdata);
  }

}

extern "C" void mem_write(long long waddr, long long wdata, char shift, char DWHB)
{
  // 总是往地址为`waddr & ~0x7ull`的8字节按写掩码`wmask`写入`wdata`
  // `wmask`中每比特表示`wdata`中1个字节的掩码,
  // 如`wmask = 0x3`代表只写入最低2个字节, 内存中的其它字节保持不变
  if(waddr == SERIAL_PORT){
    putchar(wdata & 0xff);
    #ifdef CONFIG_DIFFTEST
      difftest_skip_ref();
    #endif
    return;
  }else if(waddr == SYNC_ADDR){
    vgactl_port_base[1] = wdata;
    #ifdef CONFIG_DIFFTEST
      difftest_skip_ref();
    #endif
    return;
  }else if(waddr >= FB_ADDR){
    uint32_t * p = (uint32_t*)vmem;
    p[(waddr - FB_ADDR)/4] = wdata;
    #ifdef CONFIG_DIFFTEST
      difftest_skip_ref();
    #endif
    return;
  }else{
    int len = (int)DWHB;
    int offset = (int)shift >> 3;
    //printf("MEM_W: waddr = %llx, offset = %d, len = %d, wdata = %016llx\n",waddr,offset,len,wdata);
    paddr_write((waddr & ~0x7ull) + offset, len, wdata);
  }

}

/***************paddr.c***************/
void init_mem()
{
  // 为pmem指向的空间填满随机数
  uint32_t *p = (uint32_t *)pmem;
  int i;
  for (i = 0; i < (int)(CONFIG_MSIZE / sizeof(p[0])); i++)
  {
    p[i] = rand();
  }
}

static void out_of_bound(paddr_t addr) {
  printf("address = 0x%x is out of bound\n",addr);
}

word_t paddr_read(paddr_t addr, int len)
{
#ifdef CONFIG_MTRACE
  printf("MEM fetch: Load address is %x\n", addr);
#endif
  if (in_pmem(addr)){
    return pmem_read(addr, len);
  }
  #ifdef CONFIG_DEVICE
    return mmio_read(addr, len);
  #endif
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data)
{
#ifdef CONFIG_MTRACE
  printf("MEM fetch: Write address is %x\n", addr);
#endif
  if (in_pmem(addr))
  {
    pmem_write(addr, len, data);
    return;
  }
  #ifdef CONFIG_DEVICE
    mmio_write(addr, len, data); return;
  #endif
  out_of_bound(addr);
}

static word_t pmem_read(paddr_t addr, int len)
{ // 读一个uint64_t
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data)
{
  host_write(guest_to_host(addr), len, data);
}

static inline bool in_pmem(paddr_t addr)
{
  return addr - CONFIG_MBASE < CONFIG_MSIZE;
}

static inline word_t host_read(void *addr, int len)
{
  switch (len)
  {
  case 1:
    return *(uint8_t *)addr;
  case 2:
    return *(uint16_t *)addr;
  case 4:
    return *(uint32_t *)addr;
  case 8:
    return *(uint64_t *)addr;
  default:
    assert(0);
  }
}

static inline void host_write(void *addr, int len, word_t data)
{
  switch (len)
  {
  case 1:
    *(uint8_t *)addr = data;
    return;
  case 2:
    *(uint16_t *)addr = data;
    return;
  case 4:
    *(uint32_t *)addr = data;
    return;
  case 8:
    *(uint64_t *)addr = data;
    return;
  default:
    assert(0);
  }
}

/****************vaddr.c***************/
word_t vaddr_ifetch(vaddr_t addr, int len)
{
  return paddr_read(addr, len);
}

/***************monitor.c***************/
void init_monitor(int argc, char *argv[])
{
  parse_args(argc, argv);
  init_rand();
  init_mem();
  long img_size = load_img();

#ifdef HAS_GUI
  init_vga();
#endif

#ifdef CONFIG_DIFFTEST
  init_difftest(diff_so_file, img_size, difftest_port);
#endif

#ifdef CONFIG_ITRACE
  init_disasm("riscv64"
              "-pc-linux-gnu");
#endif
}

void init_rand()
{
  srand(time(0));
}

static long load_img()
{
  if (img_file == NULL)
  {
    printf("img_file is empty!\n");
    assert(0);
    return 0; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  if (fp == NULL)
  {
    printf("Can't open img_file!\n");
    assert(0);
  }

  fseek(fp, 0, SEEK_END); // 将fp移动至文件的末尾（距文件末尾偏移量为0）
  long size = ftell(fp);  // ftell函数是计算fp指针到文件开头的距离，若fp在文件末尾size就代表整个文件的大小

  // Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);                                    // 将fp移动至文件开头
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp); // 把image镜像载入初始PC的位置也即程序的装载
  assert(ret == 1);

  fclose(fp);
  return size;
}

static int parse_args(int argc, char *argv[])
{ // 分析参数
  const struct option table[] = {
      {"batch", no_argument, NULL, 'b'},     // 无参数值，返回'b'
      {"log", required_argument, NULL, 'l'}, // 有参数值，返回'l'
      {"diff", required_argument, NULL, 'd'},
      {"port", required_argument, NULL, 'p'},
      {"help", no_argument, NULL, 'h'},
      {"ftrace", required_argument, NULL, 'f'},
  };
  int o;
  while ((o = getopt_long(argc, argv, "-bhl:d:p:f:", table, NULL)) != -1)
  {
    switch (o)
    {
    // case 'b': sdb_set_batch_mode(); break; //将is_batch_mode设为1
    case 'p':
      sscanf(optarg, "%d", &difftest_port);
      break; // optarg为当前选项的参数值，也即终端中键入的argv
    // case 'l': log_file = optarg; break;
    case 'd':
      diff_so_file = optarg;
      break;
#ifdef CONFIG_FTRACE
    case 'f':
      sym_entries = init_ftrace(optarg);
      break;
#endif
    case 1:
      img_file = optarg;
      return 0;
    default:
      printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
      printf("\t-b,--batch              run with batch mode\n");
      printf("\t-l,--log=FILE           output log to FILE\n");
      printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
      printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
      printf("\n");
      exit(0);
    }
  }
  return 0;
}

/***************cpu_exec.c***************/
static void trace_and_difftest(Decode *_this)
{
  if (g_print_step)
  {
#ifdef CONFIG_ITRACE
    puts(_this->logbuf);
#endif
  }

#ifdef CONFIG_DIFFTEST
  difftest_step(_this->pc);
#endif

  #ifdef CONFIG_FTRACE
    ftrace(_this);
  #endif
}

void cpu_exec(uint64_t n)
{
  top->reset = 0;
  g_print_step = (n < MAX_INST_TO_PRINT);
  execute(n);
}

static void execute(uint64_t n)
{
  Decode s;
  for (; n > 0; )
  {
    top->clk = 0;
    top->eval();
    #ifdef CONFIG_WAVE
    tfp->dump(simtime);
    #endif
    simtime++;
    top->clk = 1;
    top->eval();
    #ifdef CONFIG_WAVE
    tfp->dump(simtime);
    #endif
    simtime++;
    // g_nr_guest_inst ++; //用于记录客户指令的计数器
    if(ws_valid){
      exec_once(&s, top);
      trace_and_difftest(&s);
      n--;
    }
    if(NPC_STATE == NPC_DUMP || NPC_STATE == NPC_STOP){
      return;
    }
    get_time();
    #ifdef HAS_GUI
      vga_update_screen();
    #endif
  }
}

static void exec_once(Decode *s, Vtop *top)
{
  cpu_gpr[32] = cpu_pc;
  cpu_gpr[33] = cpu_csr[1];
  cpu_gpr[34] = cpu_csr[2];
  cpu_gpr[35] = cpu_csr[3];
  cpu_gpr[36] = cpu_csr[4];
  s->pc = cpu_pc; // 把PC保存到decode结构s的pc和static next pc中
  s->snpc = cpu_pc;
  s->isa.inst.val = cpu_inst;
  exec_Iringbuf(s);
  //printf("pc: %lx inst: %08x\n",s->pc,s->isa.inst.val);
  if (s->pc < 0x80000000)
  {
    printf("PC out of bound!\n");
    exit(0);
  }
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;                                      // 填充logbuf
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc); // snprintf返回值表示实际写入的字符数,此处为其填充了PC的信息
  int ilen = 4;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i--)
  {
    p += snprintf(p, 4, " %02x", inst[i]); // 此处为logbuf填充了指令的具体16进制值
  }
  int space_len = 1;
  memset(p, ' ', space_len);
  p += space_len;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p, s->pc, (uint8_t *)&s->isa.inst.val, ilen);

  // exec_Iringbuf(s); //在附近的环形指令缓存中添加当前指令

#endif
}

/***************sdb.c***************/

char *my_getline()
{
  char *line_read = (char *)malloc(20 * sizeof(char));
  memset(line_read, '\0', 20);
  int i = 0;
  char c;

  printf("(npc) ");
  while ((c = getchar()) != '\n')
  {
    line_read[i++] = c;
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  //exit(0);
  return -1;
}

static int cmd_si(char *args)
{
  if (args == NULL)
  {
    cpu_exec(1); // 没有参数的时候执行1步
  }
  else
  {
    cpu_exec((uint64_t)atoi(args)); // 有参数的时候执行参数步
  }
  return 0;
}

static int cmd_info(char *args)
{
  if (*args == 'r')
  {
    isa_reg_display();
  }
  return 0;
}

static int cmd_help(char *args);

static struct
{
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NPC", cmd_q},
    {"si", "Let the program step through N instructions and then pause execution", cmd_si},
    {"info", "Print register status(r) / watchpoint information(w)", cmd_info},
    //{ "x", "Output N consecutive four bytes in hexadecimal form from the start address", cmd_x},
    //{ "p", "Calculate the value of the expression", cmd_p},
    //{ "w", "Set a watch point",cmd_w},
    //{ "d", "Delete number N watchpoint",cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_mainloop()
{
  /*if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }*/

  for (char *str; (str = my_getline()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    // printf("cmd is %s\n",cmd);
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

    /*#ifdef CONFIG_DEVICE
        void sdl_clear_event_queue();
        sdl_clear_event_queue();
    #endif*/

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      int a = strcmp(cmd, cmd_table[i].name);
      if (a == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

/***************ITRACE***************/
#ifdef CONFIG_IRINGBUF
Decode Iringbuf[16]; // 当前指令的附近环形指令缓存
int Iringbuf_point = 0;

static void exec_Iringbuf(Decode *s)
{
  Iringbuf[Iringbuf_point] = *s;
  if (Iringbuf_point == 15)
  {
    Iringbuf_point = 0;
  }
  else
  {
    Iringbuf_point++;
  }
}

void print_Iringbuf(void)
{
  printf("Instructions before the wrong one:\n");
  int n = Iringbuf_point;
  while (n != Iringbuf_point - 1)
  {
    printf("   0x%lx %08x\n", Iringbuf[n].pc, Iringbuf[n].isa.inst.val);
    n = (n + 1) % 16;
  }
  printf("-->0x%lx %08x\n", Iringbuf[n].pc, Iringbuf[n].isa.inst.val);
}
#endif

/***************reg.c***************/

const char *regs[] = {
    "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

const char *csr_regs[] = {
    "mepc", "mstatus", "mcause", "mtvec"
};

void isa_reg_display()
{
  for (int idx = 0; idx < 32; idx++)
  {
    const char *name = regs[idx];
    printf("%s  0x%8lx   ", name, cpu_gpr[idx]);
    if (idx % 4 == 3)
    {
      printf("\n");
    }
  }
  //csr
  for(int idx = 0; idx < 4; idx++){
    const char* csr_name = csr_regs[idx];
    printf("%s  0x%8lx   ", csr_name, cpu_gpr[idx+33]);
  }
  printf("\n");
}

void isa_ref_r_display(CPU_state ref_r)
{
  printf("\nref_regs:\n");
  for (int idx = 0; idx < 32; idx++)
  {
    const char *name = regs[idx];
    printf("%s  0x%8lx   ", name, ref_r.gpr[idx]);
    if (idx % 4 == 3)
    {
      printf("\n");
    }
  }
  for(int idx = 0; idx < 4; idx++){
    const char* csr_name = csr_regs[idx];
    printf("%s  0x%8lx   ", csr_name, ref_r.csr[idx]);
  }
  printf("\nref_pc: %lx\n", ref_r.pc);
}

/***************dut.c***************/
void (*ref_difftest_memcpy)(paddr_t addr, void *buf, size_t n, bool direction) = NULL;
void (*ref_difftest_regcpy)(void *dut, bool direction) = NULL;
void (*ref_difftest_exec)(uint64_t n) = NULL;
void (*ref_difftest_raise_intr)(uint64_t NO) = NULL;

#ifdef CONFIG_DIFFTEST

static bool is_skip_ref = false;
static bool is_last_skip_ref = false;
static int skip_dut_nr_inst = 0;

void difftest_skip_ref()
{
  is_skip_ref = true;
  skip_dut_nr_inst = 0;
}

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
  if (memcmp(ref_r, cpu_gpr, 34 * 8) == 0 && ref_r->pc == pc)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void init_difftest(const char *ref_so_file, long img_size, int port)
{
  assert(ref_so_file != NULL);
  cpu_gpr[32] = top->inst_sram_addr;
  /*cpu_gpr[34] = 0;
  cpu_gpr[35] = 0xa00001800;
  cpu_gpr[36] = 0;
  cpu_gpr[37] = 0;*/

  void *handle;
  handle = dlopen(ref_so_file, RTLD_LAZY);
  assert(handle);

  ref_difftest_memcpy = (void (*)(paddr_t, void *, size_t, bool))dlsym(handle, "difftest_memcpy");
  assert(ref_difftest_memcpy);

  ref_difftest_regcpy = (void (*)(void *dut, bool direction))dlsym(handle, "difftest_regcpy");
  assert(ref_difftest_regcpy);

  ref_difftest_exec = (void (*)(uint64_t n))dlsym(handle, "difftest_exec");
  assert(ref_difftest_exec);

  ref_difftest_raise_intr = (void (*)(uint64_t NO))dlsym(handle, "difftest_raise_intr");
  assert(ref_difftest_raise_intr);

  void (*ref_difftest_init)(int) = (void (*)(int port))dlsym(handle, "difftest_init");
  assert(ref_difftest_init);

  ref_difftest_init(port);
  ref_difftest_memcpy(RESET_VECTOR, guest_to_host(RESET_VECTOR), img_size, DIFFTEST_TO_REF); // 将DUT的guest memory拷贝到REF中
  ref_difftest_regcpy(cpu_gpr, DIFFTEST_TO_REF);                                             // 将DUT的寄存器状态拷贝到REF中
}

static void checkregs(CPU_state *ref, vaddr_t pc)
{
  if (!isa_difftest_checkregs(ref, pc))
  {
    printf("\33[1;31mNPC ABORT at pc = 0x%lx\n", pc);
    isa_reg_display();
    isa_ref_r_display(*ref);
    print_Iringbuf();
    NPC_STATE = NPC_DUMP;
  }
}

void difftest_step(vaddr_t pc)
{
  CPU_state ref_r;

  if (is_skip_ref)
  {
    // to skip the checking of an instruction, just copy the reg state to reference design
    is_last_skip_ref = true;
    is_skip_ref = false;
    return;
  }

  if(is_last_skip_ref){
    ref_difftest_regcpy(cpu_gpr, DIFFTEST_TO_REF);
    is_last_skip_ref = false;
    return;
  }

  ref_difftest_exec(1);                         // ref执行一次
  ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT); // 将ref中的寄存器状态备份到ref_r中，比较ref_r和nemu的寄存器状态

  checkregs(&ref_r, pc);
}

#endif

/***************vga.c***************/
#ifdef HAS_GUI

static SDL_Renderer *renderer = NULL; //SDL_Renderer是一个用于渲染2D图像的对象,renderer意思是渲染器
static SDL_Texture *texture = NULL; //SDL_Texture是一个包含像素数据的对象，可以被用于渲染2D图像

uint8_t * new_space(int size){
  uint8_t* p = (uint8_t*)malloc(size);
  assert(p!=NULL);
  return p;
}

static uint32_t screen_width() {
  return SCREEN_W; //400
}

static uint32_t screen_height() {
  return SCREEN_H; //300
}

static uint32_t screen_size() {
  return screen_width() * screen_height() * sizeof(uint32_t); //宽400高300,每个像素占32字节
}

static void init_screen() {
  SDL_Window *window = NULL;
  char title[128];
  sprintf(title, "riscv64-NPC");
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(SCREEN_W * 2,SCREEN_H * 2, 0, &window, &renderer); //前两个参数400*2和300*2代表的是窗口的参数,0指的是渲染器的默认类型
  SDL_SetWindowTitle(window, title); //为窗口设置标题
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H); //设置一个纹理,与相应的渲染器挂钩,设置纹理格式为ARG8888,访问方式为静态
}

static inline void update_screen() {
  SDL_UpdateTexture(texture, NULL, vmem, SCREEN_W * sizeof(uint32_t)); //函数从一个内存地址（vmem）中将像素数据写入到一个 SDL 纹理对象（texture）中
  SDL_RenderClear(renderer); //这个函数清除绘制目标（renderer）上所有的渲染信息，准备开始新一轮的渲染操作。
  SDL_RenderCopy(renderer, texture, NULL, NULL); //这个函数从纹理对象（texture）中复制像素数据到渲染目标（renderer）中，实现了将屏幕上显示的图像与纹理对象内容同步的效果
  SDL_RenderPresent(renderer); //这个函数将最终的渲染结果呈现在屏幕上
}

void vga_update_screen() {
  // TODO: call `update_screen()` when the sync register is non-zero,
  // then zero out the sync register
  uint32_t sync_reg = vgactl_port_base[1];
  if(sync_reg){
    update_screen();
    vgactl_port_base[1] = 0;
  }
}

void init_vga() {
  vgactl_port_base = (uint32_t *)new_space(8);
  vgactl_port_base[0] = (screen_width() << 16) | screen_height(); //实现了屏幕大小寄存器

  vmem = new_space(screen_size());
  init_screen();
  memset(vmem, 0, screen_size());
}

#endif