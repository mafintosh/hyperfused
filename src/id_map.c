#include "id_map.h"
#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void id_map_init (id_map_t *self) {
  self->free_count = 0;
  self->allocated = 0;
}

static uint16_t next_id (id_map_t *self) {
  if (self->free_count) return self->free_list[--self->free_count];
  return ++self->allocated;
}

uint16_t id_map_alloc (id_map_t *self, void *data) {
  pthread_mutex_lock(&mutex);
  uint16_t id = next_id(self);
  self->data[id - 1] = data;
  pthread_mutex_unlock(&mutex);
  return id;
}

void *id_map_get (id_map_t *self, uint16_t ptr) {
  pthread_mutex_lock(&mutex);
  void *result = self->data[ptr - 1];
  pthread_mutex_unlock(&mutex);
  return result;
}

void *id_map_free (id_map_t *self, uint16_t ptr) {
  pthread_mutex_lock(&mutex);
  self->free_list[self->free_count++] = ptr;
  void *result = self->data[ptr - 1];
  pthread_mutex_unlock(&mutex);
  return result;
}
