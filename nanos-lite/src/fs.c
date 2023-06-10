#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENTS, FD_FB, FD_DISPINFO};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write, 0},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write, 0},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write, 0},
  [FD_FB]     = {"/dev/fb", 0, 0, invalid_read, fb_write, 0}, //该文件表示显存
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write, 0}, //该文件仅有指示屏幕大小的功能,没有给它具体的内容,只是在调用读函数的时候写入了屏幕尺寸
#include "files.h"
};

int fs_open(const char *pathname, int flags, int mode){
  for(int idx = 0; idx < sizeof(file_table)/sizeof(Finfo); idx ++){
    if(strcmp(pathname, file_table[idx].name) == 0){
      return idx;
    }
  }
  panic("File %s not found\n",pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len){
  Finfo * file_info = &file_table[fd];
  size_t ret;

  if(file_info->read != NULL){
    ret = file_info->read(buf,file_info->disk_offset + file_info->open_offset,len);
  }else{
    if(file_info->open_offset + len > file_info->size){
      len = file_info->size - file_info->open_offset;
      printf("fs_read out of bound\n");
    }
  
    ret = ramdisk_read(buf, file_info->disk_offset + file_info->open_offset, len);
    file_info->open_offset += len;
  }
  return ret;
}

size_t fs_write(int fd, const void *buf, size_t len){
  Finfo * file_info = &file_table[fd];
  size_t ret;

  if(file_info->write != NULL){
    ret = file_info->write(buf, file_info->disk_offset + file_info->open_offset, len);
  }else{
    if(file_info->open_offset + len > file_info->size){
      len = file_info->size - file_info->open_offset;
      printf("fs_write out of bound\n");
     }

    ret = ramdisk_write(buf, file_info->disk_offset + file_info->open_offset, len);
    file_info->open_offset += len;
  }
  return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence){
  switch (whence)
  {
  case SEEK_SET:
    file_table[fd].open_offset = offset;
    break;
  case SEEK_CUR:
    file_table[fd].open_offset += offset;
    break;
  case SEEK_END:
    file_table[fd].open_offset = file_table[fd].size - offset;
    break;
  
  default:
    return -1;
  }

  return file_table[fd].open_offset;
}


int fs_close(int fd){
  assert(fd >= 0 && fd < sizeof(file_table)/sizeof(Finfo)); //检查fd是否越界
  return 0;
}

void init_fs() {
  // TODO: initialize the size of /dev/fb
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  file_table[FD_FB].size = cfg.height * cfg.width * 4; //size按照字节数算
  //printf("init_fs: file_table[FD_FB].size = %d\n", file_table[FD_FB].size);
}
