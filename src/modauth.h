#ifndef __KEYTOOL_AUTHMODULE_H
#define __KEYTOOL_AUTHMODULE_H

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"

#pragma pack(1)
typedef int AMM_Challenge(uint8_t*,int,uint8_t*,int);
typedef int AMM_SetKey(uint8_t*,int);
typedef int AMM_SaveKey(void);
typedef int AMM_Test(void);

// This is page aligned.
typedef struct challenge_plugin_hdr {
  union {
    struct {
      AMM_Challenge *challenge;
      AMM_SetKey    *setkey;
      AMM_SaveKey   *savekey;
      AMM_Test      *test;
      void *mod;
      uint64_t _pad;
      uint32_t key_max;
      uint32_t buf_max;
      uint64_t out_max;
    };
    uint64_t _[8];
  };
  uint8_t  buf[];
} __attribute__((__aligned__(4096))) challenge_plugin_hdr;

typedef challenge_plugin_hdr * AMM_Discover(void);
typedef int ModF(uint8_t*,int,uint8_t*,int);
#pragma pack(0)

challenge_plugin_hdr * load_challenge_plugin(char *pname, char *mod_path);
void close_challenge_plugin(challenge_plugin_hdr *c);

#ifdef __cplusplus
}
#endif

#endif
