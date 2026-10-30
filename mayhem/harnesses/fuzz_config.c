/* Parse an svn client/server configuration (INI-style) file and walk its sections and options. */
#include "fuzz_common.h"
#include "svn_config.h"
#include "svn_io.h"

static svn_boolean_t
option_cb(const char *name, const char *value, void *baton, apr_pool_t *pool)
{
  return TRUE;
}

static svn_boolean_t
section_cb(const char *name, void *baton, apr_pool_t *pool)
{
  svn_config_t *cfg = baton;
  svn_config_enumerate2(cfg, name, option_cb, NULL, pool);
  return TRUE;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_string_t *str;
  svn_config_t *cfg;
  svn_error_t *err;

  fuzz_init();
  pool = svn_pool_create(NULL);
  str = svn_string_ncreate((const char *)data, size, pool);
  err = svn_config_parse(&cfg, svn_stream_from_string(str, pool),
                         size > 0 && (data[0] & 1), size > 0 && (data[0] & 2), pool);
  if (!err)
    {
      svn_boolean_t b;
      const char *v;
      svn_config_enumerate_sections2(cfg, section_cb, cfg, pool);
      svn_config_get(cfg, &v, "auth", "password-stores", NULL);
      svn_error_clear(svn_config_get_bool(cfg, &b, "miscellany", "use-commit-times", FALSE));
    }
  svn_error_clear(err);
  svn_pool_destroy(pool);
  return 0;
}
