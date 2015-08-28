#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "../src/id_map.h"

int test_id_map () {
  id_map_t map;
  id_map_init(&map);

  char *tmp1 = "hello world 1";
  char *tmp2 = "hello world 2";

  uint16_t ptr1 = id_map_alloc(&map, tmp1);
  assert(ptr1 == 1);

  uint16_t ptr2 = id_map_alloc(&map, tmp2);
  assert(ptr2 == 2);

  char *tmp2_cpy = id_map_free(&map, ptr2);
  assert(strcmp(tmp2_cpy, tmp2) == 0);

  ptr2 = id_map_alloc(&map, tmp2);
  assert(ptr2 == 2);

  char *tmp1_cpy = id_map_free(&map, ptr1);
  assert(strcmp(tmp1_cpy, tmp1) == 0);

  ptr1 = id_map_alloc(&map, tmp1);
  assert(ptr1 == 1);

  return 0;
}
