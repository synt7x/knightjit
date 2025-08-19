#ifndef VM_H
#define VM_H

#include "ir.h"
#include "jit/value.h"

typedef struct vm_stack_item {
    ir_block_t* callee;
    ir_block_t* previous;
    int index;
    ir_id_t result;
    v_t* registers;
} vm_stack_item_t;

typedef struct vm_stack {
    v_t* items;
    int size;
    int capacity;
} vm_stack_t;

typedef struct vm {
    ir_block_t* block;
    ir_function_t* function;

    v_t* variables;
    v_t* registers;

    int index;
} vm_t;

static inline void vm_push(vm_stack_t* stack, v_t value) {
    if (stack->size >= stack->capacity) {
        stack->capacity = stack->capacity ? stack->capacity * 2 : 4096;
        stack->items = realloc(stack->items, sizeof(v_t) * stack->capacity);
        if (!stack->items) panic("Failed to reallocate memory for VM stack items");
    }

    stack->items[stack->size++] = value;
}

static inline v_t vm_pop(vm_stack_t* stack) {
    if (stack->size == 0) {
        panic("FATAL: Please report this as a bug!");
    }

    return stack->items[--stack->size];
}

static inline v_t vm_ascii(v_t value) {
    if (V_IS_NUMBER(value) && value >> 3 < 0xFF) {
        return (v_t) v_create_string((char[]){(char)(value >> 3), '\0'}, 1);
    } else if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t)(value & VALUE_MASK);
        if (str->length != 0) {
            return ((v_t)((v_number_t)(unsigned char)str->data[0] << 3) | TYPE_NUMBER);
        } else {
            return (v_t) 0;
        }
    } else {
        panic("Cannot convert type %s to ASCII", v_type(value));
    }
}

static inline v_t vm_box(v_t value) {
    v_list_t list = (v_list_t) ((v_create_list(1)) & VALUE_MASK);
    list->items[0] = value;
    list->length = 1;
    return (v_t) list | TYPE_LIST;
}

static inline v_t vm_prime(v_t value) {
    if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t)(value & VALUE_MASK);
        if (str->length == 0) {
            return v_create_string("", 0);
        }

        char prime[2] = { str->data[0], '\0' };
        return v_create_string(prime, 1);
    } else if (V_IS_LIST(value)) {
        v_list_t list = (v_list_t)(value & VALUE_MASK);
        if (list->length == 0) {
            panic("Cannot get the first element of an empty list");
        }
        return list->items[0];
    } else {
        panic("Cannot get the first element of type %s", v_type(value));
    }
}

static inline v_t vm_ultimate(v_t value) {
    if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t)(value & VALUE_MASK);
        if (str->length == 1) {
            return v_create_string("", 0);
        } else if (str->length == 0) {
            panic("Cannot get the tail of an empty string");
        }

        return v_create_string(str->data + 1, str->length - 1);
    } else if (V_IS_LIST(value)) {
        v_list_t list = (v_list_t)(value & VALUE_MASK);
        if (list->length == 1) {
            return v_create_list(0);
        } else if (list->length == 0) {
            panic("Cannot get the tail of an empty list");
        }
        v_list_t tail = (v_list_t) (v_create_list(list->length - 1) & VALUE_MASK);
        tail->length = list->length - 1;
        memcpy(tail->items, list->items + 1, sizeof(v_t) * tail->length);
        return (v_t)tail | TYPE_LIST;
    } else {
        panic("Cannot get the tail of type %s", v_type(value));
    }
}

static inline v_t vm_length(v_t value) {
    if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t)(value & VALUE_MASK);
        return (v_t) (str->length << 3);
    } else if (V_IS_LIST(value)) {
        v_list_t list = (v_list_t)(value & VALUE_MASK);
        return (v_t) (list->length << 3);
    } else {
        v_list_t list = (v_list_t) (v_coerce_to_list(value) & VALUE_MASK);
        v_t length = (v_t) (list->length << 3);

        if (!V_IS_LIST(value)) {
            free(list->items);
            free(list);
        }

        return length;
    }
}

