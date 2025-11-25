#define ARENA_IMPLEMENTATION_H
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

TEST(test_arena_init_valid) {
  struct Arena *arena = arena_init(1024);
  assert(arena != NULL);
  assert(arena->default_block_size == 1024);
  assert(arena->head == NULL);
  assert(arena->current == NULL);
  arena_free(arena);
}

TEST(test_arena_init_zero_size) {
  struct Arena *arena = arena_init(0);
  assert(arena == NULL);
}

TEST(test_arena_alloc_null_arena) {
  void *ptr = arena_alloc(NULL, 100, 8);
  assert(ptr == NULL);
}

TEST(test_arena_alloc_zero_size) {
  struct Arena *arena = arena_init(1024);
  assert(arena != NULL);

  void *ptr = arena_alloc(arena, 0, 8);
  assert(ptr == NULL);

  arena_free(arena);
}

TEST(test_arena_alloc_single_allocation) {
  struct Arena *arena = arena_init(1024);
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
  struct Arena *arena = arena_init(1024);
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
  struct Arena *arena = arena_init(512);
  assert(arena != NULL);

  void *ptr = arena_alloc(arena, 1024, 8);
  assert(ptr != NULL);
  assert(arena->head != NULL);
  assert(arena->current->capacity == 1024);

  arena_free(arena);
}

TEST(test_arena_alloc_multiple_blocks) {
  struct Arena *arena = arena_init(512);
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
  struct Arena *arena = arena_init(1024);
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
  struct Arena *arena = arena_init(1024);
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
  struct Arena *arena = arena_init(1024);
  assert(arena != NULL);

  void *ptr = arena_alloc(arena, 10, 1);
  assert(ptr != NULL);

  arena_free(arena);
}

TEST(test_arena_reset) {
  struct Arena *arena = arena_init(512);
  assert(arena != NULL);

  void *ptr1 = arena_alloc(arena, 400, 8);
  void *ptr2 = arena_alloc(arena, 400, 8);

  assert(ptr1 != NULL);
  assert(ptr2 != NULL);

  struct ArenaBlock *first_block = arena->head;
  size_t initial_index1 = arena->head->index;
  size_t initial_index2 = arena->current->index;

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
  struct Arena *arena = arena_init(256);
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
  struct Arena *arena = arena_init(1024);
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
  struct Arena *arena = arena_init(1024);
  assert(arena != NULL);

  for (int i = 0; i < 100; i++) {
    void *ptr = arena_alloc(arena, 10, 8);
    assert(ptr != NULL);
  }

  arena_free(arena);
}

TEST(test_arena_mixed_sizes) {
  struct Arena *arena = arena_init(512);
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

  RUN_TEST(test_arena_init_valid);
  RUN_TEST(test_arena_init_zero_size);
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
  RUN_TEST(test_arena_mixed_sizes);

  printf("\n✓ All tests passed!\n");
  return 0;
}
