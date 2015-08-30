#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "../src/enc.h"

int test_enc () {
  char buf[1024];
  char *buf_ptr = (char *) &buf;
  char *res_enc = NULL;
  char *res_dec = NULL;

  uint8_t v8;
  res_enc = write_uint8(buf_ptr, 42);
  res_dec = read_uint8(buf_ptr, &v8);
  assert(buf_ptr - res_dec == buf_ptr - res_enc);
  assert(v8 == 42);

  uint16_t v16;
  res_enc = write_uint16(buf_ptr, 4200);
  res_dec = read_uint16(buf_ptr, &v16);
  assert(buf_ptr - res_dec == buf_ptr - res_enc);
  assert(v16 == 4200);

  uint32_t v32;
  res_enc = write_uint32(buf_ptr, 420000);
  res_dec = read_uint32(buf_ptr, &v32);
  assert(buf_ptr - res_dec == buf_ptr - res_enc);
  assert(v32 == 420000);

  int32_t i32;
  res_enc = write_int32(buf_ptr, -420000);
  res_dec = read_int32(buf_ptr, &i32);
  assert(buf_ptr - res_dec == buf_ptr - res_enc);
  assert(i32 == -420000);

  char* str = "hello world";
  uint16_t str_len;
  res_enc = write_string(buf_ptr, str, strlen(str));
  res_dec = read_string(buf_ptr, &str, &str_len);
  assert(buf_ptr - res_dec == buf_ptr - res_enc);
  assert(!strcmp(str, "hello world"));
  assert(strlen(str) == str_len);

  char *tmp = "1234512345";
  res_enc = write_buffer(buf_ptr, tmp, 10);
  assert(res_enc - buf_ptr == 10);
  for (int i = 0; i < 10; i++) assert(*(buf + i) == *(tmp + i));

  return 0;
}
