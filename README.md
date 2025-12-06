# Arena Allocator

A simple, efficient memory arena allocator for C projects. Arena allocators provide fast memory allocation with minimal overhead and easy cleanup - perfect for applications that allocate many small objects with similar lifetimes.

## Introduction

An arena allocator (also known as a region-based allocator or bump allocator) is a memory management strategy where you allocate memory from large pre-allocated blocks. Instead of calling `malloc()` and `free()` for every allocation, you request memory from the arena, and when you're done, you free the entire arena at once. You can simply copy paste `arena.h` into your project

### Benefits

- **Fast allocation**: O(1) allocation time with minimal overhead
- **No fragmentation**: Linear allocation eliminates external fragmentation
- **Simple cleanup**: Free all allocations with a single call
- **Cache-friendly**: Sequential allocations improve memory locality
- **No per-allocation metadata**: Lower memory overhead than malloc

### Use Cases

- Temporary data structures (parsing, compilation passes)
- Request/response cycles in servers
- Game frame allocations
- String building and formatting
- Any scenario where many allocations share the same lifetime

## How It Works

The arena allocator manages memory through a linked list of blocks:

1. **Initialization**: Create an arena with a default block size
2. **Allocation**: Request memory with size and alignment
   - If space is available in the current block, bump the pointer and return
   - If not, allocate a new block and add it to the chain
3. **Reset**: Zero all block indices to reuse the memory
4. **Checkpoint**: Save the current allocation position
5. **Restore**: Restore the arena to a previously saved checkpoint
6. **Cleanup**: Free all blocks with a single call

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Block 1   │────▶│   Block 2   │────▶│   Block 3   │
│ [=========  ]│     │ [======     ]│     │ [==         ]│
└─────────────┘     └─────────────┘     └─────────────┘
      ▲                                        ▲
      │                                        │
     head                                   current
```


## Usage Pattern: Scoped Lifetimes

**This arena is designed for scoped lifetime allocation.** Allocate temporary data within a scope, use it, then free everything at once.

### Correct Usage

```c
void process_request(Request *req) {
    Arena *arena = arena_create(4096);

    // Allocate temporary objects
    char *buffer = arena_alloc(arena, 1024, 8);
    Node *ast = parse_with_arena(arena, req->data);
    Result *result = process_ast(arena, ast);

    // Use the data
    send_response(result);

    // Free everything at once
    arena_free(arena);
}
```

### Reusing Arena with Reset

```c
Arena *arena = arena_create(4096);

for (int i = 0; i < 1000; i++) {
    // Allocate temporary data for this iteration
    Data *temp = process_item(arena, items[i]);
    use_data(temp);

    // Reuse the arena for next iteration
    arena_reset(arena);
}

arena_free(arena);
```

### Checkpoint and Restore

For more fine-grained control over memory lifetimes, the arena supports checkpoints. A checkpoint saves the current allocation position, allowing you to roll back all subsequent allocations without affecting those made before the checkpoint.

This is useful for temporary allocations within a larger scope where you don't want to reset the entire arena.

```c
Arena *arena = arena_create(8192);

// Allocate some persistent data
Config *config = arena_alloc(arena, sizeof(Config), 8);

// Save the current state
ArenaCheckpoint cp = arena_checkpoint(arena);

// Perform some temporary allocations
for (int i = 0; i < 100; i++) {
    void *temp_data = arena_alloc(arena, 128, 8);
    // ... use temp_data

    // Restore the arena, freeing temp_data
    arena_restore(arena, cp);
}

// config is still valid here

arena_free(arena);
```

### Anti-Patterns (Don't Do This)

```c
// BAD: Long-lived arena with mixed lifetimes
Arena *global_arena = arena_create(4096);

void function_a() {
    char *data = arena_alloc(global_arena, 100, 8);
    // data lives forever, can't free it individually
}

