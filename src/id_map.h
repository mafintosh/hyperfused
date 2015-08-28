#ifndef _HYPERFUSE_ID_MAP_H_
#define _HYPERFUSE_ID_MAP_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct {
  void *data[1024];
  uint16_t free_list[1024];
  uint16_t free_count;
  uint16_t allocated;
} id_map_t;

void id_map_init (id_map_t *self);
uint16_t id_map_alloc (id_map_t *self, void *data);
void *id_map_get (id_map_t *self, uint16_t ptr);
void *id_map_free (id_map_t *self, uint16_t ptr);

#endif
