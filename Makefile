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
	-std=c11 -Wall -Werror -pedantic \
	-fPIC -fPIE \
	$(INCLUDES) \
  $(OPTIONS) \

LIBSODIUM ?= -lsodium

LIBS := \
  $(LIBSODIUM)

LDFLAGS := \
	$(LIBPATH) \
	-fPIC -fPIE \

OBJS := \
	src/storage.o \

TARGETS := \
	tests/storage

all: $(TARGETS)
.PHONY: all

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $^

tests:
	mkdir -p "$@"

tests/%_test.o: src/%.c tests
	$(CC) $(CFLAGS) $(TEST_OPTS) -c -o "$@" $<

tests/%: tests/%_test.o
	$(CC) $(CFLAGS) $(TEST_OPTS) $(LDFLAGS) -o "$@" $^ $(LIBS)

bin/%: src/%.o
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -o "$@" $^ $(LIBS)

clean:
	rm -rfv storage *.o tests
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
