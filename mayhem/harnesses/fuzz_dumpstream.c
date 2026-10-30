/* Parse a `svnadmin dump` stream with a no-op consumer (the parser behind `svnadmin load`). */
#include "fuzz_common.h"
#include "svn_repos.h"
#include "svn_io.h"

static apr_pool_t *stream_pool;

static svn_error_t *
magic_header(int version, void *pb, apr_pool_t *pool) { return SVN_NO_ERROR; }
static svn_error_t *
uuid_record(const char *uuid, void *pb, apr_pool_t *pool) { return SVN_NO_ERROR; }
static svn_error_t *
new_revision(void **rb, apr_hash_t *headers, void *pb, apr_pool_t *pool)
{
  *rb = NULL;
  return SVN_NO_ERROR;
}
static svn_error_t *
new_node(void **nb, apr_hash_t *headers, void *rb, apr_pool_t *pool)
{
  *nb = pool;
  return SVN_NO_ERROR;
}
static svn_error_t *
set_rev_prop(void *rb, const char *name, const svn_string_t *value) { return SVN_NO_ERROR; }
static svn_error_t *
set_node_prop(void *nb, const char *name, const svn_string_t *value) { return SVN_NO_ERROR; }
static svn_error_t *
delete_node_prop(void *nb, const char *name) { return SVN_NO_ERROR; }
static svn_error_t *
remove_node_props(void *nb) { return SVN_NO_ERROR; }
static svn_error_t *
set_fulltext(svn_stream_t **stream, void *nb)
{
  *stream = svn_stream_empty(stream_pool);
  return SVN_NO_ERROR;
}
static svn_error_t *
close_node(void *nb) { return SVN_NO_ERROR; }
static svn_error_t *
close_revision(void *rb) { return SVN_NO_ERROR; }

static const svn_repos_parse_fns3_t noop_parser = {
  magic_header, uuid_record, new_revision, new_node, set_rev_prop, set_node_prop,
  delete_node_prop, remove_node_props, set_fulltext, NULL, close_node, close_revision
};

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  apr_pool_t *pool;
  svn_string_t *str;
  svn_stream_t *in;

  fuzz_init();
  pool = svn_pool_create(NULL);
  stream_pool = pool;
  str = svn_string_ncreate((const char *)data, size, pool);
  in = svn_stream_from_string(str, pool);
  svn_error_clear(svn_repos_parse_dumpstream3(in, &noop_parser, NULL, TRUE, NULL, NULL, pool));
  svn_pool_destroy(pool);
  return 0;
}
