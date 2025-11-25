/**
 * @Copyright 2025 rizki rakasiwi <rizkirr.xyz@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ARENA_H
#define ARENA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// PUBLIC API (opaque handles)
// -----------------------------------------------------------------------------

/**
 * @brief Get alignment of a type in a portable way.
 *
 * This macro expands to `alignof(type)` when compiling under C11 or newer, and
 * otherwise computes alignment using struct offset hack. This ensures the arena
 * allocator can correctly align memory on all compilers.
 *
 * @param type  Any C type whose alignment is needed.
 */
#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#define ARENA_ALIGNOF(type) alignof(type)
#else
#define ARENA_ALIGNOF(type)                                                    \
  offsetof(                                                                    \
      struct {                                                                 \
        char c;                                                                \
        type d;                                                                \
      },                                                                       \
      d)
#endif

/**
 * @brief Opaque handle for an Arena allocator.
 *
 * The internal structure is hidden from users unless
 * `ARENA_IMPLEMENTATION` is defined. The arena manages memory using
 * fixed-size blocks and fast bump-pointer allocation.
 */
typedef struct Arena Arena;

/**
 * @brief Create a new arena allocator.
 *
 * This allocates an `Arena` structure but does **not** allocate any memory
 * blocks yet. Blocks are lazily allocated on the first call to `arena_alloc()`.
 *
 * @param default_block_size  The size (in bytes) of each allocated block.
 *                            Larger allocations will allocate a block sized
 *                            exactly large enough for the request.
 *
 * @return Pointer to a newly initialized Arena, or NULL if allocation fails.
 */
Arena *arena_init(size_t default_block_size);

/**
 * @brief Allocate memory from the arena with a specific alignment.
 *
 * The arena grows by allocating new blocks when needed. Allocations never
 * return memory to the system until `arena_free()` is called.
 *
 * @param arena      Pointer to a valid Arena instance.
 * @param size       Number of bytes to allocate.
 * @param alignment  Alignment requirement (must be power of two).
 *
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *arena_alloc(Arena *arena, size_t size, size_t alignment);

/**
 * @brief Reset the arena state for reuse.
 *
 * All blocks remain allocated, but their internal `index` pointers are reset
 * to zero. This effectively frees all previously allocated memory but retains
 * the capacity.
 *
 * Behavior note:
 * - The implementation resets **all blocks**.
 * - A different design may free all but the first block.
 *
 * @param arena  Pointer to an Arena instance.
 */
void arena_reset(Arena *arena);

/**
 * @brief Release all memory owned by the arena.
 *
 * This frees all blocks and the Arena structure itself. After this call,
 * the arena pointer must not be used.
 *
 * @param arena  Pointer to an Arena instance.
 */
void arena_free(Arena *arena);

// -----------------------------------------------------------------------------
// IMPLEMENTATION
// -----------------------------------------------------------------------------
#ifdef ARENA_IMPLEMENTATION

/**
 * @brief Internal structure representing a memory block.
 *
 * Each block contains:
 *   - `next` pointer (linked list)
 *   - `capacity` total size of the block
 *   - `index` current write position
 *   - `data[]` flexible array member (actual memory region)
 */
struct ArenaBlock {
  struct ArenaBlock *next;
  size_t capacity;
  size_t index;
  uint8_t data[];
};

/**
 * @brief Internal arena structure.
 *
 * Fields:
 *   - `head`    → first allocated block
 *   - `current` → block currently accepting allocations
 *   - `default_block_size` → minimum block size
 */
struct Arena {
  struct ArenaBlock *head;
  struct ArenaBlock *current;
  size_t default_block_size;
};

/**
 * @brief Compute padding needed to align a pointer.
 *
 * This uses a modulo trick:
 *
 *   padding = (alignment - (ptr % alignment)) % alignment
 *
 * This ensures:
 *   - If pointer is already aligned → padding = 0
 *   - Otherwise → padding = minimal offset to align
 *
 * @param ptr        Pointer value as integer.
 * @param alignment  Required alignment (must be power of two).
 *
 * @return Number of bytes of padding needed.
 */
static size_t align_up(uintptr_t ptr, size_t alignment) {
  return (alignment - (ptr % alignment)) % alignment;
}

Arena *arena_init(size_t default_block_size) {
  if (default_block_size == 0)
    return NULL;

  Arena *arena = (Arena *)calloc(1, sizeof(Arena));
  if (!arena)
    return NULL;

  arena->default_block_size = default_block_size;
  return arena;
}

void *arena_alloc(Arena *arena, size_t size, size_t alignment) {
  if (!arena || size == 0 || alignment == 0)
    return NULL;

  // Ensure alignment is power of two.
  if (alignment & (alignment - 1))
    return NULL;

  // Lazily allocate first block.
  if (!arena->current) {
    size_t block_size =
        (size > arena->default_block_size) ? size : arena->default_block_size;

    struct ArenaBlock *block =
        (struct ArenaBlock *)malloc(sizeof(struct ArenaBlock) + block_size);
    if (!block)
      return NULL;

    block->next = NULL;
    block->capacity = block_size;
    block->index = 0;

    arena->head = arena->current = block;
  }

  // Compute padding for alignment.
  uintptr_t current_ptr =
      (uintptr_t)(arena->current->data + arena->current->index);

  size_t padding = align_up(current_ptr, alignment);

  // If insufficient space, allocate a new block.
  if (arena->current->index + padding + size > arena->current->capacity) {

    size_t next_capacity =
        (size > arena->default_block_size) ? size : arena->default_block_size;

    struct ArenaBlock *new_block =
        (struct ArenaBlock *)malloc(sizeof(struct ArenaBlock) + next_capacity);
    if (!new_block)
      return NULL;

    new_block->next = NULL;
    new_block->capacity = next_capacity;
    new_block->index = 0;

    arena->current->next = new_block;
    arena->current = new_block;

    current_ptr = (uintptr_t)new_block->data;
    padding = align_up(current_ptr, alignment);
  }

  // Perform the allocation.
  arena->current->index += padding;
  void *ptr = arena->current->data + arena->current->index;
  arena->current->index += size;

  return ptr;
}

void arena_reset(Arena *arena) {
  if (!arena)
    return;

  struct ArenaBlock *block = arena->head;
  while (block) {
    block->index = 0;
    block = block->next;
  }
  arena->current = arena->head;
}

void arena_free(Arena *arena) {
  if (!arena)
    return;

  struct ArenaBlock *block = arena->head;
  while (block) {
    struct ArenaBlock *next = block->next;
    free(block);
    block = next;
  }
  free(arena);
}

#endif // ARENA_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // ARENA_H
