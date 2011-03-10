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
  weave.wset     = (waitset_t)NULL;
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
  delete_waitset(weave.wset, TRUE);
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


/***************************** Insertion vectors ******************************/

/* Take a weave and a vector of alternating index, chain_len, chain* words, and insert
   those atoms into the weave, in-place. Does not modify weft. */
weave_t apply_insvec_inplace(weave_t weave, vector_t insvec, uint32_t atom_count) {
  int displacement = atom_count; /* How far to move items to the right */
  int vec_len = (int)VECTOR_LEN(insvec); /* Remaining length of insertion vector */
  Word_t *vec_head = insvec + 2 + vec_len - 3; /* Head of insvec pairs */

  weave.length += atom_count;

  for (int i = weave.length - 1; i >= 0; i--) {
    uint64_t id, pred; uint32_t c;
    // printf("\ni = %i, displacement = %i\n", i, displacement);
    // printf("vec_head[0] = %i, i - displacement+1 = %i\n",
    //        vec_len > 0 ? (int)vec_head[0] : 69, i - displacement+1);
    if (vec_len == 0) break;
    if (vec_len > 0 && i - displacement + 1 == (int)vec_head[0]) {
      int chain_len = (int)vec_head[1];
      uint32_t *chain = (uint32_t *)vec_head[2];
      // Copy over the chain.
      for (int j = i - chain_len + 1; j <= i; j++) {
        READ_ATOM_SEQ(id, pred, c, chain);
        //printf("COPY id: %llu, pred: %llu, c: %X\n", id, pred, c);
        WRITE_ATOM_IDX(id, pred, c, weave.ids, weave.bodies, j);
        /* Add to memodict if necessary */
        if (YARN(id) != YARN(pred))
          memodict_add(&weave.memodict, id, pull(weave.memodict, id, pred));
      }
      /* Add chain to weft */
      weft_extend(&weave.weft, YARN(id), OFFSET(id));
      i -= chain_len; i++; displacement -= chain_len;
      vec_len -= 3; vec_head -= 3;
    } else {
      //printf("Moving from %i to %i\n", i - displacement, i);
      READ_ATOM_IDX(id, pred, c, weave.ids, weave.bodies, i - displacement);
      //printf("MOVE id: %llu, pred: %llu, c: %X\n", id, pred, c);
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
        /* Add to memodict if necessary */
        //if (YARN(id) != YARN(pred))
        //  memodict_add(&weave.memodict, id, pull(weave.memodict, id, pred));
      }
      /* Add chain to weft */
      weft_extend(&weave.weft, YARN(id), OFFSET(id));
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
  /* Debugging: show insvec */
/* #ifdef DEBUG */
/*   printf("INSVEC: [atoms: %u] ", atom_count); */
/*   for (int i = 0; i < VECTOR_LEN(insvec); i++) */
/*     printf("%llu ", (long long int)VECTOR_GET(insvec, i)); */
/*   printf("\n"); */
/*   uint32_t *chain = (uint32_t*)VECTOR_GET(insvec, 2); */
/*   uint64_t id, pred; uint32_t c; */
/*   READ_ATOM_SEQ(id, pred, c, chain); */
/*   printf("[id:(%u,%u)\tpred:(%u,%u)\t%x]\n", YARN(id), OFFSET(id), */
/*          YARN(pred), OFFSET(pred), (int)c); */
/* #endif */

  if (atom_count < weave.capacity)
    return apply_insvec_inplace(weave, insvec, atom_count);
  else
    return apply_insvec_alloc(weave, insvec, atom_count);
}


/************************** Predecessor lookup dicts **************************/

/* A predecessor lookup dict is one of two types of JudyL arrays. A deldict maps
   from ids to deletion atoms. An insdict maps from ids to insrecs. An insrec
   contains a pointer to a chain, and the chain length. These things are used in
   the patch insertion process; constructed during a preprocessing phase and
   then used during a one-pass traversal of the weave. */

typedef Pvoid_t deldict_t;
typedef Pvoid_t insdict_t;
typedef struct {
  void *chain;
  uint16_t len_atoms;
} insrec_t;

/* Insert a pointer to a thing into either an insdict or a deldict. The thing
   should be either an atom (for deldicts) or an insrec (for insdicts). */
int indeldict_insert(Pvoid_t *dict, uint64_t id, void *thing) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy;           /* Inner judy arrays */
  Pvoid_t temp = *dict;

  index_outer = YARN(id); JLI(pvalue_outer, temp, index_outer);
  if (pvalue_outer == PJERR) return -1; /* malloc() error */
  if (*pvalue_outer == 0) {
    inner_judy = (Pvoid_t)NULL; index_inner = OFFSET(id);
  } else {
    inner_judy = (Pvoid_t)*pvalue_outer; index_inner = OFFSET(id);
  }

  JLI(pvalue_inner, inner_judy, index_inner);
  if (pvalue_inner == PJERR) return -1; /* malloc() error */
  *pvalue_inner = (Word_t)thing;
  *pvalue_outer = (Word_t)inner_judy;
  *dict = temp; return 0;
}

