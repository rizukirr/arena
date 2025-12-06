#define ARRAYD_IMPLEMENTATION
#include "../arrayd.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[50];
  size_t age;
  double salary;
} Person;

int main(void) {
  printf("=== Demo 1: Integer Array ===\n");

  Arrayd *int_array = arrayd_new(4);
  if (!int_array) {
    printf("Failed to create array\n");
    return 1;
  }

  // Append integers
  arrayd_append_int(int_array, 10);
  arrayd_append_int(int_array, 20);
  arrayd_append_int(int_array, 30);
  arrayd_append_int(int_array, 40);
  arrayd_append_int(int_array, 50); // This will trigger reallocation

  printf("Integer array contents: ");
  for (size_t i = 0; i < arrayd_count(int_array); i++) {
    printf("%zu ", arrayd_get_int(int_array, i));
  }
  printf("\n");
  printf("Array count: %zu\n", arrayd_count(int_array));

  // Update value at index
  arrayd_put_at_int(int_array, 2, 99);
  printf("After updating index 2: ");
  for (size_t i = 0; i < arrayd_count(int_array); i++) {
    printf("%zu ", arrayd_get_int(int_array, i));
  }
  printf("\n");

  // Remove element
  arrayd_remove_at(int_array, 1);
  printf("After removing index 1: ");
  for (size_t i = 0; i < arrayd_count(int_array); i++) {
    printf("%zu ", arrayd_get_int(int_array, i));
  }
  printf("\n");

  arrayd_clear(int_array);

  printf("\n=== Demo 2: String Array ===\n");

  Arrayd *string_array = arrayd_new(3);

  arrayd_append_string(string_array, "Hello");
  arrayd_append_string(string_array, "World");
  arrayd_append_string(string_array, "Dynamic");
  arrayd_append_string(string_array, "Array");

  printf("String array contents:\n");
  for (size_t i = 0; i < arrayd_count(string_array); i++) {
    printf("  [%zu] %s\n", i, arrayd_get_string(string_array, i));
  }

  // Update string
  arrayd_put_at_string(string_array, 0, "Hi");
  printf("After updating index 0: %s\n", arrayd_get_string(string_array, 0));

  arrayd_clear(string_array);

  printf("\n=== Demo 3: Double Array ===\n");

  Arrayd *double_array = arrayd_new(5);

  arrayd_append_double(double_array, 3.14159);
  arrayd_append_double(double_array, 2.71828);
  arrayd_append_double(double_array, 1.41421);
  arrayd_append_double(double_array, 1.73205);

  printf("Double array contents: ");
  for (size_t i = 0; i < arrayd_count(double_array); i++) {
    printf("%.5f ", arrayd_get_double(double_array, i));
  }
  printf("\n");

  arrayd_clear(double_array);

  printf("\n=== Demo 4: Float Array ===\n");

  Arrayd *float_array = arrayd_new(3);

  arrayd_append_float(float_array, 1.5f);
  arrayd_append_float(float_array, 2.5f);
  arrayd_append_float(float_array, 3.5f);

  printf("Float array contents: ");
  for (size_t i = 0; i < arrayd_count(float_array); i++) {
    printf("%.2f ", arrayd_get_float(float_array, i));
  }
  printf("\n");

  arrayd_clear(float_array);

  printf("\n=== Demo 5: Char Array ===\n");

  Arrayd *char_array = arrayd_new(10);

  char message[] = "HELLO";
  for (size_t i = 0; i < strlen(message); i++) {
    arrayd_append_char(char_array, message[i]);
  }

  printf("Char array contents: ");
  for (size_t i = 0; i < arrayd_count(char_array); i++) {
    printf("%c", arrayd_get_char(char_array, i));
  }
  printf("\n");

  arrayd_clear(char_array);

  printf("\n=== Demo 6: Long Array ===\n");

  Arrayd *long_array = arrayd_new(3);

  arrayd_append_long(long_array, 1000000L);
  arrayd_append_long(long_array, 2000000L);
  arrayd_append_long(long_array, 3000000L);

  printf("Long array contents: ");
  for (size_t i = 0; i < arrayd_count(long_array); i++) {
    printf("%ld ", arrayd_get_long(long_array, i));
  }
  printf("\n");

  arrayd_clear(long_array);

  printf("\n=== Demo 7: Pointer Array (Structs) ===\n");

  Arrayd *person_array = arrayd_new(2);

  Person *p1 = malloc(sizeof(Person));
  strcpy(p1->name, "Alice");
  p1->age = 30;
  p1->salary = 75000.50;

  Person *p2 = malloc(sizeof(Person));
  strcpy(p2->name, "Bob");
  p2->age = 25;
  p2->salary = 65000.00;

  Person *p3 = malloc(sizeof(Person));
  strcpy(p3->name, "Charlie");
  p3->age = 35;
  p3->salary = 85000.75;

  arrayd_append(person_array, p1);
  arrayd_append(person_array, p2);
  arrayd_append(person_array, p3);

  printf("Person array contents:\n");
  for (size_t i = 0; i < arrayd_count(person_array); i++) {
    Person *p = (Person *)arrayd_get(person_array, i);
    printf("  [%zu] Name: %s, Age: %zu, Salary: $%.2f\n", i, p->name, p->age,
           p->salary);
  }

  // Update person
  Person *new_person = malloc(sizeof(Person));
  strcpy(new_person->name, "David");
  new_person->age = 40;
  new_person->salary = 95000.00;

  Person *old_person = (Person *)arrayd_get(person_array, 1);
  free(old_person);
  arrayd_put_at(person_array, 1, new_person);

  printf("\nAfter updating index 1:\n");
  for (size_t i = 0; i < arrayd_count(person_array); i++) {
    Person *p = (Person *)arrayd_get(person_array, i);
    printf("  [%zu] Name: %s, Age: %zu, Salary: $%.2f\n", i, p->name, p->age,
           p->salary);
  }

  // Clean up
  for (size_t i = 0; i < arrayd_count(person_array); i++) {
    free(arrayd_get(person_array, i));
  }
  arrayd_clear(person_array);

  printf("\n=== Demo 8: Mixed Operations ===\n");

  Arrayd *mixed_array = arrayd_new(5);

  // Add some numbers
  for (int i = 0; i < 10; i++) {
    arrayd_append_int(mixed_array, (i * 10));
  }

  printf("Initial array: ");
  for (size_t i = 0; i < arrayd_count(mixed_array); i++) {
    printf("%zu ", arrayd_get_int(mixed_array, i));
  }
  printf("\n");

  // Remove every other element
  for (size_t i = 0; i < arrayd_count(mixed_array);) {
    arrayd_remove_at(mixed_array, i);
    i++;
  }

  printf("After removing every other: ");
  for (size_t i = 0; i < arrayd_count(mixed_array); i++) {
    printf("%zu ", arrayd_get_int(mixed_array, i));
  }
  printf("\n");

  arrayd_clear(mixed_array);

  printf("\nAll demos completed successfully!\n");

  return 0;
}
