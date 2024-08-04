#include "delay_compiler.h"

void delay_regular(struct ft_pio_program *program) {
    // Equivalent PIO code:
    // PULL
    // MOV X, OSR
    // ... trigger ...
    // delay_target:
    //     JMP X-- delay_target

    // Insert PULL at instruction 0
    uint32_t inst = pio_encode_pull(0, 1);
    program->instructions[DELAY_SETUP_START] = inst;
    // Insert MOV X, OSR at instruction 1
    inst = pio_encode_mov(pio_x, pio_osr);
    program->instructions[DELAY_SETUP_START+1] = inst;

    // Append JMP X--
    // TODO: XXXX bad workaround with the +2 
    inst = pio_encode_jmp_x_dec(program->length);
    ft_pio_program_add_inst(program, inst);

    program->delay_start_offset = 2;
}
