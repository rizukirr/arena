#define ARENA_IMPLEMENTATION_H
#include "../arena.h"
#include <stdio.h>

int main() {
  struct Arena *arena = arena_init(1024);
  for (int i = 0; i < 10; i++) {
    struct Arena *ptr = (struct Arena *)arena_alloc(arena, 1024, 16);
    printf("adderes: %p\n", ptr);
  }
  arena_free(arena);
  return 0;
}
