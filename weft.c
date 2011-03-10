/* A weft is represented as a JudyL array mapping yarns to maximum offsets. */

#include "sburb.h"

/* Allocate and return a new, blank weft. */
weft_t new_weft(void) {
  return (weft_t)NULL;
}

/* Delete a weft, and free its memory. */
void delete_weft(weft_t weft) {
  Word_t rc_word;
  JLFA(rc_word, weft);
}

/* Print a weft, as a sequence of lines, one per mapping. */
void weft_print(weft_t weft) {
  Word_t index; Word_t *pvalue;

  if (weft == NULL) {
    printf("[null weft]\n");
    return;
  }

  if (weft == ERRWEFT) {
    printf("[error weft]\n");
    return;
  }

  index = 0;
  JLF(pvalue, weft, index);
  while (pvalue != NULL) {
    printf("%lu\t%lu\n", index, *pvalue);
    JLN(pvalue, weft, index);
  }
  printf("\n");
}

/* Create a copy of a weft, in new memory. If there is a malloc() failure,
   returns ERRWEFT. */
weft_t copy_weft(weft_t from) {
  Word_t index, value; Word_t *pvalue;
  weft_t to = NULL;
  Word_t rc_word;

  index = 0;
  JLF(pvalue, from, index);
  while (pvalue != NULL) {
    value = *pvalue;
    JLI(pvalue, to, index);
    if (pvalue == PJERR) {
      JLFA(rc_word, to);
      return ERRWEFT;
    }
    *pvalue = value;
    JLN(pvalue, from, index);
  }

  return to;
}

/* Get the top of a given yarn. */
uint32_t weft_get(weft_t weft, uint32_t yarn) {
  Word_t *pvalue;

  /* Special case: all wefts have (0, 2). */
  if (yarn == 0) return 2;

  JLG(pvalue, weft, yarn);
  return (pvalue == NULL || pvalue == PJERR) ? 0 : *pvalue;
}

/* Set the top of a given yarn. Return 0 on success. Needs a pointer to the
   weft. */
int weft_set(weft_t *weft, uint32_t yarn, uint32_t offset) {
  Word_t *pvalue;
  weft_t temp = *weft;

  JLI(pvalue, temp, yarn);
  if (pvalue == PJERR) return -1; /* malloc() failure */
  *pvalue = offset; *weft = temp;
  return 0;
}

/* Extend the top of a given yarn. Return 0 on success. Needs a pointer to the
   weft. */
int weft_extend(weft_t *weft, uint32_t yarn, uint32_t offset) {
  Word_t *pvalue;
  weft_t temp = *weft;

  JLI(pvalue, temp, yarn);
  if (pvalue == PJERR) return -1; /* malloc() failure */
  *pvalue = MAX(*pvalue, offset); *weft = temp;
  return 0;
}

/* Does a weft cover a given atom id? Return a bool. As a special case, wefts
   implicitly cover (0, 1) and (0, 2). */
int weft_covers(weft_t weft, uint64_t id) {
  return OFFSET(id) <= weft_get(weft, YARN(id));
}

/* Merge the contents of another weft into this one, modifying only this
   one. The resulting weft will be a superweft of the two. Return 0 on
   success. */
int weft_merge_into(weft_t *dest, weft_t other) {
  Word_t yarn, offset; Word_t *pvalue;

  yarn = 0;
  JLF(pvalue, other, yarn);
  while (pvalue != NULL) {
    offset = *pvalue;
    LIFTERR(weft_extend(dest, yarn, offset));
    JLN(pvalue, other, yarn);
  }

  return 0;
}

/* Compare wefts: is a > b? */
int weft_gt(weft_t a, weft_t b) {
  Word_t *pvalue_a; Word_t index_a; Word_t a_len;
  Word_t *pvalue_b; Word_t index_b; Word_t b_len;

  /* Iterate */
  index_a = index_b = 0;
  JLF(pvalue_a, a, index_a); JLF(pvalue_b, b, index_b);
  while (pvalue_a != NULL && pvalue_b != NULL) {
    uint32_t my_yarn = index_a, my_offset = *pvalue_a;
    uint32_t other_yarn = index_b, other_offset = *pvalue_b;

    if (my_yarn < other_yarn) return 1;
    if (my_yarn > other_yarn) return 0;
    if (my_offset > other_offset) return 1;
    if (my_offset < other_offset) return 0;
    JLN(pvalue_a, a, index_a); JLN(pvalue_b, b, index_b);
  }

  JLC(a_len, a, 0, -1); JLC(b_len, b, 0, -1);
  if (a_len > b_len) return 1;
  else return 0;
}
  

/********************************* Debugging **********************************/
#ifdef DEBUG

/* Turn a string like "a5b3d1" into a weft, and return the new weft. Does not do
   error checking, and requires that the weft be de-allocated by the client. */
weft_t quickweft(const char *str) {
  weft_t weft = new_weft();

  for (int i = 0; i < strlen(str); i += 2)
    weft_set(&weft, str[i] - 'a' + 1, str[i+1] - '0');

  return weft;
}

/* Print a weft in quickweft() format. */
void quickweft_print(weft_t weft) {
  Word_t index; Word_t *pvalue;

  printf("<"); index = 0;
  JLF(pvalue, weft, index);
  while (pvalue != NULL) {
    printf("%c%lu", (char)(index & 0xFF) + 'a' - 1, *pvalue);
    JLN(pvalue, weft, index);
  }
  printf(">\n");
}

#endif


/********************************** Testing ***********************************/

// int main(void) {
//   weft_t weft = new_weft();
//
//   /* (3, 33) (0, 108) (7, 77) */
//   LIFTERR(weft_set(&weft, 3, 33));
//   LIFTERR(weft_set(&weft, 0, 108));
//   LIFTERR(weft_extend(&weft, 7, 2));
//   LIFTERR(weft_extend(&weft, 7, 77));
//   LIFTERR(weft_extend(&weft, 7, 32));
//
//   printf("weft[3] = %u (should be 33)\n", weft_get(weft, 3));
//   weft_print(weft);
//
//   weft_t weft2 = copy_weft(weft);
//   weft_print(weft2);
//
//   printf("11010: %i%i%i%i%i\n", weft_covers(weft, PACK_ID(7, 50)),
//          weft_covers(weft, PACK_ID(7, 77)), weft_covers(weft, PACK_ID(7, 78)),
//          weft_covers(weft, PACK_ID(3, 30)), weft_covers(weft, PACK_ID(2, 1)));
//
//   weft_t weft3 = new_weft();
//   LIFTERR(weft_set(&weft3, 5, 55));
//   LIFTERR(weft_set(&weft3, 3, 13));
//   LIFTERR(weft_set(&weft3, 7, 1234567));
//
//   LIFTERR(weft_merge_into(&weft, weft3));
//   weft_print(weft);             /* 0:108, 3:33, 5:55, 7:1234567 */
//
//   weft_t qweft = quickweft("a5b3d1");
//   quickweft_print(qweft);
//
//   delete_weft(weft);
//   delete_weft(weft2);
//   delete_weft(weft3);
//   delete_weft(qweft);
//   return 0;
// }

// int main(int argc, char **argv) {
//   if (argc != 3) {
//     printf("usage: %s weft1 weft2\n", argv[0]);
//     exit(1);
//   }
//   
//   weft_t a = quickweft(argv[1]);
//   weft_t b = quickweft(argv[2]);
// 
//   if (weft_gt(a, b))
//     printf("%s > %s\n", argv[1], argv[2]);
//   else
//     printf("%s <= %s\n", argv[1], argv[2]);
// 
//   return 0;
// }