static inline v_t vm_add(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        return (v_t) (left + right);
    }

    v_t coerced = v_coerce(right, V_TYPE(left));
    if (V_IS_NUMBER(left)) {
       v_number_t l = (v_number_t)(left >> 3);
       v_number_t r = (v_number_t)(coerced >> 3);

       return (v_t) ((uintptr_t)(l + r) << 3 | TYPE_NUMBER);
    } else if (V_IS_STRING(left)) {
        v_string_t l = (v_string_t) (left & VALUE_MASK);
        v_string_t r = (v_string_t) (coerced & VALUE_MASK);

        size_t length = l->length + r->length;
        char* joined = malloc(length + 1);
        if (!joined) panic("Failed to allocate memory for string concatenation");

        memcpy(joined, l->data, l->length);
        memcpy(joined + l->length, r->data, r->length);
        joined[length] = '\0';

        v_t result = v_create_string(joined, length);
        free(joined);

        return result;
    } else if (V_IS_LIST(left)) {
        v_list_t l = (v_list_t) (left & VALUE_MASK);
        v_list_t r = (v_list_t) (coerced & VALUE_MASK);

        v_list_t joined = (v_list_t) (v_create_list(l->length + r->length) & VALUE_MASK);

        joined->length = l->length + r->length;

        memcpy(joined->items, l->items, sizeof(v_t) * l->length);
        memcpy(joined->items + l->length, r->items, sizeof(v_t) * r->length);

        return (v_t) joined | TYPE_LIST;
    } else {
        panic("Cannot add types that are not number, string, or list");
    }
}

static inline v_t vm_sub(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        return (v_t) (left - right);
    }

    v_t coerced = v_coerce(right, V_TYPE(left));
    if (V_IS_NUMBER(left)) {
        return (v_t) (left - coerced); 
    } else {
        panic("Cannot subtract non-number types");
    }
}

static inline v_t vm_mul(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) right >> 3;
        return (v_t) ((l * r) << 3 | TYPE_NUMBER);
    }

    v_t coerced = v_coerce(right, TYPE_NUMBER);
    if (V_IS_NUMBER(left)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) coerced >> 3;
        return (v_t) ((l * r) << 3 | TYPE_NUMBER);
    } else if (V_IS_STRING(left)) {
        v_string_t str = (v_string_t) (left & VALUE_MASK);
        size_t length = str->length * ((v_number_t)(coerced) >> 3);
        char* buffer = malloc(length + 1);
        if (!buffer) panic("Failed to allocate memory for string multiplication");

        if (((v_number_t)(coerced) >> 3) < 0) {
            panic("Cannot multiply string by negative number");
        }

        for (size_t i = 0; i < length; ++i) {
            buffer[i] = str->data[i % str->length];
        }
        buffer[length] = '\0';

        v_t result = v_create_string(buffer, length);
        free(buffer);
        return result;
    } else if (V_IS_LIST(left)) {
        v_list_t list = (v_list_t) (left & VALUE_MASK);
        int repeat_count = (int)(coerced >> 3);
        if (repeat_count == 0) {
            return v_create_list(0);
        } else if (repeat_count < 0) {
            panic("Cannot multiply list by negative number");
        }

        v_list_t result = malloc(sizeof(v_list_box_t));
        if (!result) panic("Failed to allocate memory for list multiplication");

        result->length = list->length * repeat_count;
        result->capacity = result->length;
        result->items = malloc(sizeof(v_t) * result->length);
        if (!result->items) panic("Failed to allocate memory for list items");

        for (int i = 0; i < repeat_count; ++i) {
            memcpy(result->items + i * list->length, list->items, sizeof(v_t) * list->length);
        }

        return (v_t) result | TYPE_LIST;
    }

    panic("Cannot multiply types that are not number, list, or string");
}

