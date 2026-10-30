/* Three-way text diff/merge. Input: [len(original):1][len(modified):1][original][modified][latest]. */
#include "fuzz_common.h"
#include "svn_diff.h"
#include "svn_io.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_diff_t *diff, *diff2;
  svn_diff_file_options_t *opts;
  svn_stringbuf_t *outbuf;
  svn_stream_t *out;
  svn_string_t *orig, *mod, *latest;
  size_t n1, n2;

  if (size < 2)
    return -1;
  n1 = data[0];
  n2 = data[1];
  data += 2;
  size -= 2;
  if (n1 + n2 > size)
    return -1;

  fuzz_init();
  pool = svn_pool_create(NULL);
  orig = svn_string_ncreate((const char *)data, n1, pool);
  mod = svn_string_ncreate((const char *)data + n1, n2, pool);
  latest = svn_string_ncreate((const char *)data + n1 + n2, size - n1 - n2, pool);
  opts = svn_diff_file_options_create(pool);
  if (!svn_diff_mem_string_diff3(&diff, orig, mod, latest, opts, pool))
    {
      outbuf = svn_stringbuf_create_empty(pool);
      out = svn_stream_from_stringbuf(outbuf, pool);
      svn_error_clear(svn_diff_mem_string_output_merge3(
          out, diff, orig, mod, latest, NULL, NULL, NULL, NULL,
          svn_diff_conflict_display_modified_latest, NULL, NULL, pool));
      if (!svn_diff_mem_string_diff(&diff2, orig, mod, opts, pool))
        svn_error_clear(svn_diff_mem_string_output_unified3(
            out, diff2, TRUE, NULL, "a", "b", "UTF-8", orig, mod, 3, NULL, NULL, pool));
    }
  svn_pool_destroy(pool);
  return 0;
}
