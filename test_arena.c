#define ARENA_IMPLEMENTATION
#include "arena.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TEST(name)                                                             \
  static void name();                                                          \
  static void name##_runner() {                                                \
    printf("Running test: %s\n", #name);                                       \
    name();                                                                    \
    printf("  ✓ %s passed\n", #name);                                          \
  }                                                                            \
  static void name()

#define RUN_TEST(name) name##_runner()

TEST(test_arena_create_valid) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);
  assert(arena->default_block_size == 1024);
  assert(arena->head == NULL);
  assert(arena->current == NULL);
  arena_free(arena);
}

TEST(test_arena_create_zero_size) {
  struct Arena *arena = arena_create(0);
  assert(arena == NULL);
}

TEST(test_arena_alloc_null_arena) {
  void *ptr = arena_alloc(NULL, 100, 8);
  assert(ptr == NULL);
}

TEST(test_arena_alloc_zero_size) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  void *ptr = arena_alloc(arena, 0, 8);
  assert(ptr == NULL);

  arena_free(arena);
}

TEST(test_arena_alloc_single_allocation) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  void *ptr = arena_alloc(arena, 100, 8);
  assert(ptr != NULL);
  assert(arena->head != NULL);
  assert(arena->current != NULL);
  assert(arena->head == arena->current);
  assert(arena->current->capacity == 1024);
  assert(arena->current->index == 100);

  arena_free(arena);
}

TEST(test_arena_alloc_multiple_allocations_same_block) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  void *ptr1 = arena_alloc(arena, 100, 8);
  void *ptr2 = arena_alloc(arena, 200, 8);
  void *ptr3 = arena_alloc(arena, 150, 8);

  assert(ptr1 != NULL);
  assert(ptr2 != NULL);
  assert(ptr3 != NULL);
  assert(arena->head == arena->current);
  assert(arena->current->index >= 450);

  arena_free(arena);
}

TEST(test_arena_alloc_larger_than_default) {
  struct Arena *arena = arena_create(512);
  assert(arena != NULL);

  void *ptr = arena_alloc(arena, 1024, 8);
  assert(ptr != NULL);
  assert(arena->head != NULL);
  assert(arena->current->capacity >= 1024);

  arena_free(arena);
}

TEST(test_arena_alloc_multiple_blocks) {
  struct Arena *arena = arena_create(512);
  assert(arena != NULL);

  void *ptr1 = arena_alloc(arena, 400, 8);
  assert(ptr1 != NULL);

  void *ptr2 = arena_alloc(arena, 400, 8);
  assert(ptr2 != NULL);

  assert(arena->head != NULL);
  assert(arena->current != NULL);
  assert(arena->head->next == arena->current);

  arena_free(arena);
}

TEST(test_arena_alloc_alignment_8) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  void *ptr1 = arena_alloc(arena, 10, 8);
  void *ptr2 = arena_alloc(arena, 10, 8);
  void *ptr3 = arena_alloc(arena, 10, 8);

  assert(ptr1 != NULL);
  assert(ptr2 != NULL);
  assert(ptr3 != NULL);

  assert(((uintptr_t)ptr1 % 8) == 0);
  assert(((uintptr_t)ptr2 % 8) == 0);
  assert(((uintptr_t)ptr3 % 8) == 0);

  arena_free(arena);
}

TEST(test_arena_alloc_alignment_16) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  void *ptr1 = arena_alloc(arena, 10, 16);
  void *ptr2 = arena_alloc(arena, 10, 16);

  assert(ptr1 != NULL);
  assert(ptr2 != NULL);
  assert(((uintptr_t)ptr1 % 16) == 0);
  assert(((uintptr_t)ptr2 % 16) == 0);

  arena_free(arena);
}

TEST(test_arena_alloc_alignment_1) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  void *ptr = arena_alloc(arena, 10, 1);
  assert(ptr != NULL);

  arena_free(arena);
}

