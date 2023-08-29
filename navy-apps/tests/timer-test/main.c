#include <stdio.h>
#include <NDL.h>
 
int main(){
  int sec = 1;
  NDL_Init(0);
  uint32_t start_time = NDL_GetTicks();
  while (sec < 10) {
    while ((NDL_GetTicks() - start_time)/1000 < sec);
    printf("sec = %d\n", sec);
    sec ++;
  }
  return 0;
}