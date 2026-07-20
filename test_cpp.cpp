// Guard: arena.h wraps its API in extern "C", so it must compile as C++.
// ARENA_ALIGNOF is exercised in real allocations rather than merely compiled.
#define ARENA_IMPLEMENTATION
#include "arena.h"

#include <cstdio>

int main() {
  Arena *arena = arena_create(1024);
  if (!arena)
    return 1;

  int *n = static_cast<int *>(
      arena_alloc(arena, sizeof(int) * 4, ARENA_ALIGNOF(int)));
  double *d = static_cast<double *>(
      arena_alloc(arena, sizeof(double), ARENA_ALIGNOF(double)));
  if (!n || !d)
    return 1;

  n[0] = 7;
  *d = 3.5;

  ArenaCheckpoint cp = arena_checkpoint(arena);
  arena_alloc(arena, 128, ARENA_ALIGNOF(double));
  arena_restore(arena, cp);

  const bool ok = (n[0] == 7) && (*d == 3.5);
  arena_free(arena);

  std::printf("c++ compile guard OK\n");
  return ok ? 0 : 1;
}
