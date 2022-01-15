#include <assert.h>

#include "mppl.h"

typedef struct impl_cross_ref_ns cross_ref_ns_t;
struct impl_cross_ref_ns {
    const ir_item_t *item;
    cross_ref_ns_t *next;
};

typedef struct {
    const source_t *source;
    cross_ref_ns_t *ns;
} cross_ref_t;

static void cross_ref_print_ns(cross_ref_t *cross_ref, cross_ref_ns_t *ns)
{
    const symbol_instance_t *instance;
    assert(cross_ref);
    if (!ns) {
        return;
    }

    instance = symbol_get_instance(ns->item->symbol);
    printf(" @ %.*s", (int) instance->len, instance->ptr);
    cross_ref_print_ns(cross_ref, ns->next);
}

static void cross_ref_print_type(cross_ref_t *cross_ref, const ir_type_t *type)
{
    printf("%s", ir_type_str(type));
}

static void cross_ref_print_location(cross_ref_t *cross_ref, size_t pos)
{
    location_t loc = source_location(cross_ref->source, pos);
    printf("%ld:%ld", loc.line, loc.col);
}

static void cross_ref_print_ref(cross_ref_t *cross_ref, ir_item_pos_t *pos)
{
    while (pos) {
        cross_ref_print_location(cross_ref, pos->pos);
        if (pos = pos->next) {
            printf(", ");
        }
    }
}

static void internal_print_cross_ref(cross_ref_t *cross_ref, const ir_item_t *item)
{
    const ir_item_t *inner_item;
    assert(cross_ref && item);
    if (!item->body) {
        return;
    }

    inner_item = item->body->items;
    while (inner_item) {
        const symbol_instance_t *symbol_instance;
        cross_ref_ns_t ns;

        symbol_instance = symbol_get_instance(inner_item->symbol);
        printf("Name | %.*s", (int) symbol_instance->len, symbol_instance->ptr);
        cross_ref_print_ns(cross_ref, cross_ref->ns);
        printf("\n");
        printf("Type | ");
        cross_ref_print_type(cross_ref, inner_item->type);
        printf("\n");
        printf("Def. | ");
        cross_ref_print_location(cross_ref, inner_item->name_region.pos);
        printf("\n");
        printf("Ref. | ");
        cross_ref_print_ref(cross_ref, inner_item->refs.head);
        printf("\n\n");

        ns.item = inner_item;
        ns.next = cross_ref->ns;
        cross_ref->ns = &ns;
        internal_print_cross_ref(cross_ref, inner_item);
        cross_ref->ns = cross_ref->ns->next;
        inner_item = inner_item->next;
    }
}

void print_cross_ref(const ir_t *ir)
{
    cross_ref_t cross_ref;
    cross_ref.source = ir->source;
    cross_ref.ns = NULL;
    internal_print_cross_ref(&cross_ref, ir->items);
}
