#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t i = 0;
  while (s[i]!='\0')
  {
    i++;
  }
  return i;
  
  //panic("Not implemented");
}

char *strcpy(char *dst, const char *src) {
  int i = 0;
  while (src[i] != '\0')
  {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return dst;
  
  //panic("Not implemented");
}

char *strncpy(char *dst, const char *src, size_t n) {
  panic("Not implemented");
}

char *strcat(char *dst, const char *src) {
  int i = 0;
  int j = 0;
  while (dst[i] != '\0')
  {
    i++;
  }
  while (src[j] != '\0')
  {
    dst[i] = src[j];
    i++;
    j++;
  }
  dst[i] = '\0';
  return dst;
  
  //panic("Not implemented");
}

int strcmp(const char *s1, const char *s2) {
  int i = 0;
  while (s1[i] == s2[i])
  {
    if(s1[i] == '\0'){
      return 0;
    }
    i++;
  }
  return (s1[i]-s2[i]);
  //panic("Not implemented");
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  unsigned char * p = (unsigned char *)s;
  for(size_t i=0;i<n;i++){
    *p++ = (unsigned char)c;
  }
  return s;
  //panic("Not implemented");
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  char * cout = (char*)out;
  char * cin = (char*)in;
  for(size_t i=0;i<n;i++){
    cout[i] = cin[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char * p1 = s1,*p2 = s2;
  for(size_t i = 0; i<n; i++){
    if(p1[i]!=p2[i]){
      return (p1[i]-p2[i]);
    }
  }
  return 0;
  //panic("Not implemented");
}

#endif
