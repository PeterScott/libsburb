#include "sburb.h"

/***************************** Extensible vectors *****************************/

/* Allocate and return a new, empty vector. */
vector_t new_vector(void) {
  vector_t vec = malloc(4 * sizeof(Word_t));
  vec[0] = 4; vec[1] = 0;
  return vec;
}

/* Append a word to a vector, and return a pointer to the result. This may
   allocate a new vector and delete the old one, or it might modify the old one
   in place.

   The vectors start out with room for two elements, and expand as so:
   2, 7, 14, ... 2^(2i) - 2. Essentially, they double in size each
   time. This gives low-overhead storage for the common case of one or
   two values, with amortied O(1) append for larger vectors. */
vector_t vector_append(vector_t vector, Word_t word) {
  Word_t len = vector[0], used = vector[1];
  vector_t vec;

  if (used + 2 == len) {        /* buffer full; reallocate */
    vec = malloc(2 * len * sizeof(Word_t));
    memcpy(vec+1, vector+1, (len-1) * sizeof(Word_t)); vec[0] = len = 2*len;
    free(vector);
  } else vec = vector;

  vec[used + 2] = word; vec[1]++;
  return vec;
}
