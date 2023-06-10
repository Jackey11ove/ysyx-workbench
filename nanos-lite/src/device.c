#include <common.h>
#include <fs.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for(int i = 0; i < len; i++){
    putch( *( (char*)(buf + i) ) );
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if(ev.keycode == AM_KEY_NONE){
    *((char *)buf) = '\0';
    return 0;
  }else{
    size_t ret = snprintf((char *)buf,len,"%s %s",ev.keydown? "kd" : "ku",keyname[ev.keycode]);
    return ret;
  }
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  size_t ret = snprintf((char *)buf,len,"WIDTH : %d\n HEIGHT : %d", cfg.width, cfg.height);
  return ret;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  AM_GPU_FBDRAW_T ctl;
  ctl.y = (offset/4)/cfg.width;
  ctl.x = (offset/4)%cfg.width;
  ctl.w = len/4;
  ctl.h = 1;
  ctl.sync = true;
  ctl.pixels = (void*)buf;

  io_write(AM_GPU_FBDRAW, ctl.x, ctl.y, ctl.pixels, ctl.w, ctl.h, ctl.sync);
  
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
