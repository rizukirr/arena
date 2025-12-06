#ifndef ARRAYD_H
#define ARRAYD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define arrayd_append_string(arrayd, str) arrayd_append(arrayd, (void *)str)
#define arrayd_append_int(arrayd, val)                                         \
  arrayd_append(arrayd, (void *)(intptr_t)val)
#define arrayd_append_double(arrayd, val)                                      \
  do {                                                                         \
    union {                                                                    \
      double d;                                                                \
      void *p;                                                                 \
    } u;                                                                       \
    u.d = val;                                                                 \
    arrayd_append(arrayd, u.p);                                                \
  } while (0)
#define arrayd_append_float(arrayd, val)                                       \
  do {                                                                         \
    union {                                                                    \
      float f;                                                                 \
      void *p;                                                                 \
    } u;                                                                       \
    u.f = val;                                                                 \
    arrayd_append(arrayd, u.p);                                                \
  } while (0)
#define arrayd_append_char(arrayd, val)                                        \
  arrayd_append(arrayd, (void *)(intptr_t)(char)val)
#define arrayd_append_short(arrayd, val)                                       \
  arrayd_append(arrayd, (void *)(intptr_t)(short)val)
#define arrayd_append_long(arrayd, val)                                        \
  arrayd_append(arrayd, (void *)(intptr_t)(long)val)
#define arrayd_append_long_long(arrayd, val)                                   \
  arrayd_append(arrayd, (void *)(intptr_t)(long long)val)

#define arrayd_get_int(arrayd, index) (intptr_t)arrayd_get(arrayd, index)
#define arrayd_get_string(arrayd, index) (char *)arrayd_get(arrayd, index)
#define arrayd_get_double(arrayd, index)                                       \
  ({                                                                           \
    union {                                                                    \
      double d;                                                                \
      void *p;                                                                 \
    } u;                                                                       \
    u.p = arrayd_get(arrayd, index);                                           \
    u.d;                                                                       \
  })
#define arrayd_get_float(arrayd, index)                                        \
  ({                                                                           \
    union {                                                                    \
      float f;                                                                 \
      void *p;                                                                 \
    } u;                                                                       \
    u.p = arrayd_get(arrayd, index);                                           \
    u.f;                                                                       \
  })
#define arrayd_get_char(arrayd, index) (char)(intptr_t)arrayd_get(arrayd, index)
#define arrayd_get_short(arrayd, index)                                        \
  (short)(intptr_t)arrayd_get(arrayd, index)
#define arrayd_get_long(arrayd, index) (long)(intptr_t)arrayd_get(arrayd, index)
#define arrayd_get_long_long(arrayd, index)                                    \
  (long long)(intptr_t)arrayd_get(arrayd, index)

#define arrayd_put_at_string(arrayd, index, str)                               \
  arrayd_put_at(arrayd, index, (void *)str)
#define arrayd_put_at_int(arrayd, index, val)                                  \
  arrayd_put_at(arrayd, index, (void *)(intptr_t)val)
#define arrayd_put_at_double(arrayd, index, val)                               \
  do {                                                                         \
    union {                                                                    \
      double d;                                                                \
      void *p;                                                                 \
    } u;                                                                       \
    u.d = val;                                                                 \
    arrayd_put_at(arrayd, index, u.p);                                         \
  } while (0)
#define arrayd_put_at_float(arrayd, index, val)                                \
  do {                                                                         \
    union {                                                                    \
      float f;                                                                 \
      void *p;                                                                 \
    } u;                                                                       \
    u.f = val;                                                                 \
    arrayd_put_at(arrayd, index, u.p);                                         \
  } while (0)
#define arrayd_put_at_char(arrayd, index, val)                                 \
  arrayd_put_at(arrayd, index, (void *)(intptr_t)(char)val)
#define arrayd_put_at_short(arrayd, index, val)                                \
  arrayd_put_at(arrayd, index, (void *)(intptr_t)(short)val)
#define arrayd_put_at_long(arrayd, index, val)                                 \
  arrayd_put_at(arrayd, index, (void *)(intptr_t)(long)val)
#define arrayd_put_at_long_long(arrayd, index, val)                            \
  arrayd_put_at(arrayd, index, (void *)(intptr_t)(long long)val)

typedef struct Arrayd Arrayd;

Arrayd *arrayd_new(int initial_length);

void arrayd_append(Arrayd *array, void *data);

void arrayd_clear(Arrayd *arrayd);

void arrayd_remove_at(Arrayd *arr, size_t index);

void *arrayd_get(Arrayd *arrayd, const size_t index);

size_t arrayd_count(Arrayd *arr);

int arrayd_put_at(Arrayd *arr, size_t index, void *data);

#ifdef ARRAYD_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct Arrayd {
  void **data;
  size_t length;
  size_t index;
};

Arrayd *arrayd_new(int initial_length) {
  assert(initial_length > 0 && "initial_length must be positive");

  Arrayd *arrayd = calloc(1, sizeof(Arrayd));
  assert(arrayd && "Failed to allocate Arrayd");

  arrayd->data = malloc(sizeof(void *) * initial_length);
  assert(arrayd->data && "Failed to allocate Arrayd data");

  arrayd->length = initial_length;
  arrayd->index = 0;
  return arrayd;
}

void arrayd_append(Arrayd *arr, void *data) {
  assert(arr && "Arrayd is NULL");
  assert(arr->data && "Arrayd data is NULL");

  if (arr->length <= arr->index) {
    size_t new_length = arr->length * 2;
    void *array_realloc = realloc(arr->data, new_length * sizeof(void *));
    assert(array_realloc && "Failed to reallocate Arrayd data");

    arr->length = new_length;
    arr->data = array_realloc;
  }

  arr->data[arr->index] = data;
  arr->index++;
}

int arrayd_put_at(Arrayd *arr, size_t index, void *data) {
  assert(arr && "Arrayd is NULL");
  assert(arr->data && "Arrayd data is NULL");
  assert(index < arr->index && "Index out of bounds");

  arr->data[index] = data;
  return 0;
}

void arrayd_remove_at(Arrayd *arr, size_t index) {
  assert(arr && "Arrayd is NULL");
  assert(arr->data && "Arrayd data is NULL");
  assert(index < arr->index && "Index out of bounds");

  memmove(&arr->data[index], &arr->data[index + 1],
          (arr->index - index - 1) * sizeof(void *));
  arr->index--;
}

size_t arrayd_count(Arrayd *arr) {
  assert(arr && "Arrayd is NULL");
  return arr->index;
}

void arrayd_clear(Arrayd *arrayd) {
  assert(arrayd && "Arrayd is NULL");
  assert(arrayd->data && "Arrayd data is NULL");

  free(arrayd->data);
  free(arrayd);
}

void *arrayd_get(Arrayd *arrayd, const size_t index) {
  assert(arrayd && "Arrayd is NULL");
  assert(arrayd->data && "Arrayd data is NULL");
  assert(index < arrayd->index && "Index out of bounds");

  return arrayd->data[index];
}

#endif // ARRAYD_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // ARRAY_D_H
