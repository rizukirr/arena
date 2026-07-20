/*
 * Copyright (c) 2025 Rizki <rizkirr.xyz@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * arena23.h — C23-native variant of arena.h.
 *
 * Identical API, identical semantics, identical memory layout. The difference
 * is that this file is written in C23 rather than in portable C with
 * compatibility shims:
 *
 *   - <stdckdint.h> ckd_add() replaces the hand-rolled overflow helper
 *   - nullptr, bool, alignof and static_assert are used as keywords, so
 *     <stdbool.h> and <stdalign.h> are not needed
 *   - [[nodiscard]] marks the returns that must not be dropped
 *   - a flexible array member expresses the block payload directly
 *
 * There is deliberately no C11/C99 fallback and no C++ support: a header that
 * includes <stdckdint.h> cannot compile as C++, so an `extern "C"` wrapper here
 * would advertise a guarantee it cannot keep. Use arena.h for C99/C11/C++.
 */

#ifndef ARENA23_H
#define ARENA23_H

#if defined(ARENA_H)
#error "arena.h and arena23.h define the same symbols; include exactly one."
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#error "arena23.h requires C23 (-std=c23). Use arena.h for older standards."
#endif

#include <assert.h>
#include <stddef.h>
#include <stdckdint.h>
#include <stdint.h>
#include <stdlib.h>

/* Allocator hooks. Define either macro before including this header to
   substitute your own allocator. */
#ifdef _WIN32
#include <windows.h>

#ifndef ARENA_MALLOC
#define ARENA_MALLOC(size) HeapAlloc(GetProcessHeap(), 0, (size))
#endif

#ifndef ARENA_FREE
#define ARENA_FREE(ptr)                     \
  do {                                      \
    if (ptr)                                \
      HeapFree(GetProcessHeap(), 0, (ptr)); \
  } while (0)
#endif

#else /* Linux / macOS / POSIX */

#ifndef ARENA_MALLOC
#define ARENA_MALLOC(size) malloc(size)
#endif

#ifndef ARENA_FREE
#define ARENA_FREE(ptr) free(ptr)
#endif

#endif

/**
 * @brief Get the alignment of a type.
 *
 * `alignof` is a keyword in C23, so this is a thin alias kept only so that code
 * written against arena.h ports across unchanged.
 *
 * @param type  Any C type whose alignment is needed.
 */
#define ARENA_ALIGNOF(type) alignof(type)

// -----------------------------------------------------------------------------
// PUBLIC API (opaque handles)
// -----------------------------------------------------------------------------

/**
 * @brief Opaque handle for an Arena allocator.
 *
 * The internal structure is hidden from users unless
 * `ARENA_IMPLEMENTATION` is defined. The arena manages memory using
 * fixed-size blocks and fast bump-pointer allocation.
 */
typedef struct Arena Arena;

/**
 * @brief Checkpoint structure for saving/restoring arena state.
 *
 * Represents a specific point in the arena's allocation history.
 * Can be used to restore the arena to a previous state, effectively
 * freeing all allocations made after the checkpoint while keeping
 * allocations made before it.
 */
typedef struct ArenaCheckpoint {
  struct ArenaBlock *block; // Block pointer at checkpoint
  size_t index;             // Index within block at checkpoint
} ArenaCheckpoint;

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
 * @return Pointer to a newly initialized Arena, or nullptr if allocation fails.
 */
[[nodiscard]] Arena *arena_create(size_t default_block_size);

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
 * @return Pointer to allocated memory, or nullptr on failure.
 *
 * Deliberately not [[nodiscard]]: unlike malloc, discarding this pointer does
 * not leak. The arena owns the memory and arena_free() reclaims it, so
 * advancing the bump pointer and ignoring the result is a legitimate use.
 */
void *arena_alloc(Arena *arena, size_t size, size_t alignment);

