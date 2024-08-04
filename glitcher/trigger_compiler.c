#include "trigger_compiler.h"

#include "hardware/pio.h"
#include "hardware/pio_instructions.h"

void trigger_high(struct ft_pio_program *program) {
    uint inst = pio_encode_wait_pin(1, 0);
    ft_pio_program_add_inst(program, inst);
}

void trigger_low(struct ft_pio_program *program) {
    uint inst = pio_encode_wait_pin(0, 0);
    ft_pio_program_add_inst(program, inst);
}

void trigger_rising(struct ft_pio_program *program) {
    uint inst = pio_encode_wait_pin(0, 0);
    ft_pio_program_add_inst(program, inst);
    inst = pio_encode_wait_pin(1, 0);
    ft_pio_program_add_inst(program, inst);
}

void trigger_falling(struct ft_pio_program *program) {
    uint inst = pio_encode_wait_pin(1, 0);
    ft_pio_program_add_inst(program, inst);
    inst = pio_encode_wait_pin(0, 0);
    ft_pio_program_add_inst(program, inst);
}

void trigger_pulse_positive(struct ft_pio_program *program) {
    uint inst = pio_encode_wait_pin(0, 0);
    ft_pio_program_add_inst(program, inst);
    inst = pio_encode_wait_pin(1, 0);
    ft_pio_program_add_inst(program, inst);
    inst = pio_encode_wait_pin(0, 0);
    ft_pio_program_add_inst(program, inst);
}

void trigger_pulse_negative(struct ft_pio_program *program) {
    uint inst = pio_encode_wait_pin(1, 0);
    ft_pio_program_add_inst(program, inst);
    inst = pio_encode_wait_pin(0, 0);
    ft_pio_program_add_inst(program, inst);
    inst = pio_encode_wait_pin(1, 0);
    ft_pio_program_add_inst(program, inst);
}
