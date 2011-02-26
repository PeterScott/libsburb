/* An id-to-weft memoization dict. This stores awareness wefts for every atom
   which has a predecessor in another yarn. If this invariant is maintained,
   then it makes O(1) pulling of awareness wefts for any atom possible. 

   Internally, it's a JudyL array of JudyL arrays. The outer array has yarns as
   keys; the inner array has offsets as keys. To look up an id in this monstrosity, here's
   the procedure:

   1. Look up the yarn. If it's not found, return an empty weft. Otherwise:
   2. Look up the offset or its earliest ancestor, using JLL to find the index,
      and JLG to get the associated weft. Neither of these should fail; if they
      do, then return an error code.
   3. Return a copy of this weft, extended to cover the current id. Hooray!
  
   To add a weft to the monstrosity:
  
   1. Look up the yarn. If it's not found, create a new JudyL array; call this
      array =inner=. Insert an (offset -> weft) mapping into =inner=, and insert
      (yarn -> =inner=) into the outer array. Return. OTHERWISE:
   2. Use JLI to ensure that there is a mapping of offset->weft. If there's
      already a weft at that position in the array, de-allocate it. Then set the
      pointer (from JLI) to point to the new weft.
   3. Return a success value! Hooray!
*/

#include "sburb.h"


/* Allocate and return a new, empty memoization dict. */
memodict_t new_memodict(void) {
  return (memodict_t)NULL;
}

/* Delete a memoization dict, and free its memory. Also frees the memory of all
   wefts in the dict, so be careful if you were sharing those with other code. */
void delete_memodict(memodict_t memodict) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Word_t rc_word;               /* Return status. Not used. */
  Pvoid_t inner_judy;           /* Inner judy arrays */
  /* Traverse the outer JudyL */
  index_outer = 0;
  JLF(pvalue_outer, memodict, index_outer);
  while (pvalue_outer != NULL) {
    inner_judy = (Pvoid_t)*pvalue_outer;
    /* Traverse the inner JudyL, freeing wefts */
    index_inner = 0;
    JLF(pvalue_inner, inner_judy, index_inner);
    while (pvalue_inner != NULL) {
      delete_weft((weft_t)*pvalue_inner);
      JLN(pvalue_inner, inner_judy, index_inner);
    }
    /* Free the inner JudyL, and proceed to the next one */
    JLFA(rc_word, inner_judy);
    JLN(pvalue_outer, memodict, index_outer);
  }
  JLFA(rc_word, memodict);
}

/* Print a memoization dict, for debugging purposes. */
void memodict_print(memodict_t memodict) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy;           /* Inner judy arrays */
  /* Traverse the outer JudyL */
  index_outer = 0;
  JLF(pvalue_outer, memodict, index_outer);
  while (pvalue_outer != NULL) {
    inner_judy = (Pvoid_t)*pvalue_outer;
    /* Traverse the inner JudyL */
    index_inner = 0;
    JLF(pvalue_inner, inner_judy, index_inner);
    while (pvalue_inner != NULL) {
      printf("/-----------------------------\\\n");
      printf("  ID: %u, %u\n", (uint32_t)index_outer, (uint32_t)index_inner);
      weft_print((weft_t)*pvalue_inner);
      printf("\\-----------------------------/\n\n");
      JLN(pvalue_inner, inner_judy, index_inner);
    }
    JLN(pvalue_outer, memodict, index_outer);
  }
}

/* Add an (id, weft) pair to a memoization dict. You must give a pointer to the
   memoization dict to be modified. If there is already a weft mapped to the
   given id, then that previous weft will be deallocated and replaced by the new
   one. */
int memodict_add(memodict_t *memodict, uint64_t id, weft_t weft) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy;           /* Inner judy arrays */
  memodict_t temp = *memodict;

  /* Look up the yarn. */
  index_outer = YARN(id);
  JLI(pvalue_outer, temp, index_outer);
  if (pvalue_outer == PJERR) return -1; /* malloc() error */
  if (*pvalue_outer == 0) {
    /* No entry was found for the yarn. Create a new yarn JudyL with a single
       mapping in it, and add that to the memodict. */
    inner_judy = (Pvoid_t)NULL; index_inner = OFFSET(id);
    JLI(pvalue_inner, inner_judy, index_inner);
    if (pvalue_inner == PJERR) return -1; /* malloc() error */
    *pvalue_inner = (Word_t)weft;
    *pvalue_outer = (Word_t)inner_judy;
  } else {
    inner_judy = (Pvoid_t)*pvalue_outer; index_inner = OFFSET(id);
    JLI(pvalue_inner, inner_judy, index_inner);
    if (pvalue_inner == PJERR) return -1; /* malloc() error */
    if (*pvalue_inner != 0) delete_weft((weft_t)*pvalue_inner);
    *pvalue_inner = (Word_t)weft;
    *pvalue_outer = (Word_t)inner_judy;
  }

  *memodict = temp; return 0;
}

