#include "power_cycler.h"

void power_cycler(struct ft_pio_program *program) {
    // PULL
    uint32_t inst = pio_encode_pull(0, 1);
    program->instructions[POWER_CYCLER_SETUP_START] = inst;
    // MOV X, OSR
    inst = pio_encode_mov(pio_x, pio_osr);
    program->instructions[POWER_CYCLER_SETUP_START+1] = inst;


    // MOV PINS, !NULL
    inst = pio_encode_mov_not(pio_pins, pio_null);
    program->instructions[POWER_CYCLER_SETUP_START+2] = inst;

    // JMP X--
    inst = pio_encode_jmp_x_dec(POWER_CYCLER_SETUP_START+3);
    program->instructions[POWER_CYCLER_SETUP_START+3] = inst;

    // MOV PINS, NULL
    inst = pio_encode_mov(pio_pins, pio_null);
    program->instructions[POWER_CYCLER_SETUP_START+4] = inst;
}