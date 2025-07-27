#include "vm.h"
#include "math.h"

typedef struct register_pool {
    v_t** arrays;
    int count;
    int capacity;
    int frame_size;
    arena_t* arena;
} register_pool_t;

static inline void register_pool_init(register_pool_t* pool, int frame_size, arena_t* arena) {
    pool->count = 0;
    pool->capacity = 16;
    pool->frame_size = frame_size;
    pool->arena = arena;
    pool->arrays = arena_alloc(arena, sizeof(v_t*) * pool->capacity);
    if (!pool->arrays) panic("Failed to allocate register pool");
}

static inline v_t* register_pool_acquire(register_pool_t* pool) {
    if (pool->count > 0) {
        return pool->arrays[--pool->count];
    }
    v_t* arr = arena_alloc(pool->arena, sizeof(v_t) * pool->frame_size);
    if (!arr) panic("Register pool/arena exhausted: too much recursion or too many calls");
    return arr;
}

static inline void register_pool_release(register_pool_t* pool, v_t* array) {
    if (pool->count >= pool->capacity) {
        pool->capacity *= 4;
        pool->arrays = arena_realloc(pool->arena, pool->arrays, sizeof(v_t*) * pool->capacity);
        if (!pool->arrays) panic("Failed to grow register pool");
    }
    pool->arrays[pool->count++] = array;
}

vm_t* vm_init(ir_function_t* function, arena_t* arena) {
    vm_t* vm = arena_alloc(arena, sizeof(vm_t));
    if (!vm) panic("Failed to allocate memory for VM");

    vm->block = function->block;
    vm->function = function;
    vm->variables = arena_alloc(arena, sizeof(v_t*) * (function->var_id + 1));
    vm->registers = arena_alloc(arena, sizeof(v_t*) * function->next_value_id);

    memset(vm->variables, 0, sizeof(v_t*) * (function->var_id + 1));
    memset(vm->registers, 0, sizeof(v_t*) * function->next_value_id);

    if (!vm->variables || !vm->registers) {
        panic("Failed to allocate memory for VM");
    }

    return vm;
}

static inline v_t vm_prompt() {
    int capacity = 16;
    int length = 0;
    char* buffer = malloc(16);
    if (!buffer) panic("Failed to allocate memory for prompt buffer");

    char character;
    while ((character = getchar()) != EOF && character != '\n') {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char* str = realloc(buffer, capacity);
            if (!str) {
                free(buffer);
                panic("Failed to reallocate memory for prompt buffer");
            }

            buffer = str;
        }
        buffer[length++] = character;
    }

    if (character == EOF && length == 0) {
        free(buffer);
        return (v_t)TYPE_NULL;
    }

    if (length > 0 && buffer[length - 1] == '\r') {
        length--;
    }

    buffer[length] = '\0';

    v_t result = v_create_string(buffer, length);
    free(buffer);
    return result;
}

typedef struct {
    int* indices;
    int count;
    int capacity;
} load_reg_list_t;

static void load_reg_list_init(load_reg_list_t* list) {
    list->count = 0;
    list->capacity = 8;
    list->indices = malloc(sizeof(int) * list->capacity);
}

static void load_reg_list_add(load_reg_list_t* list, int reg) {
    for (int i = 0; i < list->count; ++i) {
        if (list->indices[i] == reg) return;
    }
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->indices = realloc(list->indices, sizeof(int) * list->capacity);
    }
    list->indices[list->count++] = reg;
}

static inline ir_block_t* vm_call(
    ir_block_t* origin, ir_block_t* previous, int index, ir_id_t result, v_t block,
    vm_stack_t* stack, arena_t* arena, v_t** registers_ptr, register_pool_t* reg_pool, load_reg_list_t* load_regs
) {
    ir_block_t* target = (ir_block_t*) (block & VALUE_MASK);
    if (!V_IS_BLOCK(block)) panic("Expected a block type for call, got %s", v_type(block));
    if (!target) panic("Unknown block type in call");

    if (stack->size >= stack->capacity) {
        stack->capacity = stack->capacity ? stack->capacity * 2 : 8;
        stack->items = arena_realloc(arena, stack->items, sizeof(vm_stack_item_t) * stack->capacity);
        if (!stack->items) panic("Failed to reallocate memory for VM stack items");
    }

    vm_stack_item_t* item;
    if (stack->size < stack->capacity && stack->items && &stack->items[stack->size]) {
        item = &stack->items[stack->size];
    } else {
        item = arena_alloc(arena, sizeof(vm_stack_item_t));
    }

    if (!item) panic("Failed to allocate memory for VM stack item");
    item->callee = origin;
    item->index = index;
    item->result = result;
    item->previous = previous;
    item->registers = *registers_ptr;

    stack->items[stack->size++] = *item;

    v_t* new_registers = register_pool_acquire(reg_pool);

    for (int i = 0; i < load_regs->count; ++i) {
        int reg = load_regs->indices[i];
        new_registers[reg] = (*registers_ptr)[reg];
    }
    *registers_ptr = new_registers;

    return target;
}

