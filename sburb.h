#ifndef __SBURB_H
#define __SBURB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <Judy.h>

/* Serialization/deserialization */
inline void write_packed_uint32(uint32_t w, uint8_t **ptr);
inline uint32_t read_packed_uint32(uint8_t **ptr);

#endif
