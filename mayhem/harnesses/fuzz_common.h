#ifndef FUZZ_COMMON_H
#define FUZZ_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <apr_general.h>
#include <apr_pools.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_string.h"

static int fuzz_apr_ready;

static void
fuzz_init(void)
{
  if (!fuzz_apr_ready)
    {
      apr_initialize();
      fuzz_apr_ready = 1;
    }
}

#endif
