#include "modauth.h"
#include <dlfcn.h>

/* This is currently just a copy of test/mod.c as a starting point. */

challenge_plugin_hdr * load_challenge_plugin(char *pname, char *mod_path) {
  void* mod;
  challenge_plugin_hdr *header;
  AMM_Discover *discover;

  const char _prefx[] = "lib"; // FIXME: This default should be build-time
  const char *prefx = mod_path ? mod_path : _prefx;
  int prefx_sz = strlen(prefx);
  const char suffx[] = ".auth";
  int suffx_sz = strlen(suffx);
  int pname_sz = strlen(pname);

  // Can't load an empty module
  if (pname_sz == 0) {
    return NULL;
  }

  int fname_sz = prefx_sz + pname_sz + suffx_sz + 1; // + 1 for extra '/'

  char *fname = malloc(fname_sz + 1); // terminating '\0'
  if (fname == NULL) {
    printf("Could not allocate ram: %s", strerror(errno));
    return NULL;
  }
  memset(fname, 0, sizeof(fname_sz));

  static const char fmt_wi_slash[] = "%.*s/%.*s%.*s";
  static const char fmt_wo_slash[] = "%.*s%.*s%.*s";

  snprintf(fname, fname_sz + 1,
    prefx_sz ? fmt_wi_slash : fmt_wo_slash,
    prefx_sz, prefx, pname_sz, pname, suffx_sz, suffx);

  //load
  if ((mod = dlopen(fname, RTLD_LOCAL)) == NULL) {
    printf("Error loading module: %s\n", dlerror());
    sodium_memzero(fname, sizeof(fname));
    free(fname);
    fname = NULL;
    return NULL;
  }

  sodium_memzero(fname, sizeof(fname));
  free(fname);
  fname = NULL;

  /* use loaded module */
  if ((*(void**) (&discover) = dlsym(mod, "challenge_discover")) == NULL) {
    printf("Error resolving challenge: %s\n", dlerror());
    return NULL;
  }

  if ((header = (*discover)()) == NULL) {
    dlclose(mod);
    return NULL;
  }

  // TODO: re-evaluate, allows modules to use static globals.
  if (header->mod != NULL) {
    printf("Duplicate load of module.\n");
    dlclose(mod);
    return NULL;
  }

  header->mod = mod;
  return header;
}

// unload a module
void close_challenge_plugin(challenge_plugin_hdr *c) {
  if (dlclose(c->mod) != 0) {
    printf("Error unloading module: %s\n", dlerror());
  }
}
