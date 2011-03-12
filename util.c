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

/* Shorthand for patches: each shorthand patch is a string, in a
   Grishchenko-like format. Atoms are in the format [char][pred][id], where char
   is a character, and pred and id are sequences of two characters which
   translate to an atom id. An ASCII atom id such as "b3" will turn into (2,3),
   because 'b' is the second letter of the alphabet. There are two special
   characters: ^ is a deletor, and * is save-awareness.

   This function takes a shorthand string and creates a new patch, which must be
   freed by the client code. It also takes the number of chains, and an array of
   the chain lengths. Note that this does no bounds checking, so if you give the
   wrong arguments, there will be crashing. */
patch_t shorthand_to_patch(char *shorthand, uint8_t chain_count, uint32_t *chain_lengths) {
  /* Allocate everything and write header. */
  void *patch, *patch_cursor;
  uint32_t atom_count = 0;
  for (int i = 0; i < chain_count; i++) atom_count += chain_lengths[i];
  uint32_t patch_len = patch_necessary_buffer_length(chain_count, atom_count);
  patch = malloc(patch_len); patch_cursor = patch;
  write_patch_header(&patch_cursor, patch_len, chain_count);
  
  /* Write the chain descriptors. */
  uint32_t offset = 0;
  for (int i = 0; i < chain_count; i++) {
    write_chain_descriptor(&patch_cursor, offset, chain_lengths[i]);
    offset += chain_size_bytes(chain_lengths[i]);
  }

  /* Write the atoms themselves. */
  uint32_t *p32 = patch_cursor;
  for (int i = 0; i < atom_count; i++) {
    int pos = i*5;              /* position in shorthand string */
    char c_char = shorthand[pos];
    char c_pred[2]; c_pred[0] = shorthand[pos+1]; c_pred[1] = shorthand[pos+2];
    char c_id[2]; c_id[0] = shorthand[pos+3]; c_id[1] = shorthand[pos+4];

    uint64_t id = PACK_ID(c_id[0] == '0' ? 0 : (uint32_t)(c_id[0] - 'a' + 1),
                          (uint32_t)(c_id[1] - '0'));
    uint64_t pred = PACK_ID(c_pred[0] == '0' ? 0 : (uint32_t)(c_pred[0] - 'a' + 1),
                            (uint32_t)(c_pred[1] - '0'));
    uint32_t c = (uint32_t)c_char;
    if (c == '^') c = ATOM_CHAR_DEL;
    if (c == '*') c = ATOM_CHAR_SAVE;

    WRITE_ATOM_SEQ(id, pred, c, p32);
  }
  assert((uint8_t*)p32 - (uint8_t*)patch == patch_len);
  assert(patch_length_atoms(patch) == atom_count);
  return patch;
}

#define VECTOR_SCOUR_PRINT_BUFLEN 256

void weave_scour_print(weave_t weave) {
  weave_traversal_state_t wts = starting_traversal_state(weave);
  wchar_t buf[VECTOR_SCOUR_PRINT_BUFLEN]; int len;

  while ((len = scour(buf, VECTOR_SCOUR_PRINT_BUFLEN, &wts)) > 0)
    for (int i = 0; i < len; i++)
      putchar(buf[i]);
}

#endif
