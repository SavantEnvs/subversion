/* Feed arbitrary bytes to the svndiff window parser (as sent by a client to the server). */
#include "fuzz_common.h"
#include "svn_delta.h"
#include "svn_io.h"

static svn_error_t *
window_handler(svn_txdelta_window_t *window, void *baton)
{
  return SVN_NO_ERROR;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_stream_t *in, *out;
  svn_string_t *str;

  fuzz_init();
  pool = svn_pool_create(NULL);
  str = svn_string_ncreate((const char *)data, size, pool);
  in = svn_stream_from_string(str, pool);
  out = svn_txdelta_parse_svndiff(window_handler, NULL, TRUE, pool);
  svn_error_clear(svn_stream_copy3(in, out, NULL, NULL, pool));
  svn_pool_destroy(pool);
  return 0;
}
