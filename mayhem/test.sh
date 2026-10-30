#!/usr/bin/env bash
#
# mayhem/test.sh — RUN Subversion's own C unit tests (built by mayhem/build.sh with normal flags).
# Each test program prints one "PASS:/FAIL:/XFAIL:/XPASS:/SKIP:" line per test case, asserting
# computed values (canonicalised paths, parsed mergeinfo, checksums, diffs, ...); the counts feed CTRF.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"
TEST_BD="$SRC/mayhem/_build/test"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

passed=0; failed=0; skipped=0
while read -r prog; do
  [ -n "$prog" ] || continue
  dir=$(dirname "$prog"); exe=$(basename "$prog")
  if [ ! -x "$TEST_BD/$prog" ]; then
    echo "missing test runner $TEST_BD/$prog (mayhem/build.sh should have built it)" >&2
    failed=$((failed + 1)); continue
  fi
  out=$(cd "$TEST_BD/$dir" && timeout 900 "./$exe" --srcdir="$SRC/$dir" 2>&1); rc=$?
  p=$(printf '%s\n' "$out" | grep -cE '^(PASS|XFAIL):')
  f=$(printf '%s\n' "$out" | grep -cE '^(FAIL|XPASS):')
  s=$(printf '%s\n' "$out" | grep -cE '^SKIP:')
  if [ "$rc" -ne 0 ] && [ "$f" -eq 0 ]; then f=1; fi
  if [ "$p" -eq 0 ] && [ "$f" -eq 0 ] && [ "$s" -eq 0 ]; then f=1; fi
  printf '%s\n' "$out" | grep -E '^(FAIL|XPASS):' >&2 || true
  echo "$prog: pass=$p fail=$f skip=$s (exit $rc)"
  passed=$((passed + p)); failed=$((failed + f)); skipped=$((skipped + s))
done < "$SRC/mayhem/tests.list"

emit_ctrf "svn-c-tests" "$passed" "$failed" "$skipped"
