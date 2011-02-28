/* Patches: a set of chains. */

#include "sburb.h"

/* A patch is a collection of chains. Each chain is a sequence of atoms (in
   sequential format), consisting of a single type of atom: insertions,
   deletors, or save-awareness atoms. Each atom in an insertion chain must have
   the previous atom as its predecessor, except for the first one (the
   head). Every atom in a patch must be in the same yarn. Every atom in a patch
   (except for the head of the first chain) must have an offset one higher than
   the atom before it. This has a few implications:

   1. You can determine the id range of a patch trivially. Call the yarn and
      offset of the head atom y0 and o0, and the length (in atoms) len. The ids
      in the patch range from (y0, o0) to (y0, o0 + len - 1).

   2. You can check the predecessors-outside-patch rule with a simple range
      check, once you've used property (1).

   The patch format is as follows:

   <length in bytes: uint32_t><number-of-chains: uint8_t>
   # Offsets in bytes, and lengths in atoms, of each chain. Offsets relative to
   # [MARK]. Can use this to calculate total length, in atoms.
   <chain1-offset-len: uint32_t, uint16_t>, <chain2-offset-len: uint32_t, uint16_t>, ...
   <atoms of chain1><atoms of chain2> ...
*/

/****************************** Reading patches *******************************/

/* Return the length of a patch, in bytes. */
inline uint32_t patch_length_bytes(patch_t patch) {
  return *(uint32_t *)patch;
}

/* Return the number of chains in a patch. */
inline uint8_t patch_chain_count(patch_t patch) {
  return *((uint8_t *)patch + 4);
}

/* Return the length of a patch, in atoms. */
uint32_t patch_length_atoms(patch_t patch) {
  uint8_t *ptr = patch; ptr += 5;
  uint32_t offset; uint16_t len_atoms = 0; uint32_t atom_count = 0;

  for (int i = patch_chain_count(patch); i > 0; i--) {
    READ_CHAIN_DESCRIPTOR(offset, len_atoms, ptr);
    atom_count += len_atoms;
  }

  return atom_count;
}

/* Return a pointer to the start of the atoms in a patch. */
inline void *patch_atoms(patch_t patch) {
  uint8_t *ptr = patch;
  ptr += 5;
  ptr += 6 * patch_chain_count(patch);
  return ptr;
}

/****************************** Writing patches *******************************/

/* Return the necessary buffer length, in bytes, to hold a patch with a given
   number of chains and atoms. Includes header. */
uint32_t patch_necessary_buffer_length(uint8_t chain_count, uint32_t atom_count) {
  uint32_t len = 5;                 /* header */
  len += 6 * (uint32_t)chain_count; /* chain descriptors */
  len += 20 * atom_count;           /* atoms */
  return len;
}

/* Write the length and chain count of a patch to a buffer, and advance the
   pointer past there. This takes a pointer to a pointer. */
void write_patch_header(void **dest, uint32_t length_bytes, uint8_t chain_count) {
  uint32_t *p32 = *dest;
  uint8_t *p8 = (uint8_t *)(p32 + 1);

  *p32 = length_bytes; *p8 = chain_count;
  *dest = (void *)(p8 + 1);
}

/* Write the offset and length of a chain descriptor. Offset is in bytes,
   relative to the start of the atoms in the chain buffer. Length is in
   atoms. */
void write_chain_descriptor(void **dest, uint32_t offset, uint16_t len_atoms) {
  uint32_t *p32 = *dest;
  uint16_t *p16 = (uint16_t *)(p32 + 1);

  *p32 = offset; *p16 = len_atoms;
  *dest = (void *)(p16 + 1);
}

/* Calculate the chain size, in bytes, given the atom count. */
inline size_t chain_size_bytes(uint32_t atom_count) {
  return 20 * (size_t)atom_count;
}

