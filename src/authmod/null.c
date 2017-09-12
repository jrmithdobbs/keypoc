#include "modauth.h"

#include <unistd.h>

// forward decls
int challenge(uint8_t *in, int ilen, uint8_t* out, int olen);
int setckey(uint8_t *k, int klen);
int savekey(void);
int test(void);

#define buf_sz (4096 - sizeof(challenge_plugin_hdr))

// This union provides a full page of memory, header included, for the plugin
// and app to communicate.
static struct { union {
  challenge_plugin_hdr hdr;
  uint8_t buf[4096];
}; } challenge_plugin_info = { .hdr = {
    .challenge = &challenge,
    .setkey = &setckey,
    .savekey = &savekey,
    .test = &test,
    .key_max = 32,
    .buf_max = buf_sz,
    .out_max = 20,
  },
};

// Entry point the app looks for to find the challenge_plugin_hdr
challenge_plugin_hdr *challenge_discover (void)
{
  return &challenge_plugin_info.hdr;
}

int challenge(uint8_t *in, int ilen, uint8_t* out, int olen) {
  sodium_memzero(out, olen);
  return 0;
}

int setckey(uint8_t *k, int klen) {
  return 0;
}

// no-op for in-memory fobs
int savekey(void) {
  return 0;
}

// future
int test(void) {
  return 0;
}
