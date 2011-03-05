/* Miscellaneous utilities. */

#include "sburb.h"

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
