CC := clang
CXX := clang++

ifdef DEBUG_PRINT
OPTIONS += -DDEBUG_PRINT
endif

INCLUDES := \
	-I/usr/local/include \
	-I./ \
	-I./src \

LIBPATH := \
  -L/usr/local/lib \

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

MODULES := \
	hmacsha1mem \
	hmacsha2mem \

MODULES := $(strip $(MODULES))

MODULES_PATHS := \
	$(foreach mod,$(strip $(MODULES)),$(addprefix lib/,$(addsuffix .auth,$(mod))))

TARGETS := \
	tests/storage \
  tests/mod \
	$(MODULES_PATHS) \

all: $(TARGETS)
.PHONY: all

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $^

tests:
	mkdir -p "$@"

tests/%_test.o: src/test/%.c tests
	$(CC) $(CFLAGS) $(TEST_OPTS) -c -o "$@" $(filter %.c %.o %.a,$^)

tests/%: tests/%_test.o tests
	$(CC) $(CFLAGS) $(TEST_OPTS) $(LDFLAGS) -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

bin:
	mkdir -p "$@"

bin/%: src/%.o bin
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

lib:
	mkdir -p "$@"

lib/%.auth.o: src/auth_%.c lib
	$(CC) $(CFLAGS) $(OPTS) -fPIC -c -o "$@" $(filter %.c %.o %.a,$^)

lib/%.auth: lib/%.auth.o lib
	echo $(TARGETS)
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -shared -o "$@" $(filter %.c %.o %.a,$^) $(LIBS)

clean:
	rm -rfv tests bin lib src/*.o
.PHONY: clean

TEST_MODULE := LD_LIBRARY_PATH="$$PWD/lib"; export LD_LIBRARY_PATH; ./tests/mod

test: tests/storage tests/mod $(MODULES_PATHS)
	# Test loading each module individually
	@$(foreach mod,$(MODULES),$(strip $(TEST_MODULE) $(mod)); )
	# Test loading all modules simultaneouslly
	@$(TEST_MODULE) $(MODULES)
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
.PRECIOUS: lib/%.auth.o
.PRECIOUS: lib/%
.PRECIOUS: bin/%
