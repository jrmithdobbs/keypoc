#ifndef __KEYTOOL_STORAGE_H
#define __KEYTOOL_STORAGE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#pragma pack(1)
typedef uint8_t key_store_b[64];
typedef struct key_store { union {
  struct {
    uint8_t tf[32];
    uint8_t in[32];
  };
  key_store_b b;
}; } key_store __attribute__((__aligned__(32)));

typedef uint8_t aont_key_b[48];
typedef struct aont_key { union {
  struct {
    uint8_t nonce[16];
    uint8_t input[16];
  };
  aont_key_b b;
};} aont_key __attribute__((__aligned__(32)));

typedef struct challenge {
  union {
    struct {
      uint8_t   pw_nonce[16];
      uint8_t   tf_nonce[16];
    };
    uint8_t nonce[32];
  };
  key_store keys;
  aont_key aont;
} challenge __attribute__((__aligned__(32)));
#pragma pack(0)

void challenge_new_random_tf_key(challenge * c);
void challenge_new_with_tf_key(challenge * c, const uint8_t * k);
void challenge_encode(challenge * c, const uint8_t * pw, uint64_t pwlen);
void challenge_decode(challenge * c, const uint8_t * pw, uint64_t pwlen, uint8_t *resp, uint64_t resplen);

#ifdef DEBUG_PRINT
#define debugprint(n,t,c)\
    { printf("%s\n", (n)); \
      for (uint8_t *p = (uint8_t*) &(c) ; p < ((uint8_t*)&(c) ) + sizeof(t); ++p) { \
        printf("%02x", *p); \
        if (((size_t)p) % 16 == 15) printf("\n"); \
        else if (((size_t)p) % 4 == 3) printf("-"); \
      } }

#else
#define debugprint(n,t,c) {}
#endif

#ifdef __cplusplus
}
#endif

#endif