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
  return -1;                    /* FIXME */
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
  uint8_t arr8[1024*16];        /* 15 kB array */
  void *ptr = arr8;

  write_patch_header(&ptr, 42, 3);  

  printf("Length: %u bytes\n", patch_length_bytes(arr8));
  printf("Number of chains: %u\n", patch_chain_count(arr8));

  return 0;
}
