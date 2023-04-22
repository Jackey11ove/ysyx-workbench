#include <am.h>
#include <nemu.h>

extern char _heap_start;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END);  //指示堆区的起始和末尾
//堆区是给程序自由使用的一段内存区间, 为程序提供动态分配内存的功能. TRM的API只提供堆区的起始和末尾, 而堆区的分配和管理需要程序自行维护
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

void putch(char ch) {  //用于输出一个字符
  outb(SERIAL_PORT, ch);
}

void halt(int code) {  //结束程序的运行
  nemu_trap(code);

  // should not reach here
  while (1);
}

void _trm_init() {  //进行TRM相关的初始化工作
  int ret = main(mainargs);
  halt(ret);
}
