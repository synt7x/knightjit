#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

typedef uintptr_t v_t;
typedef int32_t v_number_t;
typedef char* v_string_t;
typedef int32_t v_boolean_t;
typedef uintptr_t v_block_t;
typedef uintptr_t v_list_t;

typedef enum {
    TYPE_NUMBER = 0,
    TYPE_STRING = 1,
    TYPE_BOOLEAN = 2,
    TYPE_NULL = 3,
    TYPE_BLOCK = 4,
    TYPE_LIST = 5,

    TYPE_MASK = 7,
    VALUE_MASK = (~TYPE_MASK)
} v_type_t;

#define V_TYPE(v) ((v) & TYPE_MASK)
#define V_IS_NUMBER(v) (V_TYPE(v) == TYPE_NUMBER)
#define V_IS_STRING(v) (V_TYPE(v) == TYPE_STRING)
#define V_IS_BOOLEAN(v) (V_TYPE(v) == TYPE_BOOLEAN)
#define V_IS_NULL(v) (V_TYPE(v) == TYPE_NULL)
#define V_IS_BLOCK(v) (V_TYPE(v) == TYPE_BLOCK)
#define V_IS_LIST(v) (V_TYPE(v) == TYPE_LIST)

static inline v_t v_create_string(const char* str) {
    size_t len = strlen(str);
    char* heap_str = malloc(len + 1);
    if (!heap_str) {
        panic("Memory allocation failed in v_create");
    }
    strcpy(heap_str, str);
    return (v_t)heap_str | TYPE_STRING;
}


static inline v_t v_create_static_string(const char* str, size_t length) {
    char* heap_str = malloc(length + 1);
    if (!heap_str) {
        panic("Memory allocation failed in v_create");
    }
    strncpy(heap_str, str, length);
    heap_str[length] = '\0';
    return (v_t)heap_str | TYPE_STRING;
}

static inline v_t v_create(v_type_t type, void* v) {
    if (type < TYPE_NUMBER || type > TYPE_BLOCK) {
        panic("Invalid type for v_create");
    }
    
    if (type == TYPE_BOOLEAN || type == TYPE_NUMBER) {
        return (v_t)((uint64_t)v<<3) | type;
    } else {
        if (type == TYPE_STRING) {
            return v_create_string((const char*)v);
        }

        if ((uintptr_t)v & 0x7) {
            panic("Value must be 8-byte aligned in v_create");
        }

        return (v_t)v | type;
    }
}

static inline v_t v_coerce_to_number(v_t v) {
    if (V_IS_NUMBER(v)) return v;

    panic("Cannot coerce v to number type");
}

static inline v_t v_coerce_to_string(v_t v) {
    if (V_IS_STRING(v)) return v;

    panic("Cannot coerce v to string type");
}

static inline v_t v_coerce_to_boolean(v_t v) {
    if (V_IS_BOOLEAN(v)) return v;

    panic("Cannot coerce v to boolean type");
}

static inline v_t v_coerce_to_list(v_t v) {
    if (V_IS_LIST(v)) return v;

    panic("Cannot coerce v to list type");
}

static inline v_t v_coerce(v_t v, v_type_t type) {
    if (V_TYPE(v) == type) return v;

    switch (type) {
        case TYPE_NUMBER: return v_coerce_to_number(v);
        case TYPE_STRING: return v_coerce_to_string(v);
        case TYPE_BOOLEAN: return v_coerce_to_boolean(v);
        case TYPE_LIST: return v_coerce_to_list(v);
        case TYPE_NULL: panic("Cannot coerce to null type");
        case TYPE_BLOCK: panic("Cannot coerce to block type");
        default: return 0;
    }
}

#endif