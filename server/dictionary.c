#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dictionary.h"

struct dictionary_t {
  size_t array_size;  /* always a power of two */
  size_t fill_count;  /* array slots filled; always less than array_size * FILL_FACTOR */
  size_t map_count;   /* keys mapped; always less than fill_count */
  char **keys;
  void **vals;
  int is_ci;
  free_proc_t free_value; /* for values */
};

#define FILL_FACTOR 0.8

/* Guaranteed to be otherwise invalid as a string pointer: */
#define REMOVED   ((char *)0x1)

/************************************************************/

static size_t cs_hash(const char *str);
static int cs_is_equivalent(const char *str1, const char *str2);
static size_t ci_hash(const char *str);
static int ci_is_equivalent(const char *str1, const char *str2);
static size_t hash2(const char *str);
static void grow_and_rehash(dictionary_t *d);

dictionary_t *make_dictionary(int compare_mode, free_proc_t free_value)
{
  dictionary_t *d = malloc(sizeof(dictionary_t));

  d->fill_count = 0;
  d->map_count = 0;
  d->array_size = 16;
  d->keys = calloc(d->array_size, sizeof(char *));
  d->vals = calloc(d->array_size, sizeof(void *));
  d->is_ci = (compare_mode == COMPARE_CASE_INSENS);
  d->free_value = free_value;

  return d;
}

void dictionary_free(dictionary_t *d)
{
  int i;
  
  for (i = 0; i < d->array_size; i++) {
    if (d->keys[i] && d->keys[i] != REMOVED) {
      free(d->keys[i]);
      if (d->free_value)
        d->free_value(d->vals[i]);
    }
  }

  free(d->keys);
  free(d->vals);
  free(d);
}

#define ADD_MODE    0
#define REMOVE_MODE 1
#define GET_MODE    2
#define TEST_MODE   3

#define DUP_KEY    0
#define NO_DUP_KEY 1

static void *hash_op(dictionary_t *d, const char *key, void *value, int mode, int dup_key)
{
  size_t i, j, sz = d->array_size;
  size_t avail_i = sz; /* if not sz, a slot with REMOVED */

  if (d->is_ci) {
    i = ci_hash(key) & (sz-1);
    j = hash2(key);
    while (d->keys[i] && ((d->keys[i] == REMOVED)
                          || !ci_is_equivalent(key, d->keys[i]))) {
      if ((avail_i == sz) && (d->keys[i] == REMOVED))
        avail_i = i;
      i = (i + j) & (sz-1);
    }
  } else {
    i = cs_hash(key) & (sz-1);
    j = hash2(key);
    while (d->keys[i] && ((d->keys[i] == REMOVED)
                          || !cs_is_equivalent(key, d->keys[i]))) {
      if ((avail_i == sz) && (d->keys[i] == REMOVED))
        avail_i = i;
      i = (i + j) & (sz-1);
    }
  }

  if (!d->keys[i]) {
    if (mode == ADD_MODE) {
      if (avail_i != sz) {
        /* Use old slot containing REMOVED, instead of using up a new one */
        if (dup_key == DUP_KEY)
          key = strdup(key);
        d->map_count++;
        d->keys[avail_i] = (char *)key;
        d->vals[avail_i] = value;
      } else if ((d->fill_count + 1) >= (FILL_FACTOR * d->array_size)) {
        grow_and_rehash(d);
        (void)hash_op(d, key, value, mode, dup_key);
      } else {
        if (dup_key == DUP_KEY)
          key = strdup(key);
        d->fill_count++;
        d->map_count++;
        d->keys[i] = (char *)key;
        d->vals[i] = value;
      }
    }
  } else if (mode == GET_MODE) {
    return d->vals[i];
  } else if (mode == TEST_MODE) {
    return REMOVED; /* any non-NULL value works */
  } else {
    if (d->free_value)
      d->free_value(d->vals[i]);
    if (mode == REMOVE_MODE) {
      --d->map_count;
      free(d->keys[i]);
      d->keys[i] = REMOVED;
    } else
      d->vals[i] = value;
  }

  return NULL;
}

static void grow_and_rehash(dictionary_t *d)
{
  size_t old_array_size = d->array_size, i;
  char **old_keys = d->keys;
  void **old_vals = d->vals;

  d->fill_count = 0;
  d->map_count = 0;
  d->array_size *= 2;
  d->keys = calloc(d->array_size, sizeof(char*));
  d->vals = calloc(d->array_size, sizeof(char*));

  for (i = 0; i < old_array_size; i++) {
    if (old_keys[i] && (old_keys[i] != REMOVED)) {
      (void)hash_op(d, old_keys[i], old_vals[i], ADD_MODE, NO_DUP_KEY);
    }
  }

  free(old_keys);
  free(old_vals);
}

void dictionary_set(dictionary_t *d, const char *key, void *value)
{
  (void)hash_op(d, key, value, ADD_MODE, DUP_KEY);
}

void dictionary_remove(dictionary_t *d, const char *key)
{
  (void)hash_op(d, key, NULL, REMOVE_MODE, NO_DUP_KEY);
}

void *dictionary_get(dictionary_t *d, const char *key)
{
  return hash_op(d, key, NULL, GET_MODE, NO_DUP_KEY);
}

int dictionary_has_key(dictionary_t *d, const char *key)
{
  return (hash_op(d, key, NULL, TEST_MODE, NO_DUP_KEY) != NULL);
}

size_t dictionary_count(dictionary_t *d)
{
  return d->map_count;
}

const char **dictionary_keys(dictionary_t *d)
{
  int i, j;
  const char **keys = malloc(sizeof(char *) * (d->map_count + 1));

  for (i = 0, j = 0; i < d->array_size; i++) {
    if (d->keys[i] && (d->keys[i] != REMOVED)) {
      keys[j++] = d->keys[i];
    }
  }

  keys[j] = NULL;
  return keys;
}

/************************************************************/

static size_t cs_hash(const char *str) {
  size_t result = 0;
  while (*str) {
    result += *str;
    str++;
  }
  return result;
}

static int cs_is_equivalent(const char *str1, const char *str2) {
  while (*str1 != 0) {
    if (*str1 != *str2)
      return 0;
    str1++;
    str2++;
  }
  return *str2 == 0;
}

static size_t ci_hash(const char *str) {
  size_t result = 0;
  while (*str != 0) {
    result += tolower((unsigned char)*str);
    str++;
  }
  return result;
}

static int ci_is_equivalent(const char *str1, const char *str2) {
  while (*str1 != 0) {
    if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2))
      return 0;
    str1++;
    str2++;
  }
  return *str2 == 0;
}

static size_t hash2(const char *str) {
  int result = 0;
  while (*str != 0) {
    result++;
    str++;
  }
  return result | 0x1;
}