/**
 * @brief Reset the arena state for reuse.
 *
 * All blocks remain allocated, but their internal `index` pointers are reset
 * to zero. This effectively frees all previously allocated memory but retains
 * the capacity.
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

/**
 * @brief Save current arena state as a checkpoint.
 *
 * Returns a checkpoint representing the current allocation position.
 * Allocations made after this point can be freed by restoring to this
 * checkpoint using arena_restore(), while allocations made before remain
 * intact.
 *
 * Supports nested checkpoints. Blocks are rewound rather than freed, so a
 * checkpoint stays valid for the lifetime of the arena and restoring is
 * memory-safe in any order. Restore in LIFO order for the resulting
 * allocation position to be meaningful.
 *
 * @param arena Pointer to Arena instance
 * @return Checkpoint representing current state
 *
 * @example Basic usage:
 *   Arena *arena = arena_create(4096);
 *   void *persistent = arena_alloc(arena, 1024, 8);
 *
 *   ArenaCheckpoint cp = arena_checkpoint(arena);
 *
 *   for (int i = 0; i < 1000; i++) {
 *       void *temp = arena_alloc(arena, 512, 8);
 *       // Use temp...
 *       arena_restore(arena, cp);  // Reclaim temp, keep persistent
 *   }
 */
[[nodiscard]] ArenaCheckpoint arena_checkpoint(Arena *arena);

/**
 * @brief Restore arena to a previous checkpoint.
 *
 * Resets the arena's allocation position to the saved checkpoint state.
 * All allocations made after the checkpoint are effectively freed
 * (their memory becomes available for reuse).
 *
 * IMPORTANT:
 * - The checkpoint must be valid (from the same arena)
 * - Blocks are rewound, never freed; memory is returned to the system only by
 *   arena_free(). A checkpoint's block pointer therefore stays valid for the
 *   lifetime of the arena.
 * - Restore is memory-safe in any order, but checkpoints must be restored in
 *   LIFO order for the resulting allocation position to be meaningful.
 * - Using a checkpoint after arena_free() is undefined behavior
 * - Debug builds include validation checks via assertions
 *
 * @param arena Pointer to Arena instance
 * @param checkpoint Previously saved checkpoint from arena_checkpoint()
 */
void arena_restore(Arena *arena, ArenaCheckpoint checkpoint);

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
 *
 * The flexible array member is standard C and is used directly here. arena.h
 * avoids it only because ISO C++ forbids it and that header must be includable
 * from C++; this one has no such constraint. The layout is identical either
 * way — the payload begins at `sizeof(struct ArenaBlock)`.
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

static_assert(sizeof(size_t) <= sizeof(uintptr_t),
              "arena23: size_t must fit in uintptr_t for padding arithmetic");

/**
 * @brief Compute padding needed to align a pointer.
 *
 * This uses a bitmask trick (requires alignment to be power of two):
 *
 *   padding = (-ptr) & (alignment - 1)
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
[[nodiscard]] static size_t arena_align_padding(uintptr_t ptr,
                                                size_t alignment) {
  return ((size_t)0 - (size_t)ptr) & (alignment - 1);
}

/* General-purpose blocks are always default_block_size; a request too large to
   fit one gets its own exact-fit dedicated block. Keeping the two cases
   separate stops one large allocation from inflating every later block. */
[[nodiscard]] static size_t arena_block_size_for(const Arena *arena,
                                                 size_t min_needed) {
  return min_needed > arena->default_block_size ? min_needed
                                                : arena->default_block_size;
}

[[nodiscard]] static struct ArenaBlock *arena_block_create(size_t capacity) {
  size_t total_size = 0;
  /* ckd_add returns true on overflow and still stores the wrapped result. */
  if (ckd_add(&total_size, sizeof(struct ArenaBlock), capacity))
    return nullptr;

  struct ArenaBlock *block = ARENA_MALLOC(total_size);
  if (!block)
    return nullptr;

  block->next = nullptr;
  block->capacity = capacity;
  block->index = 0;
  return block;
}

Arena *arena_create(size_t default_block_size) {
  if (default_block_size == 0)
    return nullptr;

  Arena *arena = ARENA_MALLOC(sizeof(Arena));
  if (!arena)
    return nullptr;

  arena->head = nullptr;
  arena->current = nullptr;
  arena->default_block_size = default_block_size;
  return arena;
}

