#define ARENA_IMPLEMENTATION
#include "../arena.h"
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

int main() {
  struct Arena *arena = arena_init(1024); // 1 KB blocks

  if (!arena) {
    printf("Failed to create arena\n");
    return 1;
  }

  // Allocate some memory
  char *name = arena_alloc(arena, 32, ARENA_ALIGNOF(char));
  strcpy(name, "Rizki Rakasiwi");

  int *numbers = arena_alloc(arena, 5 * sizeof(int), ARENA_ALIGNOF(int));
  for (int i = 0; i < 5; i++)
    numbers[i] = i * 10;

  printf("Name: %s\n", name);
  printf("Numbers:");
  for (int i = 0; i < 5; i++)
    printf(" %d", numbers[i]);
  printf("\n");

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
