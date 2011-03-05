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


/* /\******************************** Waiting sets ********************************\/ */

/* /\* Allocate and return a new, empty waiting set. Does not actually allocate */
/*    anything, so unless you put something in the waiting set, you don't actually */
/*    need to free this. *\/ */
/* waiting_set_t new_waiting_set(void) { */
/*   return (waiting_set_t)NULL; */
/* } */

/* /\* Free a waiting set, deleting everything. If delete_patches is true, then it */
/*    will also delete the patches themselves; otherwise it won't. *\/ */
/* void delete_waiting_set(waiting_set_t wset, int delete_patches) { */
/*   Word_t index_outer; Word_t *pvalue_outer; */
/*   Word_t index_inner; Word_t *pvalue_inner; */
/*   Pvoid_t inner_judy; Word_t rc_word; */

/*   /\* Traverse the outer JudyL *\/ */
/*   index_outer = 0; */
/*   JLF(pvalue_outer, wset, index_outer); */
/*   while (pvalue_outer != NULL) { */
/*     inner_judy = (Pvoid_t)*pvalue_outer; */
/*     /\* Traverse the inner JudyL, freeing vectors and maybe patches *\/ */
/*     index_inner = 0; */
/*     JLF(pvalue_inner, inner_judy, index_inner); */
/*     while (pvalue_inner != NULL) { */
/*       vector_t vec = (vector_t)*pvalue_inner; */
/*       if (delete_patches) */
/*         for (Word_t i = 0; i < VECTOR_LEN(vec); i++) */
/*           free((void *)VECTOR_GET(vec, i)); */
/*       free(vec); */
/*       JLN(pvalue_inner, inner_judy, index_inner); */
/*     } */
/*     /\* Free the inner JudyL, and proceed to the next one *\/ */
/*     JLFA(rc_word, inner_judy); */
/*     JLN(pvalue_outer, wset, index_outer); */
/*   } */
/*   JLFA(rc_word, wset); */
/* } */

/* /\* Get the waiting set blocking on a given id, removing it from the waiting */
/*    set. Takes a pointer to a waiting set, which will be modified, and the */
/*    id. Will return either a vector of pointers to patches, or NULL if there was */
/*    nothing found to be waiting on the id. */

/*    The intended use-case for this function is attempting to apply patches */
/*    waiting on the id of an atom which you just inserted into a weave. In that */
/*    case, you would call this function for every id that you insert, copying the */
/*    patches into a vector as you go. Then, once you've finished inserting that */
/*    patch, you will have a vector of patches which *might* be ready to apply to */
/*    the resulting weave. Those which are not ready to apply, can be put back into */
/*    the waiting set. *\/ */
/* vector_t take_waiting_set(waiting_set_t *wset, uint64_t id) { */
/*   Word_t index_outer; Word_t *pvalue_outer; */
/*   Word_t index_inner; Word_t *pvalue_inner; */
/*   Pvoid_t inner_judy; int rc_int; Word_t rc_word; */
/*   vector_t vec; waiting_set_t temp = *wset; */

/*   /\* Look up the yarn *\/ */
/*   if (temp == NULL) return NULL; /\* common case: empty set *\/ */
/*   index_outer = YARN(id); JLG(pvalue_outer, temp, index_outer); */
/*   if (pvalue_outer == PJERR) return NULL; /\* malloc() error *\/ */
/*   if (pvalue_outer == NULL) return NULL;  /\* yarn not found *\/ */
  
/*   /\* Yarn found. Look up the offset. *\/ */
/*   inner_judy = (Pvoid_t)*pvalue_outer; index_inner = OFFSET(id); */
/*   JLG(pvalue_inner, inner_judy, index_inner); */
/*   if (pvalue_inner == PJERR) return NULL; /\* malloc() error *\/ */
/*   if (pvalue_inner == NULL) return NULL;  /\* offset not found *\/ */
  
/*   /\* Id found. Get and remove vector. *\/ */
/*   vec = (vector_t)*pvalue_inner; */
/*   JLD(rc_int, inner_judy, index_inner); */
/*   *pvalue_outer = (Word_t)inner_judy; */

/*   /\* If inner judy is now empty, remove it from outer one. *\/ */
/*   JLC(rc_word, inner_judy, 0, -1); */
/*   if (rc_word == 0) JLD(rc_int, temp, index_outer); */

/*   *wset = temp; return vec; */
/* } */

/* /\* Add a patch to the waiting set, blocking on blocking_id. Takes a pointer to a */
/*    waiting set, and modifies it. Returns zero on success, nonzero on error. *\/ */
/* int add_to_waiting_set(waiting_set_t *wset, uint64_t blocking_id, patch_t patch) { */
/*   Word_t index_outer; Word_t *pvalue_outer; */
/*   Word_t index_inner; Word_t *pvalue_inner; */
/*   Pvoid_t inner_judy;           /\* Inner judy arrays *\/ */
/*   waiting_set_t temp = *wset; */

