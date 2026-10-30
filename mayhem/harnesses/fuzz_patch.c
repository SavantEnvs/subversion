/* Parse a patch file (unified diff / git diff / property diff). The first input byte selects the
   reverse and ignore-whitespace flags; the remainder is the patch text. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fuzz_common.h"
#include "svn_diff.h"
#include "svn_io.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_patch_file_t *pf;
  svn_error_t *err;
  char path[4096];
  const char *tmp;
  int fd, n;
  int reverse, ignore_ws;

  if (size < 1)
    return -1;
  reverse = data[0] & 1;
  ignore_ws = (data[0] >> 1) & 1;
  data++;
  size--;

  fuzz_init();
  tmp = getenv("TMPDIR");
  snprintf(path, sizeof(path), "%s/fuzz_patch_XXXXXX", tmp && *tmp ? tmp : "/tmp");
  fd = mkstemp(path);
  if (fd < 0)
    return -1;
  if (size && write(fd, data, size) != (ssize_t)size)
    {
      close(fd);
      unlink(path);
      return -1;
    }
  close(fd);

  pool = svn_pool_create(NULL);
  err = svn_diff_open_patch_file(&pf, path, pool);
  if (!err)
    {
      for (n = 0; n < 64; n++)
        {
          svn_patch_t *patch = NULL;
          apr_pool_t *iter = svn_pool_create(pool);

          err = svn_diff_parse_next_patch(&patch, pf, reverse, ignore_ws, iter, iter);
          if (err || !patch)
            break;
          if (patch->hunks)
            {
              int i;
              for (i = 0; i < patch->hunks->nelts; i++)
                {
                  svn_diff_hunk_t *hunk = APR_ARRAY_IDX(patch->hunks, i, svn_diff_hunk_t *);
                  svn_stringbuf_t *line;
                  svn_boolean_t eof = FALSE;
                  const char *eol;
                  svn_diff_hunk_reset_diff_text(hunk);
                  while (!eof)
                    {
                      svn_error_t *e2 = svn_diff_hunk_readline_diff_text(hunk, &line, &eol, &eof, iter, iter);
                      if (e2)
                        {
                          svn_error_clear(e2);
                          break;
                        }
                    }
                }
            }
        }
      svn_error_clear(err);
      svn_error_clear(svn_diff_close_patch_file(pf, pool));
    }
  else
    svn_error_clear(err);
  svn_pool_destroy(pool);
  unlink(path);
  return 0;
}
