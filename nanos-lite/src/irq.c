#include <common.h>

void do_syscall(Context *c);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: printf("YIELD, event ID = %d\n", e.event);break;
    case EVENT_SYSCALL: do_syscall(c);break;
    default: panic("Unhandled event ID = %d", e.event);
  } //没有匹配项的时候调用panic报出一个错误信息

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
