/*
 * ty.c -- type system test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stddef.h>

#include "ty.h"

int main(void)
{
  struct ty_ctxt ctxt;
  ty_init(&ctxt);

  {
    struct ty const *a1, *a2, *a3, *a4;

    a1 = ty_array(&ctxt, ty_int(), 10);
    a2 = ty_array(&ctxt, ty_int(), 10);
    a3 = ty_array(&ctxt, ty_bool(), 10);
    a4 = ty_array(&ctxt, ty_int(), 20);

    assert(a1 == a2);
    assert(a1 != a3);
    assert(a1 != a4);
  }

  {
    struct ty const *params[2];
    struct ty const *p1, *p2, *p3, *p4;

    params[0] = ty_int();
    params[1] = ty_bool();
    p1 = ty_proc(&ctxt, params, 2);

    params[0] = ty_int();
    params[1] = ty_bool();
    p2 = ty_proc(&ctxt, params, 2);

    params[0] = ty_bool();
    params[1] = ty_int();
    p3 = ty_proc(&ctxt, params, 2);

    params[0] = ty_int();
    p4 = ty_proc(&ctxt, params, 1);

    assert(p1 == p2);
    assert(p1 != p3);
    assert(p1 != p4);
  }

  ty_deinit(&ctxt);

  return EXIT_SUCCESS;
}