/* Get the thing corresponding to the given id in an insdict or deldict. Returns
   NULL if nothing was found corresponding to that id. */
void *indeldict_get(Pvoid_t dict, uint64_t id) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy;

  /* Look up the yarn */
  if (dict == NULL) return NULL; /* common case: empty set */
  index_outer = YARN(id); JLG(pvalue_outer, dict, index_outer);
  if (pvalue_outer == PJERR) return NULL; /* malloc() error */
  if (pvalue_outer == NULL) return NULL;  /* yarn not found */
  
  /* Yarn found. Look up the offset. */
  inner_judy = (Pvoid_t)*pvalue_outer; index_inner = OFFSET(id);
  JLG(pvalue_inner, inner_judy, index_inner);
  if (pvalue_inner == PJERR) return NULL; /* malloc() error */
  if (pvalue_inner == NULL) return NULL;  /* offset not found */
  
  /* Id found. Return thing. */
  return (void *)*pvalue_inner;
}

/* Delete an insdict, de-allocating all insrecs in it. */
void delete_insdict(insdict_t insdict) {
  Word_t index_outer; Word_t *pvalue_outer;
  Word_t index_inner; Word_t *pvalue_inner;
  Pvoid_t inner_judy; Word_t rc_word;

  /* Traverse the outer JudyL */
  index_outer = 0;
  JLF(pvalue_outer, insdict, index_outer);
  while (pvalue_outer != NULL) {
    inner_judy = (Pvoid_t)*pvalue_outer;
    /* Traverse the inner JudyL, freeing insrecs */
    index_inner = 0;
    JLF(pvalue_inner, inner_judy, index_inner);
    while (pvalue_inner != NULL) {
      free((void *)*pvalue_inner);
      JLN(pvalue_inner, inner_judy, index_inner);
    }
    /* Free the inner JudyL, and proceed to the next one */
    JLFA(rc_word, inner_judy);
    JLN(pvalue_outer, insdict, index_outer);
  }
  JLFA(rc_word, insdict);
}

/* Delete a deldict. */
void delete_deldict(deldict_t deldict) {
  Word_t index_outer; Word_t *pvalue_outer;
  Pvoid_t inner_judy; Word_t rc_word;

  /* Traverse the outer JudyL */
  index_outer = 0;
  JLF(pvalue_outer, deldict, index_outer);
  while (pvalue_outer != NULL) {
    inner_judy = (Pvoid_t)*pvalue_outer;
    JLFA(rc_word, inner_judy);
    JLN(pvalue_outer, deldict, index_outer);
  }
  JLFA(rc_word, deldict);
}

/* Allocate an insrec with a given chain and number of atoms in the chain. Has no
   overhead over making it manually. Must be freed by the user. */
static inline insrec_t *make_insrec(void *chain, uint16_t len_atoms) {
  insrec_t *insrec = malloc(sizeof(insrec_t));
  insrec->chain = chain; insrec->len_atoms = len_atoms;
  return insrec;
}


/************************ Making insdicts and deldicts ************************/