/* Look up an id in a memoization dict. Returns either an empty weft, or the
   weft in the given yarn with the highest offset less than or equal to the
   given offset. Does not copy or modify any wefts, nor allocate new ones. */
weft_t memodict_get(memodict_t memodict, uint64_t id) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy;           /* Inner judy arrays */

  /* Look up the yarn. */
  index_outer = YARN(id);
  JLG(pvalue_outer, memodict, index_outer);
  if (pvalue_outer == NULL || pvalue_outer == PJERR) {
    /* Yarn not found. Return empty weft. */
    return new_weft();
  } else {
    /* Look up offset or earliest ancestor. */
    inner_judy = (Pvoid_t)*pvalue_outer; index_inner = OFFSET(id);
    JLL(pvalue_inner, inner_judy, index_inner);
    if (pvalue_inner == NULL) return new_weft();
    return (weft_t)*pvalue_inner;
  }
}

/* Pull the awareness weft of a given atom id, assuming a properly filled-out
   memoization dict. This allocates and returns a new weft, which must be
   explicitly freed by the caller. Optionally takes a predecessor id; if 0 is
   passed in place of the predecessor id, then it will be ignored.

   In the event of an error, returns ERRWEFT. */
weft_t pull(memodict_t memodict, uint64_t id, uint64_t pred) {
  weft_t weft = copy_weft(memodict_get(memodict, id));
  if (weft == ERRWEFT) return ERRWEFT;
  if (weft_extend(&weft, YARN(id), OFFSET(id)) != 0) return ERRWEFT;

  if (pred != 0) {
    weft_t pred_weft = memodict_get(memodict, pred); /* not extended */
    if (weft_merge_into(&weft, pred_weft) != 0) {
      delete_weft(weft);
      return ERRWEFT;
    }
    if (weft_extend(&weft, YARN(pred), OFFSET(pred)) != 0) return ERRWEFT;
  }

  return weft;
}

/********************************* Debugging **********************************/

// void print_keys(Pvoid_t judy) {
//   Word_t index = 0; Word_t *pvalue;
// 
//   JLF(pvalue, judy, index);
//   while (pvalue != NULL) {
//     printf("Index: %llu\n", (uint64_t)index);
//     JLN(pvalue, judy, index);
//   }
// }

weft_t demoweft(void) {
  weft_t weft = new_weft();

  /* (3, 33) (0, 108) (7, 77) */
  weft_set(&weft, 3, 33);
  weft_set(&weft, 0, 108);
  weft_extend(&weft, 7, 2);
  weft_extend(&weft, 7, 77);
  weft_extend(&weft, 7, 32);

  return weft;
}

int main(void) {
  weft_t weft1 = demoweft(), weft2 = demoweft();
  memodict_t memodict = new_memodict();
  weft_set(&weft1, 1, 1111);
  weft_set(&weft2, 2, 2222);

  LIFTERR(memodict_add(&memodict, PACK_ID(1, 119), weft1));
  LIFTERR(memodict_add(&memodict, PACK_ID(2, 229), weft2));
  LIFTERR(memodict_add(&memodict, PACK_ID(2, 69), copy_weft(weft2)));
  LIFTERR(memodict_add(&memodict, PACK_ID(2, 229), demoweft()));
  memodict_print(memodict);

  printf("[1]\n"); weft_print(memodict_get(memodict, PACK_ID(1, 30)));  /* null */
  printf("[2]\n"); weft_print(memodict_get(memodict, PACK_ID(1, 119))); /* weft1 */
  printf("[3]\n"); weft_print(memodict_get(memodict, PACK_ID(1, 125))); /* weft1 */
  printf("[4]\n"); weft_print(memodict_get(memodict, PACK_ID(3, 33)));  /* null */
  printf("[5]\n"); weft_print(memodict_get(memodict, PACK_ID(2, 70)));  /* weft2 */
  printf("[6]\n"); weft_print(memodict_get(memodict, PACK_ID(2, 230))); /* demoweft */

  DELETE_MEMODICT(memodict);
  return 0;
}
