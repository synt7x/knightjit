#include "vm.h"
#include "math.h"

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

void vm_constants(ir_function_t* function, v_t* registers) {
    for (int b = 0; b < function->block_count; b++) {
        ir_block_t* block = function->blocks[b];

        for (int i = 0; i < block->instruction_count; i++) {
            ir_instruction_t* instruction = &block->instructions[i];
            ir_id_t result = instruction->result;
            ir_op_t op = instruction->op;

            switch (instruction->op) {
                case IR_CONST_NUMBER:
                case IR_CONST_STRING:
                case IR_CONST_BOOLEAN:
                case IR_CONST_NULL:
                case IR_CONST_ARRAY:
                    registers[instruction->result] = instruction->constant.value;
                    break;
                case IR_OUTPUT:
                    registers[instruction->result] = TYPE_NULL;
                    break;
                default:
                    break;
            }
        }
    }
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

static inline ir_block_t* vm_call(
    ir_block_t* origin, ir_block_t* previous, int index, ir_id_t result, v_t block,
    vm_stack_t* stack, arena_t* arena, v_t** registers_ptr, vm_t* vm
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
    if (stack->size < stack->capacity && stack->items) {
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
    vm->registers = malloc(sizeof(v_t) * vm->function->next_value_id);
    vm_constants(vm->function, vm->registers);

    return target;
}

static inline vm_stack_item_t* vm_return(vm_stack_t* stack) {
    if (stack->size == 0) {
        panic("Attempted to return from a non-BLOCK");
    }

    return &stack->items[--stack->size];
}

static inline void vm_dump(v_t value) {
    if (V_IS_LIST(value)) {
        v_list_t list = (v_list_t)(value & VALUE_MASK);

        printf("[");
        for (size_t i = 0; i < list->length; ++i) {
            if (i > 0) printf(",");
            if (V_IS_STRING(list->items[i])) {
                printf("'");
            }

            vm_dump(list->items[i]);

            if (V_IS_STRING(list->items[i])) {
                printf("'");
            }
        }
        printf("]");
    } else if (V_IS_NULL(value)) {
        printf("null");
    } else if (V_IS_BLOCK(value)) {
        printf("BLOCK (0x%llx)", (unsigned long long)(value & VALUE_MASK));
    } else {
        v_t string = v_coerce_to_string(value);
        v_string_t str = (v_string_t) (string & VALUE_MASK);
        
        if (V_IS_NULL(value)) {
            printf("null");
        } else {
            printf("%s", str->data);
        }
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

    v_t* registers = vm->registers;
    v_t* variables = vm->variables;
    ir_block_t* block = vm->block;
    ir_block_t* previous = NULL;

    int index = 0;

    vm_constants(function, registers);

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
                block = vm_call(block, previous, index, result, registers[instruction->generic.operands[0]], stack, arena, &registers, vm);
                registers = vm->registers;
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
                free(pool);
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
            case IR_SAVE:
            case IR_RESTORE:
                break;
            default: panic("unimplemented");
        }
    }

    return vm;
}