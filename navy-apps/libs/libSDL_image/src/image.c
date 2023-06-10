#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  FILE * fp = fopen(filename,"rb");

  if(!fp){
    printf("Failed to open file %s\n", filename);
    return NULL;
  }

  fseek(fp,0,SEEK_END);
  size_t size = ftell(fp);
  fseek(fp,0,SEEK_SET);    //获取文件大小并返回文件开头

  void *buf = malloc(size);//申请大小为size的内存空间
  
  size_t read_size = fread(buf, 1, size, fp);
  if(read_size != size){
    printf("read file fail!\n");
    return NULL;
  }

  SDL_Surface * surface = STBIMG_LoadFromMemory(buf,size);

  fclose(fp);
  free(buf);

  return surface;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
