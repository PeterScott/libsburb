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
}

/********************************** Testing ***********************************/

int main(void) {
  weave_t w = new_weave();
  weave_print(w);

  delete_weave(w);
  return 0;
}
