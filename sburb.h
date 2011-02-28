#ifndef __SBURB_H
#define __SBURB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <Judy.h>

/***************************** General utilities ******************************/

/* Evaluate the expression. If it is not zero, return -1. */
#define LIFTERR(expr) do { if ((expr) != 0) return -1; } while (0);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*********************** Serialization/deserialization ************************/

/* Yarn and offset getters, for atom ids. */
#define YARN(id)   ((uint32_t)(((id) & (uint64_t)0xFFFFFFFF00000000) >> 32))
#define OFFSET(id) ((uint32_t)((id) & (uint64_t)0xFFFFFFFF))
#define PACK_ID(yarn, offset) (((uint64_t)(yarn) << 32) | (uint64_t)(offset))

/* Read an atom from a location. Location pointers are incremented. Pass in only
   variable names. */
#define READ_ATOM(id, pred, c, id_ptr, body_ptr) do {  \
    id = *id_ptr++;                                    \
    pred = *((uint64_t *)body_ptr);                    \
    body_ptr += 2;                                     \
    c = *body_ptr++;                                   \
  } while (0);

/* Write an atom to a location. Location pointers are incremented. Pass in only
   variable names. */
#define WRITE_ATOM(id, pred, c, id_ptr, body_ptr) do { \
    *id_ptr++ = id;                                    \
    *(uint64_t *)body_ptr = pred; body_ptr += 2;       \
    *body_ptr++ = c;                                   \
  } while (0);

/* The four special atom characters. These are the only invisible chars. */
#define ATOM_CHAR_START 0xE000
#define ATOM_CHAR_END   0xE001
#define ATOM_CHAR_DEL   0xE002
#define ATOM_CHAR_SAVE  0xE003

/* Is an atom character c visible? May evaluate c twice. */
#define ATOM_CHAR_IS_VISIBLE(c) ((c) < 0xE000 || (c) > 0xE003)


/*********************************** Wefts ************************************/

/* A weft_t is a pointer to the weft structure itself. */
typedef Pvoid_t weft_t;

/* Not an actual weft, but an error value. */
#define ERRWEFT ((weft_t)(-1))

/* Safer deletor macro. Sets pointer to NULL afterward. */
#define DELETE_WEFT(weft) do { delete_weft(weft); weft = (weft_t)NULL; } while (0);

weft_t new_weft(void);
void delete_weft(weft_t weft);
void weft_print(weft_t weft);
weft_t copy_weft(weft_t from);
uint32_t weft_get(weft_t weft, uint32_t yarn);
int weft_set(weft_t *weft, uint32_t yarn, uint32_t offset);
int weft_extend(weft_t *weft, uint32_t yarn, uint32_t offset);
int weft_covers(weft_t weft, uint64_t id);
int weft_merge_into(weft_t *dest, weft_t other);

/************************ Id-to-weft memoization dicts ************************/

/* A memodict is a JudyL array of JudyL arrays of wefts. */
typedef Pvoid_t memodict_t;

/* Safer deletor macro. Sets pointer to NULL afterward. */
#define DELETE_MEMODICT(md) do { delete_memodict(md); md = (memodict_t)NULL; } while (0);

memodict_t new_memodict(void);
void delete_memodict(memodict_t memodict);
void memodict_print(memodict_t memodict);
int memodict_add(memodict_t *memodict, uint64_t id, weft_t weft);


/*********************************** Weaves ***********************************/

#include "vector_weave.h"

/**************************** Debugging functions *****************************/
#ifdef DEBUG

weft_t quickweft(const char *str);
void quickweft_print(weft_t weft);

#endif


#endif
