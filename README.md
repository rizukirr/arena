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
4. **Cleanup**: Free all blocks with a single call

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Block 1   │────▶│   Block 2   │────▶│   Block 3   │
│ [=========  ]│     │ [======     ]│     │ [==         ]│
└─────────────┘     └─────────────┘     └─────────────┘
      ▲                                        ▲
      │                                        │
     head                                   current
```

## Usage

### 1. Include the header

```c
#define ARENA_IMPLEMENTATION  // Define this in exactly ONE .c file
#include "arena.h"
```

### 2. Initialize an arena

```c
struct Arena *arena = arena_init(4096);  // 4KB default block size
if (arena == NULL) {
    // Handle allocation failure
}
```

### 3. Allocate memory

```c
// Allocate 100 bytes with 8-byte alignment
void *ptr = arena_alloc(arena, 100, 8);

// Allocate typed objects
int *numbers = arena_alloc(arena, sizeof(int) * 10, sizeof(int));
char *string = arena_alloc(arena, 256, 1);
```

### 4. Use the memory

```c
for (int i = 0; i < 10; i++) {
    numbers[i] = i * 2;
}
strcpy(string, "Hello from arena!");
```

### 5. Reset or free

```c
// Option 1: Reset to reuse the arena (keeps allocated blocks)
arena_reset(arena);

// Option 2: Free everything when done
arena_free(arena);
```

## API Reference

### `arena_init`
```c
struct Arena *arena_init(size_t default_block_size);
```
Creates a new arena allocator with the specified default block size.

- **Parameters**: `default_block_size` - Size in bytes for new blocks
- **Returns**: Pointer to arena, or `NULL` on failure
- **Note**: If an allocation is larger than the default size, a bigger block is created

### `arena_alloc`
```c
void *arena_alloc(struct Arena *arena, size_t size, size_t alignment);
```
Allocates memory from the arena with proper alignment.

- **Parameters**:
  - `arena` - The arena to allocate from
  - `size` - Number of bytes to allocate
  - `alignment` - Alignment requirement (e.g., 1, 4, 8, 16)
- **Returns**: Pointer to allocated memory, or `NULL` on failure

### `arena_reset`
```c
void arena_reset(struct Arena *arena);
```
Resets all blocks to reuse the arena without freeing memory.

- **Parameters**: `arena` - The arena to reset
- **Note**: This is very fast - just resets indices, doesn't touch the actual memory

### `arena_free`
```c
void arena_free(struct Arena *arena);
```
Frees all memory associated with the arena.

- **Parameters**: `arena` - The arena to free
- **Note**: All pointers obtained from the arena become invalid

## Complete Example

```c
#define ARENA_IMPLEMENTATION
#include "arena.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    char *name;
    int age;
} Person;

int main() {
    // Create arena with 1KB blocks
    struct Arena *arena = arena_init(1024);

    // Allocate some people
    Person *people = arena_alloc(arena, sizeof(Person) * 3, sizeof(Person));

    // Allocate strings for names
    people[0].name = arena_alloc(arena, 20, 1);
    people[1].name = arena_alloc(arena, 20, 1);
    people[2].name = arena_alloc(arena, 20, 1);

    // Fill in data
    strcpy(people[0].name, "Alice");
    people[0].age = 30;

    strcpy(people[1].name, "Bob");
    people[1].age = 25;

    strcpy(people[2].name, "Charlie");
    people[2].age = 35;

    // Use the data
    for (int i = 0; i < 3; i++) {
        printf("%s is %d years old\n", people[i].name, people[i].age);
    }

    // Clean up everything at once
    arena_free(arena);

    return 0;
}
```

## Best Practices

1. **Choose appropriate block sizes**: Larger blocks reduce allocation overhead but may waste memory
2. **Align properly**: Use proper alignment for your data types (typically `sizeof(type)`)
3. **Don't mix arenas and malloc**: Objects allocated from the arena cannot be individually freed
4. **Reset when possible**: Use `arena_reset()` instead of `arena_free()` in loops for better performance
5. **One arena per lifetime**: Group allocations that should live and die together

## Limitations

- No individual deallocation (free entire arena or nothing)
- May waste memory if allocations vary greatly in size
- Not thread-safe (requires external synchronization)
- Cannot shrink allocated memory

## License

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

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
