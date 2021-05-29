CC := clang
CXX := clang++

EXTRAINCLUDES ?=
EXTRALIBS ?=

INCLUDES := \
	-I./ \
	-I./src \
	-I/usr/local/include \
	$(EXTRAINCLUDES)

LIBPATH := \
	-L/usr/local/lib \
	$(EXTRALIBS)

ARCH_OPTS ?= -march=native -mavx2 -mrdseed -mrdrnd

OPTS := \
	-g -O2 $(ARCH_OPTS) \

TEST_OPTS := \
	-g -O2 $(ARCH_OPTS) \

CFLAGS := \
	-std=c11 -pedantic -Wall -Wextra -Werror -fpic -fpie -fPIE -fPIC \
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

MOD_TARGETS := \
	$(AUTHMODULES_PATHS) \

TEST_TARGETS := \
	tests/storage \
	tests/mod \

all: $(TARGETS) $(MOD_TARGETS) $(TEST_TARGETS)
.PHONY: all


define create_dir
@mkdir -p $(dir $@)
endef

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

%.bc: %.c
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/bins/%.o: src/bins/%.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/bins/%.bc: src/bins/%.c
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/%.bc: src/%.c
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/test/%.o: src/test/%.c
	$(CC) $(CFLAGS) $(TEST_OPTS) -c -o "$@" $(filter %.c,$^)

src/test/%.bc: src/test/%.c
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/authmod/%$(AUTHMODULE_FTY).o: src/authmod/%.c
	$(CC) $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

src/authmod/%$(AUTHMODULE_FTY).bc: src/authmod/%.c
	$(CC) -emit-llvm $(CFLAGS) $(OPTS) -c -o "$@" $(filter %.c,$^)

bin/%: src/bins/%.$(OBJ_EXT) $(OBJS)
	$(call create_dir)
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -o "$@" $(filter %.bc %.o %.a,$^) $(LIBS)

lib/%$(AUTHMODULE_FTY): src/authmod/%$(AUTHMODULE_FTY).$(OBJ_EXT)
	$(call create_dir)
	$(CC) $(CFLAGS) $(OPTS) $(LDFLAGS) -shared -o "$@" $(filter %.bc %.c %.o %.a,$^) $(LIBS)

tests/%: src/test/%.$(OBJ_EXT) src/modauth.$(OBJ_EXT)
	$(call create_dir)
	$(CC) $(CFLAGS) $(TEST_OPTS) $(LDFLAGS) -o "$@" $(filter %.bc %.o %.a,$^) $(LIBS)

define storage_test_fmt
@((${2}) | tr '\n' ':'; echo ${1};) | sed '/^$$/d;s,:$$,,'
endef

test_storage: tests/storage $(MOD_TARGETS)
	$(call storage_test_fmt,,"./$<")
.PHONY: test_storage

test_storage_lib: tests/storage $(MOD_TARGETS)
	$(call storage_test_fmt,,"./$<" -p lib)
.PHONY: test_storage_lib

test_storage_null: tests/storage $(MOD_TARGETS)
	$(call storage_test_fmt,,"./$<" -m null)
.PHONY: test_storage_null

test_storage_lib_null: tests/storage $(MOD_TARGETS)
	$(call storage_test_fmt,,"./$<" -m null -p lib)
.PHONY: test_storage_lib_null

test_storage_sha2256: tests/storage $(MOD_TARGETS)
	$(call storage_test_fmt,,"./$<" -m sha2256)
.PHONY: test_storage_sha2256

test_storage_lib_sha2256: tests/storage $(MOD_TARGETS)
	$(call storage_test_fmt,,"./$<" -m sha2256 -p lib)
.PHONY: test_storage_lib_sha2256

test_storage_loading: test_storage_lib \
                      test_storage_null test_storage_lib_null \
                      test_storage_sha2256 test_storage_lib_sha2256
.PHONY: test_storage_loading

test_storage_failures: tests/storage $(MOD_TARGETS)
	$(call storage_test_fmt,Success!,"./$<" -m null -p '' || true)
	$(call storage_test_fmt,Success!,"./$<" -m sha2256 -p '' || true)
	$(call storage_test_fmt,Success!,"./$<" -m nonexistant || true)
	$(call storage_test_fmt,Success!,"./$<" -m nonexistant -p lib || true)
	$(call storage_test_fmt,Success!,"./$<" -m nonexistant -p '' || true)
.PHONY: test_storage_failures

test_storage_combined: test_storage test_storage_loading test_storage_failures
.PHONY: test_storage_combined

test_mods_loading: tests/mod $(MOD_TARGETS)
	@"./$<" '$(AUTHMODULE_DIR)' $(AUTHMODULES)
.PHONY: test_mods_loading

test_mods_separate: tests/mod $(MOD_TARGETS)
	@$(foreach mod,$(AUTHMODULES),"./$<" $(AUTHMODULE_DIR) $(mod);)
.PHONY: test_mods_separate

test_mods_combined: test_mods_loading test_mods_separate
.PHONY: test_mods_combined

test: test_storage test_mods_loading
.PHONY: test

full_test: test_storage_combined test_mods_combined
.PHONY: full_test

clean:
	@rm -rf tests bin lib src/*.{o,bc,s} src/*/*.{o,bc,s,dSYM} src/*.{o,bc,s,dSYM}
.PHONY: clean

# Don't delete intermediary files, the easy way!
.SECONDARY:
