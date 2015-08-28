#include <string.h>
#include <arpa/inet.h>
#include "enc.h"

inline char *write_uint8 (char *buf, uint8_t val) {
  *buf = val;
  return buf + 1;
}

inline char *read_uint8 (char *buf, uint8_t *val) {
  *val = *buf;
  return buf + 1;
}

inline char *write_uint16 (char *buf, uint16_t val) {
  uint16_t tmp = htons(val);
  memcpy(buf, &tmp, 2);
  return buf + 2;
}

inline char *read_uint16 (char *buf, uint16_t *val) {
  uint16_t tmp = ntohs(*((uint32_t *) buf));
  memcpy(val, &tmp, 2);
  return buf + 2;
}

inline char *write_uint32 (char *buf, uint32_t val) {
  uint32_t tmp = htonl(val);
  memcpy(buf, &tmp, 4);
  return buf + 4;
}

inline char *read_uint32 (char *buf, uint32_t *val) {
  uint32_t tmp = ntohl(*((uint32_t *) buf));
  memcpy(val, &tmp, 4);
  return buf + 4;
}

inline char *read_int32 (char *buf, int32_t *val) {
  int32_t tmp = ntohl(*((int32_t *) buf));
  memcpy(val, &tmp, 4);
  return buf + 4;
}

inline char *write_string (char *buf, char *str, uint16_t str_len) {
  buf = write_uint16(buf, str_len);
  memcpy(buf, str, str_len);
  buf += str_len;
  *buf = '\0';
  return buf + 1;
}

inline char *read_string (char *buf, char **str, uint16_t *str_len) {
  buf = read_uint16(buf, str_len);
  *str = buf;
  return buf + *str_len + 1;
}

inline char *write_buffer (char *buf, char *data, uint32_t data_len) {
  memcpy(buf, data, data_len);
  return buf + data_len;
}