/* Take a patch that we've previously verified is ready to apply, and make the
   insdict and deldict for it. Takes pointers to an insdict and a deldict, which
   should initially be empty, and modifies them. Returns 0 on success.

   How this works is, it goes through all the chains in the patch. For insertion
   chains, it creates an insrec and inserts that into insdict. For deletion
   chains, it iterates through all the atoms and adds each one to the deldict.
*/
int make_indeldict(patch_t patch, insdict_t *insdict, deldict_t *deldict, 
                   weave_t *weave) {
  uint32_t *p32 = patch; void *ptr = patch;
  uint32_t length_bytes; uint8_t chain_count;
  READ_PATCH_HEADER(length_bytes, chain_count, ptr); p32 = ptr;
  uint16_t *chain_lengths = malloc(chain_count * sizeof(uint16_t));

  /* Read chain lengths */
  for (uint32_t chain = 0; chain < chain_count; chain++) {
    uint16_t len_atoms = 0; uint32_t offset = 0;
    READ_CHAIN_DESCRIPTOR(offset, len_atoms, p32);
    chain_lengths[chain] = len_atoms;
  }

  /* Process each chain */
  for (uint32_t chain = 0; chain < chain_count; chain++) {
    uint64_t id, pred; uint32_t c;
    READ_ATOM_SEQ(id, pred, c, p32); p32 -= 5; /* peek */
    //    printf("$ Processing patch atom: (%u,%u),\t(%u,%u),\t%X\n",
    //           YARN(id), OFFSET(id), YARN(pred), OFFSET(pred), (int)c);
    if (c == ATOM_CHAR_DEL) {
      /* Deletion chain. Add deletors to deldict. */
      for (uint16_t i = chain_lengths[chain]; i > 0; i--) {
        READ_ATOM_SEQ(id, pred, c, p32);
        LIFTERR(indeldict_insert(deldict, pred, (void*)(p32 - 5)));
        if (YARN(id) != YARN(pred))
          memodict_add(&(weave->memodict), id, pull(weave->memodict, id, pred));
      }
      continue;
    } else if (c == ATOM_CHAR_SAVE) {
      /* Save-awareness chain. Add to insrec for end atom. */
      LIFTERR(indeldict_insert(insdict, PACK_ID(0,2),
                               (void*)make_insrec((void*)p32, chain_lengths[chain])));
    } else {
      /* Regular insertion chain. Create insrec and add to insdict. */
      LIFTERR(indeldict_insert(insdict, pred,
                               (void*)make_insrec((void*)p32, chain_lengths[chain])));
    }
    for (uint16_t i = chain_lengths[chain]; i > 0; i--) {
      READ_ATOM_SEQ(id, pred, c, p32);
      if (YARN(id) != YARN(pred))
        memodict_add(&(weave->memodict), id, pull(weave->memodict, id, pred));
    }
  }
  free(chain_lengths);
  return 0;
}

/****************************** Applying patches ******************************/

/* Apply a patch to a weave, modifying the weave. Takes a pointer to the weave,
   so it can modify it. Returns 0 on success. Does not check patch validity.

   This may just put the patch in the waiting set, and it will not go through
   the waiting set after applying the patch to try to pull out waiting patches,
   so that's all up to the client code. */
