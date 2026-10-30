/* Parse a Subversion "skel" (the s-expression-like encoding used for FS metadata). */
#include "fuzz_common.h"
#include "private/svn_skel.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_skel_t *skel;

  fuzz_init();
  pool = svn_pool_create(NULL);
  skel = svn_skel__parse((const char *)data, size, pool);
  if (skel)
    {
      svn_stringbuf_t *unparsed = svn_skel__unparse(skel, pool);
      (void)unparsed;
      (void)svn_skel__list_length(skel);
    }
  svn_pool_destroy(pool);
  return 0;
}
