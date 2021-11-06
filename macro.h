#ifndef MACRO_H
#define MACRO_H

#define CONCAT(x, y) PREMITIVE_CONCAT(x, y)
#define PREMITIVE_CONCAT(x, y) x##y
#define EXPAND(x) x
#define INVOKE(f, x) f x
#define SWALLOW(x)
#define BREAK_PARENTHESES(_) )(

#define IS_UNIT(x) CONCAT(EVAL_, IMPL_IS_UNIT_0(BREAK_PARENTHESES x))
#define IMPL_IS_UNIT_0(x) EXPAND(IMPL_IS_UNIT_VALUE_0 SWALLOW(x))
#define IMPL_IS_UNIT_VALUE_0() IMPL_IS_UNIT_VALUE_1
#define EVAL_IMPL_IS_UNIT_VALUE_0 0
#define EVAL_IMPL_IS_UNIT_VALUE_1 1

#define NOT(x) IS_UNIT(PREMITIVE_CONCAT(IMPL_NOT_, x))
#define IMPL_NOT_0 ()

#define COMPL(x) PREMITIVE_CONCAT(IMPL_COMPL_, x)
#define IMPL_COMPL_0 1
#define IMPL_COMPL_1 0

#define BOOL(x) COMPL(NOT(x))

#define WHEN(cond) IF(cond)(EXPAND, SWALLOW)
#define IF(cond) IMPL_IF(cond)
#define IMPL_IF(cond) PREMITIVE_CONCAT(IMPL_IF_, cond)
#define IMPL_IF_0(t, f) f
#define IMPL_IF_1(t, f) t

#define EMPTY()
#define DEFER(x) x EMPTY()
#define OBSTRUCT(x) x DEFER(EMPTY)()

#define EVAL(x)        IMPL_EVAL_0(IMPL_EVAL_0(IMPL_EVAL_0(IMPL_EVAL_0(x))))
#define IMPL_EVAL_0(x) IMPL_EVAL_1(IMPL_EVAL_1(IMPL_EVAL_1(IMPL_EVAL_1(x))))
#define IMPL_EVAL_1(x) IMPL_EVAL_2(IMPL_EVAL_2(IMPL_EVAL_2(IMPL_EVAL_2(x))))
#define IMPL_EVAL_2(x) IMPL_EVAL_3(IMPL_EVAL_3(IMPL_EVAL_3(IMPL_EVAL_3(x))))
#define IMPL_EVAL_3(x) x

#define LIST_COMMA(head) (head),

#define LIST_HEAD(list) INVOKE(EXPAND, IMPL_LIST_HEAD_0(LIST_COMMA list))
#define IMPL_LIST_HEAD_0(x) IMPL_LIST_HEAD_1(x)
#define IMPL_LIST_HEAD_1(head, tail) head

#define LIST_TAIL(list) IMPL_LIST_TAIL_0(LIST_COMMA list)
#define IMPL_LIST_TAIL_0(x) IMPL_LIST_TAIL_1(x)
#define IMPL_LIST_TAIL_1(head, tail) tail

#define LIST_IS_EMPTY(list) NOT(CONCAT(EVAL_, EXPAND(IMPL_LIST_IS_EMPTY_0 list)))
#define IMPL_LIST_IS_EMPTY_0(_) LIST_IS_EMPTY_VALUE_1
#define EVAL_IMPL_LIST_IS_EMPTY_0 0
#define EVAL_IMPL_LIST_IS_EMPTY_1 1

#define IMPL_INVOKE_ALL(macro, list) \
    WHEN(NOT(LIST_IS_EMPTY(list))) ( \
        OBSTRUCT(macro) (LIST_HEAD(list)) \
        OBSTRUCT(IMPL_INVOKE_ALL_INDIRECT) () (macro, LIST_TAIL(list)) \
    )
#define IMPL_INVOKE_ALL_INDIRECT() IMPL_INVOKE_ALL
#define INVOKE_ALL(macro, list) OBSTRUCT(IMPL_INVOKE_ALL_INDIRECT) () (macro, list)

#endif /* MACRO_H */