int apply_patch(weave_t *weave, patch_t patch) {
  /* Check if the patch is ready to insert. If not, block. */
  if (patch_blocking_id(patch, weave->weft) != 0) {
    uint64_t id = patch_blocking_id(patch, weave->weft);
    printf("blocking on (%u,%u)\n", YARN(id), OFFSET(id));
    add_to_waitset(&weave->wset, patch);
    return 0;
  }

  /* Build insdict and deldict */
  insdict_t insdict = NULL; deldict_t deldict = NULL;
  LIFTERR(make_indeldict(patch, &insdict, &deldict, weave));
  print_judyl2(insdict);

  /* Iterate through the weave, looking at each atom to see if it's an anchor
     for anything in the insdict or deldict. If so, add that to an insertion
     vector. */
  vector_t insvec = new_vector();
  uint64_t id, pred; uint32_t c;
  uint64_t *ids = weave->ids; uint32_t *bodies = weave->bodies;

  for (uint32_t i = 0; i < weave->length; i++) {
    READ_ATOM(id, pred, c, ids, bodies);
    
    /* Check deldict */
    void *delatom = indeldict_get(deldict, id);
    if (delatom != NULL) {
      insvec = vector_append(insvec, (Word_t)i+1);
      insvec = vector_append(insvec, 1);
      insvec = vector_append(insvec, (Word_t)delatom);
      continue;
    }
    /* Check insdict */
    insrec_t *insrec = indeldict_get(insdict, id);
    if (insrec != NULL) {
      /* Easy insertion: save-awareness chains */
      uint32_t *irptr = insrec->chain;
      uint64_t id_head, pred_head; uint32_t c_head;
      READ_ATOM_SEQ(id_head, pred_head, c_head, irptr);
      if (c_head == ATOM_CHAR_SAVE) {
        printf("+ Saving awareness\n");
        insvec = vector_append(insvec, (Word_t)i+1);
        insvec = vector_append(insvec, (Word_t)insrec->len_atoms);
        insvec = vector_append(insvec, (Word_t)insrec->chain);
        goto cont;
      }

      /* Begin local subtraversal */
      uint64_t *ids_local = ids; uint32_t *bodies_local = bodies;

      /* Skip past any deletor atoms, and put cursor after them. */
      uint64_t id_neighbor, pred_neighbor; uint32_t c_neighbor;
      do {
        READ_ATOM(id_neighbor, pred_neighbor, c_neighbor, ids_local, bodies_local);
      } while (c_neighbor == ATOM_CHAR_DEL);
      ids_local--; bodies_local -= 3;

      /* Pull the awareness weft of the insrec's head. */
      weft_t head_weft = pull(weave->memodict, id_head, pred_head);
      weft_print(head_weft);

      // NOTE: we can and should break out of here early.
      for (uint32_t j = i; j <= weave->length;) {
        /* Peek at right neighbor. If we're aware of it, insert chain here. */
        if (weft_covers(head_weft, id_neighbor)) {
          printf("+ Aware of neighbor insertion\n");
          insvec = vector_append(insvec, (Word_t)j+1);
          insvec = vector_append(insvec, (Word_t)insrec->len_atoms);
          insvec = vector_append(insvec, (Word_t)insrec->chain);
          free(head_weft); goto cont;
        }

        /* We must insert in weft order. First, check if my weftI is greater
           than that of the atom to my right (that I see with cur.current()),
           which I shall call r. If so, insert me here. */
        uint64_t rid = id_neighbor;
        weft_t r_weft = pull(weave->memodict, rid, 0);
        if (weft_gt(head_weft, r_weft)) { /* Insert here. */
          printf("+ Weftgt insertion\n");
          insvec = vector_append(insvec, (Word_t)j+1);
          insvec = vector_append(insvec, (Word_t)insrec->len_atoms);
          insvec = vector_append(insvec, (Word_t)insrec->chain);
          free(head_weft); free(r_weft); goto cont;
        }

        /* Step past the causal block of r, and try looking at the new right
           neighbor on the next iteration of this loop. We do this by stepping
           until READ_ATOM returns an atom whose predecessor p is not r, and
           where r is aware of p. If we come to the end, we will run into the
           end atom sentinel, which will stop the iteration and give us our
           insertion point. */
        uint64_t p; uint32_t cur_c;
        ids_local++; bodies_local += 3; /* Skip past neighbor */
        while (!(p != rid && weft_covers(r_weft, p))) {
          READ_ATOM(id_neighbor, p, cur_c, ids_local, bodies_local); j++;
          printf("Skipping causal block: (%u,%u)\n", YARN(id_neighbor), OFFSET(id_neighbor));
        }
        free(r_weft);
      }
      printf("WTF??\n");
      free(head_weft); return -1;                /* What happened? */
      
      cont: continue;           /* Good end */
    }
  }
  
  delete_insdict(insdict); delete_deldict(deldict);

  /* Apply the insertion vector */
  *weave = apply_insvec(*weave, insvec, patch_length_atoms(patch));
  free(insvec);

  /* Update the weft */
  uint64_t high_id = patch_highest_id(patch);
  LIFTERR(weft_extend(&weave->weft, YARN(high_id), OFFSET(high_id)));
  return 0;
}

/********************************** Testing ***********************************/

