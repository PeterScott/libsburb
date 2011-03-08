/* Miscellaneous utilities. */

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


/**************************** Debugging functions *****************************/

#ifdef DEBUG

/* Print a JudyL array in [(key, val), ...] format. */
void print_judyl(Pvoid_t judy) {
  Word_t index; Word_t *pvalue; int count;

  JLC(count, judy, 0, -1);
  printf("[");
  index = 0; JLF(pvalue, judy, index);
  for (; pvalue != NULL; count--) {
    printf("(%X, %X)", (unsigned int)index, (unsigned int)*pvalue);
    if (count > 1) printf(", ");
    JLN(pvalue, judy, index);
  }
  printf("]\n");
}

/* Print a two-level JudyL array in [(<k1,k2>, val), ...] format. */
void print_judyl2(Pvoid_t judy) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy; int nonempty = 0;

  /* Traverse the outer JudyL */
  printf("[");
  index_outer = 0;
  JLF(pvalue_outer, judy, index_outer);
  while (pvalue_outer != NULL) {
    nonempty = 1;
    inner_judy = (Pvoid_t)*pvalue_outer;
    /* Traverse the inner JudyL */
    index_inner = 0;
    JLF(pvalue_inner, inner_judy, index_inner);
    while (pvalue_inner != NULL) {
      printf("(<%u,%u>, %X), ", (unsigned int)index_outer,
             (unsigned int)index_inner, (unsigned int)*pvalue_inner);
      JLN(pvalue_inner, inner_judy, index_inner);
    }
    JLN(pvalue_outer, judy, index_outer);
  }
  if (nonempty) printf("\b\b");
  printf("]\n");
}

#endif
