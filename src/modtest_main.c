#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>

typedef int ModF(uint8_t*,int,uint8_t*,int);

int main(int argc, char**argv, char**envp) {
  void* mod;
  ModF *challenge;

  //load
  if ((mod = dlopen("hmacsha1mem.auth", RTLD_LOCAL)) == NULL) {
    printf("Error loading module: %s\n", dlerror());
    return 1;
  }

  /* use loaded module */
  if ((challenge = dlsym(mod, "challenge")) == NULL) {
    printf("Error resolving challenge: %s\n", dlerror());
    return 1;
  }

  printf("Module result: %d\n", (*challenge)(NULL, 0, NULL, 0));

  // unload
  if (dlclose(mod) != 0) {
    printf("Error unloading module: %s\n", dlerror());
    return 1;
  }

  printf("Success!\n");

  return 0;
}
