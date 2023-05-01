#include <common.h>

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: printf("nanos already yield\n");break;
    default: panic("Unhandled event ID = %d", e.event);
  } //没有匹配项的时候调用panic报出一个错误信息

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
