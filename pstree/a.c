#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#define BUFF_SIZE 32

int test_read_file(const char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  char buffer[BUFF_SIZE];
  int len = vsnprintf(buffer,sizeof(buffer),__format, args);
  va_end(args);
  
  /* 判断是否溢出 */
  assert(len>=0 && len<BUFF_SIZE);
  printf("%s", buffer);
  return 0;
}

int test(void) {
  test_read_file("%s %d\n", "helsslo1789", 456789);
  return 0;
}