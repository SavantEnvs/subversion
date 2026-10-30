#!/usr/bin/env bash
#
# mayhem/build.sh — build Apache Subversion's static libraries twice (sanitized+libFuzzer-instrumented
# for the fuzz harnesses, plain for the upstream C test programs) and link the harnesses.
# Both builds are out-of-tree under mayhem/_build (gitignored). Re-runnable and offline: every
# dependency (apr, apr-util, sqlite3, zlib, expat) is an apt -dev package installed in the image.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
BUILD="$SRC/mayhem/_build"
FUZZ_BD="$BUILD/fuzz"
TEST_BD="$BUILD/test"
mkdir -p "$FUZZ_BD" "$TEST_BD"

# Generates configure + build-outputs.mk in the source tree (idempotent).
./autogen.sh >"$BUILD/autogen.log" 2>&1 || { tail -30 "$BUILD/autogen.log" >&2; exit 1; }

CONFIGURE_ARGS=(--disable-shared --enable-static --without-serf --without-apxs
  --without-berkeley-db --disable-nls --with-lz4=internal --with-utf8proc=internal)

# The Makefile does not create its out-of-tree object directories; mirror the source dirs.
configure_in() {
  local bd="$1"; shift
  (cd "$SRC" && find subversion -type d) | (cd "$bd" && xargs mkdir -p)
  if [ ! -f "$bd/Makefile" ]; then
    (cd "$bd" && "$SRC/configure" "${CONFIGURE_ARGS[@]}" CC="$CC" "$@" >configure.log 2>&1) \
      || { tail -30 "$bd/configure.log" >&2; exit 1; }
  fi
}

# 1) Sanitized, libFuzzer-instrumented libraries for the harnesses.
FUZZ_CFLAGS="-O1 $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
configure_in "$FUZZ_BD" CFLAGS="$FUZZ_CFLAGS"
make -C "$FUZZ_BD" -j"$MAYHEM_JOBS" libsvn_subr libsvn_delta libsvn_diff libsvn_fs libsvn_fs_fs \
  libsvn_fs_x libsvn_fs_util libsvn_repos libsvn_ra_svn >"$FUZZ_BD/make.log" 2>&1 \
  || { tail -40 "$FUZZ_BD/make.log" >&2; exit 1; }

# 2) Harnesses: libFuzzer binary + run-once standalone reproducer, LSan disabled at build time.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$BUILD/lsan_off.o"
APR_INC="$(apr-1-config --includes) $(apu-1-config --includes)"
SVN_LIBS=$(ls "$FUZZ_BD"/subversion/*/.libs/*.a)
SYS_LIBS="$(apu-1-config --link-ld --libs) $(apr-1-config --link-ld --libs) -lsqlite3 -lz -lexpat -lm -lpthread -ldl"
HARNESS_CFLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -Werror=incompatible-pointer-types -Werror=implicit-function-declaration -Wno-deprecated-declarations"
HARNESS_INC="-I$SRC/subversion/include -I$FUZZ_BD/subversion $APR_INC -I$SRC/mayhem/harnesses"

for h in "$SRC"/mayhem/harnesses/fuzz_*.c; do
  name=$(basename "$h" .c)
  # shellcheck disable=SC2086
  $CC $HARNESS_CFLAGS $HARNESS_INC $LIB_FUZZING_ENGINE "$h" "$BUILD/lsan_off.o" \
      -Wl,--start-group $SVN_LIBS -Wl,--end-group $SYS_LIBS -o "$SRC/$name"
  # shellcheck disable=SC2086
  $CC $HARNESS_CFLAGS $HARNESS_INC "$STANDALONE_FUZZ_MAIN" "$h" "$BUILD/lsan_off.o" \
      -Wl,--start-group $SVN_LIBS -Wl,--end-group $SYS_LIBS -o "$SRC/$name-standalone"
done

# 3) Upstream C test programs with the project's NORMAL flags (independent of the sanitized build).
configure_in "$TEST_BD" CFLAGS="-O2 $COVERAGE_FLAGS" LDFLAGS="$COVERAGE_FLAGS"
TEST_PROGS=$(grep -E '^[a-z_./0-9-]+$' "$SRC/mayhem/tests.list")
# shellcheck disable=SC2086
make -C "$TEST_BD" -j"$MAYHEM_JOBS" $TEST_PROGS >"$TEST_BD/make.log" 2>&1 \
  || { tail -40 "$TEST_BD/make.log" >&2; exit 1; }