// int main(void) {
//   weave_t w = new_weave(40);
//   weave_print(w);
// 
//   patch_t patch1 = make_patch1();
//   patch_t patch2 = make_patch2();
//   patch_t patch3 = make_patch3();
// 
//   void *chain1 = patch_atoms(patch1);
//   void *chain3 = patch_atoms(patch3);
//   
//   vector_t insvec = new_vector();
//   insvec = vector_append(insvec, 1);
//   insvec = vector_append(insvec, 4);
//   insvec = vector_append(insvec, (Word_t)chain1);
//   insvec = vector_append(insvec, 2);
//   insvec = vector_append(insvec, 1);
//   insvec = vector_append(insvec, (Word_t)chain3);
//   
//   w = apply_insvec(w, insvec, 5); free(insvec);
//   weave_print(w);
//   printf("WEFT:\n"); weft_print(w.weft);
//   printf("MEMODICT:\n"); memodict_print(w.memodict);
// 
//   /* Look at deldicts */
//   deldict_t deldict = NULL;
//   LIFTERR(indeldict_insert(&deldict, PACK_ID(2, 22), (void*)22242));
//   LIFTERR(indeldict_insert(&deldict, PACK_ID(2, 18), (void*)21842));
//   LIFTERR(indeldict_insert(&deldict, PACK_ID(5, 55), (void*)55542));
// 
//   printf("(2,22) => %li\n", (long)indeldict_get(deldict, PACK_ID(2, 22)));
//   printf("(2,18) => %li\n", (long)indeldict_get(deldict, PACK_ID(2, 18)));
//   printf("(5,55) => %li\n", (long)indeldict_get(deldict, PACK_ID(5, 55)));
// 
//   delete_deldict(deldict);
// 
//   /* Look at insdicts */
//   insdict_t insdict = NULL;
//   LIFTERR(indeldict_insert(&insdict, PACK_ID(2, 22), make_insrec((void*)22242, 111)));
//   LIFTERR(indeldict_insert(&insdict, PACK_ID(2, 18), make_insrec((void*)21842, 222)));
//   LIFTERR(indeldict_insert(&insdict, PACK_ID(5, 55), make_insrec((void*)55542, 333)));
//   
//   printf("(2,22) => %li\n", (long)(((insrec_t*)indeldict_get(insdict, PACK_ID(2, 22)))->chain));
//   printf("(2,18) => %li\n", (long)(((insrec_t*)indeldict_get(insdict, PACK_ID(2, 18)))->chain));
//   printf("(5,55) => %li\n", (long)(((insrec_t*)indeldict_get(insdict, PACK_ID(5, 55)))->chain));
// 
//   delete_insdict(insdict);
// 
//   /* Make insdict and deldict from patches, and show them. */
//   deldict = NULL; insdict = NULL;
//   LIFTERR(make_indeldict(patch1, &insdict, &deldict));
//   print_judyl2(insdict); print_judyl2(deldict);
//   delete_insdict(insdict); delete_deldict(deldict);
//   printf("\n");
// 
//   deldict = NULL; insdict = NULL;
//   LIFTERR(make_indeldict(patch2, &insdict, &deldict));
//   print_judyl2(insdict); print_judyl2(deldict);
//   delete_insdict(insdict); delete_deldict(deldict);
//   printf("\n");
// 
//   deldict = NULL; insdict = NULL;
//   LIFTERR(make_indeldict(patch3, &insdict, &deldict));
//   print_judyl2(insdict); print_judyl2(deldict);
//   delete_insdict(insdict); delete_deldict(deldict);
//   printf("\n");
// 
//   delete_weave(w);
//   free(patch1); free(patch2); free(patch3);
//   return 0;
// }

// int main(void) {
//   weave_t w = new_weave(40);
//   weave_print(w);
// 
//   uint32_t lens[4] = {4, 1, 1, 1};
//   patch_t patch1 = shorthand_to_patch("T01a1ea1a2sa2a3ta3a4", 1, lens);
//   patch_t patch2 = shorthand_to_patch("^a3b1xa2b2", 2, lens+1);
//   patch_t patch3 = shorthand_to_patch("*b2a5", 1, lens+3);
// 
//   LIFTERR(apply_patch(&w, patch1));
//   weave_print(w);
// 
//   LIFTERR(apply_patch(&w, patch2));
//   weave_print(w);
// 
//   LIFTERR(apply_patch(&w, patch3));
//   weave_print(w);
//   
//   printf("WEFT:\n");     weft_print(w.weft);
//   printf("MEMODICT:\n"); memodict_print(w.memodict);
//   delete_weave(w);
//   free(patch1); free(patch2); free(patch3);
//   return 0;
// }

int main(void) {
  weave_t w = new_weave(400);
  weave_print(w);

  uint32_t lens[5] = {4, 1, 1, 1, 1};
  patch_t patch1 = shorthand_to_patch("H01a1oa1a2pa2a3~a3a4", 1, lens); /* Hop~ */
  patch_t patch2 = shorthand_to_patch("*a4b1ia3b2", 2, lens+1);         /* i */
  patch_t patch3 = shorthand_to_patch("*a4c1!a3c2", 2, lens+3);         /* ! */
  /* Goal: either Hopi!~ or Hop!i~, but not both. */

  LIFTERR(apply_patch(&w, patch1)); weave_print(w);
  LIFTERR(apply_patch(&w, patch2)); weave_print(w);
  LIFTERR(apply_patch(&w, patch3)); weave_print(w);
  
  printf("WEFT:\n");     weft_print(w.weft);
  printf("MEMODICT:\n"); memodict_print(w.memodict);
  delete_weave(w);
  free(patch1); free(patch2); free(patch3);
  return 0;
}
