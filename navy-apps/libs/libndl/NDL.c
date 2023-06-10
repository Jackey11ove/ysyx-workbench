#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  uint32_t current_time;
  gettimeofday(&tv,NULL);
  current_time = tv.tv_sec * 1000 + tv.tv_usec/1000;
  return current_time;
}

int NDL_PollEvent(char *buf, int len) {
  evtdev = open("/dev/events",O_RDONLY);
  if(read(evtdev,buf,len)){
    close(evtdev);
    return 1;
  }else{
    close(evtdev);
    return 0;
  }
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }

  int fd = open("/proc/dispinfo", 0, 0);
  char buf[100];
  int len = read(fd,buf,sizeof(buf)-1);
  buf[len] = '\0';
  if(len <= 0){
    printf("/proc/dispinfo open fail!\n");
    assert(0);
  }else{
    char * width_str;
    char * height_str;
    width_str = strchr(buf, ':') + 1;
    height_str = strchr(width_str, ':') + 1;
    screen_w = atoi(width_str);   //screen_w, screen_h指的是屏幕,传入的参数是画布相关的
    screen_h = atoi(height_str);
  }

  if(*w == 0 && *h == 0){
    *w = screen_w;
    *h = screen_h;
  }

  canvas_w = *w;
  canvas_h = *h;

  printf("screen_width = %d, screen_height = %d\n",screen_w,screen_h);
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  fbdev = open("/dev/fb", O_RDWR);

  x += (screen_w - canvas_w)/2;
  y += (screen_h - canvas_h)/2;

  size_t offset,len;
  for(int i = 0; i < h; i ++){
    offset = ( (y + i) * screen_w + x ) * sizeof(int); //乘以4是因为文件是以字节计数的，括号里算出来的是按照uint32来计算的
    len = w * sizeof(int);
    lseek(fbdev, offset, SEEK_SET);
    write(fbdev, pixels, len);
    pixels += w;
  }
  close(fbdev);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
