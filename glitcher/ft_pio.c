#include "ft_pio.h"

void ft_pio_program_init(struct ft_pio_program *program) {
    for(int i=0; i < 32; i++) {
        program->instructions[i] = pio_encode_nop();
    }
    program->length = CODE_START;
}

void ft_pio_program_insert_inst(struct ft_pio_program *program, uint32_t index, uint32_t instruction) {
    // TODO: This function does not do any error checking if
    //       the program is already 32 instructions long.

    for(unsigned i = 31; i > index; i--) {
        program->instructions[i] = program->instructions[i - 1];
    }
    program->instructions[index] = instruction;
    program->length++;
}

void ft_pio_program_add_inst(struct ft_pio_program *program, uint32_t instruction) {
    program->instructions[program->length++] = instruction;
}

pio_program_t ft_pio_get_program(struct ft_pio_program *program) {
    pio_program_t p;
    p.instructions = &program->instructions[0];
    p.length = program->length;
    p.origin = -1;
    return p;
}

void ft_pio_add_program(struct ft_pio_program *program) {
    pio_program_t p  = ft_pio_get_program(program);
    if(!pio_can_add_program(pio0, &p)) {
        return;
    }
    uint loaded = pio_add_program(pio0, &p);
    program->loaded = 1;
    program->loaded_offset = loaded;

    // for(int i = 0; i < program->length; i++) {
    //     printf("%02X%02X", (program->instructions[i] >> 8) & 0xFF, program->instructions[i] & 0xFf);
    // }
    // printf("\n");
}

void ft_pio_remove_program(struct ft_pio_program *program) {
    if(!program->loaded) {
        return;
    }
    pio_program_t p = ft_pio_get_program(program);
    pio_remove_program(pio0, &p, program->loaded_offset);
}


// void ft_pio_run_sm() {
//     pio_sm_config c = pio_get_default_sm_config();
//     sm_config_set_set_pins(&c, pin_mosi, 1);
// }