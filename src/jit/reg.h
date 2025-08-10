#ifndef REG_H
#define REG_H

typedef struct {
    int reg;
    int stack_slot;
    int start, end;
} reg_info_t;

#endif