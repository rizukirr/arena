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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct ArenaBlock {
  struct ArenaBlock *next; // Point to the next block
  size_t capacity;         // How big is this specific block
  size_t index;            // Where is the current cursor on this block
  uint8_t data[];          // The actual memory
};
struct Arena {
  struct ArenaBlock *head;    // The first block (so we can free them all later)
  struct ArenaBlock *current; // The block we are currently writing on
  size_t default_block_size;  // The default block size
};

static inline struct Arena *arena_init(size_t default_block_size) {
  if (default_block_size == 0)
    return NULL;

  struct Arena *arena = (struct Arena *)calloc(1, sizeof(struct Arena));
  if (arena == NULL)
    return NULL;

  arena->default_block_size = default_block_size;
  return arena;
}

static inline void *arena_alloc(struct Arena *arena, size_t size,
                                size_t alignment) {
  if (arena == NULL || size == 0)
    return NULL;

  // If we don't have a current block, we need to allocate a new one
  if (arena->current == NULL) {
    size_t block_size =
        (size > arena->default_block_size) ? size : arena->default_block_size;

    struct ArenaBlock *block =
        (struct ArenaBlock *)malloc(sizeof(struct ArenaBlock) + block_size);
    if (block == NULL)
      return NULL;

    block->next = NULL;
    block->capacity = block_size;
    block->index = 0;

    arena->head = block;
    arena->current = block;
  }

  // Align the current index
  uintptr_t current_ptr =
      (uintptr_t)(arena->current->data + arena->current->index);
  uintptr_t offset = current_ptr % alignment;
  size_t padding = (offset == 0) ? 0 : (alignment - offset);

  // check if we fit int the current block
  if (arena->current->index + size + padding > arena->current->capacity) {
    // We are full, create a new block
    size_t next_capacity =
        (size > arena->default_block_size) ? size : arena->default_block_size;
    struct ArenaBlock *new_block =
        (struct ArenaBlock *)malloc(sizeof(struct ArenaBlock) + next_capacity);
    if (new_block == NULL)
      return NULL;

    new_block->next = NULL;
    new_block->capacity = next_capacity;
    new_block->index = 0;

    arena->current->next = new_block;
    arena->current = new_block;

    current_ptr = (uintptr_t)new_block->data;
    offset = current_ptr % alignment;
    padding = (offset == 0) ? 0 : (alignment - offset);
  }

  // Perform the actual allocation
  arena->current->index += padding;
  void *ptr = (void *)(arena->current->data + arena->current->index);
  arena->current->index += size;
  return ptr;
}

static inline void arena_reset(struct Arena *arena) {
  struct ArenaBlock *block = arena->head;
  while (block != NULL) {
    block->index = 0;
    block = block->next;
  }
  // Point current back to the start
  arena->current = arena->head;
}

static inline void arena_free(struct Arena *arena) {
  struct ArenaBlock *block = arena->head;
  while (block != NULL) {
    struct ArenaBlock *next = block->next;
    free(block); // Free the "Page"
    block = next;
  }
  free(arena);
}

#endif // ARENA_H
