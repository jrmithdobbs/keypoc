#include "util.h"
#include <dlfcn.h>

typedef int ModF(uint8_t*,int,uint8_t*,int);

int main(int argc, char**argv, char**envp) {
  void* mod;
  ModF *challenge;
  int errcount = 0;

  if (argc < 3) {
    printf("Usage:\t%s <module_path> <<module> ...>\n\n", argv[0]);
    printf("At least one module must be specified!\n\n");
    printf("While not optional module_path may be the empty string to search the normal linker paths.\n");
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

    printf("test-0x%04x:%.*s:", i - 1, pname_sz, pname);

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
    if ((challenge = dlsym(mod, "challenge")) == NULL) {
      printf("Error resolving challenge: %s\n", dlerror());
      errcount += 1;
      continue;
    }

    printf("located:");

    printf("%04x:", (*challenge)(NULL, 0, NULL, 0));

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