static inline v_t vm_div(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) right >> 3;
        if (r == 0) panic("Division by zero");
        return (v_t) ((l / r) << 3 | TYPE_NUMBER);
    }

    v_t coerced = v_coerce(right, TYPE_NUMBER);
    if (V_IS_NUMBER(left)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) coerced >> 3;
        if (r == 0) panic("Division by zero");
        return (v_t) ((l / r) << 3 | TYPE_NUMBER);
    }

    panic("Cannot divide non-number types");
}

static inline v_t vm_mod(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) right >> 3;
        if (r == 0) panic("Division by zero");
        if (l < 0 || r < 0) {
            panic("Cannot perform modulo operation with negative numbers");
        }

        return (v_t) ((l % r) << 3 | TYPE_NUMBER);
    }

    v_t coerced = v_coerce(right, TYPE_NUMBER);
    if (V_IS_NUMBER(left)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) coerced >> 3;
        if (r == 0) panic("Division by zero");
        if (l < 0 || r < 0) {
            panic("Cannot perform modulo operation with negative numbers");
        }

        return (v_t) ((l % r) << 3 | TYPE_NUMBER);
    }

    panic("Cannot mod non-number types");
}

static inline v_t vm_pow(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) right >> 3;
        v_number_t result = 1;

        int negative = 0;
        if (r < 0) {
            negative = 1;
            r = -r;
        }

        while (r > 0) {
            if (r & 1) result *= l;
            l *= l;
            r >>= 1;
        }

        if (negative) {
            if (result != 0) {
                result = 1 / result;
            } else {
                result = 0;
            }
        }

        return (v_t) ((result << 3) | TYPE_NUMBER);
    }

    if (V_IS_NUMBER(left)) {
        v_t coerced = v_coerce(right, TYPE_NUMBER);
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) coerced >> 3;
        v_number_t result = 1;

        int negative = 0;
        if (r < 0) {
            negative = 1;
            r = -r;
        }

        while (r > 0) {
            if (r & 1) result *= l;
            l *= l;
            r >>= 1;
        }

        if (negative) {
            if (result != 0) {
                result = 1 / result;
            } else {
                result = 0;
            }
        }

        return (v_t) ((result << 3) | TYPE_NUMBER);
    } else if (V_IS_LIST(left)) {
        v_t coerced = v_coerce(right, TYPE_STRING);
        v_list_t list = (v_list_t) (left & VALUE_MASK);

        v_string_t sep = (v_string_t)(coerced & VALUE_MASK);
        if (list->length == 0) {
            return v_create_string("", 0);
        }
        size_t total_length = 0;
        for (size_t i = 0; i < list->length; ++i) {
            v_t elem = list->items[i];
            v_string_t elem_str = (v_string_t)(v_coerce_to_string(elem) & VALUE_MASK);
            total_length += elem_str->length;
            if (i < list->length - 1) {
                total_length += sep->length;
            }
        }
        char* buffer = malloc(total_length + 1);
        if (!buffer) panic("Failed to allocate memory for join result");
        size_t offset = 0;
        for (size_t i = 0; i < list->length; ++i) {
            v_t elem = list->items[i];
            v_string_t elem_str = (v_string_t)(v_coerce_to_string(elem) & VALUE_MASK);
            memcpy(buffer + offset, elem_str->data, elem_str->length);
            offset += elem_str->length;
            if (i < list->length - 1 && sep->length > 0) {
                memcpy(buffer + offset, sep->data, sep->length);
                offset += sep->length;
            }
        }
        buffer[total_length] = '\0';
        v_t result = v_create_string(buffer, total_length);
        free(buffer);
        return result;
    }

    panic("Cannot power non-number/list types");
}

static inline v_t vm_gt(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        return (v_t) (((v_number_t) left > (v_number_t) right) << 3 | TYPE_BOOLEAN);
    }

    v_t coerced = v_coerce(right, V_TYPE(left));
    if (V_IS_NUMBER(left)) {
        return (v_t) (((v_number_t) left > (v_number_t) coerced) << 3 | TYPE_BOOLEAN);
    } else if (V_IS_STRING(left)) {
        v_string_t l = (v_string_t) (left & VALUE_MASK);
        v_string_t r = (v_string_t) (coerced & VALUE_MASK);

        return ((v_t) (strcmp(l->data, r->data) > 0) << 3) | TYPE_BOOLEAN;
    } else if (V_IS_BOOLEAN(left)) {
        return ((v_t) (left > coerced) << 3 | TYPE_BOOLEAN);
    }

    panic("Cannot compare %s and %s", v_type(left), v_type(right));
}

