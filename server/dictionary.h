
/* A dictionary maps a string to a pointer. The pointer can be
   anything, such as a pointer to another string or to another
   dictionary. The dictionary is implemented as a hash table, so
   operations on the dictionary take amortized constant time (except
   for `dictionary_keys` and `dictionary_free`.). */

typedef struct dictionary_t dictionary_t;
/* Opaque type for a dictionary instance. */

typedef void (*free_proc_t)(void*);
/* A function provided to make_dictionary() that is used to destroy
   each values in the dictionary when the dictionary itself is
   destroyed or when the value is replaced with another value. */

#define COMPARE_CASE_SENS   0
#define COMPARE_CASE_INSENS 1
/* Use one of these constants as the first argument to
   `make_dictionary` to selects between case-sensitive and
   case-insensitive string comparsion for dictionary keys. */

dictionary_t *make_dictionary(int compare_mode, free_proc_t free_value);
/* Creates a dictionary with the given comparion mode and
   value-destruction function. The value-destruiction function is used
   to destroy a value when it is removed from the table. The
   value-destruction function can be NULL for a no-op destrcution
   operation. */

void dictionary_free(dictionary_t *d);
/* Destroys the given dictionary, which frees all key strings --- and
   also destroys all values using the function provided to
   make_dictionary(). */

void dictionary_set(dictionary_t *d, const char *key, void *value);
/* Sets the given dictionary's mapping for `key` to `value`,
   destroying the value (if any) that `key` is currently mapped to.
   The dictionary makes its own copy of `key` and does not refer to
   that pointer on return. It keeps the `value` pointer as-is and
   effectively takes ownership of the value. */

void dictionary_remove(dictionary_t *d, const char *key);
/* Removes the given dictionary's mapping, if any, for `key`,
   destroying the value (if any) that `key` is currently mapped to. */

void *dictionary_get(dictionary_t *d, const char *key);
/* Returns the given dictionary's value for `key`, or NULL if the
   dictionary has no value for `key`. The dictionary retains ownership
   of the result value, so beware that the result can be destroyed if
   the mapping for `key` is changed. */

int dictionary_has_key(dictionary_t *d, const char *key);
/* Returns 1 if `key` is mapped in the given dictionary, 0 if not. */

size_t dictionary_count(dictionary_t *d);
/* Returns the number of keys that are mapped in the dictionary. */

const char **dictionary_keys(dictionary_t *d);
/* Returns a NULL-terminated array of all keys in `d`. The dictionary
   retains ownership of each key, and each key is valid only as long
   as is is not removed from the dictionary. The caller takes
   ownership of the freshly allocated array. This operation takes time
   proportional to the number of key/value mappings in the
   dictionary. */
