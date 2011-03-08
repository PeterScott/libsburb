/* Waiting sets implemented as sparse arrays of patch pointers. Which in turn
   are represented as JudyL arrays mapping from indices to patch pointers. */

#include "sburb.h"

/* Allocate and return a new, empty waiting set. Does not actually allocate
   anything, so unless you put something in the waiting set, you don't actually
   need to free this. */
waitset_t new_waitset(void) {
  return (waitset_t)NULL;
}

/* Free a waiting set, deleting everything. If delete_patches is true, then it
   will also delete the patches themselves; otherwise it won't. */
void delete_waitset(waitset_t waitset, int delete_patches) {
  Word_t index; Word_t *pvalue; Word_t rc_word;

  if (delete_patches) {
    index = 0; JLF(pvalue, waitset, index);
    while (pvalue != NULL) {
      free((void *)*pvalue);
      JLN(pvalue, waitset, index);
    }
  }
  JLFA(rc_word, waitset);
}

/* Add a patch to the waiting set. Takes a pointer to a waiting set, and
   modifies it. Returns zero on success, nonzero on error. */
int add_to_waitset(waitset_t *wset, patch_t patch) {
  Word_t index; Word_t *pvalue; waitset_t temp = *wset;

  index = -1; JLL(pvalue, temp, index); index++;
  JLI(pvalue, temp, index);
  if (pvalue == PJERR) return -1; /* malloc() error */
  *pvalue = (Word_t)patch;
  *wset = temp;
  return 0;
}

/* Is the waiting set empty? */
int waitset_empty(waitset_t wset) {
  Word_t rc_word;
  JLC(rc_word, wset, 0, -1);
  return rc_word == 0;
}

/* Remove item i from the waitset. */
int remove_from_waitset(waitset_t *wset, int i) {
  Word_t index; waitset_t temp = *wset; int rc_int;

  index = (Word_t)i; JLD(rc_int, temp, index);
  *wset = temp; return rc_int != 1;
}

/* Print a waiting set, in a quite verbose format for debugging. */
void print_waitset(waitset_t wset) {
  Word_t index; Word_t *pvalue;

  index = 0; JLF(pvalue, wset, index);
  while (pvalue != NULL) {
    patch_t patch = (patch_t)*pvalue;
    print_patch(patch);
    JLN(pvalue, wset, index);
  }
}

/* Pop the oldest patch from the waitset, removing it. Returns NULL if the
   waitset is empty. */
patch_t waitset_pop(waitset_t *wset) {
  Word_t index; Word_t *pvalue; waitset_t temp = *wset; int rc_int;

  index = 0; JLF(pvalue, temp, index);
  if (pvalue == NULL) return NULL; /* empty waitset */
  patch_t patch = (patch_t)*pvalue;
  JLD(rc_int, temp, index);
  *wset = temp;
  return patch;
}

/********************************** Testing ***********************************/

// int main(void) {
//   patch_t patch1 = make_patch1();
//   patch_t patch2 = make_patch2();
//   patch_t patch3 = make_patch3();
//   waitset_t wset = new_waitset();
// 
//   assert(waitset_empty(wset));
//   print_waitset(wset);
// 
//   LIFTERR(add_to_waitset(&wset, patch1));
//   LIFTERR(add_to_waitset(&wset, patch2));
//   LIFTERR(add_to_waitset(&wset, patch3));
// 
//   assert(!waitset_empty(wset));
//   print_waitset(wset);
// 
//   LIFTERR(remove_from_waitset(&wset, 1));
//   printf("----------------------\n"); print_waitset(wset);
//   LIFTERR(add_to_waitset(&wset, patch2));
// 
//   printf("----------------------\n"); print_waitset(wset);
//   patch_t p1 = waitset_pop(&wset);
//   assert(p1 == patch1);
//   printf("----------------------\n"); print_waitset(wset);
//   LIFTERR(add_to_waitset(&wset, patch1));
//   printf("----------------------\n"); print_waitset(wset);
// 
//   delete_waitset(wset, TRUE);
//   return 0;
// }
