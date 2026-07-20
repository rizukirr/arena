# Arena

A lightweight, header-only arena allocator for C with checkpoint/restore support.

## Overview

`arena.h` is a single-header library implementing a fast bump-pointer arena allocator. It manages memory in growable blocks and supports saving/restoring allocation state via checkpoints. Include the header and define `ARENA_IMPLEMENTATION` in exactly one translation unit.

## Features

- Fast bump-pointer allocation with minimal overhead
- Configurable default block size; over-sized requests get a dedicated block
- Power-of-two alignment handling for any type (`ARENA_ALIGNOF`)
- Checkpoint and restore for scoped/temporary allocations; checkpoints nest
  safely and are never invalidated by another restore
- Memory is returned to the system only by `arena_free()`; `arena_reset()` and
  `arena_restore()` rewind and retain blocks for reuse
- Custom allocator support via `ARENA_MALLOC` / `ARENA_FREE`
- Overflow-safe size arithmetic
- Portable: C99+ with C11/C23 alignment niceties; works on POSIX and Windows

## Usage

Include the header and define the implementation in one source file:

```c
#define ARENA_IMPLEMENTATION
#include "arena.h"
```

Basic usage:

```c
// Create arena with 4KB default blocks
Arena *arena = arena_create(4096);

// Allocate aligned memory
int  *numbers = arena_alloc(arena, sizeof(int) * 100, ARENA_ALIGNOF(int));
char *string  = arena_alloc(arena, 256, ARENA_ALIGNOF(char));

// Use a checkpoint for temporary allocations
ArenaCheckpoint cp = arena_checkpoint(arena);
void *temp = arena_alloc(arena, 1024, 8);
// ... use temp ...
arena_restore(arena, cp);  // Reclaims temp for reuse; earlier allocations remain

// Reset for reuse (keeps allocated blocks)
arena_reset(arena);

// Release everything
arena_free(arena);
```

See [`example/arena_example.c`](example/arena_example.c) for a runnable demo covering basic allocation, loop reuse, and nested checkpoints.

## Building

The library has no build system — just compile your code against `arena.h`. To build the example and tests:

```sh
cc -std=c11 -Wall -Wextra -O2 -o arena_example example/arena_example.c
cc -std=c11 -Wall -Wextra -O2 -o test_arena test_arena.c
./test_arena
```

The full check the library is developed against — the test suite under both
sanitizers, plus two guard translation units that would silently rot otherwise:

```sh
# Test suite under AddressSanitizer + UndefinedBehaviorSanitizer
cc -std=c11 -Wall -Wextra -Wpedantic -Wundef -fsanitize=address,undefined -g \
   -o test_arena test_arena.c && ./test_arena

# Guard: the header must not claim unprefixed global macros (FREE, MALLOC, ...)
cc -std=c11 -Wall -Wextra -o test_macro_pollution test_macro_pollution.c \
   && ./test_macro_pollution

# Guard: the header must compile and run as C++
g++ -std=c++17 -Wall -Wextra -Wpedantic -o test_cpp test_cpp.cpp && ./test_cpp
```

### Windows

The Windows branch (`HeapAlloc`/`HeapFree`) is verified by cross-compiling with
mingw-w64, which catches signature and macro-expansion errors in that branch:

```sh
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror -c test_arena.c -o /dev/null
i686-w64-mingw32-gcc   -std=c11 -Wall -Wextra -Werror -c test_arena.c -o /dev/null
```

Note that this is compile-and-link coverage only. The suite has not been
*executed* on Windows, so runtime behavior on that platform rests on the
POSIX runtime results plus the cross-compile above.

## API Reference

### Types

- `Arena` — opaque arena handle
- `ArenaCheckpoint` — saved allocation state (`{ block, index }`)

### Lifecycle

- `Arena *arena_create(size_t default_block_size)` — create a new arena. Blocks are allocated lazily on first `arena_alloc`.
- `void arena_reset(Arena *arena)` — reset all blocks to empty without freeing them.
- `void arena_free(Arena *arena)` — free all blocks and the arena itself.

### Allocation

- `void *arena_alloc(Arena *arena, size_t size, size_t alignment)` — allocate `size` bytes with the given power-of-two alignment. Returns `NULL` on invalid arguments or allocation failure.

### Checkpoints

- `ArenaCheckpoint arena_checkpoint(Arena *arena)` — capture current allocation position.
- `void arena_restore(Arena *arena, ArenaCheckpoint cp)` — roll the arena back to `cp`. Blocks created after `cp` are rewound and kept for reuse, never freed, so a checkpoint stays valid for the lifetime of the arena. Restore is memory-safe in any order, but checkpoints should be restored in LIFO order for the resulting position to be meaningful. Peak memory is retained until `arena_reset()` or `arena_free()`.

### Macros

- `ARENA_ALIGNOF(type)` — portable alignment-of (uses `alignof` on C11+, falls back to the offset-of trick).
- `ARENA_MALLOC(size)` / `ARENA_FREE(ptr)` — allocator hooks. Define either before including `arena.h` to substitute your own allocator; both default to `malloc`/`free` on POSIX and `HeapAlloc`/`HeapFree` on Windows.

## License

MIT License. See `LICENSE` and the header comment in `arena.h` for full text.

## Author

Copyright 2025 Rizki Rakasiwi <rizkirr.xyz@gmail.com>
