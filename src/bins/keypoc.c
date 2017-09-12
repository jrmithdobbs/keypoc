#include "util.h"
#include "storage.h"
#include "modauth.h"

#include <unistd.h>

enum { Cempty = 0LL, Cdecoded, Cencoded };

#pragma pack(1)
typedef struct keypoc_state {
  challenge             c;
  challenge_plugin_hdr *mod;
  uint64_t              final;
  uint64_t              state;
  uint64_t              nexts;
  char                 *mod_path;
  char                 *mod_name;
} __attribute__((__aligned__(32))) keypoc_config;
#pragma pack(0)

static keypoc_config gconfig = {
  .final=Cencoded,
  .state=Cempty,
  .nexts=Cdecoded,
  .mod_path="./lib",
  .mod_name="hmacsha2mem",
};

int main(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, ":dehim:")) != -1) {
    switch (c) {
      case 'd': // decode
        gconfig.final = Cdecoded;
        gconfig.state = Cencoded;
        gconfig.nexts = Cdecoded;
        break;
      case 'e': // encode
        gconfig.final = Cencoded;
        gconfig.state = Cdecoded;
        gconfig.nexts = Cencoded;
        break;
      case 'i': // init
        gconfig.final = Cencoded;
        gconfig.state = Cempty;
        gconfig.nexts = Cdecoded;
        break;
      case 'm': // auth module (default: hmacsha2mem)
        gconfig.mod_name = strdup(optarg);
        break;

      // getopt record keeping
      case ':':
          printf("Option '-%c' requires argument.\n\n", optopt);
          return 1;
      case '?':
          printf("Unknown option: '-%c'.\n\n", optopt);
      default:
      case 'h':
        printf("TODO: Usage text here.\n");
        return 1;
    }
  }

  // TODO: read password from stdin
  uint8_t pw[] = "testpassword";
  uint64_t pwlen = strlen((char*)pw);

  // load auth module
  gconfig.mod = load_challenge_plugin(gconfig.mod_name, gconfig.mod_path);
  if (gconfig.mod == NULL) {
    printf("Could not load auth module: %s", gconfig.mod_name);
  }

  // Just mock transitions of our inefficiently represented state machine.
  //_debugprint("empty:", challenge, gconfig.c);

  challenge_new_random_tf_key(&gconfig.c, gconfig.mod);
  gconfig.state = Cdecoded;
  gconfig.nexts = Cencoded;
  //_debugprint("plain:", challenge, gconfig.c);

  challenge_encode(&gconfig.c, gconfig.mod, pw, pwlen);
  gconfig.state = Cencoded;
  //_debugprint("encoded:", challenge, gconfig.c);

  challenge_decode(&gconfig.c, gconfig.mod, pw, pwlen);
  gconfig.state = Cdecoded;
  //_debugprint("decoded:", challenge, gconfig.c);

  close_challenge_plugin(gconfig.mod);

  return 0;
}
