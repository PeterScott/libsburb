#include "sburb.h"

/***************************** Atom serialization *****************************/

/* Atoms are stored in two parallel arrays. The id array stores the ids, and the
   body array stores (pred, char) pairs. Ids are (yarn, weft) pairs, and
   chars are 32-bit unsigned ints. Hooray for UTF-32, I guess. */

int main(void) {
  uint64_t id_a[64];
  uint32_t body_a[64*3];

  uint64_t id; uint64_t pred; uint32_t c; uint64_t *id_ptr; uint32_t *body_ptr;
  id_ptr = id_a; body_ptr = body_a;
  id = PACK_ID(4, 44);
  pred = PACK_ID(666, 6543210);
  c = 'Q';

  printf("id:(%u, %u) pred:(%u, %u) '%c'\n", YARN(id), OFFSET(id), YARN(pred), OFFSET(pred), c);
  WRITE_ATOM(id, pred, c, id_ptr, body_ptr);

  id = PACK_ID(0, 42);
  pred = PACK_ID(77, 108);
  c = 'Z';

  printf("id:(%u, %u) pred:(%u, %u) '%c'\n", YARN(id), OFFSET(id), YARN(pred), OFFSET(pred), c);
  WRITE_ATOM(id, pred, c, id_ptr, body_ptr);

  id = 0; pred = 0; c = '%'; id_ptr = id_a; body_ptr = body_a;

  READ_ATOM(id, pred, c, id_ptr, body_ptr);
  printf("id:(%u, %u) pred:(%u, %u) '%c'\n", YARN(id), OFFSET(id), YARN(pred), OFFSET(pred), c);
  READ_ATOM(id, pred, c, id_ptr, body_ptr);
  printf("id:(%u, %u) pred:(%u, %u) '%c'\n", YARN(id), OFFSET(id), YARN(pred), OFFSET(pred), c);

  return 0;
}
