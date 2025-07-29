#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

typedef uintptr_t v_t;
typedef int64_t v_number_t;
typedef int32_t v_boolean_t;
typedef uintptr_t v_block_t;

typedef struct v_string {
    int ref_count;
    int length;
    char* data;
} v_string_box_t;

typedef struct v_list {
    int ref_count;
    int length;
    int capacity;
    v_t* items;
} v_list_box_t;

typedef v_list_box_t* v_list_t;
typedef v_string_box_t* v_string_t;

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

static inline const char* v_type(v_t v) {
    switch (V_TYPE(v)) {
        case TYPE_NUMBER: return "number";
        case TYPE_STRING: return "string";
        case TYPE_BOOLEAN: return "boolean";
        case TYPE_NULL: return "null";
        case TYPE_BLOCK: return "block";
        case TYPE_LIST: return "list";
        default: return "unknown";
    }
}

static inline v_t v_create_string(const char* str, size_t length) {
    v_string_box_t* box = malloc(sizeof(v_string_box_t) + length + 1);
    if (!box) panic("Failed to allocate memory for string box");
    box->ref_count = 1;
    box->length = length;
    box->data = (char*)(box + 1);
    memcpy(box->data, str, length);
    box->data[length] = '\0';
    return (v_t)box | TYPE_STRING;
}

static inline v_t v_create_list(int capacity) {
    v_list_box_t* box = malloc(sizeof(v_list_box_t));
    box->ref_count = 1;
    box->length = 0;
    box->capacity = capacity;
    box->items = (v_t*) malloc(sizeof(v_t) * capacity);
    return (v_t)box | TYPE_LIST;
}

static inline v_t v_create(v_type_t type, void* v) {
    if (type < TYPE_NUMBER || type > TYPE_BLOCK) {
        panic("Invalid type for v_create");
    }
    
    if (type == TYPE_BOOLEAN || type == TYPE_NUMBER) {
        return (v_t) ((uint64_t)v << 3) | type;
    } else {
        if ((uintptr_t)v & 0x7) {
            panic("Value must be 8-byte aligned in v_create");
        }

        return (v_t)v | type;
    }
}

static inline v_t v_coerce_to_number(v_t v) {
    if (V_IS_NUMBER(v)) return v;

    if (V_IS_STRING(v)) {
        v_string_t str = (v_string_t) (v & VALUE_MASK);

        char* endptr;
        v_number_t number = strtoll(str->data, &endptr, 10);

        if (number < -(1LL << 60) || number > ((1LL << 60) - 1)) {
            panic("Number %lld out of range (%lld to %lld) when coerced from %s", number, -(1LL << 60), (1LL << 60) - 1, str->data);
        }

        return number << 3;
    } else if (V_IS_BOOLEAN(v)) {
        return (v_t) (v & VALUE_MASK);
    } else if (V_IS_LIST(v)) {
        v_list_t list = (v_list_t) (v & VALUE_MASK);

        return (v_t) (list->length << 3);
    } else if (V_IS_NULL(v)) {
        return 0;
    }

    panic("Cannot coerce %s to number type", v_type(v));
}

static inline v_t v_coerce_to_string(v_t v) {
    if (V_IS_STRING(v)) return v;

    if (V_IS_NUMBER(v)) {
        v_number_t number = (v_number_t) v >> 3;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", (int64_t)number);
        return v_create_string(buffer, strlen(buffer));
    } else if (V_IS_BOOLEAN(v)) {
        return v_create_string((v_boolean_t)(v >> 3) ? "true" : "false", (v_boolean_t)(v >> 3) ? 4 : 5);
    } else if (V_IS_NULL(v)) {
        return v_create_string("", 0);
    } else if (V_IS_LIST(v)) {
        v_list_t list = (v_list_t) (v & VALUE_MASK);
        int length = 0;

        for (int i = 0; i < list->length; ++i) {
            v_t item = list->items[i];
            v_t str = V_IS_STRING(item) ? item : v_coerce_to_string(item);
            v_string_box_t* box = (v_string_box_t*)(str & VALUE_MASK);
            length += box->length;

            if (i < list->length - 1) length += 1;
        }

        char* buffer = malloc(length + 1);
        if (!buffer) panic("Failed to allocate memory for list string conversion");

        size_t offset = 0;
        for (int i = 0; i < list->length; ++i) {
            v_t item = list->items[i];
            v_t str = V_IS_STRING(item) ? item : v_coerce_to_string(item);
            v_string_box_t* box = (v_string_box_t*)(str & VALUE_MASK);

            memcpy(buffer + offset, box->data, box->length);

            offset += box->length;
            if (i < list->length - 1) buffer[offset++] = '\n';
        }

        buffer[length] = '\0';

        v_t result = v_create_string(buffer, length);
        free(buffer);
        return result;
    }

    panic("Cannot coerce type %s to string type", v_type(v));
}

static inline v_t v_coerce_to_boolean(v_t v) {
    if (V_IS_BOOLEAN(v)) return v;

    if (V_IS_NUMBER(v)) {
        return (v != 0) << 3 | TYPE_BOOLEAN;
    } else if (V_IS_STRING(v)) {
        v_string_t box = (v_string_t) (v & VALUE_MASK);
        return (box->length > 0) << 3 | TYPE_BOOLEAN;
    } else if (V_IS_NULL(v)) {
        return TYPE_BOOLEAN;
    } else if (V_IS_LIST(v)) {
        v_list_t list = (v_list_t) (v & VALUE_MASK);
        return (list->length > 0) << 3 | TYPE_BOOLEAN;
    }

    panic("Cannot coerce %s to boolean type", v_type(v));
}

static inline v_t v_coerce_to_list(v_t v) {
    if (V_IS_LIST(v)) return v;

    if (V_IS_NUMBER(v)) {
        v_number_t number = (v_number_t)(v >> 3);
        if (number < 0) {
            panic("Cannot coerce negative number to list of digits");
        }
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", number);
        size_t len = strlen(buffer);
        v_t list = v_create_list((int)len);
        v_list_t box = (v_list_t)(list & VALUE_MASK);
        for (int i = 0; i < (int) len; ++i) {
            if (i >= box->capacity) {
                int cap = box->capacity * 2;
                if (cap == 0) cap = 4;
                v_t* items = realloc(box->items, sizeof(v_t) * cap);
                if (!items) panic("Failed to realloc list items");
                box->items = items;
                box->capacity = cap;
            }

            v_t digit = ((v_t)(buffer[i] - '0') << 3) | TYPE_NUMBER;
            box->items[i] = digit;
            box->length++;
        }

        return list;
    } else if (V_IS_STRING(v)) {
        v_string_t str = (v_string_t) (v & VALUE_MASK);
        v_t list = v_create_list(str->length);
        v_list_t box = (v_list_t)(list & VALUE_MASK);

        for (int i = 0; i < str->length; ++i) {
            if (i >= box->capacity) {
                int cap = box->capacity * 2;
                if (cap == 0) cap = 4;
                v_t* items = realloc(box->items, sizeof(v_t) * cap);
                if (!items) panic("Failed to realloc list items");
                box->items = items;
                box->capacity = cap;
            }

            box->items[i] = v_create_string(&str->data[i], 1);
            box->length++;
        }

        return list;
    } else if (V_IS_NULL(v)) {
        return v_create_list(0);
    }

    panic("Cannot coerce %s to list type", v_type(v));
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