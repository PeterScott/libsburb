/* Vector weaves: a weave represented as a pair of arrays. Requires
   re-allocation and copying on any change. Simple, though. */

#include "sburb.h"

/* Allocate and return a new weave, blank but for the start and end atoms. The
   weft and memoization dicts are blank, and will work correctly, but do NOT
   need to be de-allocated unless you modify them.

   The weave will have the capacity to store potentially more atoms than you put
   in it. The argument capacity determines how many atoms the weave will
   allocate space for. If it's zero, then the weave will have a default capacity
   of 4 atoms. A capacity of 1 is invalid, and will be bumped up to the minimum
   of 2 needed to store the start and end atoms. */
weave_t new_weave(uint32_t capacity) {
  weave_t weave;
  if (capacity == 0) capacity = 4;
  if (capacity == 1) capacity = 2;
  weave.ids      = malloc(capacity * sizeof(uint64_t));
  weave.bodies   = malloc(capacity * 3 * sizeof(uint32_t));
  weave.length   = 2;
  weave.capacity = capacity;
  weave.weft     = (weft_t)NULL;
  weave.memodict = (memodict_t)NULL;
  weave.wset     = (waiting_set_t)NULL;
  uint64_t *ids  = weave.ids; uint32_t *bodies = weave.bodies;

  WRITE_ATOM(PACK_ID(0, 1), PACK_ID(0, 1), ATOM_CHAR_START, ids, bodies);
  WRITE_ATOM(PACK_ID(0, 2), PACK_ID(0, 1), ATOM_CHAR_END,   ids, bodies);
  return weave;
}

/* Delete a weave, and free its memory. */
void delete_weave(weave_t weave) {
  free(weave.ids); free(weave.bodies);
  delete_weft(weave.weft);
  delete_memodict(weave.memodict);
  delete_waiting_set(weave.wset, TRUE);
}

/* Print a weave, for debugging. Not a concise format! */
void weave_print(weave_t weave) {
  uint64_t id, pred; uint32_t c;
  uint64_t *ids = weave.ids; uint32_t *bodies = weave.bodies;

  for (int i = 0; i < weave.length; i++) {
    READ_ATOM(id, pred, c, ids, bodies);
    printf("<id: %u,%u\tpred: %u,%u\t",
           YARN(id), OFFSET(id), YARN(pred), OFFSET(pred));
    if (c < 128) printf("%c>\n", (char)c);
    else printf("0x%X>\n", c);
  }
  printf("\n");
}

/* Take a weave and a vector of alternating index, chain_len, chain* words, and insert
   those atoms into the weave, in-place. Does not modify weft. */
weave_t apply_insvec_inplace(weave_t weave, vector_t insvec, uint32_t atom_count) {
  int displacement = atom_count; /* How far to move items to the right */
  int vec_len = (int)VECTOR_LEN(insvec); /* Remaining length of insertion vector */
  Word_t *vec_head = insvec + 2 + vec_len - 3; /* Head of insvec pairs */

  weave.length += atom_count;

  for (int i = weave.length - 1; i >= 0; i--) {
    uint64_t id, pred; uint32_t c;
    //printf("\ni = %i, displacement = %i\n", i, displacement);
    //printf("vec_head[0] = %i, i - displacement+1 = %i\n",
    //       vec_len > 0 ? (int)vec_head[0] : 69, i - displacement+1);
    if (vec_len == 0) break;
    if (vec_len > 0 && i - displacement + 1 == (int)vec_head[0]) {
      int chain_len = (int)vec_head[1];
      uint32_t *chain = (uint32_t *)vec_head[2];
      // Copy over the chain.
      for (int j = i - chain_len + 1; j <= i; j++) {
        //printf("Copying from %i to %i\t[i=%i]\n", removeme, j, i);
        READ_ATOM_SEQ(id, pred, c, chain);
        //printf("COPY id: %llu, pred: %llu, c: %u\n", id, pred, c);
        WRITE_ATOM_IDX(id, pred, c, weave.ids, weave.bodies, j);
      }
      i -= chain_len; i++; displacement -= chain_len;
      vec_len -= 3; vec_head -= 3;
    } else {
      //printf("Moving from %i to %i\n", i - displacement, i);
      READ_ATOM_IDX(id, pred, c, weave.ids, weave.bodies, i - displacement);
      //printf("MOVE id: %llu, pred: %llu, c: %u\n", id, pred, c);
      WRITE_ATOM_IDX(id, pred, c, weave.ids, weave.bodies, i);
    }
  }

  return weave;
}

