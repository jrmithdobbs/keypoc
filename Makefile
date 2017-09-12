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
	-std=c11 -pedantic -Wall -Werror -fpic \
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

bin/%: bin/%.o bin
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -fpie -o "$@" $(filter %.o %.a,$^) $(LIBS)

lib/%$(AUTHMODULE_FTY).o: src/authmod/%.c lib
	$(CC) $(CFLAGS) $(OPTS) -fpic -c -o "$@" $(filter %.c,$^)

lib/%$(AUTHMODULE_FTY): lib/%$(AUTHMODULE_FTY).o lib
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -shared -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

test: test_storage test_mods $(TARGETS) $(TEST_TARGETS)
.PHONY: test

test_storage: tests/storage $(AUTHMODULES_PATHS)
	"./$<"
	"./$<" -p lib
	"./$<" -m null
	"./$<" -m null -p lib
	"./$<" -m sha2256
	"./$<" -m sha2256 -p lib
	# Expected errors
	"./$<" -m null -p '' || true
	"./$<" -m sha2256 -p '' || true
	"./$<" -m nonexistant || true
	"./$<" -m nonexistant -p lib || true
	# End expected errors
.PHONY: test_storage

test_mods: test_mods_each test_mods_combined
.PHONY: test_mods

test_mods_each: tests/mod $(AUTHMODULES_PATHS)
	$(foreach mod,$(AUTHMODULES),"./$<" '$(AUTHMODULE_DIR)' $(mod) &&) true
.PHONY: test_mods_each

test_mods_combined: tests/mod $(AUTHMODULES_PATHS)
	"./$<" '$(AUTHMODULE_DIR)' $(AUTHMODULES)
.PHONY: test_mods_combined

clean:
	@rm -rf tests bin lib src/*.o src/*/*.o src/*.s src/*/*.s
.PHONY: clean

# Don't delete intermediary files, the easy way!
.SECONDARY:
