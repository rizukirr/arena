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

## API Reference

### Creating and Destroying

```c
Arrayd *arrayd_new(int initial_length);
void arrayd_clear(Arrayd *arrayd);
```

- `arrayd_new()`: Creates a new dynamic array with the specified initial capacity
- `arrayd_clear()`: Frees the array and all its internal memory

### Adding and Removing Elements

```c
int arrayd_append(Arrayd *array, void *data);
void arrayd_remove_at(Arrayd *arr, unsigned int index);
int arrayd_put_at(Arrayd *arr, unsigned int index, void *data);
```

- `arrayd_append()`: Adds an element to the end of the array, returns 0 on success or -1 on failure
- `arrayd_remove_at()`: Removes the element at the specified index, shifts remaining elements left
- `arrayd_put_at()`: Replaces the element at the specified index, returns 0 on success or -1 on failure

### Accessing Elements

```c
void *arrayd_get(Arrayd *arrayd, const unsigned int index);
unsigned int arrayd_count(Arrayd *arr);
```

- `arrayd_get()`: Returns the element at the specified index, or NULL if index is invalid
- `arrayd_count()`: Returns the current number of elements in the array

## Usage Example

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
    unsigned int count = arrayd_count(people); // Returns 2

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
    for (unsigned int i = 0; i < arrayd_count(people); i++) {
        free(arrayd_get(people, i));
    }
    arrayd_clear(people);

    return 0;
}
```

## Important Notes

- The array stores **pointers only** - you are responsible for allocating and freeing the actual objects
- `arrayd_clear()` only frees the array structure itself, not the objects it points to
- The array automatically doubles in size when it reaches capacity
- All index-based operations perform bounds checking and return error codes or NULL on failure

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
