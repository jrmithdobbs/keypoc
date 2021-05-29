#include "modauth.h"
#include <dlfcn.h>

int main(int argc, char**argv) {
  void* mod;
  challenge_plugin_hdr *header;
  AMM_Discover *discover;
  int errcount = 0;

  if (argc < 3) {
    printf("Usage:\t%s <module_path> <<module> ...>\n\n", argv[0]);
    printf("At least one module must be specified!\n\n");
    printf("While not optional module_path may be the empty string to search the normal linker paths.\n");
    return 1;
  }

  if (sodium_init() != 0) {
    printf("Failed to load libsodium.");
    return 1;
  }

  const char *prefx = argv[1];
  int prefx_sz = strlen(prefx);

  const char suffx[] = ".auth";
  int suffx_sz = 5;

  for (int i = (argc < 3) ? 1 : 2; i < argc; ++i) {

    char *pname = argv[i];
    int pname_sz = strlen(pname);

    // Can't load an empty module
    if (pname_sz == 0) {
      continue;
    }

    int fname_sz = prefx_sz + pname_sz + suffx_sz + 1; // + 1 for extra '/'

    char *fname = malloc(fname_sz + 1); // terminating '\0'
    if (fname == NULL) {
      printf("Could not allocate ram: %s", strerror(errno));
      return 1;
    }
    memset(fname, 0, sizeof(fname_sz));

    printf("modtest:%.*s:", pname_sz, pname);

    static const char fmt_wi_slash[] = "%.*s/%.*s%.*s";
    static const char fmt_wo_slash[] = "%.*s%.*s%.*s";

    snprintf(fname, fname_sz + 1,
      prefx_sz ? fmt_wi_slash : fmt_wo_slash,
      prefx_sz, prefx, pname_sz, pname, suffx_sz, suffx);

    printf("lib-%.*s:", fname_sz, fname);

    //load
    if ((mod = dlopen(fname, RTLD_LOCAL)) == NULL) {
      printf("Error loading module: %s\n", dlerror());
      sodium_memzero(fname, sizeof(fname));
      free(fname);
      fname = NULL;
      errcount += 1;
      continue;
    }

    sodium_memzero(fname, sizeof(fname));
    free(fname);
    fname = NULL;

    printf("loaded:");

    /* use loaded module */
    if ((*(void**) (&discover) = dlsym(mod, "challenge_discover")) == NULL) {
      printf("Error resolving challenge: %s\n", dlerror());
      errcount += 1;
      continue;
    }

    if ((header = (*discover)()) != NULL) {
      printf("located:0x%016llx:%s", (uint64_t) header,
        ((uint64_t) header) % 4096 == 0 ? "aligned" : "unaligned");
    } else {
      dlclose(mod);
      errcount += 1;
      continue;
    }


    // unload
    if (dlclose(mod) != 0) {
      printf("Error unloading module: %s\n", dlerror());
      errcount += 1;
      continue;
    }

    printf(":unloaded:Success!\n");
  }

  return errcount;
}
