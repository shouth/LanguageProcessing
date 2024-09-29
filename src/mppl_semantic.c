#include <stddef.h>

#include "mppl_semantic.h"
#include "util.h"

/* mppl semantics */

static unsigned long mppl_semantics_offset_hash(const void *value)
{
  return hash_fnv1a(NULL, value, sizeof(unsigned long));
}

static int mppl_semantics_offset_equal(const void *a, const void *b)
{
  return *(unsigned long *) a == *(unsigned long *) b;
}

void mppl_semantics_free(MpplSemantics *semantics)
{
  if (semantics) {
    slice_free(&semantics->bindings);
    hashmap_free(&semantics->ref);
    free(semantics);
  }
}
