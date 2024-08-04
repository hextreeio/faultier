#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "hardware/pio.h"
#include "hardware/pio_instructions.h"

#define POWER_CYCLER_SETUP_START 0
#define DELAY_SETUP_START 5
#define GLITCH_SETUP_START 7
#define CODE_START 9

struct ft_pio_program {
    uint16_t instructions[32];
    uint32_t delay_start_offset;
    uint32_t length;
    bool loaded;
    uint32_t loaded_offset;
};

void ft_pio_program_init(struct ft_pio_program *program);
void ft_pio_program_insert_inst(struct ft_pio_program *program, uint32_t index, uint32_t instruction);
void ft_pio_program_add_inst(struct ft_pio_program *program, uint32_t instruction);
pio_program_t ft_pio_get_program(struct ft_pio_program *program);
void ft_pio_add_program(struct ft_pio_program *program);
void ft_pio_remove_program(struct ft_pio_program *program);
