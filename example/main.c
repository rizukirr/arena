#define ARENA_IMPLEMENTATION
#include "../arena.h"
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

int main() {
  struct Arena *arena = arena_init(1024); // 1 KB blocks

  if (!arena) {
    printf("Failed to create arena\n");
    return 1;
  }

  printf("=== Demo 1: Basic Checkpoint Usage ===\n");

  // Allocate persistent data that we want to keep
  char *persistent_name = arena_alloc(arena, 32, ARENA_ALIGNOF(char));
  strcpy(persistent_name, "Rizki Rakasiwi");
  printf("Persistent allocation: %s\n", persistent_name);

  int *persistent_numbers =
      arena_alloc(arena, 5 * sizeof(int), ARENA_ALIGNOF(int));
  for (int i = 0; i < 5; i++)
    persistent_numbers[i] = i * 10;
  printf("Persistent numbers: %d %d %d %d %d\n", persistent_numbers[0],
         persistent_numbers[1], persistent_numbers[2], persistent_numbers[3],
         persistent_numbers[4]);

  // Save checkpoint before temporary allocations
  ArenaCheckpoint cp = arena_checkpoint(arena);
  printf("\nCheckpoint saved!\n");

  // Allocate temporary data
  char *temp_buffer = arena_alloc(arena, 256, ARENA_ALIGNOF(char));
  strcpy(temp_buffer, "This is temporary data that will be freed");
  printf("Temporary allocation: %s\n", temp_buffer);

  double *temp_array =
      arena_alloc(arena, 10 * sizeof(double), ARENA_ALIGNOF(double));
  for (int i = 0; i < 10; i++)
    temp_array[i] = i * 3.14;
  printf("Temporary array: %.2f %.2f %.2f\n", temp_array[0], temp_array[1],
         temp_array[2]);

  // Restore to checkpoint - this frees temp_buffer and temp_array
  arena_restore(arena, cp);
  printf("\nRestored to checkpoint - temporary data freed!\n");

  // Persistent data is still valid
  printf("Persistent data still accessible: %s\n", persistent_name);
  printf("Persistent numbers still accessible: %d %d %d %d %d\n",
         persistent_numbers[0], persistent_numbers[1], persistent_numbers[2],
         persistent_numbers[3], persistent_numbers[4]);

  printf("\n=== Demo 2: Checkpoint in Loop (Memory Reuse) ===\n");

  // Common pattern: reuse same memory in each iteration
  ArenaCheckpoint loop_cp = arena_checkpoint(arena);

  for (int iteration = 0; iteration < 5; iteration++) {
    // Allocate temporary workspace for this iteration
    char *workspace = arena_alloc(arena, 512, ARENA_ALIGNOF(char));
    snprintf(workspace, 512, "Processing iteration %d with temporary buffer",
             iteration);

    int *temp_data = arena_alloc(arena, 100 * sizeof(int), ARENA_ALIGNOF(int));
    for (int i = 0; i < 100; i++)
      temp_data[i] = iteration * 1000 + i;

    printf("%s - First temp value: %d\n", workspace, temp_data[0]);

    // Restore checkpoint - reuse the same memory for next iteration
    arena_restore(arena, loop_cp);
  }

  printf("\nLoop completed - all temporary allocations reused same memory!\n");

  printf("\n=== Demo 3: Nested Checkpoints ===\n");

  ArenaCheckpoint outer_cp = arena_checkpoint(arena);

  char *outer_data = arena_alloc(arena, 64, ARENA_ALIGNOF(char));
  strcpy(outer_data, "Outer scope data");
  printf("Allocated: %s\n", outer_data);

  ArenaCheckpoint inner_cp = arena_checkpoint(arena);

  char *inner_data = arena_alloc(arena, 64, ARENA_ALIGNOF(char));
  strcpy(inner_data, "Inner scope data");
  printf("Allocated: %s\n", inner_data);

  // Restore inner checkpoint - frees inner_data, keeps outer_data
  arena_restore(arena, inner_cp);
  printf("Restored inner checkpoint - inner_data freed, outer_data kept\n");

  // Restore outer checkpoint - frees outer_data too
  arena_restore(arena, outer_cp);
  printf("Restored outer checkpoint - all temporary data freed\n");

  // Free all
  arena_free(arena);

  return 0;
}