void function_b() {
    int *nums = arena_alloc(global_arena, 1000, 4);
    // nums also lives forever
}
// Arena keeps growing, never freed
```

```c
// BAD: Trying to free individual allocations
void *ptr = arena_alloc(arena, 100, 8);
free(ptr);  // WRONG! This will corrupt memory
```

## Best Practices

1. **Use scoped lifetimes**: Create arena at scope start, free at scope end
2. **One arena per scope**: Group allocations that should live and die together
3. **Choose appropriate block sizes**: Larger blocks reduce allocation overhead
4. **Align properly**: Use `ARENA_ALIGNOF(type)` for type-safe alignment
5. **Reset in loops**: Use `arena_reset()` to reuse memory across iterations
6. **Free the arena**: Always call `arena_free()` when done - this is your only cleanup

## Design Characteristics

These are **intentional design choices**, not limitations:

| Characteristic | Why It's Fine for Scoped Lifetimes |
|----------------|-------------------------------------|
| **No individual deallocation** | You free the entire arena at scope end |
| **Cannot shrink memory** | Arena is freed soon anyway |
| **Varying size allocations may waste space** | Waste is temporary and bounded by scope |
| **Not thread-safe** | Use one arena per thread or add external locking |

## When NOT to Use Arena

- Long-lived objects with different lifetimes
- Need to free individual allocations
- Unpredictable memory growth without clear cleanup point
- Shared mutable state across threads (without external sync)

**For these cases, use `malloc/free` or a different allocator.**

## When TO Use Arena

- Per-request allocation in servers
- Compiler passes (per compilation unit)
- Game/render frame allocations
- Parsing temporary data
- Test fixtures (per test)
- Any temporary data with clear start/end scope

# Dynamic Array (Arrayd)

A simple dynamic array implementation for storing pointers in C. This header-only library provides automatic resizing and basic array operations.

## Features

- **Dynamic resizing**: Automatically doubles capacity when full
- **Simple API**: Easy-to-use functions for common array operations
- **Header-only**: Just include the header with `ARRAYD_IMPLEMENTATION` defined once
- **Pointer storage**: Stores `void*` pointers for maximum flexibility
- **Helper macros**: Convenient macros for working with primitive types (int, float, double, char, etc.)

## API Reference

### Creating and Destroying

```c
Arrayd *arrayd_new(int initial_length);
void arrayd_clear(Arrayd *arrayd);
```

- `arrayd_new()`: Creates a new dynamic array with the specified initial capacity. Aborts if initial_length is not positive or memory allocation fails.
- `arrayd_clear()`: Frees the array and all its internal memory. Aborts if array is NULL.

### Adding and Removing Elements

```c
void arrayd_append(Arrayd *array, void *data);
void arrayd_remove_at(Arrayd *arr, size_t index);
int arrayd_put_at(Arrayd *arr, size_t index, void *data);
```

- `arrayd_append()`: Adds an element to the end of the array. Aborts if array is NULL or memory allocation fails.
- `arrayd_remove_at()`: Removes the element at the specified index, shifts remaining elements left. Aborts if index is out of bounds.
- `arrayd_put_at()`: Replaces the element at the specified index. Aborts if index is out of bounds. Returns 0 on success.

### Accessing Elements

```c
void *arrayd_get(Arrayd *arrayd, const size_t index);
size_t arrayd_count(Arrayd *arr);
```

- `arrayd_get()`: Returns the element at the specified index. Aborts if index is out of bounds.
- `arrayd_count()`: Returns the current number of elements in the array. Aborts if array is NULL.

### Helper Macros for Primitive Types

The library provides convenient macros for working with primitive types:

#### Append Macros
```c
arrayd_append_int(arrayd, val)       // Append int
arrayd_append_char(arrayd, val)      // Append char
arrayd_append_short(arrayd, val)     // Append short
arrayd_append_long(arrayd, val)      // Append long
arrayd_append_long_long(arrayd, val) // Append long long
arrayd_append_float(arrayd, val)     // Append float
arrayd_append_double(arrayd, val)    // Append double
arrayd_append_string(arrayd, str)    // Append string (char*)
```

#### Get Macros
```c
arrayd_get_int(arrayd, index)        // Get int
arrayd_get_char(arrayd, index)       // Get char
arrayd_get_short(arrayd, index)      // Get short
arrayd_get_long(arrayd, index)       // Get long
arrayd_get_long_long(arrayd, index)  // Get long long
arrayd_get_float(arrayd, index)      // Get float
arrayd_get_double(arrayd, index)     // Get double
arrayd_get_string(arrayd, index)     // Get string (char*)
```

#### Put Macros
```c
arrayd_put_at_int(arrayd, index, val)        // Replace with int
arrayd_put_at_char(arrayd, index, val)       // Replace with char
arrayd_put_at_short(arrayd, index, val)      // Replace with short
arrayd_put_at_long(arrayd, index, val)       // Replace with long
arrayd_put_at_long_long(arrayd, index, val)  // Replace with long long
arrayd_put_at_float(arrayd, index, val)      // Replace with float
arrayd_put_at_double(arrayd, index, val)     // Replace with double
arrayd_put_at_string(arrayd, index, str)     // Replace with string (char*)
```

## Usage Examples

### Example 1: Storing Pointers

```c
#define ARRAYD_IMPLEMENTATION
#include "arrayd.h"

