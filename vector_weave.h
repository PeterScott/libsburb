#ifndef __VECTOR_WEAVE_H
#define __VECTOR_WEAVE_H

/* A weave consists of a pair of arrays. This struct has pointers for both. */
typedef struct {
  uint32_t capacity;       /* How many atoms could be in here */
  uint32_t length;         /* How many atoms actually are here */
  uint64_t *ids;           /* Array of ids */
  uint32_t *bodies;        /* Array of (pred, char) pairs */
  weft_t weft;             /* Weft covering all atoms in weave */
  memodict_t memodict;     /* Id-to-weft memoization dict */
  waitset_t wset;          /* Waiting set: vectors of patches */
} weave_t;

/* The state of a weave traversal. */
typedef struct {
  uint64_t *ids;
  uint32_t *bodies;
  uint32_t remaining_atoms;
} weave_traversal_state_t;

weave_t new_weave(uint32_t capacity);
void delete_weave(weave_t weave);
void weave_print(weave_t weave);
weave_t apply_insvec(weave_t weave, vector_t insvec, uint32_t atom_count);
int apply_patch(weave_t *weave, patch_t patch);
weave_traversal_state_t starting_traversal_state(weave_t weave);
int scour(wchar_t *buf, int buflen, weave_traversal_state_t *wts);

#endif
