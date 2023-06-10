#include <common.h>
#include "syscall.h"
#include <fs.h>
#include <timer.h>

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; //$a7
  a[1] = c->GPR2; //$a0
  a[2] = c->GPR3; //$a1
  a[3] = c->GPR4; //$a2

  switch (a[0]) {

    case SYS_exit: 
      //printf("SYS_exit, syscall ID = %d\n",a[0]);
      halt(a[1]); //实际上是根据$a0来确定
      break;

    case SYS_yield:
      //printf("SYS_yield, syscall ID = %d\n",a[0]);
      yield();
      c->GPRx = 0;
      break;

    case SYS_open:
      //printf("SYS_yield, syscall ID = %d\n",a[0]);
      c->GPRx = fs_open((const char *)a[1],a[2],a[3]);
      break;

    case SYS_read:
      //printf("SYS_read, syscall ID = %d\n",a[0]);
      c->GPRx = fs_read(a[1], (void *)a[2], a[3]);
      break;

    case SYS_write:
      //printf("SYS_write, syscall ID = %d\n",a[0]);
      c->GPRx = fs_write(a[1], (void*)a[2], a[3]);
      break;
    
    case SYS_close:
      //printf("SYS_close, syscall ID = %d\n",a[0]);
      c->GPRx = fs_close(a[1]);
      break;

    case SYS_lseek:
      //printf("SYS_lseek, syscall ID = %d\n",a[0]);
      c->GPRx = fs_lseek(a[1],a[2],a[3]);
      break;

    case SYS_brk:
      //printf("SYS_brk, syscall ID = %d\n",a[0]); 
      c->GPRx = 0;
      break;

    case SYS_gettimeofday:
      //printf("SYS_gettimeofday, syscall ID = %d\n",a[0]);
      c->GPRx = gettimeofday((struct timeval *)a[1]);
      break;
    
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
