#include "glitch_compiler.h"

void glitcher_simple(struct ft_pio_program *program) {
    // Insert PULL at instruction at glitch setup time
    // PULL
    uint32_t inst = pio_encode_pull(0, 1);
    program->instructions[GLITCH_SETUP_START] = inst;
    // MOV Y, OSR
    inst = pio_encode_mov(pio_y, pio_osr);
    program->instructions[GLITCH_SETUP_START+1] = inst;

    // .. trigger & delay inserted here ..

    // SET PINS, 1
    inst = pio_encode_set(pio_pins, 1);
    ft_pio_program_add_inst(program, inst);

    // JMP Y--
    inst = pio_encode_jmp_y_dec(program->length);
    ft_pio_program_add_inst(program, inst);

    // SET PINS, 0
    inst = pio_encode_set(pio_pins, 0);
    ft_pio_program_add_inst(program, inst);
}
