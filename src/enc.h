#ifndef _HYPERFUSE_ENC_H_
#define _HYPERFUSE_ENC_H_

#include <stdlib.h>
#include <stdint.h>

char *write_uint8 (char *buf, uint8_t val);
char *read_uint8 (char *buf, uint8_t *val);

char *write_uint16 (char *buf, uint16_t val);
char *read_uint16 (char *buf, uint16_t *val);

char *write_uint32 (char *buf, uint32_t val);
char *read_uint32 (char *buf, uint32_t *val);

char *read_int32 (char *buf, int32_t *val);

char *write_string (char *buf, char *str, uint16_t str_len);
char *read_string (char *buf, char **str, uint16_t *str_len);

char *write_buffer (char *buf, char *data, uint32_t data_len);

#endif
