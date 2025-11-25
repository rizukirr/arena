#define ARENA_IMPLEMENTATION
#include "../arena.h"
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

int main() {
  struct Arena *arena = arena_init(50); // 1 KB blocks

  if (!arena) {
    printf("Failed to create arena\n");
    return 1;
  }

  // Allocate some memory
  char *name = arena_alloc(arena, 32, ARENA_ALIGNOF(char));
  strcpy(name, "Rizki Rakasiwi");
  printf("Name: %s, Pointer: %p\n", name, arena->current);

  int *numbers = arena_alloc(arena, 5 * sizeof(int), ARENA_ALIGNOF(int));
  for (int i = 0; i < 5; i++)
    numbers[i] = i * 10;

  printf("Numbers:");
  for (int i = 0; i < 5; i++)
    printf(" %d", numbers[i]);
  printf(" Pointer: %p\n", arena->current);

  struct ArenaBlock *block = arena->head;
  while (block) {
    struct ArenaBlock *next = block->next;
    printf("Block: %p, Index: %zu, Capacity: %zu, Next: %p\n", block,
           block->index, block->capacity, block->next);
    block = next;
  }

  // Reset (keep blocks but reuse memory)
  arena_reset(arena);

  // Allocate again after reset
  char *msg = arena_alloc(arena, 64, alignof(char));
  strcpy(msg, "Arena reset reused memory successfully!");
  printf("Message: %s\n", msg);

  // Free all
  arena_free(arena);

  return 0;
}
