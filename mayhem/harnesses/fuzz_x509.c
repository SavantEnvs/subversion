/* Parse an arbitrary DER-encoded X.509 certificate. */
#include "fuzz_common.h"
#include "svn_x509.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_x509_certinfo_t *certinfo;
  svn_error_t *err;

  fuzz_init();
  pool = svn_pool_create(NULL);
  err = svn_x509_parse_cert(&certinfo, (const char *)data, size, pool, pool);
  if (!err)
    {
      (void)svn_x509_certinfo_get_subject(certinfo, pool);
      (void)svn_x509_certinfo_get_issuer(certinfo, pool);
      (void)svn_x509_certinfo_get_hostnames(certinfo);
      (void)svn_x509_certinfo_get_digest(certinfo);
    }
  svn_error_clear(err);
  svn_pool_destroy(pool);
  return 0;
}
