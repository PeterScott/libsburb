#include "sburb.h"

/* Bytes of a 32-bit word: ABCD, where A is most significant. */
#define BYTE_A(word) (((word) & 0xFF000000) >> 24)
#define BYTE_B(word) (((word) & 0x00FF0000) >> 16)
#define BYTE_C(word) (((word) & 0x0000FF00) >> 8)
#define BYTE_D(word) ((word) & 0xFF)

/* Write a uint32 to ptr, and advance the pointer. */
inline void write_packed_uint32(uint32_t w, uint8_t **ptr) {
  uint8_t *p = *ptr;
  if (w < 254) {                /* Write in one byte */
    *p = w;
    *ptr = p+1;
  } else if (w < 65536) {       /* Use three bytes, big-endian */
    *p++ = 254;
    *p++ = BYTE_C(w);
    *p++ = BYTE_D(w);
    *ptr = p;
  } else {                      /* Write five bytes, big-endian */
    *p++ = 255;
    *p++ = BYTE_A(w);
    *p++ = BYTE_B(w);
    *p++ = BYTE_C(w);
    *p++ = BYTE_D(w);
    *ptr = p;
  }
}

/* Read a uint32 from ptr, advancing the pointer and returning the int. */
inline uint32_t read_packed_uint32(uint8_t **ptr) {
  uint8_t *p = *ptr;
  uint8_t b = *p++;
  uint32_t w = 0;

  if (b < 254) {
    w = b;
  } else if (b == 254) {
    w |= *p++;
    w <<= 8; w |= *p++;
  } else if (b == 255) {
    w |= *p++;
    w <<= 8; w |= *p++;
    w <<= 8; w |= *p++;
    w <<= 8; w |= *p++;
  }
  
  *ptr = p; return w;
}

// int main(void) {
//   uint8_t arr[128];
//   uint8_t *ptr = arr;
// 
//   write_packed_uint32(42, &ptr);
//   write_packed_uint32(0, &ptr);
//   write_packed_uint32(543, &ptr);
//   write_packed_uint32(99899, &ptr);
//   ptr = arr;
//   printf("%u\n", read_packed_uint32(&ptr));
//   printf("%u\n", read_packed_uint32(&ptr));
//   printf("%u\n", read_packed_uint32(&ptr));
//   printf("%u\n", read_packed_uint32(&ptr));
//   return 0;
// }
