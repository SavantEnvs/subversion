/* Parse an svn:mergeinfo property value and exercise the common mergeinfo operations on it. */
#include <stdlib.h>
#include <string.h>
#include "fuzz_common.h"
#include "svn_mergeinfo.h"
#include "svn_string.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  char *input;
  svn_mergeinfo_t mi, copy;
  svn_string_t *out;
  svn_error_t *err;

  fuzz_init();
  input = malloc(size + 1);
  if (!input)
    return -1;
  memcpy(input, data, size);
  input[size] = '\0';

  pool = svn_pool_create(NULL);
  err = svn_mergeinfo_parse(&mi, input, pool);
  if (!err)
    {
      svn_error_clear(svn_mergeinfo_to_string(&out, mi, pool));
      svn_error_clear(svn_mergeinfo_sort(mi, pool));
      copy = svn_mergeinfo_dup(mi, pool);
      svn_error_clear(svn_mergeinfo_merge2(copy, mi, pool, pool));
      svn_error_clear(svn_mergeinfo_remove2(&copy, mi, copy, TRUE, pool, pool));
    }
  svn_error_clear(err);
  svn_pool_destroy(pool);
  free(input);
  return 0;
}