TEST(test_arena_reset) {
  struct Arena *arena = arena_create(512);
  assert(arena != NULL);

  void *ptr1 = arena_alloc(arena, 400, 8);
  void *ptr2 = arena_alloc(arena, 400, 8);

  assert(ptr1 != NULL);
  assert(ptr2 != NULL);

  struct ArenaBlock *first_block = arena->head;

  arena_reset(arena);

  assert(arena->head == first_block);
  assert(arena->current == first_block);
  assert(arena->head->index == 0);
  if (arena->head->next != NULL) {
    assert(arena->head->next->index == 0);
  }

  void *ptr3 = arena_alloc(arena, 100, 8);
  assert(ptr3 != NULL);

  arena_free(arena);
}

TEST(test_arena_reset_multiple_blocks) {
  struct Arena *arena = arena_create(256);
  assert(arena != NULL);

  arena_alloc(arena, 200, 8);
  arena_alloc(arena, 200, 8);
  arena_alloc(arena, 200, 8);

  struct ArenaBlock *first_block = arena->head;

  arena_reset(arena);

  struct ArenaBlock *block = arena->head;
  while (block != NULL) {
    assert(block->index == 0);
    block = block->next;
  }

  assert(arena->current == first_block);

  arena_free(arena);
}

TEST(test_arena_data_integrity) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  int *num1 = arena_alloc(arena, sizeof(int), sizeof(int));
  int *num2 = arena_alloc(arena, sizeof(int), sizeof(int));
  char *str = arena_alloc(arena, 20, 1);

  assert(num1 != NULL);
  assert(num2 != NULL);
  assert(str != NULL);

  *num1 = 42;
  *num2 = 100;
  strcpy(str, "Hello, Arena!");

  assert(*num1 == 42);
  assert(*num2 == 100);
  assert(strcmp(str, "Hello, Arena!") == 0);

  arena_free(arena);
}

TEST(test_arena_stress_many_small_allocations) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  for (int i = 0; i < 100; i++) {
    void *ptr = arena_alloc(arena, 10, 8);
    assert(ptr != NULL);
  }

  arena_free(arena);
}

TEST(test_arena_checkpoint_restore_basic) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  int *persistent = arena_alloc(arena, sizeof(int), ARENA_ALIGNOF(int));
  assert(persistent != NULL);
  *persistent = 12345;

  ArenaCheckpoint cp = arena_checkpoint(arena);
  assert(cp.block != NULL);

  void *temp1 = arena_alloc(arena, 128, 8);
  void *temp2 = arena_alloc(arena, 256, 8);
  assert(temp1 != NULL);
  assert(temp2 != NULL);

  arena_restore(arena, cp);

  // Persistent allocation must still hold its value.
  assert(*persistent == 12345);

  // After restore the next allocation should reuse the freed region.
  void *reused = arena_alloc(arena, 128, 8);
  assert(reused == temp1);

  arena_free(arena);
}

TEST(test_arena_checkpoint_empty_state) {
  struct Arena *arena = arena_create(512);
  assert(arena != NULL);

  // Checkpoint before any allocation -> empty state checkpoint.
  ArenaCheckpoint cp = arena_checkpoint(arena);
  assert(cp.block == NULL);
  assert(cp.index == 0);

  void *p = arena_alloc(arena, 64, 8);
  assert(p != NULL);
  assert(arena->head != NULL);

  // Restoring the empty checkpoint rewinds every block but keeps them.
  arena_restore(arena, cp);
  assert(arena->head != NULL);
  assert(arena->current == arena->head);
  assert(arena->head->index == 0);

  // The rewound capacity is reused, not re-malloc'd.
  void *q = arena_alloc(arena, 64, 8);
  assert(q == p);

  arena_free(arena);
}

