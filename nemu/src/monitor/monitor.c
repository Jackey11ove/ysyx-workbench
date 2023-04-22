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
#include <memory/paddr.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>

void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm(const char *triple);
#ifdef CONFIG_FTRACE
int  init_ftrace(char *argv);
Elf64_Sym* sym_table;
char* str_table;
int sym_entries;
#endif


static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
  //Log("Exercise: Please remove me in the source code and compile NEMU again.");
  //assert(0); 断言中的表达式为0时表示程序出错，此处作为PA0的练习，删除assert（0）
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int difftest_port = 1234;


static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image."); //外部没有为nemu载入程序的时候，使用内置的程序
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END); //将fp移动至文件的末尾（距文件末尾偏移量为0）
  long size = ftell(fp); //ftell函数是计算fp指针到文件开头的距离，若fp在文件末尾size就代表整个文件的大小

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET); //将fp移动至文件开头
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp); //把image镜像载入初始PC的位置也即程序的装载
  assert(ret == 1);

  fclose(fp);
  return size;
}

static int parse_args(int argc, char *argv[]) { //分析参数
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'}, //无参数值，返回'b'
    {"log"      , required_argument, NULL, 'l'}, //有参数值，返回'l'
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    {"ftrace"   , required_argument, NULL, 'f' },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:f:", table, NULL)) != -1) {
    /*解析getopt_long函数(本质上就是在处理长短选项参数)：
    int getopt_long(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex);
    前两个参数与main函数中的参数相同，*optstring代表短选项字符串，longopts代表长选项结构体          
    struct option 
    {  
        const char *name;         name:表示选项的名称,比如daemon,dir,out等
        int         has_arg;      has_arg:表示选项后面是否携带参数，有三个值：1)no_argument(0)选项后面不跟参数值 2）required_argument(1)参数输入格式为 --参数 值 或者 --参数=值
        int        *flag;         flag：这个参数有空和非空两个值，若为NULL则函数返回val值，若非空则函数返回0,flag指针指向val，也即为flag赋值val
        int         val;          val：作用参见flag
    };
    longindex非空，它指向的变量将记录当前找到参数符合longopts里的第几个元素的描述，即是longopts的下标值。
    */
    switch (o) {
      case 'b': sdb_set_batch_mode(); break; //将is_batch_mode设为1                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
      case 'p': sscanf(optarg, "%d", &difftest_port); break; //optarg为当前选项的参数值，也即终端中键入的argv
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      #ifdef CONFIG_FTRACE
      case 'f': sym_entries = init_ftrace(optarg);break;
      #endif
      case 1: img_file = optarg; return 0;
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

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand(); //设置随机种子，根据宏CONFIG_TARGET_AM是否被定义来决定种子是否是相同的

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device()); //如果定义了CONFIG_DEVICE则调用init_device()函数

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img(); //将一个有意义的客户程序从镜像文件中载入内存

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

#ifndef CONFIG_ISA_loongarch32r
  IFDEF(CONFIG_ITRACE, init_disasm(
    MUXDEF(CONFIG_ISA_x86,     "i686",
    MUXDEF(CONFIG_ISA_mips32,  "mipsel",
    MUXDEF(CONFIG_ISA_riscv32, "riscv32",
    MUXDEF(CONFIG_ISA_riscv64, "riscv64", "bad")))) "-pc-linux-gnu"
  ));
#endif

  /* Display welcome message. */
  welcome();
}
#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand(); //设置随机种子，根据宏CONFIG_TARGET_AM是否被定义来决定种子是否是相同的
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif

#ifdef CONFIG_FTRACE
int init_ftrace(char *argv){
    FILE * fd;
    Elf64_Ehdr* header;
    Elf64_Shdr* section_headers;
    size_t symbol_entries;

    symbol_entries = 0;

    //打开ELF文件
    fd = fopen(argv, "rb");
    if(fd == NULL){
      perror("open");
    }

    //读取ELF文件头部信息
    fseek(fd, 0, SEEK_SET);
    header = (Elf64_Ehdr*)malloc(sizeof(Elf64_Ehdr));
    if (fread( header, sizeof(Elf64_Ehdr), 1, fd) != 1) {
        perror("read");
    }

    // 定位到符号表和字符串表所在的节
    section_headers = (Elf64_Shdr*)malloc(sizeof(Elf64_Shdr) * header->e_shnum);
    fseek(fd, header->e_shoff, SEEK_SET);
    if(fread(section_headers, sizeof(Elf64_Shdr) , header->e_shnum, fd) != header->e_shnum){
      perror("read section");
    }


    for (int i = 0; i < header->e_shnum; i++) {
        if (section_headers[i].sh_type == SHT_SYMTAB) {
            // 找到符号表节
            symbol_entries = section_headers[i].sh_size/section_headers[i].sh_entsize; //该数据记录符号表的条目
            sym_table = (Elf64_Sym*)malloc(sizeof(Elf64_Sym)*symbol_entries);
            fseek(fd,section_headers[i].sh_offset,SEEK_SET);
            if(fread(sym_table, sizeof(Elf64_Sym),symbol_entries,fd) != symbol_entries){
              perror("read sym_table");
            }
            str_table = (char *)malloc(section_headers[section_headers[i].sh_link].sh_size);
            fseek(fd,section_headers[section_headers[i].sh_link].sh_offset,SEEK_SET);
            if(fread(str_table,section_headers[section_headers[i].sh_link].sh_size,1,fd) != 1){
              perror("read string_table");
            };
            break;
        }
    }

    /*if (sym_table == NULL) {
        printf("Symbol or string table not found.\n");
    }else{
        for(int j=0;j<symbol_entries;j++){
          printf("Symbol_table: name=%u type=%u value=0x%lx size=%lu\n", sym_table[j].st_name, ELF64_ST_TYPE(sym_table[j].st_info), sym_table[j].st_value, sym_table[j].st_size);
        }
    }
    for(int a=39;a<48;a++){
      if(str_table[a] == '\0'){
        printf("\n");
      }else{
        printf("%c",str_table[a]);
      }
    }*/

    free(header);
    free(section_headers);
    fclose(fd);

    return symbol_entries;
}
#endif
