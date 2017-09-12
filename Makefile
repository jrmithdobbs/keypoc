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
  -DRUN_TESTS \

CFLAGS := \
	-std=c11 -pedantic -Wall -Wextra -Werror -fpic \
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
	-fpic \
	$(LIBPATH) \

OBJS := \
	src/storage.o \
	src/modauth.o \

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

bin/%.o: src/bins/%.c bin
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

tests/%.o: src/test/%.c tests
	$(CC) $(CFLAGS) $(TEST_OPTS) -c -o "$@" $(filter %.c,$^)

tests/%: tests/%.o src/modauth.o tests
	$(CC) $(CFLAGS) $(TEST_OPTS) $(LDFLAGS) -fpie -o "$@" $(filter %.o %.a,$^) $(LIBS)

bin/%: bin/%.o $(OBJS) bin
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -fpie -o "$@" $(filter %.o %.a,$^) $(LIBS)

lib/%$(AUTHMODULE_FTY).o: src/authmod/%.c lib
	$(CC) $(CFLAGS) $(OPTS) -fpic -c -o "$@" $(filter %.c,$^)

lib/%$(AUTHMODULE_FTY): lib/%$(AUTHMODULE_FTY).o lib
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -shared -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

test: test_storage test_mods_loading $(TARGETS) $(TEST_TARGETS)
.PHONY: test

full_test: test_storage_loading $(TARGETS) $(TEST_TARGETS)
.PHONY: full_test

test_storage: tests/storage $(AUTHMODULES_PATHS) test_mods_loading
	$(call storage_test_fmt,,"./$<")
.PHONY: test_storage

define storage_test_fmt
@(((${2}) | tr '\n' ':'; echo ${1};) | sed 's,:$$,,')
endef

test_storage_loading: tests/storage $(AUTHMODULES_PATHS) test_storage
	$(call storage_test_fmt,,"./$<" -p lib)
	$(call storage_test_fmt,,"./$<" -m null)
	$(call storage_test_fmt,,"./$<" -m null -p lib)
	$(call storage_test_fmt,,"./$<" -m sha2256)
	$(call storage_test_fmt,,"./$<" -m sha2256 -p lib)
	$(call storage_test_fmt,Expected Error!,"./$<" -m null -p '' || true)
	$(call storage_test_fmt,Expected Error!,"./$<" -m sha2256 -p '' || true)
	$(call storage_test_fmt,Expected Error!,"./$<" -m nonexistant || true)
	$(call storage_test_fmt,Expected Error!,"./$<" -m nonexistant -p lib || true)
.PHONY: test_storage_long

test_mods_loading: tests/mod $(AUTHMODULES_PATHS)
	@"./$<" '$(AUTHMODULE_DIR)' $(AUTHMODULES)
.PHONY: test_mods_combined

clean:
	@rm -rf tests bin lib src/*.o src/*/*.o src/*.s src/*/*.s
.PHONY: clean

# Don't delete intermediary files, the easy way!
.SECONDARY:
