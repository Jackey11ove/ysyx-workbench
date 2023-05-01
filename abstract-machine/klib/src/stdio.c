#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define ZEROPAD 1
#define SIGN 2
#define PLUS 4
#define SPACE 8
#define LEFT 16
#define SPECIAL 32
#define LARGE 64
static void reverse(char *s, int len) {
  char *end = s + len - 1;
  char tmp;
  while (s < end) {
    tmp = *s;
    *s = *end;
    *end = tmp;
    s++;
    end--;
  }
}
//无符号数itoa
static int uitoa(unsigned int n, char *s, int base) {
  assert(base <= 16);

  int i = 0, bit;
  do {
    bit = n % base;
    if (bit >= 10) s[i++] = 'a' + bit - 10;
    else s[i++] = '0' + bit;
  } while ((n /= base) > 0);
  s[i] = '\0';
  reverse(s, i);

  return i;
}
static int itoa(int n, char *s, int base) {
  assert(base <= 16);

  int i = 0, sign = n, bit;
  if (sign < 0) n = -n;
  do {
    bit = n % base;
    if (bit >= 10) s[i++] = 'a' + bit - 10;
    else s[i++] = '0' + bit;
  } while ((n /= base) > 0);
  if (sign < 0) s[i++] = '-';
  s[i] = '\0';
  reverse(s, i);

  return i;
}
int sprintf(char *out, const char *fmt, ...){
  va_list ap;
  char *start = out;
  va_start(ap, fmt);
  
  for(;*fmt != '\0'; ++fmt){
    if(*fmt != '%'){
      *out = *fmt;
      ++out;
    }else{
      switch(*(++fmt)){
        case '%':
          *out = *fmt;
          ++out;
          break;
        case 'd':
          out += itoa(va_arg(ap, int), out, 10); 
          break;
        case 's':
          strcpy(out, va_arg(ap, char*));
          out += strlen(out);
          break;
        case 'c':
          *out = va_arg(ap, int);
          ++out;
          break;
        case 'p':
          *out = '0';
          ++out;
          *out = 'x';
          ++out;
          out += uitoa(va_arg(ap, int), out, 16);
          break;
        case 'x':
          out += itoa(va_arg(ap, int), out, 16);
          break;
      }
    }
  }
  *out = '\0';
  va_end(ap);
  return out - start;
}

int vsprintf(char *out, const char *fmt, va_list ap){
  char *start = out;
  
  for(;*fmt != '\0'; ++fmt){
    if(*fmt != '%'){
      *out = *fmt;
      ++out;
    }else{
      switch(*(++fmt)){
        case '%':
          *out = *fmt;
          ++out;
          break;
        case 'd':
          out += itoa(va_arg(ap, int), out, 10); 
          break;
        case 's':
          strcpy(out, va_arg(ap, char*));
          out += strlen(out);
          break;
        case 'c':
          *out = va_arg(ap, int);
          ++out;
          break;
        case 'p':
          //在前面添加0x
          *out = '0';
          ++out;
          *out = 'x';
          ++out;
          out += uitoa(va_arg(ap, int), out, 16);
          break;
        case 'x':
          out += itoa(va_arg(ap, int), out, 16);
          break;
      }
    }
  }
  *out = '\0';
  return out - start;
}
int printf(const char *fmt, ...) {
  va_list ap;
  char buf[10000];
  va_start(ap, fmt);
  int len = vsprintf(buf, fmt, ap);
  va_end(ap);
  for (int i = 0; i < len; i ++) putch(buf[i]);
  return 0;
}


int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  char *start = out;
  va_start(ap, fmt);
  
  for(;*fmt != '\0'; ++fmt){
    n--;
    if(n == 0) break;
    if(*fmt != '%'){
      *out = *fmt;
      ++out;
    }else{
      switch(*(++fmt)){
        case '%':
          *out = *fmt;
          ++out;
          break;
        case 'd':
          out += itoa(va_arg(ap, int), out, 10); 
          break;
        case 's':
          strcpy(out, va_arg(ap, char*));
          out += strlen(out);
          break;
        case 'c':
          *out = va_arg(ap, int);
          ++out;
          break;
        case 'p':
          *out = '0';
          ++out;
          *out = 'x';
          ++out;
          out += uitoa(va_arg(ap, int), out, 16);
          break;
        case 'x':
          out += itoa(va_arg(ap, int), out, 16);
          break;
      }
    }
  }
  *out = '\0';
  va_end(ap);
  return out - start;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