/* Take a weave and a vector of alternating index, chain* words, and insert
   those atoms into the weave, allocating new memory to hold the atom_count new
   atoms. Does not modify weft. */
weave_t apply_insvec_alloc(weave_t weave, vector_t insvec, uint32_t atom_count) {
  int vec_len = (int)VECTOR_LEN(insvec); /* Remaining length of insertion vector */
  Word_t *vec_head = insvec + 2; /* Head of insvec pairs */
  /* Old value variables */
  uint64_t *old_ids = weave.ids; uint32_t *old_bodies = weave.bodies;
  void *old_ids_head = weave.ids; void *old_bodies_head = weave.bodies;
  weave.length += atom_count;
  
  /* Allocate new weave vectors. New capacity is lowest power of two greater
     than length; e.g. if weave.length is 21, then capacity will be 32. */
  weave.capacity = (uint32_t)pow(2.0, ceil(log2((double)weave.length)));
  weave.ids      = malloc(weave.capacity * sizeof(uint64_t));
  weave.bodies   = malloc(weave.capacity * 3 * sizeof(uint32_t));
  uint64_t *new_ids = weave.ids; uint32_t *new_bodies = weave.bodies;
  
  /* Copy items, inserting chains in their proper places. */
  for (int i = 0, k = 0; i < weave.length; i++) {
    uint64_t id, pred; uint32_t c; /* Current atom */
    if (vec_len > 0 && k == (int)vec_head[0]) { /* chain here */
      int chain_len = (int)vec_head[1]; uint32_t *chain = (uint32_t *)vec_head[2];
      for (int j = 0; j < chain_len; j++) {
        READ_ATOM_SEQ(id, pred, c, chain);
        WRITE_ATOM(id, pred, c, new_ids, new_bodies);
      }
      i += chain_len - 1;
      vec_len -= 3; vec_head += 3;
    } else {
      READ_ATOM(id, pred, c, old_ids, old_bodies);
      WRITE_ATOM(id, pred, c, new_ids, new_bodies);
      k++;
    }
  }

  free(old_ids_head); free(old_bodies_head);
  return weave;
}

/* Take a weave and a vector of alternating index, chain* words, and insert
   those atoms into the weave. You must explicitly tell this function how many
   atoms will be inserted, so that it can allocate the right amount of
   memory. Does not modify weft. */
weave_t apply_insvec(weave_t weave, vector_t insvec, uint32_t atom_count) {
  if (atom_count < weave.capacity)
    return apply_insvec_inplace(weave, insvec, atom_count);
  else
    return apply_insvec_alloc(weave, insvec, atom_count);
}

/********************************** Testing ***********************************/

int main(void) {
  weave_t w = new_weave(40);
  weave_print(w);

  patch_t patch1 = make_patch1();
  //patch_t patch2 = make_patch2();
  patch_t patch3 = make_patch3();

  void *chain1 = patch_atoms(patch1);
  void *chain3 = patch_atoms(patch3);
  
  vector_t insvec = new_vector();
  insvec = vector_append(insvec, 1);
  insvec = vector_append(insvec, 4);
  insvec = vector_append(insvec, (Word_t)chain1);
  insvec = vector_append(insvec, 2);
  insvec = vector_append(insvec, 1);
  insvec = vector_append(insvec, (Word_t)chain3);
  
  w = apply_insvec(w, insvec, 5);
  weave_print(w);

  delete_weave(w);
  return 0;
}
