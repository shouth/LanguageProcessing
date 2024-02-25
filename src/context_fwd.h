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

#ifndef CONTEXT_FWD_H
#define CONTEXT_FWD_H

typedef enum {
  TYPE_BOOLEAN,
  TYPE_CHAR,
  TYPE_INTEGER,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_PROC
} TypeKind;

typedef enum {
  DEF_PROGRAM,
  DEF_PROC,
  DEF_VAR,
  DEF_PARAM,
  DEF_LOCAL
} DefKind;

typedef struct String    String;
typedef struct TypeList  TypeList;
typedef struct Type      Type;
typedef struct ArrayType ArrayType;
typedef struct ProcType  ProcType;
typedef struct Def       Def;
typedef struct Ctx       Ctx;

#endif