static inline v_t vm_lt(v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        return (v_t) (((v_number_t) left < (v_number_t) right) << 3 | TYPE_BOOLEAN);
    }

    v_t coerced = v_coerce(right, V_TYPE(left));
    if (V_IS_NUMBER(left)) {
        return (v_t) (((v_number_t) left < (v_number_t) coerced) << 3 | TYPE_BOOLEAN);
    } else if (V_IS_STRING(left)) {
        v_string_t l = (v_string_t) (left & VALUE_MASK);
        v_string_t r = (v_string_t) (right & VALUE_MASK);

        return ((v_t) (strcmp(l->data, r->data) < 0) << 3) | TYPE_BOOLEAN;
    } else if (V_IS_BOOLEAN(left)) {
        return ((v_t) (left < coerced) << 3 | TYPE_BOOLEAN);
    }

    panic("Cannot compare %s and %s", v_type(left), v_type(right));
}

static inline v_t vm_eq(v_t left, v_t right) {
    if (V_TYPE(left) != V_TYPE(right)) {
        return (v_t) TYPE_BOOLEAN;
    }

    if (V_IS_NUMBER(left)) {
        return (v_t) (((v_number_t) left == (v_number_t) right) << 3 | TYPE_BOOLEAN);
    } else if (V_IS_STRING(left) && V_IS_STRING(right)) {
        v_string_t l = (v_string_t) (left & VALUE_MASK);
        v_string_t r = (v_string_t) (right & VALUE_MASK);

        return ((v_t) (strcmp(l->data, r->data) == 0) << 3) | TYPE_BOOLEAN;
    } else if (V_IS_LIST(left) && V_IS_LIST(right)) {
        v_list_t l = (v_list_t) (left & VALUE_MASK);
        v_list_t r = (v_list_t) (right & VALUE_MASK);

        if (l->length != r->length) return (v_t) TYPE_BOOLEAN;

        for (size_t i = 0; i < l->length; ++i) {
            if (!vm_eq(l->items[i], r->items[i])) {
                return (v_t) TYPE_BOOLEAN;
            }
        }

        return ((v_t) 1 << 3 | TYPE_BOOLEAN);
    } else if (V_IS_NULL(left)) {
        return ((v_t) 1 << 3 | TYPE_BOOLEAN);
    } else if (V_IS_BOOLEAN(left)) {
        return ((v_t) (left == right) << 3 | TYPE_BOOLEAN);
    } else if (V_IS_BLOCK(left)) {
        return ((v_t) (left == right) << 3 | TYPE_BOOLEAN);
    }

    panic("Cannot compare %s and %s", v_type(left), v_type(right));
}

