# Arena

A lightweight, header-only memory management library for C providing an arena allocator and dynamic array implementation.

## Overview

This library provides two main components:

- **Arena Allocator** (`arena.h`): Fast bump-pointer memory allocator with checkpoint/restore functionality
- **Dynamic Array** (`arrayd.h`): Generic dynamic array implementation for storing pointers

Both components are designed as single-header libraries using the implementation pattern. Simply include the header and define the implementation macro in one compilation unit.

## Features

### Arena Allocator

- Fast bump-pointer allocation with minimal overhead
- Configurable block sizes for memory efficiency
- Proper alignment handling for all allocations
- Checkpoint and restore support for temporary allocations
- Complete memory reuse via reset functionality
- Zero fragmentation within blocks

### Dynamic Array

- Automatic growth with 2x expansion strategy
- Type-safe macros for common C types
- Support for integers, floats, doubles, strings, and pointers
- Random access, insertion, and removal operations

## Usage

### Arena Allocator

Include the header and define the implementation in one source file:

```c
#define ARENA_IMPLEMENTATION
#include "arena.h"
```

Basic usage:

```c
// Create arena with 4KB blocks
Arena *arena = arena_create(4096);

// Allocate memory
int *numbers = arena_alloc(arena, sizeof(int) * 100, ARENA_ALIGNOF(int));
char *string = arena_alloc(arena, 256, ARENA_ALIGNOF(char));

// Use checkpoint for temporary allocations
ArenaCheckpoint cp = arena_checkpoint(arena);
void *temp = arena_alloc(arena, 1024, 8);
// ... use temp ...
arena_restore(arena, cp);  // Free temp, keep previous allocations

// Reset for reuse (keeps allocated blocks)
arena_reset(arena);

// Free all memory
arena_free(arena);
```

### Dynamic Array

Include the header and define the implementation in one source file:

```c
#define ARRAYD_IMPLEMENTATION
#include "arrayd.h"
```

Basic usage:

```c
// Create array with initial capacity of 10
Arrayd *arr = arrayd_new(10);

// Append various types
arrayd_append_int(arr, 42);
arrayd_append_string(arr, "hello");
arrayd_append_double(arr, 3.14);

// Access elements
int value = arrayd_get_int(arr, 0);
char *str = arrayd_get_string(arr, 1);
double pi = arrayd_get_double(arr, 2);

// Modify elements
arrayd_put_at_int(arr, 0, 100);

// Remove elements
arrayd_remove_at(arr, 1);

// Get count
size_t count = arrayd_count(arr);

// Free memory
arrayd_clear(arr);
```

## API Reference

### Arena Allocator API

**Arena Management**

- `Arena *arena_create(size_t default_block_size)` - Create a new arena with specified block size
- `void arena_free(Arena *arena)` - Free arena and all its blocks
- `void arena_reset(Arena *arena)` - Reset arena for reuse, keeping allocated blocks

**Memory Allocation**

- `void *arena_alloc(Arena *arena, size_t size, size_t alignment)` - Allocate aligned memory from arena

**Checkpoint/Restore**

- `ArenaCheckpoint arena_checkpoint(Arena *arena)` - Save current allocation state
- `void arena_restore(Arena *arena, ArenaCheckpoint checkpoint)` - Restore to previous checkpoint

**Helper Macros**

- `ARENA_ALIGNOF(type)` - Get alignment requirement for a type (C11 or fallback)

### Dynamic Array API

**Array Management**

- `Arrayd *arrayd_new(int initial_length)` - Create new array with initial capacity
- `void arrayd_clear(Arrayd *arrayd)` - Free array and its data
- `size_t arrayd_count(Arrayd *arr)` - Get number of elements

**Element Operations**

- `void arrayd_append(Arrayd *array, void *data)` - Append generic pointer
- `void *arrayd_get(Arrayd *arrayd, size_t index)` - Get element at index
- `int arrayd_put_at(Arrayd *arr, size_t index, void *data)` - Update element at index
- `void arrayd_remove_at(Arrayd *arr, size_t index)` - Remove element at index

**Type-Safe Macros**

Append operations:
- `arrayd_append_int(arrayd, val)`
- `arrayd_append_string(arrayd, str)`
- `arrayd_append_double(arrayd, val)`
- `arrayd_append_float(arrayd, val)`
- `arrayd_append_char(arrayd, val)`
- `arrayd_append_long(arrayd, val)`

Get operations:
- `arrayd_get_int(arrayd, index)`
- `arrayd_get_string(arrayd, index)`
- `arrayd_get_double(arrayd, index)`
- `arrayd_get_float(arrayd, index)`
- `arrayd_get_char(arrayd, index)`
- `arrayd_get_long(arrayd, index)`

Put operations:
- `arrayd_put_at_int(arrayd, index, val)`
- `arrayd_put_at_string(arrayd, index, str)`
- `arrayd_put_at_double(arrayd, index, val)`
- `arrayd_put_at_float(arrayd, index, val)`
- `arrayd_put_at_char(arrayd, index, val)`
- `arrayd_put_at_long(arrayd, index, val)`

## License

Licensed under the Apache License, Version 2.0. See the source files for full license text.

## Author

Copyright 2025 rizki rakasiwi <rizkirr.xyz@gmail.com>
