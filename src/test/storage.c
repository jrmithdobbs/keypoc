#include "src/storage.c"

#include <unistd.h>

static int challenge_test(char *mod_name, char *mod_path) {
  static uint8_t * pw = (uint8_t*) "whowhatwhere";
  static uint64_t pwlen = 0;

  pwlen = strlen((char*)pw);

  challenge_plugin_hdr *mod = load_challenge_plugin(mod_name, mod_path);
  if (mod == NULL) {
    printf("Error loading module.\n");
    return 1;
  }

  challenge c, c1;
  challenge_new_random_tf_key(&c, mod);
  memcpy(&c1, &c, sizeof(challenge));

  challenge_encode(&c, mod, pw, pwlen);
  challenge_decode(&c, mod, pw, pwlen);

  if (memcmp(&c, &c1, sizeof(challenge)) != 0) {
    #ifdef DEBUG_PRINT
    printf("\nFailed!\n\n");
    #endif
    debugprint("\nc1:",challenge, c1);
    return 0;
  }
  else {
    #ifdef DEBUG_PRINT
    printf("\n");
    #endif
    printf("Success!\n");
    return 0;
  }
  return 1;
}

typedef struct test_config {
  char * mod_name;
  char * mod_path;
} test_config;

static char mod_name_d[] = "null";
static test_config gconfig = {
  .mod_name = mod_name_d,
  .mod_path = NULL,
};

int main(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, ":hm:p:")) != -1) {
    switch (c) {
      case 'p': // auth module load path (default: lib)
        gconfig.mod_path = strdup(optarg);
        break;
      case 'm': // auth module (default: hmacsha2mem)
        gconfig.mod_name = strdup(optarg);
        break;

      // getopt record keeping
      case ':':
          printf("Option '-%c' requires argument.\n", optopt);
          return 1;
      case '?':
          printf("Unknown option: '-%c'.\n\n", optopt);
      default:
      case 'h':
        printf("Usage: %.*s [-l <mod_path>] [-m <auth_mod>]\n", (int)strlen(argv[0]), argv[0]);
        return 1;
    }
  }

  // Ensure basic #pragma pack(1) assumptions are met.
  bool challenge_size_matches_keystore_plus_aont_key =
    sizeof(challenge) == 32+sizeof(key_store)+sizeof(aont_key);
  assert(challenge_size_matches_keystore_plus_aont_key);

  // Ensure absolute sizing assumptions are met.
  bool challenge_size_is_128b =
    sizeof(challenge) == 128;
  assert(challenge_size_is_128b); // Ensure 128 byte size

  // encode and decode using same material
  int ret = ! ((
    #ifndef DEBUG_PRINT
      printf("storage:%s:%s:", gconfig.mod_path, gconfig.mod_name) < 0 ||
    #endif
    sodium_init() ||
    challenge_test(gconfig.mod_name, gconfig.mod_path)) == 0);

  // Because free'ing strdup()'ed config options matters.
  free(gconfig.mod_path);
  if (gconfig.mod_name != mod_name_d) free(gconfig.mod_name);

  return ret;
}
