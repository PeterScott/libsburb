#ifndef __VECTOR_WEAVE_H
#define __VECTOR_WEAVE_H

/* A weave consists of a pair of arrays. This struct has pointers for both. */
typedef struct {
  uint32_t capacity;            /* How many atoms could be in here */
  uint32_t length;              /* How many atoms actually are here */
  uint64_t *ids;                /* Array of ids */
  uint32_t *bodies;             /* Array of (pred, char) pairs */
  weft_t weft;                  /* Weft covering all atoms in weave */
  memodict_t memodict;          /* Id-to-weft memoization dict */
  waiting_set_t wset;           /* Waiting set: vectors of patches blocking on ids */
} weave_t;

weave_t new_weave(uint32_t capacity);
void delete_weave(weave_t weave);
void weave_print(weave_t weave);
weave_t apply_insvec(weave_t weave, vector_t insvec, uint32_t atom_count);

#endif
