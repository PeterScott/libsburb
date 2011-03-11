/* Snarfstrip: a simple driver program which reads in a data file containing a
   sequence of patches, applies those in turn to a blank weave, and then scours
   the weave. */

#include "sburb.h"

int main(int argc, char **argv) {
  weave_t weave = new_weave(128);

  /* Check for right number of args */
  if (argc != 2) {
    printf("usage: %s file\n", argv[0]);
    exit(1);
  }

  /* Open the input file. */
  FILE *file = fopen(argv[1], "r");
  if (file == NULL) {
    printf("%s: could not open file %s\n", argv[0], argv[1]);
    exit(1);
  }

  /* Read and apply the patches */
  unsigned int chain_count;
  unsigned int chain_lengths[4096];
  while (fscanf(file, "%u", &chain_count) == 1) {
    /* Read chain lengths, calculate atom count */
    uint32_t atom_count = 0;
    for (int i = 0; i < chain_count; i++) {
      int numsread = fscanf(file, "%u", &chain_lengths[i]);
      assert(numsread == 1);
      atom_count += chain_lengths[i];
    }

    /* Allocate everything and write header. */
    void *patch, *patch_cursor;
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
      uint32_t c, py, po, iy, io;
      int numsread = fscanf(file, "%u %u %u %u %u", &c, &py, &po, &iy, &io);
      assert(numsread == 5);
      uint64_t id = PACK_ID(iy, io), pred = PACK_ID(py, po);
      WRITE_ATOM_SEQ(id, pred, c, p32);
    }
    assert((uint8_t*)p32 - (uint8_t*)patch == patch_len);
    assert(patch_length_atoms(patch) == atom_count);
    
    /* Apply the patch, and free it. */
    LIFTERR(apply_patch(&weave, patch));
    free(patch);
  }

  weave_print(weave);
  
  /* Clean up and exit. */
  fclose(file);
  delete_weave(weave);
  return 0;
}