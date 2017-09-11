#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef int ModF(uint8_t*,int,uint8_t*,int);

int main(int argc, char**argv, char**envp) {
  void* mod;
  ModF *challenge;

  if (argc < 2) {
    printf("At least one module must be specified!\n");
  }

  for (int i = 1; i < argc; ++i) {

    int   sname_sz = 6;
    static const char sname[] = ".auth";
    int pname_sz = strlen(argv[i]);
    char *pname = argv[i];
    int fname_sz = pname_sz + sname_sz;
    char *fname = malloc(fname_sz);

    if (fname == NULL) {
      printf("Could not allocate ram: %s", strerror(errno));
      return 1;
    }

    memset(fname, 0, fname_sz);
    strncat(fname, pname, pname_sz);
    strncat(fname, sname, fname_sz - 1 - pname_sz);

    printf("test-%04x:%.*s:", i, pname_sz, pname);

    //load
    if ((mod = dlopen(fname, RTLD_LOCAL)) == NULL) {
      printf("Error loading module: %s\n", dlerror());
      return 1;
    }

    printf("loaded:");

    /* use loaded module */
    if ((challenge = dlsym(mod, "challenge")) == NULL) {
      printf("Error resolving challenge: %s\n", dlerror());
      return 1;
    }

    printf("located:");

    printf("return %04x:", (*challenge)(NULL, 0, NULL, 0));

    // unload
    if (dlclose(mod) != 0) {
      printf("Error unloading module: %s\n", dlerror());
      return 1;
    }

    printf(":unloaded:Success!\n");
  }

  return 0;
}
