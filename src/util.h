#ifndef __KEYPOC_UTIL_H
#define __KEYPOC_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sodium/core.h>
#include <sodium/randombytes.h>
#include <sodium/crypto_pwhash_argon2i.h>
#include <sodium/crypto_stream_chacha20.h>
#include <sodium/crypto_auth_hmacsha256.h>
#include <sodium/crypto_generichash_blake2b.h>
#include <sodium/utils.h>

#define _debugprintend(t,c) (((uint8_t*)&(c)) + sizeof(t))
#define _debugprint(n,t,c)\
    { printf("%s\n\n", (n)); \
      uint8_t *p; \
      for (p = (uint8_t*) &(c) ; p < _debugprintend(t,c); ++p) { \
        printf("%02x", *p); \
        if (((size_t)p) % 16 == 15) printf("\n"); \
        else if (p < _debugprintend(t,c) - 1 && ((size_t)p) % 4 == 3) printf("-"); \
      } if (((size_t)p) % 16 != 0) printf("\n"); }
#ifdef DEBUG_PRINT
#define debugprint(n,t,c) _debugprint(n,t,c)
#else
#define debugprint(n,t,c) {}
#endif

#ifdef __cplusplus
}
#endif

#endif // __KEYPOC_UTIL_H
