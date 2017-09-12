CC := clang
CXX := clang++

INCLUDES := \
	-I./ \
	-I./src \
	-I/usr/local/include \

LIBPATH := \
  -L/usr/local/lib \

ARCH_OPTS ?= -march=corei7-avx -mavx2 -mrdseed

OPTS := \
	-g -O2 $(ARCH_OPTS) \

TEST_OPTS := \
	-g -O2 $(ARCH_OPTS) \

CFLAGS := \
	-std=c11 -pedantic -Wall -Wextra -Werror -fpic -fpie \
	-ffunction-sections -fdata-sections \
	$(INCLUDES) \

ifdef DEBUG_PRINT
OPTS += -DDEBUG_PRINT
TEST_OPTS += -DDEBUG_PRINT
endif

# Override with path to a .a static lib for static build
# eg: make LIBSODIUM=./libsodium.a all
LIBSODIUM ?= -lsodium

LIBS := \
  $(LIBSODIUM)

LDFLAGS := \
	$(LIBPATH) \
	-Wl,-pie \

ifeq ($(shell uname -s),Darwin)
LDFLAGS += -flto -Wl,-dead_strip
endif

ifdef UseBitcode
	OBJ_EXT = bc
endif

OBJ_EXT ?= o

OBJS := \
	src/storage.$(OBJ_EXT) \
	src/modauth.$(OBJ_EXT) \

AUTHMODULES := \
	null \
	sha2256 \

AUTHMODULE_DIR := lib
AUTHMODULE_FTY := .auth

AUTHMODULES_PATHS := \
	$(foreach mod,$(AUTHMODULES),$(addprefix $(AUTHMODULE_DIR)/,$(addsuffix $(AUTHMODULE_FTY),$(mod))))

TARGETS := \
	bin/keypoc \
	$(AUTHMODULES_PATHS) \

TEST_TARGETS := \
	tests/storage \
  tests/mod \

all: $(TARGETS) $(TEST_TARGETS)
.PHONY: all

bin:
	@mkdir -p "$@"

tests:
	@mkdir -p "$@"

lib:
	@mkdir -p "$@"

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

%.bc: %.c
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

bin/%.o: src/bins/%.c bin
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

bin/%.bc: src/bins/%.c bin
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/%.bc: src/%.c
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

tests/%.o: src/test/%.c tests
	$(CC) $(CFLAGS) $(TEST_OPTS) -c -o "$@" $(filter %.c,$^)

tests/%.bc: src/test/%.c tests
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

tests/%: tests/%.$(OBJ_EXT) src/modauth.$(OBJ_EXT) tests
	$(CC) $(CFLAGS) $(TEST_OPTS) $(LDFLAGS) -o "$@" $(filter %.bc %.o %.a,$^) $(LIBS)

bin/%: bin/%.$(OBJ_EXT) $(OBJS) bin
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -o "$@" $(filter %.bc %.o %.a,$^) $(LIBS)

lib/%$(AUTHMODULE_FTY).o: src/authmod/%.c lib
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

lib/%$(AUTHMODULE_FTY).bc: src/authmod/%.c lib
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

lib/%$(AUTHMODULE_FTY): lib/%$(AUTHMODULE_FTY).$(OBJ_EXT) lib
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -shared -o "$@" $(filter %.bc %.c %.o %.a,$^) $(LIBS)

test: test_storage test_mods_loading $(TARGETS) $(TEST_TARGETS)
.PHONY: test

full_test: test_storage_loading $(TARGETS) $(TEST_TARGETS)
.PHONY: full_test

test_storage: tests/storage $(AUTHMODULES_PATHS) test_mods_loading
	$(call storage_test_fmt,,"./$<")
.PHONY: test_storage

define storage_test_fmt
@((${2}) | tr '\n' ':'; echo ${1};) | sed 's,:$$,,'
endef

test_storage_loading: tests/storage $(AUTHMODULES_PATHS) test_storage
	$(call storage_test_fmt,,"./$<" -p lib)
	$(call storage_test_fmt,,"./$<" -m null)
	$(call storage_test_fmt,,"./$<" -m null -p lib)
	$(call storage_test_fmt,,"./$<" -m sha2256)
	$(call storage_test_fmt,,"./$<" -m sha2256 -p lib)
	$(call storage_test_fmt,Success!,"./$<" -m null -p '' || true)
	$(call storage_test_fmt,Success!,"./$<" -m sha2256 -p '' || true)
	$(call storage_test_fmt,Success!,"./$<" -m nonexistant || true)
	$(call storage_test_fmt,Success!,"./$<" -m nonexistant -p lib || true)
	$(call storage_test_fmt,Success!,"./$<" -m nonexistant -p '' || true)
.PHONY: test_storage_long

test_mods_loading: tests/mod $(AUTHMODULES_PATHS)
	@"./$<" '$(AUTHMODULE_DIR)' $(AUTHMODULES)
.PHONY: test_mods_combined

clean:
	@rm -rf tests bin lib src/*.{o,bc,s} src/*/*.{o,bc,s} $(OBJS)
.PHONY: clean

# Don't delete intermediary files, the easy way!
.SECONDARY:
