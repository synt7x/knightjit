#include "vm.h"

vm_t* vm_init(ir_function_t* function, arena_t* arena) {
    vm_t* vm = arena_alloc(arena, sizeof(vm_t));
    if (!vm) panic("Failed to allocate memory for VM");

    vm->block = function->block;
    vm->function = function;
    vm->variables = arena_alloc(arena, sizeof(v_t*) * (function->var_id + 1));
    vm->registers = arena_alloc(arena, sizeof(v_t*) * function->next_value_id);

    if (!vm->variables || !vm->registers) {
        panic("Failed to allocate memory for VM");
    }

    vm->index = 0;

    return vm;
}

static inline v_t vm_add(vm_t* vm, v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        v_number_t l = (v_number_t)(left >> 3);
        v_number_t r = (v_number_t)(right >> 3);
        return (v_t) ((uintptr_t)(l + r) << 3 | TYPE_NUMBER);
    }

    v_t coerced = v_coerce(right, V_TYPE(left));
    if (V_IS_NUMBER(left)) {
       v_number_t l = (v_number_t)(left >> 3);
       v_number_t r = (v_number_t)(coerced >> 3);
       return (v_t) ((uintptr_t)(l + r) << 3 | TYPE_NUMBER);
    } else if (V_IS_STRING(left)) {
        v_string_t str_left = string_from_v(left);
        v_string_t str_right = string_from_v(coerced);
        size_t new_length = str_left->length + str_right->length;
        char* new_data = malloc(new_length + 1);
        if (!new_data) panic("Failed to allocate memory for string concatenation");
        memcpy(new_data, str_left->data, str_left->length);
        memcpy(new_data + str_left->length, str_right->data, str_right->length);
        new_data[new_length] = '\0';
        v_t result = v_create_string(new_data, new_length);
        free(new_data);
        return result;
    } else if (V_IS_LIST(left)) {
        v_list_t list_left = (v_list_t)(left & VALUE_MASK);
        v_list_t list_right = (v_list_t)(coerced & VALUE_MASK);
        v_list_t new_list = malloc(sizeof(v_list_box_t) + sizeof(v_t) * (list_left->length + list_right->length));
        if (!new_list) panic("Failed to allocate memory for list concatenation");
        new_list->ref_count = 1;
        new_list->length = list_left->length + list_right->length;
        new_list->capacity = list_left->length + list_right->length;
        memcpy(new_list->items, list_left->items, sizeof(v_t) * list_left->length);
        memcpy(new_list->items + list_left->length, list_right->items, sizeof(v_t) * list_right->length);
        return (v_t)new_list | TYPE_LIST;
    } else {
        panic("unimplemented");
    }
}

static inline v_t vm_gt(vm_t* vm, v_t left, v_t right) {
    if (V_IS_NUMBER(left) && V_IS_NUMBER(right)) {
        v_number_t l = (v_number_t)(left >> 3);
        v_number_t r = (v_number_t)(right >> 3);
        return (v_t) ((uintptr_t)(l > r) << 3 | TYPE_BOOLEAN);
    }

    v_t coerced = V_TYPE(left) == V_TYPE(right) ? right : v_coerce(right, V_TYPE(left));
    if (V_IS_NUMBER(left)) {
        v_number_t l = (v_number_t)(left >> 3);
        v_number_t r = (v_number_t)(coerced >> 3);
        return v_create(TYPE_BOOLEAN, (void*)(intptr_t)(l > r));
    } else if (V_IS_STRING(left)) {
        v_string_t str_left = string_from_v(left);
        v_string_t str_right = string_from_v(coerced);
        return v_create(TYPE_BOOLEAN, (void*)(intptr_t)(strcmp(str_left->data, str_right->data) > 0));
    } else {
        panic("unimplemented");
    }
}

vm_t* vm_run(ir_function_t* function, arena_t* arena) {
    vm_t* vm = vm_init(function, arena);
    v_t* registers = vm->registers;
    v_t* variables = vm->variables;

    while (vm->block->instruction_count > vm->index) {
        ir_instruction_t* instruction = &vm->block->instructions[vm->index++];
        ir_id_t result = instruction->result;

        switch (instruction->op) {
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
                vm->variables[instruction->var.var_id] = registers[instruction->var.value];
                break;
            case IR_ADD:
                registers[result] = vm_add(vm, registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_GT:
                registers[result] = vm_gt(vm, registers[instruction->generic.operands[0]], registers[instruction->generic.operands[1]]);
                break;
            case IR_OUTPUT:
                v_t string = v_coerce_to_string(registers[instruction->generic.operands[0]]);
                v_string_t str = string_from_v(string);
                puts(str->data);
                break;
            case IR_BRANCH:
                if (registers[instruction->branch.condition] >> 3) {
                    vm->block = instruction->branch.truthy;
                } else {
                    vm->block = instruction->branch.falsey;
                }
                vm->index = 0;
                break;
            case IR_JUMP:
                vm->block = instruction->jump.block;
                vm->index = 0;
                break;
            default: panic("unimplemented");
        }
    }
}