void *arena_alloc(Arena *arena, size_t size, size_t alignment) {
  if (!arena || size == 0 || alignment == 0)
    return nullptr;

  // Ensure alignment is power of two.
  if (alignment & (alignment - 1))
    return nullptr;

  /* size + alignment - 1 is what guarantees an over-aligned request fits the
     block reserved for it; without the slack a padded request could never be
     satisfied and the allocator would chain blocks forever. */
  size_t min_needed = 0;
  if (ckd_add(&min_needed, size, alignment - 1))
    return nullptr;

  // Lazily allocate first block.
  if (!arena->current) {
    struct ArenaBlock *block =
        arena_block_create(arena_block_size_for(arena, min_needed));
    if (!block)
      return nullptr;
    arena->head = arena->current = block;
  }

  for (;;) {
    // Compute padding for alignment in the current block.
    uintptr_t current_ptr =
        (uintptr_t)(arena->current->data + arena->current->index);
    size_t padding = arena_align_padding(current_ptr, alignment);

    size_t used = 0;
    if (ckd_add(&used, arena->current->index, padding) ||
        ckd_add(&used, used, size))
      return nullptr;

    if (used <= arena->current->capacity) {
      arena->current->index += padding;
      void *ptr = arena->current->data + arena->current->index;
      arena->current->index += size;
      return ptr;
    }

    // Reuse existing next block (important after arena_reset()).
    if (arena->current->next) {
      arena->current = arena->current->next;
      continue;
    }

    struct ArenaBlock *new_block =
        arena_block_create(arena_block_size_for(arena, min_needed));
    if (!new_block)
      return nullptr;

    arena->current->next = new_block;
    arena->current = new_block;
  }
}

/* Rewind a chain of blocks to empty without releasing any of them. */
static void arena_rewind_chain(struct ArenaBlock *block) {
  while (block) {
    block->index = 0;
    block = block->next;
  }
}

void arena_reset(Arena *arena) {
  if (!arena)
    return;

  arena_rewind_chain(arena->head);
  arena->current = arena->head;
}

void arena_free(Arena *arena) {
  if (!arena)
    return;

  struct ArenaBlock *block = arena->head;
  while (block) {
    struct ArenaBlock *next = block->next;
    ARENA_FREE(block);
    block = next;
  }
  ARENA_FREE(arena);
}

ArenaCheckpoint arena_checkpoint(Arena *arena) {
  ArenaCheckpoint cp = {nullptr, 0};

  assert(arena != nullptr && "arena_checkpoint: arena is NULL");
  if (!arena || !arena->current)
    return cp; // Arena not yet allocated: zero checkpoint means "empty state".

  cp.block = arena->current;
  cp.index = arena->current->index;
  return cp;
}

void arena_restore(Arena *arena, ArenaCheckpoint checkpoint) {
  assert(arena != nullptr && "arena_restore: arena is NULL");
  if (!arena)
    return;

  // Checkpoint taken before the first allocation: rewind everything.
  if (checkpoint.block == nullptr) {
    assert(checkpoint.index == 0 &&
           "arena_restore: invalid empty-state checkpoint index");
    arena_reset(arena);
    return;
  }

// Debug validation: ensure checkpoint belongs to this arena.
#ifndef NDEBUG
  struct ArenaBlock *owned = arena->head;
  bool found = false;
  while (owned) {
    if (owned == checkpoint.block) {
      found = true;
      break;
    }
    owned = owned->next;
  }
  assert(found && "arena_restore: checkpoint does not belong to this arena");
  assert(checkpoint.index <= checkpoint.block->capacity &&
         "arena_restore: checkpoint index is invalid");
#endif

  // Blocks after the checkpoint are rewound, never freed: freeing them would
  // dangle any outstanding checkpoint pointing into them. Only arena_free()
  // returns memory to the system.
  arena_rewind_chain(checkpoint.block->next);
  checkpoint.block->index = checkpoint.index;
  arena->current = checkpoint.block;
}

#endif // ARENA_IMPLEMENTATION

#endif // ARENA23_H