typedef struct {
    int id;
    char *name;
} Person;

int main() {
    // Create array with initial capacity of 10
    Arrayd *people = arrayd_new(10);

    // Add some people
    Person *p1 = malloc(sizeof(Person));
    p1->id = 1;
    p1->name = "Alice";
    arrayd_append(people, p1);

    Person *p2 = malloc(sizeof(Person));
    p2->id = 2;
    p2->name = "Bob";
    arrayd_append(people, p2);

    // Get count
    size_t count = arrayd_count(people); // Returns 2

    // Access elements
    Person *first = arrayd_get(people, 0);
    printf("First person: %s\n", first->name);

    // Replace an element
    Person *p3 = malloc(sizeof(Person));
    p3->id = 3;
    p3->name = "Charlie";
    arrayd_put_at(people, 1, p3);

    // Remove an element
    arrayd_remove_at(people, 0);

    // Clean up (note: you must free the stored objects yourself)
    for (size_t i = 0; i < arrayd_count(people); i++) {
        free(arrayd_get(people, i));
    }
    arrayd_clear(people);

    return 0;
}
```

### Example 2: Using Helper Macros for Primitive Types

```c
#define ARRAYD_IMPLEMENTATION
#include "arrayd.h"
#include <stdio.h>

int main() {
    // Create arrays for different types
    Arrayd *numbers = arrayd_new(10);
    Arrayd *strings = arrayd_new(5);
    Arrayd *floats = arrayd_new(5);

    // Add integers using helper macro
    arrayd_append_int(numbers, 42);
    arrayd_append_int(numbers, 100);
    arrayd_append_int(numbers, -5);

    // Add strings using helper macro
    arrayd_append_string(strings, "Hello");
    arrayd_append_string(strings, "World");

    // Add floats using helper macro
    arrayd_append_float(floats, 3.14f);
    arrayd_append_float(floats, 2.71f);

    // Access elements using helper macros
    printf("First number: %ld\n", arrayd_get_int(numbers, 0));
    printf("First string: %s\n", arrayd_get_string(strings, 0));
    printf("First float: %.2f\n", arrayd_get_float(floats, 0));

    // Modify elements
    arrayd_put_at_int(numbers, 1, 200);
    printf("Modified number: %ld\n", arrayd_get_int(numbers, 1));

    // Clean up
    arrayd_clear(numbers);
    arrayd_clear(strings);
    arrayd_clear(floats);

    return 0;
}
```

## Important Notes

- The array stores **pointers only** (or primitive values encoded as pointers via helper macros)
- When storing objects: you are responsible for allocating and freeing them
- `arrayd_clear()` only frees the array structure itself, not the objects it points to
- The array automatically doubles in size when it reaches capacity
- **All functions use assertions** - the program will abort on errors:
  - NULL array or data pointer
  - Out of bounds index access
  - Memory allocation failures
  - Invalid initial length (must be positive)
- All index parameters use `size_t` type
- `arrayd_put_at()` returns 0 on success (assertions handle errors)

## When to Use Arrayd

- Need a simple growable array of pointers
- Want automatic memory management for the array structure
- Don't need complex data structure features (sorting, searching, etc.)
- Working with heterogeneous pointer types

## When NOT to Use Arrayd

- Need type-safety (use typed arrays or macros instead)
- Need to store values inline (not pointers)
- Need advanced features like sorting or binary search
- Working with primitive types (consider a specialized implementation)

## License

Licensed under the Apache License, Version 2.0 (the "License") - see the [LICENSE](LICENSE) file for details.

## Support

If you find this project useful, consider supporting the development:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/rizukirr)

Your support helps maintain and improve this project!

## Contributing

Contributions are welcome! Feel free to:
- Report bugs
- Suggest improvements
- Submit pull requests
- Share how you're using the arena allocator

---

Made with ❤️ by rizukirr
