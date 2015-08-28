#include "id_map.h"

void id_map_init (id_map_t *self) {
  self->free_count = 0;
  self->allocated = 0;
}

static uint16_t next_id (id_map_t *self) {
  if (self->free_count) return self->free_list[--self->free_count];
  return ++self->allocated;
}

uint16_t id_map_alloc (id_map_t *self, void *data) {
  uint16_t id = next_id(self);
  self->data[id - 1] = data;
  return id;
}

void *id_map_get (id_map_t *self, uint16_t ptr) {
  return self->data[ptr - 1];
}

void *id_map_free (id_map_t *self, uint16_t ptr) {
  self->free_list[self->free_count++] = ptr;
  return self->data[ptr - 1];
}