static inline vm_stack_item_t* vm_return(vm_stack_t* stack) {
    if (stack->size == 0) {
        panic("Attempted to return from a non-BLOCK");
    }
    return &stack->items[--stack->size];
}

static inline v_t vm_ascii(v_t value) {
    if (V_IS_NUMBER(value) && value >> 3 < 0xFF) {
        return (v_t) v_create_string((char[]){(char)(value >> 3), '\0'}, 1);
    } else if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t)(value & VALUE_MASK);
        if (str->length != 0) {
            return ((v_t)((v_number_t)(unsigned char)str->data[0] << 3) | TYPE_NUMBER);
        } else {
            panic("Cannot convert string of length %d to ASCII", str->length);
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
            panic("Cannot get the first element of an empty string");
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
        tail->ref_count = 1;
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
        panic("Cannot get length of type %s", v_type(value));
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
        joined->ref_count = 1;

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
        size_t length = str->length * ((v_number_t)(coerced >> 3));
        char* buffer = malloc(length + 1);
        if (!buffer) panic("Failed to allocate memory for string multiplication");

        if (length < 0) {
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
        result->ref_count = 1;
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
        return (v_t) ((l % r) << 3 | TYPE_NUMBER);
    }

    v_t coerced = v_coerce(right, TYPE_NUMBER);
    if (V_IS_NUMBER(left)) {
        v_number_t l = (v_number_t) left >> 3;
        v_number_t r = (v_number_t) coerced >> 3;
        if (r == 0) panic("Division by zero");
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
        for (int i = 0; i < list->length; ++i) {
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
        for (int i = 0; i < list->length; ++i) {
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

        for (int i = 0; i < l->length; ++i) {
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

        if (idx < 0 || idx >= list->length || idx + len > list->length) {
            panic("Index %lld with range %lld out of bounds for list of length %d", idx, len, list->length);
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

        if (idx < 0 || idx >= str->length || idx + len > str->length) {
            panic("Index %lld with range %lld out of bounds for string of length %d", (int64_t) idx, (int64_t) len, str->length);
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

        if (idx < 0 || idx > list->length) {
            panic("Index %lld with range %lld out of bounds for list of length %d", idx, len, list->length);
        }

        v_list_t alt = (v_list_t) (v_create_list(list->length - len + replace_list->length) & VALUE_MASK);
        int pos = 0;

        for (int i = 0; i < idx; ++i) {
            alt->items[pos++] = list->items[i];
        }

        for (int i = 0; i < replace_list->length; ++i) {
            alt->items[pos++] = replace_list->items[i];
        }

        for (int i = idx + len; i < list->length; ++i) {
            alt->items[pos++] = list->items[i];
        }

        alt->length = pos;
        return (v_t) alt | TYPE_LIST;
    } else if (V_IS_STRING(value)) {
        v_string_t str = (v_string_t) (value & VALUE_MASK);
        v_string_t substr = (v_string_t) (v_coerce_to_string(replace) & VALUE_MASK);

        if (idx < 0 || idx >= str->length || idx + len > str->length) {
            panic("Index %lld with range %lld out of bounds for string of length %d", idx, len, str->length);
        }

        size_t new_length = str->length - len + substr->length;
        char* new_data = malloc(new_length + 1);
        if (!new_data) panic("Failed to allocate memory for modified string");

        memcpy(new_data, str->data, idx);
        memcpy(new_data + idx, substr->data, substr->length);
        memcpy(new_data + idx + substr->length, str->data + idx + len, str->length - idx - len);
        new_data[new_length] = '\0';

        v_t alt = v_create_string(new_data, new_length);
        free(new_data);
        return alt;
    }

    panic("Cannot set index %s with range %s on type %s", v_type(index), v_type(range), v_type(value));
}

static inline void vm_dump(v_t value) {
    if (V_IS_LIST(value)) {
        printf("[");
        v_list_t list = (v_list_t)(value & VALUE_MASK);
        for (int i = 0; i < list->length; ++i) {
            if (i > 0) printf(", ");
            vm_dump(list->items[i]);
        }
        printf("]\n");
    } else {
        v_t string = v_coerce_to_string(value);
        v_string_t str = (v_string_t) (string & VALUE_MASK);
        printf("%s", str->data);
    }
}

static inline v_t vm_phi(ir_block_t* previous, ir_id_t* phi_values, ir_block_t** phi_blocks, int phi_count, v_t* registers) {
    for (int i = 0; i < phi_count; ++i) {
        if (phi_blocks[i] == previous) {
            return registers[phi_values[i]];
        }
    }

    panic("No matching predecessor block found for PHI instruction");
}

vm_t* vm_run(ir_function_t* function, arena_t* arena) {
    vm_t* vm = vm_init(function, arena);

    vm_stack_t* stack = arena_alloc(arena, sizeof(vm_stack_t));
    stack->items = arena_alloc(arena, sizeof(vm_stack_item_t) * 16);
    stack->size = 0;
    stack->capacity = 16;

    register_pool_t reg_pool;
    register_pool_init(&reg_pool, function->next_value_id, arena);

    v_t* registers = vm->registers;
    v_t* variables = vm->variables;
    ir_block_t* block = vm->block;
    ir_block_t* previous = NULL;

    int index = 0;

    static load_reg_list_t load_regs;
    static int load_regs_initialized = 0;
    if (!load_regs_initialized) {
        load_reg_list_init(&load_regs);
        for (int b = 0; b < function->block_count; ++b) {
            ir_block_t* blk = function->blocks[b];
            for (int i = 0; i < blk->instruction_count; ++i) {
                ir_instruction_t* instr = &blk->instructions[i];
                if (instr->op == IR_LOAD) {
                    load_reg_list_add(&load_regs, instr->result);
                }
            }
        }
        load_regs_initialized = 1;
    }

    while (block->instruction_count > index) {
        ir_instruction_t* instruction = &block->instructions[index++];
        ir_id_t result = instruction->result;
        ir_op_t op = instruction->op;

        switch (op) {
            case IR_CONST_NUMBER:
            case IR_CONST_STRING:
            case IR_CONST_BOOLEAN:
            case IR_CONST_NULL:
            case IR_CONST_ARRAY:
                registers[result] = instruction->constant.value;
                break;
            case IR_LOAD:
                registers[result] = variables[instruction->var.var_id];
                break;
            case IR_STORE:
                variables[instruction->var.var_id] = registers[instruction->var.value];
                break;
            case IR_PROMPT:
                registers[result] = vm_prompt();
                break;
            case IR_RANDOM:
                registers[result] = ((v_number_t) (rand()) << 3) | TYPE_NUMBER;
                break;
            case IR_BLOCK:
                registers[result] = (v_t) instruction->block.function | TYPE_BLOCK;
                break;
            case IR_CALL:
                block = vm_call(block, previous, index, result, registers[instruction->generic.operands[0]], stack, arena, &registers, &reg_pool, &load_regs);
                index = 0;
                break;
            case IR_RETURN:
                vm_stack_item_t* item = vm_return(stack);
                previous = item->previous;
                block = item->callee;
                index = item->index;
                v_t* pool = registers;
                registers = item->registers;
                registers[item->result] = pool[instruction->generic.operands[0]];
                register_pool_release(&reg_pool, pool);
                break;
            case IR_NOT:
                registers[result] = v_coerce_to_boolean(registers[instruction->generic.operands[0]]) ^ (1 << 3);
                break;
            case IR_NEG:
                registers[result] = ((v_number_t) v_coerce_to_number(registers[instruction->generic.operands[0]])) * -1;
                break;
            case IR_LENGTH:
                registers[result] = vm_length(registers[instruction->generic.operands[0]]);
                break;
            case IR_ASCII:
                registers[result] = vm_ascii(registers[instruction->generic.operands[0]]);
                break;
            case IR_BOX:
                registers[result] = vm_box(registers[instruction->generic.operands[0]]);
                break;
            case IR_PRIME:
                registers[result] = vm_prime(registers[instruction->generic.operands[0]]);
                break;
            case IR_ULTIMATE:
                registers[result] = vm_ultimate(registers[instruction->generic.operands[0]]);
                break;
            case IR_ADD:
                registers[result] = vm_add(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_SUB:
                registers[result] = vm_sub(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_MUL:
                registers[result] = vm_mul(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_DIV:
                registers[result] = vm_div(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_MOD:
                registers[result] = vm_mod(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_POW:
                registers[result] = vm_pow(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_GT:
                registers[result] = vm_gt(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_LT:
                registers[result] = vm_lt(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_EQ:
                registers[result] = vm_eq(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_OUTPUT:
                v_t string = v_coerce_to_string(registers[instruction->generic.operands[0]]);
                v_string_t str = (v_string_t) (string & VALUE_MASK);
                if (str->length > 0 && str->data[str->length - 1] == '\\') {
                    printf("%.*s", (int)(str->length - 1), str->data);
                    break;
                }
                
                puts(str->data);
                break;
            case IR_DUMP:
                vm_dump(registers[instruction->generic.operands[0]]);
                break;
            case IR_GET:
                registers[result] = vm_get(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]], registers[instruction->generic.operands[2]]);
                break;
            case IR_SET:
                registers[result] = vm_set(registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]], registers[instruction->generic.operands[2]], registers[instruction->generic.operands[3]]);
                break;
            case IR_BRANCH:
                previous = block;
                v_t condition = v_coerce_to_boolean(registers[instruction->branch.condition]) >> 3;
                block = (ir_block_t*) (((v_number_t) instruction->branch.truthy) * condition + ((v_number_t) instruction->branch.falsey) * !condition);
                index = 0;
                break;
            case IR_JUMP:
                previous = block;
                block = instruction->jump.block;
                index = 0;
                break;
            case IR_QUIT:
                exit(v_coerce_to_number(registers[instruction->generic.operands[0]]) >> 3);
            case IR_PHI:
                registers[result] = vm_phi(previous, instruction->phi.phi_values, instruction->phi.phi_blocks, instruction->phi.phi_count, registers);
                break;
            default: panic("unimplemented");
        }
    }

    return vm;
}