TEST(test_arena_checkpoint_nested) {
  struct Arena *arena = arena_create(1024);
  assert(arena != NULL);

  ArenaCheckpoint outer = arena_checkpoint(arena);
  void *outer_data = arena_alloc(arena, 64, 8);
  assert(outer_data != NULL);

  ArenaCheckpoint inner = arena_checkpoint(arena);
  void *inner_data = arena_alloc(arena, 64, 8);
  assert(inner_data != NULL);

  arena_restore(arena, inner);
  // outer_data still valid; next alloc should land on inner_data's slot.
  void *reused_inner = arena_alloc(arena, 64, 8);
  assert(reused_inner == inner_data);

  arena_restore(arena, outer);
  void *reused_outer = arena_alloc(arena, 64, 8);
  assert(reused_outer == outer_data);

  arena_free(arena);
}

TEST(test_arena_checkpoint_retains_later_blocks) {
  struct Arena *arena = arena_create(256);
  assert(arena != NULL);

  void *first = arena_alloc(arena, 200, 8);
  assert(first != NULL);
  assert(arena->head == arena->current);

  ArenaCheckpoint cp = arena_checkpoint(arena);

  // Force a new block to be chained on after the checkpoint.
  void *second = arena_alloc(arena, 200, 8);
  assert(second != NULL);
  assert(arena->current != arena->head);
  assert(arena->head->next != NULL);

  arena_restore(arena, cp);

  // The later block is retained and rewound, not freed.
  assert(arena->current == arena->head);
  assert(arena->head->next != NULL);
  assert(arena->head->next->index == 0);

  // Refilling walks back into the retained block and reuses its memory.
  void *refill = arena_alloc(arena, 200, 8);
  assert(refill == second);

  arena_free(arena);
}

TEST(test_arena_checkpoint_out_of_order_restore) {
  // Regression: restoring an outer checkpoint used to free the blocks that
  // inner checkpoints pointed into, making a later restore a use-after-free.
  // Restore is memory-safe in any order; LIFO is required only for meaningful
  // positioning, which this test deliberately does not assert.
  struct Arena *arena = arena_create(64);
  assert(arena != NULL);

  void *base = arena_alloc(arena, 32, 8);
  assert(base != NULL);

  ArenaCheckpoint cp1 = arena_checkpoint(arena);

  void *spill = arena_alloc(arena, 64, 8); // forces a second block
  assert(spill != NULL);
  assert(arena->current != arena->head);

  ArenaCheckpoint cp2 = arena_checkpoint(arena);

  arena_restore(arena, cp1); // must not free the block cp2 points into
  void *after = arena_alloc(arena, 16, 8);
  assert(after != NULL);

  arena_restore(arena, cp2); // was a heap-use-after-free
  void *tail = arena_alloc(arena, 8, 8);
  assert(tail != NULL);

  arena_free(arena);
}

TEST(test_arena_large_alloc_does_not_inflate_next_block) {
  struct Arena *arena = arena_create(4096);
  assert(arena != NULL);

  // A one-off large request gets its own dedicated exact-fit block...
  void *big = arena_alloc(arena, 1u << 20, 8); // 1 MiB
  assert(big != NULL);
  assert(arena->head->capacity >= (1u << 20));

  // ...and must not inflate the size of the next general-purpose block.
  void *small = arena_alloc(arena, 8, 8);
  assert(small != NULL);
  assert(arena->head->next != NULL);
  assert(arena->head->next->capacity == 4096);

  arena_free(arena);
}

TEST(test_arena_alloc_over_aligned) {
  // Guards the `+ alignment - 1` term in the block-sizing path. Without it an
  // over-aligned request could not be guaranteed to fit the block reserved for
  // it, so simplifying min_needed to just `size` must fail here rather than
  // silently break page- and cache-line-aligned allocations.
  size_t alignment;
  for (alignment = 32; alignment <= 4096; alignment <<= 1) {
    // Default block size deliberately smaller than the alignment.
    struct Arena *arena = arena_create(64);
    assert(arena != NULL);

    void *p = arena_alloc(arena, 100, alignment);
    assert(p != NULL);
    assert(((uintptr_t)p % alignment) == 0);

    arena_free(arena);
  }

  // Over-aligned allocation into a block that is already partially used, so
  // the padding computation does real work rather than returning zero.
  struct Arena *arena = arena_create(8192);
  assert(arena != NULL);

  void *first = arena_alloc(arena, 1, 1);
  assert(first != NULL);

  void *aligned = arena_alloc(arena, 64, 512);
  assert(aligned != NULL);
  assert(((uintptr_t)aligned % 512) == 0);

  arena_free(arena);
}

