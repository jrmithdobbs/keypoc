CC := clang
CXX := clang++

INCLUDES := \
	-I/usr/local/include \
	-I./ \
	-I./src \

LIBPATH := \
  -L/usr/local/lib \

ARCH_OPTS ?= -march=corei7-avx -mavx2 -mrdseed

OPTS := \
	-g -O2 $(ARCH_OPTS) \

TEST_OPTS := \
	-g -O  $(ARCH_OPTS) \
  -DRUN_TESTS \

CFLAGS := \
	-std=c11 -Wall -Werror -fPIC \
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
	-fPIC \
	$(LIBPATH) \

OBJS := \
	src/storage.o \

AUTHMODULES := \
	hmacsha1mem \
	hmacsha2mem \

AUTHMODULE_DIR := lib
AUTHMODULE_FTY := .auth

AUTHMODULES_PATHS := \
	$(foreach mod,$(AUTHMODULES),$(addprefix $(AUTHMODULE_DIR)/,$(addsuffix $(AUTHMODULE_FTY),$(mod))))

TARGETS := \
	tests/storage \
  tests/mod \
	$(AUTHMODULES_PATHS) \

all: $(TARGETS)
.PHONY: all

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $^

tests:
	@mkdir -p "$@"

tests/%.o: src/test/%.c tests
	$(CC) $(CFLAGS) $(TEST_OPTS) -c -o "$@" $(filter %.c,$^)

tests/%: tests/%.o tests
	$(CC) $(CFLAGS) $(TEST_OPTS) $(LDFLAGS) -o "$@" $(filter %.o %.a,$^) $(LIBS)

bin:
	@mkdir -p "$@"

bin/%: src/%.o bin
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -o "$@" $(filter %.o %.a,$^) $(LIBS)

lib:
	@mkdir -p "$@"

lib/%$(AUTHMODULE_FTY).o: src/authmod/%.c lib
	$(CC) $(CFLAGS) $(OPTS) -fPIC -c -o "$@" $(filter %.c,$^)

lib/%$(AUTHMODULE_FTY): lib/%$(AUTHMODULE_FTY).o lib
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -shared -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

test: test_storage test_mods
.PHONY: test

test_storage: tests/storage
	"./$<"
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
	rm -rfv tests bin lib src/*.o
.PHONY: clean

.PRECIOUS: %.o
.PRECIOUS: src/%.o
.PRECIOUS: %.c
.PRECIOUS: src/%.c
.PRECIOUS: %.s
.PRECIOUS: src/%.s
.PRECIOUS: tests/%_test.o
.PRECIOUS: tests/%.o
.PRECIOUS: tests/%
.PRECIOUS: lib/%$(AUTHMODULE_FTY).o
.PRECIOUS: lib/%
.PRECIOUS: bin/%
