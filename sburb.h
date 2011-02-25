#ifndef __SBURB_H
#define __SBURB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <Judy.h>

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

#endif
