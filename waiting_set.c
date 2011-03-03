/* Waiting sets. Maps from ids to vectors of pointers to patches blocking on
   that id. Yeah, it's kind of irritatingly complicated.

   A vector is represented as an array of machine words, with the first one
   telling the size of the array (including the first two words), the second
   telling the number of array elements used by data, and the rest being the
   data itself. This can serve as an array of pointers, with amortized O(1)
   append. It must be freed with free() by the client code.

   The rest of the mapping is done with JudyL arrays. */

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
   in place. */
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


/******************************** Waiting sets ********************************/

/* Allocate and return a new, empty waiting set. Does not actually allocate
   anything, so unless you put something in the waiting set, you don't actually
   need to free this. */
waiting_set_t new_waiting_set(void) {
  return (waiting_set_t)NULL;
}

/* Free a waiting set, deleting everything. If delete_patches is true, then it
   will also delete the patches themselves; otherwise it won't. */
void delete_waiting_set(waiting_set_t wset, int delete_patches) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy; Word_t rc_word;

  /* Traverse the outer JudyL */
  index_outer = 0;
  JLF(pvalue_outer, wset, index_outer);
  while (pvalue_outer != NULL) {
    inner_judy = (Pvoid_t)*pvalue_outer;
    /* Traverse the inner JudyL, freeing vectors and maybe patches */
    index_inner = 0;
    JLF(pvalue_inner, inner_judy, index_inner);
    while (pvalue_inner != NULL) {
      vector_t vec = (vector_t)*pvalue_inner;
      if (delete_patches)
        for (Word_t i = 0; i < VECTOR_LEN(vec); i++)
          free((void *)VECTOR_GET(vec, i));
      free(vec);
      JLN(pvalue_inner, inner_judy, index_inner);
    }
    /* Free the inner JudyL, and proceed to the next one */
    JLFA(rc_word, inner_judy);
    JLN(pvalue_outer, wset, index_outer);
  }
  JLFA(rc_word, wset);
}

/* Add a patch to the waiting set, blocking on blocking_id. Takes a pointer to a
   waiting set, and modifies it. Returns zero on success, nonzero on error. */
int add_to_waiting_set(waiting_set_t *wset, uint64_t blocking_id, patch_t patch) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy;           /* Inner judy arrays */
  waiting_set_t temp = *wset;

  /* Look up the yarn. */
  index_outer = YARN(blocking_id); JLI(pvalue_outer, temp, index_outer);
  if (pvalue_outer == PJERR) return -1; /* malloc() error */
  if (*pvalue_outer == 0) {
    /* No entry was found for the yarn. Create a new yarn JudyL. */
    inner_judy = (Pvoid_t)NULL; index_inner = OFFSET(blocking_id);
    JLI(pvalue_inner, inner_judy, index_inner);
    if (pvalue_inner == PJERR) return -1; /* malloc() error */
    vector_t vec = new_vector(); vec = vector_append(vec, (Word_t)patch);
    *pvalue_inner = (Word_t)vec;
    *pvalue_outer = (Word_t)inner_judy;
  } else {
    inner_judy = (Pvoid_t)*pvalue_outer; index_inner = OFFSET(blocking_id);
    JLI(pvalue_inner, inner_judy, index_inner);
    if (pvalue_inner == PJERR) return -1; /* malloc() error */
    vector_t vec;
    if (*pvalue_inner != 0) vec = (vector_t)*pvalue_inner;
    else vec = new_vector();
    vec = vector_append(vec, (Word_t)patch);
    *pvalue_inner = (Word_t)vec;
    *pvalue_outer = (Word_t)inner_judy;
  }

  *wset = temp; return 0;
}

/********************************** Testing ***********************************/

int main(void) {
  patch_t patch1 = make_patch1();
  patch_t patch2 = make_patch2();
  patch_t patch3 = make_patch3();
  waiting_set_t wset = new_waiting_set();

  LIFTERR(add_to_waiting_set(&wset, PACK_ID(1, 3), patch1));
  LIFTERR(add_to_waiting_set(&wset, PACK_ID(1, 3), patch2));
  LIFTERR(add_to_waiting_set(&wset, PACK_ID(2, 2), patch3));

  delete_waiting_set(wset, TRUE);
  return 0;
}