/*   /\* Look up the yarn. *\/ */
/*   index_outer = YARN(blocking_id); JLI(pvalue_outer, temp, index_outer); */
/*   if (pvalue_outer == PJERR) return -1; /\* malloc() error *\/ */
/*   if (*pvalue_outer == 0) { */
/*     /\* No entry was found for the yarn. Create a new yarn JudyL. *\/ */
/*     inner_judy = (Pvoid_t)NULL; index_inner = OFFSET(blocking_id); */
/*     JLI(pvalue_inner, inner_judy, index_inner); */
/*     if (pvalue_inner == PJERR) return -1; /\* malloc() error *\/ */
/*     vector_t vec = new_vector(); vec = vector_append(vec, (Word_t)patch); */
/*     *pvalue_inner = (Word_t)vec; */
/*     *pvalue_outer = (Word_t)inner_judy; */
/*   } else { */
/*     inner_judy = (Pvoid_t)*pvalue_outer; index_inner = OFFSET(blocking_id); */
/*     JLI(pvalue_inner, inner_judy, index_inner); */
/*     if (pvalue_inner == PJERR) return -1; /\* malloc() error *\/ */
/*     vector_t vec; */
/*     if (*pvalue_inner != 0) vec = (vector_t)*pvalue_inner; */
/*     else vec = new_vector(); */
/*     vec = vector_append(vec, (Word_t)patch); */
/*     *pvalue_inner = (Word_t)vec; */
/*     *pvalue_outer = (Word_t)inner_judy; */
/*   } */

/*   *wset = temp; return 0; */
/* } */

/* /\* Is the waiting set empty? *\/ */
/* int waiting_set_empty(waiting_set_t wset) { */
/*   Word_t rc_word; */
/*   JLC(rc_word, wset, 0, -1); */
/*   return rc_word == 0; */
/* } */

/* /\* Print a waiting set, in a quite verbose format for debugging. *\/ */
/* void print_waiting_set(waiting_set_t wset) { */
/*   Word_t index_outer; Word_t *pvalue_outer; */
/*   Word_t index_inner; Word_t *pvalue_inner; */
/*   Pvoid_t inner_judy;           /\* Inner judy arrays *\/ */
  
/*   /\* Traverse the outer JudyL *\/ */
/*   index_outer = 0; */
/*   JLF(pvalue_outer, wset, index_outer); */
/*   while (pvalue_outer != NULL) { */
/*     inner_judy = (Pvoid_t)*pvalue_outer; */
/*     /\* Traverse the inner JudyL *\/ */
/*     index_inner = 0; */
/*     JLF(pvalue_inner, inner_judy, index_inner); */
/*     while (pvalue_inner != NULL) { */
/*       vector_t vec = (vector_t)*pvalue_inner; */
/*       printf("===== WAITING SET (%u,%u) size=%u =====\n", (unsigned int)index_outer, */
/*              (unsigned int)index_inner, (unsigned int)VECTOR_LEN(vec)); */
/*       for (Word_t i = 0; i < VECTOR_LEN(vec); i++) { */
/*         patch_t patch = (patch_t)vec[i+2]; */
/*         print_patch(patch); */
/*       } */
/*       JLN(pvalue_inner, inner_judy, index_inner); */
/*     } */
/*     JLN(pvalue_outer, wset, index_outer); */
/*   } */
/* } */

/********************************** Testing ***********************************/

// int main(void) {
//   patch_t patch1 = make_patch1();
//   patch_t patch2 = make_patch2();
//   patch_t patch3 = make_patch3();
//   waiting_set_t wset = new_waiting_set();
// 
//   assert(waiting_set_empty(wset));
//   print_waiting_set(wset);
// 
//   LIFTERR(add_to_waiting_set(&wset, PACK_ID(1, 3), patch1));
//   LIFTERR(add_to_waiting_set(&wset, PACK_ID(1, 3), patch2));
//   LIFTERR(add_to_waiting_set(&wset, PACK_ID(2, 2), patch3));
// 
//   assert(!waiting_set_empty(wset));
//   print_waiting_set(wset);
// 
//   vector_t vec = take_waiting_set(&wset, PACK_ID(1,3));
//   printf("vec[%u]\n", (unsigned int)VECTOR_LEN(vec));
//   printf("----------------------\n"); print_waiting_set(wset);
//   LIFTERR(add_to_waiting_set(&wset, PACK_ID(1, 3), patch1));
//   LIFTERR(add_to_waiting_set(&wset, PACK_ID(1, 3), patch2));
//   free(vec);
// 
//   printf("----------------------\n"); print_waiting_set(wset);
// 
//   DELETE_WAITING_SET(wset, TRUE);
//   return 0;
// }
