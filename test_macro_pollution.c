/* Guard: arena.h must not claim unprefixed identifiers in the global namespace.
   This translation unit defines its own FREE, CALLOC and MALLOC and must still
   compile after including the header. */
#define ARENA_IMPLEMENTATION
#include "arena.h"

#include <stdio.h>

typedef enum { FREE, IN_USE } SlotState;

struct Slot {
  SlotState state;
  int CALLOC;
};

static void MALLOC(void) { /* a local function that happens to be named MALLOC */
}

int main(void) {
  struct Slot slot;
  slot.state = FREE;
  slot.CALLOC = 0;
  MALLOC();

  Arena *arena = arena_create(1024);
  if (!arena)
    return 1;
  if (!arena_alloc(arena, 64, 8))
    return 1;
  arena_free(arena);

  printf("macro pollution guard OK\n");
  return (slot.state == FREE && slot.CALLOC == 0) ? 0 : 1;
}