static inline v_t vm_get(v_t value, v_t index, v_t range) {
    v_t coerced_index = v_coerce(index, TYPE_NUMBER);
    v_t coerced_range = v_coerce(range, TYPE_NUMBER);

    if (V_IS_LIST(value)) {
        v_list_t list = (v_list_t) (value & VALUE_MASK);
        v_number_t idx = (v_number_t) coerced_index >> 3;
        v_number_t len = (v_number_t) coerced_range >> 3;

        if (idx == 0 && len == 0) {
            return v_create_list(0);
        }

        if (idx < 0 || (size_t) idx >= list->length || (size_t) idx + (size_t) len > list->length) {
            panic("Index %zu with range %zu out of bounds for list of length %zu", idx, len, list->length);
        }

        if (len == 1) {
            v_list_t sublist = (v_list_t) (v_create_list(1) & VALUE_MASK);
            sublist->items[0] = list->items[idx];
            sublist->length = 1;
            return (v_t) sublist | TYPE_LIST;
        } else {
            v_list_t sublist = (v_list_t) (v_create_list(len) & VALUE_MASK);
            sublist->length = len;
            memcpy(sublist->items, list->items + idx, sizeof(v_t) * len);
            return (v_t) sublist | TYPE_LIST;
        }
    } else if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t) (value & VALUE_MASK);
        v_number_t idx = (v_number_t) coerced_index >> 3;
        v_number_t len = (v_number_t) coerced_range >> 3;

        if (idx == 0 && len == 0) {
            return v_create_string("", 0);
        }

        if (idx < 0 || (size_t) idx >= str->length || (size_t) idx + (size_t) len > str->length) {
            panic("Index %lld with range %lld out of bounds for string of length %zu", idx, len, str->length);
        }

        if (len == 1) {
            char* sub_str = malloc(2);
            if (!sub_str) panic("Failed to allocate memory for substring");
            sub_str[0] = str->data[idx];
            sub_str[1] = '\0';
            v_t str = v_create_string(sub_str, 1);
            free(sub_str);
            return str;
        } else {
            char* sub_str = malloc(len + 1);
            if (!sub_str) panic("Failed to allocate memory for substring");
            memcpy(sub_str, str->data + idx, len);
            sub_str[len] = '\0';
            v_t str = v_create_string(sub_str, len);
            free(sub_str);
            return str;
        }
    }

    panic("Cannot get index %s with range %s from type %s", v_type(index), v_type(range), v_type(value));
}

static inline v_t vm_set(v_t value, v_t index, v_t range, v_t replace) {
    v_t coerced_index = v_coerce(index, TYPE_NUMBER);
    v_t coerced_range = v_coerce(range, TYPE_NUMBER);

    v_number_t idx = (v_number_t) coerced_index >> 3;
    v_number_t len = (v_number_t) coerced_range >> 3;

    if (V_IS_LIST(value)) {
        v_list_t list = (v_list_t) (value & VALUE_MASK);
        v_list_t replace_list = (v_list_t) (v_coerce_to_list(replace) & VALUE_MASK);

        if (idx < 0 || (size_t) idx > list->length) {
            panic("Index %lld with range %lld out of bounds for list of length %zu", idx, len, list->length);
        }

        v_list_t alt = (v_list_t) (v_create_list(list->length - len + replace_list->length) & VALUE_MASK);
        size_t pos = 0;

        for (size_t i = 0; i < (size_t) idx; ++i) {
            alt->items[pos++] = list->items[i];
        }

        for (size_t i = 0; i < replace_list->length; ++i) {
            alt->items[pos++] = replace_list->items[i];
        }

        for (size_t i = idx + len; i < list->length; ++i) {
            alt->items[pos++] = list->items[i];
        }

        alt->length = pos;
        return (v_t) alt | TYPE_LIST;
    } else if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t) (value & VALUE_MASK);
        v_string_t substr = (v_string_t) (v_coerce_to_string(replace) & VALUE_MASK);

        if (idx < 0 || (size_t) idx > str->length || (size_t) idx + (size_t) len > str->length || len < 0) {
            panic("Index %lld with range %lld out of bounds for string of length %zu", idx, len, str->length);
        }

        size_t new_length = str->length - len + substr->length;
        char* new_data = malloc(new_length + 1);
        if (!new_data) panic("Failed to allocate memory for modified string");

        if (idx > 0) memcpy(new_data, str->data, idx);
        if (substr->length > 0) memcpy(new_data + idx, substr->data, substr->length);
        if (str->length > (size_t) idx + (size_t) len) memcpy(new_data + idx + substr->length, str->data + idx + len, str->length - idx - len);
        new_data[new_length] = '\0';

        v_t alt = v_create_string(new_data, new_length);
        free(new_data);
        return alt;
    }

    panic("Cannot set index %s with range %s on type %s", v_type(index), v_type(range), v_type(value));
}

vm_t* vm_run(ir_function_t* function, arena_t* arena);

#endif