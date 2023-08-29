#include <am.h>
#include <npc.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
#define W 400
#define H 300

void __am_gpu_init() {
  /*int i;
  int w = 400;  // TODO: get the correct width
  int h = 300;  // TODO: get the correct height
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i ++) fb[i] = i;
  outl(SYNC_ADDR, 1);*/
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t screen_para = inl(VGACTL_ADDR);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = screen_para >> 16, .height = (screen_para << 16) >> 16,
    .vmemsz = 0
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  uint32_t* pixels = ctl->pixels;
  for (int i=0; i<h; i++) {
    for (int j=0; j<w; j++) {
      // 计算像素在 pixels 中的索引
      int idx = i*w + j;

      // 从像素值中提取 RGB 分量
      uint8_t r = (uint8_t)((pixels[idx] >> 16) & 0xff);
      uint8_t g = (uint8_t)((pixels[idx] >> 8) & 0xff);
      uint8_t b = (uint8_t)(pixels[idx] & 0xff);

      // 计算像素在帧缓冲中的地址
      uint32_t* fb = (uint32_t*)(uintptr_t)(FB_ADDR + ((y+i)*W + (x+j))*sizeof(uint32_t));

      // 将像素值写入帧缓冲
      *fb = (r << 16) | (g << 8) | b;
    }
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
  
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
