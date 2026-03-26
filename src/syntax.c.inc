/* SPDX-License-Identifier: Apache-2.0 */

#include "util.h"

#if !defined(SYN_DEF) || !defined(SYN_PREFIX)
#error "SYN_DEF and SYN_PREFIX must be defined"
#else

#define JOIN(a, b) a##b
#define CONCAT(a, b) JOIN(a, b)
#define PREFIXED(x) CONCAT(SYN_PREFIX, x)

#undef JOIN
#undef CONCAT
#undef PREFIXED

#endif