TEST(test_arena_restore_retains_bounded_capacity) {
  // Restore never frees, so retained capacity must converge rather than grow
  // without bound across repeated checkpoint/restore cycles.
  struct Arena *arena = arena_create(256);
  assert(arena != NULL);

  void *persistent = arena_alloc(arena, 64, 8);
  assert(persistent != NULL);

  ArenaCheckpoint cp = arena_checkpoint(arena);

  size_t capacity_after_first_cycle = 0;
  int cycle;
  for (cycle = 0; cycle < 100; cycle++) {
    int i;
    for (i = 0; i < 20; i++) {
      void *temp = arena_alloc(arena, 100, 8);
      assert(temp != NULL);
    }
    arena_restore(arena, cp);

    size_t total = 0;
    struct ArenaBlock *b = arena->head;
    while (b) {
      total += b->capacity;
      b = b->next;
    }

    if (cycle == 0)
      capacity_after_first_cycle = total;
    else
      // Blocks are recycled, so no cycle after the first may allocate more.
      assert(total == capacity_after_first_cycle);
  }

  // The persistent allocation survived every restore.
  assert(arena->head->index >= 64);

  arena_free(arena);
}

TEST(test_arena_mixed_sizes) {
  struct Arena *arena = arena_create(512);
  assert(arena != NULL);

  void *ptr1 = arena_alloc(arena, 8, 8);
  void *ptr2 = arena_alloc(arena, 256, 8);
  void *ptr3 = arena_alloc(arena, 16, 8);
  void *ptr4 = arena_alloc(arena, 512, 8);
  void *ptr5 = arena_alloc(arena, 1, 1);

  assert(ptr1 != NULL);
  assert(ptr2 != NULL);
  assert(ptr3 != NULL);
  assert(ptr4 != NULL);
  assert(ptr5 != NULL);

  arena_free(arena);
}

int main() {
  printf("Starting arena tests...\n\n");

  RUN_TEST(test_arena_create_valid);
  RUN_TEST(test_arena_create_zero_size);
  RUN_TEST(test_arena_alloc_null_arena);
  RUN_TEST(test_arena_alloc_zero_size);
  RUN_TEST(test_arena_alloc_single_allocation);
  RUN_TEST(test_arena_alloc_multiple_allocations_same_block);
  RUN_TEST(test_arena_alloc_larger_than_default);
  RUN_TEST(test_arena_alloc_multiple_blocks);
  RUN_TEST(test_arena_alloc_alignment_8);
  RUN_TEST(test_arena_alloc_alignment_16);
  RUN_TEST(test_arena_alloc_alignment_1);
  RUN_TEST(test_arena_reset);
  RUN_TEST(test_arena_reset_multiple_blocks);
  RUN_TEST(test_arena_data_integrity);
  RUN_TEST(test_arena_stress_many_small_allocations);
  RUN_TEST(test_arena_checkpoint_restore_basic);
  RUN_TEST(test_arena_checkpoint_empty_state);
  RUN_TEST(test_arena_checkpoint_nested);
  RUN_TEST(test_arena_checkpoint_retains_later_blocks);
  RUN_TEST(test_arena_checkpoint_out_of_order_restore);
  RUN_TEST(test_arena_large_alloc_does_not_inflate_next_block);
  RUN_TEST(test_arena_alloc_over_aligned);
  RUN_TEST(test_arena_restore_retains_bounded_capacity);
  RUN_TEST(test_arena_mixed_sizes);

  printf("\n✓ All tests passed!\n");
  return 0;
}