/* Write atom_count atoms from src to dest, advancing dest. */
void write_chain(void **dest, void *src, uint32_t atom_count) {
  size_t size = chain_size_bytes(atom_count);
  uint8_t *p8 = *dest; p8 += size;
  memcpy(*dest, src, size);
  *dest = (void *)p8;
}

int main(void) {
  /* Patch 1: Alice types "Test" */
  void *patch1, *patch1_cursor;
  uint32_t patch1_len = patch_necessary_buffer_length(1, 4);
  patch1 = malloc(patch1_len); patch1_cursor = patch1;
  write_patch_header(&patch1_cursor, patch1_len, 1);
  write_chain_descriptor(&patch1_cursor, 0, 4);
  uint32_t *p32 = patch1_cursor;
  WRITE_ATOM_SEQ(PACK_ID(1,1), PACK_ID(0,1), 'T', p32);
  WRITE_ATOM_SEQ(PACK_ID(1,2), PACK_ID(1,1), 'e', p32);
  WRITE_ATOM_SEQ(PACK_ID(1,3), PACK_ID(1,2), 's', p32);
  WRITE_ATOM_SEQ(PACK_ID(1,4), PACK_ID(1,3), 't', p32);
  assert((uint8_t*)p32 - (uint8_t*)patch1 == patch1_len);
  assert(patch_length_atoms(patch1) == 4);

  printf("Patch 1: Alice types 'Test'\n");
  printf("  Length: %u bytes\n", patch_length_bytes(patch1));
  printf("  Number of chains: %u\n", patch_chain_count(patch1));
  printf("  Number of atoms: %u\n", patch_length_atoms(patch1));

  /* Patch 2: Bob deletes 's', inserts 'x' */
  void *patch2, *patch2_cursor;
  uint32_t patch2_len = patch_necessary_buffer_length(2, 2);
  patch2 = malloc(patch2_len); patch2_cursor = patch2;
  write_patch_header(&patch2_cursor, patch2_len, 2);
  write_chain_descriptor(&patch2_cursor, 0, 1);
  write_chain_descriptor(&patch2_cursor, chain_size_bytes(1), 1);
  p32 = patch2_cursor;
  WRITE_ATOM_SEQ(PACK_ID(2,1), PACK_ID(1,3), ATOM_CHAR_DEL, p32);
  WRITE_ATOM_SEQ(PACK_ID(2,2), PACK_ID(1,2), 'x', p32);
  assert((uint8_t*)p32 - (uint8_t*)patch2 == patch2_len);
  assert(patch_length_atoms(patch2) == 2);

  printf("Patch 2: Bob deletes 's', inserts 'x'\n");
  printf("  Length: %u bytes\n", patch_length_bytes(patch2));
  printf("  Number of chains: %u\n", patch_chain_count(patch2));
  printf("  Number of atoms: %u\n", patch_length_atoms(patch2));

  /* Patch 3: Alice saves awareness of Bob's patches */
  void *patch3, *patch3_cursor;
  uint32_t patch3_len = patch_necessary_buffer_length(1, 1);
  patch3 = malloc(patch3_len); patch3_cursor = patch3;
  write_patch_header(&patch3_cursor, patch3_len, 1);
  write_chain_descriptor(&patch3_cursor, 0, 1);
  p32 = patch3_cursor;
  WRITE_ATOM_SEQ(PACK_ID(1,5), PACK_ID(2,2), ATOM_CHAR_SAVE, p32);
  assert((uint8_t*)p32 - (uint8_t*)patch3 == patch3_len);
  assert(patch_length_atoms(patch3) == 1);

  printf("Patch 3: Alice saves awareness of Bob's patches\n");
  printf("  Length: %u bytes\n", patch_length_bytes(patch3));
  printf("  Number of chains: %u\n", patch_chain_count(patch3));
  printf("  Number of atoms: %u\n", patch_length_atoms(patch3));
  

  free(patch1); free(patch2); free(patch3);
  return 0;
}
