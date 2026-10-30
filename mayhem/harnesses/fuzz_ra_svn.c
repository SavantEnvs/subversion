/* Feed arbitrary bytes to the ra_svn wire-protocol item reader (what svnserve / an svn:// client parse). */
#include "fuzz_common.h"
#include "svn_ra_svn.h"
#include "svn_io.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_string_t *str;
  svn_ra_svn_conn_t *conn;
  int i;

  fuzz_init();
  pool = svn_pool_create(NULL);
  str = svn_string_ncreate((const char *)data, size, pool);
  conn = svn_ra_svn_create_conn5(NULL, svn_stream_from_string(str, pool),
                                 svn_stream_empty(pool), 0, 0, 0, 0, 0, pool);
  for (i = 0; i < 1024; i++)
    {
      svn_ra_svn_item_t *item;
      svn_error_t *err = svn_ra_svn_read_item(conn, pool, &item);
      if (err)
        {
          svn_error_clear(err);
          break;
        }
    }
  svn_pool_destroy(pool);
  return 0;
}
