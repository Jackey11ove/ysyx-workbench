#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

void print_num(int number){
  int digits=0;
  for(int a = 1; number/a != 0; a*=10){
    digits ++;
  }
  for(int i=digits-1;i>=0;i--){
    int div = 1;
    for(int j=0;j<i;j++){
      div*=10;
    }
    int h = number/div;
    putch(h+'0');
    number = number%div;
  }
}

int vprintf(const char *fmt, va_list ap){
  int count = 0;
  int i = 0;
  while (fmt[i] != '\0') //i是fmt字符数组的指针
  {
    if(fmt[i]!='%'){
      putch(fmt[i]);
      i++;
    }else{
      i++;
      count++;
      switch (fmt[i])
      {
      case 's':
        char * str = va_arg(ap,char*);
        if(str == NULL){
          panic("empty str!");
          break;
        }
        for(int j=0;str[j]!='\0';j++){
          putch(str[j]);
        }
        break;
      case 'd':
        int num = va_arg(ap,int);
        if(num == 0){
          putch('0');
        }
        if(num<0){
          putch('-');
          num *=-1;
        }
        print_num(num);
        break;
        
      default:
        break;
      }
      i++;     
    }
  }
  return count;
}

int printf(const char *fmt, ...) {
  int number;
  va_list args;
  va_start(args, fmt);
  number = vprintf(fmt, args);
  va_end(args);
  return number;
  //panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  int count = 0;
  int i = 0;
  int j = 0;
  while (fmt[i] != '\0') //i是fmt字符数组的指针
  {
    if(fmt[i]!='%'){
      out[j] = fmt[i];
      j++;
      i++;
    }else{
      i++;
      count++;
      switch (fmt[i])
      {
      case 's':
        char * str = va_arg(ap,char*);
        if(str == NULL){
          printf("empty str!\n");
          break;
        }
        strcpy(out+j,str);
        j += strlen(str);
        break;
      case 'd':
        int num = va_arg(ap,int);
        if(num == 0){
          out[j] = '0';
          j++;
        }
        if(num<0){
          out[j] = '-';
          j++;
          num *=-1;
        }
        int start = j;
        do{
          out[j] = num%10 + '0';
          j++;
        }while((num /= 10) > 0);
        int end = j-1;
        while (start<end)
        {
          char temp = out[start];
          out[start] = out[end];
          out[end] = temp;
          start++;
          end--;
        }
        
        break;
        
      default:
        break;
      }
      i++;     
    }
  }
  out[j] = '\0';
  return count;
  
  //panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  int number;
  va_list args;

  va_start(args,fmt); //va_list指针args被初始化并指向可变参数列表，而fmt是函数参数中最后一个命名参数的名称
  
  number = vsprintf(out,fmt,args); //调用上面的函数vsprintf，该函数的参数正是可变参数列表的指针

  va_end(args); //结束可变参数列表的处理
  
  return number;

  //panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
