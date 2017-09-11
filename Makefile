CC := clang
CXX := clang++

ifdef DEBUG_PRINT
OPTIONS += -DDEBUG_PRINT
endif

INCLUDES := \
	-I/usr/local/include

LIBPATH := \
  -L/usr/local/lib

OPTS := \
	-g -O2 -march=corei7-avx -mavx2 \

TEST_OPTS := \
	-g -O \
  -DRUN_TESTS \
  -DDEBUG_PRINT \

CFLAGS := \
	-std=c11 -Wall -Werror -fPIC \
	$(INCLUDES) \
  $(OPTIONS) \

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

TARGETS := \
	tests/storage \
	lib/hmacsha1mem.auth \
	bin/modtest_main \

all: $(TARGETS)
.PHONY: all

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $^

tests:
	mkdir -p "$@"

tests/%_test.o: src/%.c tests
	$(CC) $(CFLAGS) $(TEST_OPTS) -c -o "$@" $(filter %.c %.o %.a,$^)

tests/%: tests/%_test.o tests
	$(CC) $(CFLAGS) $(TEST_OPTS) $(LDFLAGS) -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

bin:
	mkdir -p "$@"

bin/%: src/%.o bin
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

lib:
	mkdir -p "$@"

lib/%_mod.o: src/%_mod.c lib
	$(CC) $(CFLAGS) $(OPTS) -fPIC -c -o "$@" $(filter %.c %.o %.a,$^)

lib/%.auth: lib/%_mod.o lib
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -shared -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

clean:
	rm -rfv tests bin lib src/*.o
.PHONY: clean

test: tests/storage
	./tests/storage

.PRECIOUS: %.o
.PRECIOUS: src/%.o
.PRECIOUS: %.c
.PRECIOUS: src/%.c
.PRECIOUS: %.s
.PRECIOUS: src/%.s
.PRECIOUS: tests/%_test.o
.PRECIOUS: tests/%.o
.PRECIOUS: tests/%
.PRECIOUS: lib/%_mod.o
.PRECIOUS: lib/%
.PRECIOUS: bin/%
