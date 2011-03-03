#ifndef __SBURB_H
#define __SBURB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <Judy.h>

/***************************** General utilities ******************************/

/* Evaluate the expression. If it is not zero, return -1. */
#define LIFTERR(expr) do { if ((expr) != 0) return -1; } while (0);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define TRUE  1
#define FALSE 0

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

/* Read an atom from a location, sequentially. Location pointer (uint32_t *) is
   incremented. Pass in only variable names. */
#define READ_ATOM_SEQ(id, pred, c, ptr) do {  \
    id   = *((uint64_t *)ptr); ptr += 2;      \
    pred = *((uint64_t *)ptr); ptr += 2;      \
    c = *ptr++;                               \
  } while (0);

/* Write an atom to a location, sequentially. Location pointer (uint32_t *) is
   incremented. Pass in only variable names. */
#define WRITE_ATOM_SEQ(id, pred, c, ptr) do { \
    *(uint64_t *)ptr = id;   ptr += 2;        \
    *(uint64_t *)ptr = pred; ptr += 2;        \
    *ptr++ = c;                               \
  } while (0);

/* The four special atom characters. These are the only invisible chars. */
#define ATOM_CHAR_START 0xE000
#define ATOM_CHAR_END   0xE001
#define ATOM_CHAR_DEL   0xE002
#define ATOM_CHAR_SAVE  0xE003

/* Is an atom character c visible? May evaluate c twice. */
#define ATOM_CHAR_IS_VISIBLE(c) ((c) < 0xE000 || (c) > 0xE003)


/******************************* Data typedefs ********************************/

/* A weft_t is a pointer to the weft structure itself. */
typedef Pvoid_t weft_t;

/* A memodict is a JudyL array of JudyL arrays of wefts. */
typedef Pvoid_t memodict_t;

/* A vector is represented as an array of machine words, with the first one
   telling the size of the array (including the first two words), the second
   telling the number of array elements used by data, and the rest being the
   data itself. This can serve as an array of pointers, with amortized O(1)
   append. It must be freed with free() by the client code. */
typedef Word_t* vector_t;

/* A patch, or rather, a pointer to a patch data structure. */
typedef void* patch_t;

/*********************************** Wefts ************************************/

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

/* Safer deletor macro. Sets pointer to NULL afterward. */
#define DELETE_MEMODICT(md) do { delete_memodict(md); md = (memodict_t)NULL; } while (0);

memodict_t new_memodict(void);
void delete_memodict(memodict_t memodict);
void memodict_print(memodict_t memodict);
int memodict_add(memodict_t *memodict, uint64_t id, weft_t weft);


/*********************************** Weaves ***********************************/

#include "vector_weave.h"


/************************** Waiting sets and vectors **************************/

/* Get the nth element of a vector. */
#define VECTOR_GET(vector, n) ((vector)[(n) + 2])
/* Get the number of elements in a vector. */
#define VECTOR_LEN(vector) ((vector)[1])

vector_t new_vector(void);
vector_t vector_append(vector_t vector, Word_t word);

typedef Pvoid_t waiting_set_t;

waiting_set_t new_waiting_set(void);
void delete_waiting_set(waiting_set_t wset, int delete_patches);
vector_t take_waiting_set(waiting_set_t *wset, uint64_t id);
int add_to_waiting_set(waiting_set_t *wset, uint64_t blocking_id, patch_t patch);
int waiting_set_empty(waiting_set_t wset);
void print_waiting_set(waiting_set_t wset);

/* Safer deletor macro. Sets pointer to NULL afterward. */
#define DELETE_WAITING_SET(wset, delete_patches) do {                     \
    delete_waiting_set(wset, delete_patches); wset = (waiting_set_t)NULL; \
  } while (0);

/********************************** Patches ***********************************/

#define READ_PATCH_HEADER(length_bytes, chain_count, ptr) do {               \
    length_bytes = patch_length_bytes((patch_t)ptr);                         \
    chain_count  = patch_chain_count((patch_t)ptr);                          \
    ptr = ((uint8_t *)ptr) + 5;                                              \
  } while (0);

#define READ_CHAIN_DESCRIPTOR(offset, len_atoms, ptr) do {                   \
    offset = *(uint32_t *)ptr; ptr = (typeof (ptr))((uint32_t *)ptr + 1);    \
    len_atoms = *(uint16_t *)ptr; ptr = (typeof (ptr))((uint16_t *)ptr + 1); \
  } while (0);

uint32_t patch_length_bytes(patch_t patch);
uint8_t patch_chain_count(patch_t patch);
uint32_t patch_length_atoms(patch_t patch);
void *patch_atoms(patch_t patch);
uint32_t patch_necessary_buffer_length(uint8_t chain_count, uint32_t atom_count);
void write_patch_header(void **dest, uint32_t length_bytes, uint8_t chain_count);
void write_chain_descriptor(void **dest, uint32_t offset, uint16_t len_atoms);
size_t chain_size_bytes(uint32_t atom_count);
void write_chain(void **dest, void *src, uint32_t atom_count);
void print_patch(patch_t patch);
uint64_t patch_blocking_id(patch_t patch, weft_t weft);


/**************************** Debugging functions *****************************/
#ifdef DEBUG

weft_t quickweft(const char *str);
void quickweft_print(weft_t weft);

patch_t make_patch1(void);
patch_t make_patch2(void);
patch_t make_patch3(void);

#endif


#endif
