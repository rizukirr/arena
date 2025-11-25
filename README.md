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


## Best Practices

1. **Choose appropriate block sizes**: Larger blocks reduce allocation overhead but may waste memory
2. **Align properly**: Use proper alignment for your data types (typically `sizeof(type)`)
3. **Don't mix arenas and malloc**: Objects allocated from the arena cannot be individually freed
4. **Reset when possible**: Use `arena_reset()` instead of `arena_free()` in loops for better performance
5. **One arena per lifetime**: Group allocations that should live and die together

## Limitations (Not yet implemented)

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
