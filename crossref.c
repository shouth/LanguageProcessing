#include <assert.h>

#include "mppl.h"

typedef struct impl_crossref_item crossref_item_t;
struct impl_crossref_item {
    const ir_item_t *item;
    const crossref_item_t *owner;
};

typedef struct {
    const source_t *source;
} crossref_t;

static void crossref_print_ns(crossref_t *crossref, const crossref_item_t *ns)
{
    const symbol_t *symbol;
    assert(crossref);
    if (!ns) {
        return;
    }

    symbol = ns->item->symbol;
    printf(" @ %.*s", (int) symbol->len, symbol->ptr);
    crossref_print_ns(crossref, ns->owner);
}

static void crossref_print_type(crossref_t *crossref, const ir_type_t *type)
{
    printf("%s", ir_type_str(type));
}

static void crossref_print_location(crossref_t *crossref, size_t pos)
{
    location_t loc = source_location(crossref->source, pos);
    printf("%ld:%ld", loc.line, loc.col);
}

static void crossref_print_ref(crossref_t *crossref, ir_item_pos_t *pos)
{
    while (pos) {
        crossref_print_location(crossref, pos->pos);
        if (pos = pos->next) {
            printf(", ");
        }
    }
}

static size_t crossref_count_item(const ir_item_t *item)
{
    size_t ret;

    ret = 0;
    while (item) {
        if (item->kind != IR_ITEM_PROGRAM) {
            ++ret;
        }
        if (item->body) {
            ret += crossref_count_item(item->body->items);
        }
        item = item->next;
    }
    return ret;
}

static crossref_item_t *crossref_init_item(crossref_item_t *crossref_items, const ir_item_t *ir_item, const crossref_item_t *owner)
{
    while (ir_item) {
        if (ir_item->kind != IR_ITEM_PROGRAM) {
            crossref_items->item = ir_item;
            crossref_items->owner = owner;
            crossref_items++;
            if (ir_item->body) {
                crossref_items = crossref_init_item(crossref_items, ir_item->body->items, crossref_items - 1);
            }
        } else {
            crossref_items = crossref_init_item(crossref_items, ir_item->body->items, NULL);
        }
        ir_item = ir_item->next;
    }
    return crossref_items;
}

static int crossref_item_compare(const void *lhs, const void *rhs)
{
    const crossref_item_t *l = lhs, *r = rhs;
    int ret;

    if (!l && !r) {
        return 0;
    } else if (!l && r) {
        return -1;
    } else if (l && !r) {
        return 1;
    } else if (ret = symbol_compare(l->item->symbol, r->item->symbol)) {
        return ret;
    } else {
        return crossref_item_compare(l->owner, r->owner);
    }
}

static int crossref_item_ptr_compare(const void *lhs, const void *rhs)
{ return crossref_item_compare(*(crossref_item_t **) lhs, *(crossref_item_t **) rhs); }

void print_crossref(const ir_t *ir)
{
    crossref_t crossref;
    size_t i, cnt;
    crossref_item_t *items, **sort;
    assert(ir);

    cnt = crossref_count_item(ir->items);
    items = new_arr(crossref_item_t, cnt);
    crossref_init_item(items, ir->items, NULL);
    sort = new_arr(crossref_item_t *, cnt);
    for (i = 0; i < cnt; i++) {
        sort[i] = items + i;
    }
    qsort(sort, cnt, sizeof(*sort), crossref_item_ptr_compare);
    crossref.source = ir->source;
    for (i = 0; i < cnt; i++) {
        const ir_item_t *item = sort[i]->item;
        printf("Name | %.*s", (int) item->symbol->len, item->symbol->ptr);
        crossref_print_ns(&crossref, sort[i]->owner);
        printf("\n");
        printf("Type | ");
        crossref_print_type(&crossref, item->type);
        printf("\n");
        printf("Def. | ");
        crossref_print_location(&crossref, item->name_region.pos);
        printf("\n");
        printf("Ref. | ");
        crossref_print_ref(&crossref, item->refs.head);
        printf("\n\n");
    }
    free(items);
    free(sort);
}
