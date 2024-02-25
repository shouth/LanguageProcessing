/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef CONTEXT_H
#define CONTEXT_H

#include "context_fwd.h"
#include "syntax_tree.h"

Ctx            *ctx_new(void);
void            ctx_free(Ctx *ctx);
const String   *ctx_string(Ctx *ctx, const char *data, unsigned long length);
const Type     *ctx_array_type(Ctx *ctx, const Type *base, unsigned long length);
const Type     *ctx_proc_type(Ctx *ctx, const TypeList *params);
const Type     *ctx_type(TypeKind kind);
const TypeList *ctx_type_list(Ctx *ctx, const Type **types, unsigned long length);
const TypeList *ctx_take_type_list(Ctx *ctx, const Type **types, unsigned long length);
const Def      *ctx_define(Ctx *ctx, DefKind kind, const String *name, const SyntaxTree *syntax);
const Def      *ctx_resolve(Ctx *ctx, const SyntaxTree *syntax, const Def *def);
const Type     *ctx_type_of(const Ctx *ctx, const SyntaxTree *syntax, const Type *type);

const char   *string_data(const String *string);
unsigned long string_length(const String *string);

const Type   *type_list_at(const TypeList *list, unsigned long index);
unsigned long type_list_count(const TypeList *list);

TypeKind type_kind(const Type *type);
int      type_is_std(const Type *type);
char    *type_to_string(const Type *type);

const Type   *array_type_base(const ArrayType *type);
unsigned long array_type_length(const ArrayType *type);

const TypeList *proc_type_params(const ProcType *type);

DefKind           def_kind(const Def *def);
const String     *def_name(const Def *def);
const SyntaxTree *def_syntax(const Def *def);

#endif
