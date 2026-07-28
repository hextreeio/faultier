/*
 * Licensed under GNU Public License v3
 * Copyright (c) 2022-2024 Thomas "stacksmashing" Roth <code@stacksmashing.net>
 * Based on Picoprobe by:
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 */

#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "faultier/faultier.h"

#define PROBE_SM 0
#define PROBE_PIN_SWDIO SWD_IO
#define PROBE_PIN_SWCLK SWD_CLK
#define PROBE_PIN_SWRST SWD_RST
#include <hardware/clocks.h>
#include <hardware/gpio.h>

// #include "tamarin_hw.h"
#include "probe.pio.h"
#include "tusb.h"
#include "swd/tamarin_probe.h"
// #include "util.h"


#if (TUSB_VERSION_MAJOR == 0) && (TUSB_VERSION_MINOR <= 12)
#define tud_vendor_n_flush(x, y) ((void)0)
#endif

#define TAMARIN_RESET_IMPLEMENTED 1


void vuprintf(const char* format, va_list args) {
    char buf[128];
    vsnprintf(buf, 128, format, args);
    tud_cdc_n_write_str(0, buf);
    tud_cdc_n_write_flush(0);
    tud_task();
}

void uprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vuprintf(format, args);
    va_end(args);
}


#if false
#define tamarin_debug(format,args...) uprintf(format, ## args)
#define tamarin_info(format,args...) uprintf(format, ## args)
#define serprint(format, args...) uprintf(format, ## args)
#else
#define tamarin_debug(format,...) ((void)0)
#define tamarin_info(format,...) ((void)0)
// Disable all prints for now
#define serprint(...)
#endif

// #define serprint(format, args...) uprintf(format, ## args)

// Number of SWCLK cycles the bus is left undriven when ownership changes between
// the host and the target. This is the ADIv5 reset/default turnaround period.
#define SWD_TURNAROUND_CYCLES 1

// Idle cycles driven low before every packet request. A few idle cycles keep the
// line quiet between transfers; anything beyond this is requested per command by
// the host through tamarin_cmd_hdr.idle_cycles.
#define SWD_LEADING_IDLE_CYCLES 8

// How often a transfer is retried locally when the target answers WAIT. Retrying on
// the probe avoids a USB round trip per retry; if the target is still not ready
// afterwards the host is told so it can abort the transfer.
#define SWD_MAX_WAIT_RETRIES 8

// This struct is the direct struct that is sent to
// the probe.
struct __attribute__((__packed__)) tamarin_cmd_hdr
{
    // Currently unused
    uint8_t id;
    // One of TAMARIN_CMDS
    uint8_t cmd;
    // The full (incl. start/stop/parity) SWD command
    // Unused for LINE_RESET and SET_FREQ.
    uint8_t request;
    // The data for writes (unused otherwise)
    uint32_t data;
    // Number of (8 bit) idle cycles to perform after this op
    uint8_t idle_cycles;
};

// This is the struture returned by the probe for each command
struct __attribute__((__packed__)) tamarin_res_hdr
{
    // Echo of tamarin_cmd_hdr.id, so the host can detect a lost or duplicated
    // response instead of silently pairing data with the wrong command.
    uint8_t id;
    // One of TAMARIN_STATUS
    uint8_t res;
    // The data for successful reads, 0 otherwise
    uint32_t data;
};

#define PROBE_BUF_SIZE 8192
struct _probe
{
    // Data received from computer
    struct tamarin_cmd_hdr probe_cmd;

    // Data that will be sent back to the computer
    struct tamarin_res_hdr probe_res;

    // PIO offset
    uint offset;
};

static struct _probe probe;

bool probe_set_swclk_freq(uint freq_khz)
{
    // The PIO program uses two cycles per SWD clock, so the usable range is
    // clk_sys/2 down to clk_sys/(2*65535).
    uint clk_sys_freq_khz = clock_get_hz(clk_sys) / 1000;
    if (freq_khz == 0 || freq_khz > clk_sys_freq_khz)
    {
        tamarin_info("Rejecting SWD frequency of %d kHz\r\n", freq_khz);
        return false;
    }

    tamarin_info("Setting SWD frequency to %d kHz\r\n", freq_khz);
    // Worked out with saleae
    uint32_t divider = clk_sys_freq_khz / freq_khz / 2;
    if (divider < 1)
    {
        divider = 1;
    }
    if (divider > 65535)
    {
        divider = 65535;
    }
    pio_sm_set_clkdiv_int_frac(pio1, PROBE_SM, divider, 0);
    return true;
}

inline void probe_write_bits(uint bit_count, uint32_t data_byte)
{
    // The PIO loop counter is bit_count - 1, so a zero length burst would clock out
    // 2^32 bits and never return.
    if (bit_count == 0 || bit_count > 32)
    {
        return;
    }

    serprint(">> %X (%d)\r\n", data_byte, bit_count);
    pio_sm_put_blocking(pio1, PROBE_SM, bit_count - 1);
    pio_sm_put_blocking(pio1, PROBE_SM, data_byte);
    pio_sm_get_blocking(pio1, PROBE_SM);
}

uint32_t tamarin_probe_read_bits(uint bit_count)
{
    if (bit_count == 0 || bit_count > 32)
    {
        return 0;
    }

    pio_sm_put_blocking(pio1, PROBE_SM, bit_count - 1);
    uint32_t data = pio_sm_get_blocking(pio1, PROBE_SM);
    data = data >> (32 - bit_count);
    serprint("<< %X (%d)\r\n", data, bit_count);
    return data;
}

void tamarin_probe_read_mode(void)
{
    // gpio_put(PROBE_PIN_SWDIO_DIR, SHIFTER_DIRECTION_IN);
    pio_sm_exec(pio1, PROBE_SM, pio_encode_jmp(probe.offset + probe_offset_in_posedge));
    while (pio1->dbg_padoe & (1 << PROBE_PIN_SWDIO))
        ;
}

void tamarin_probe_write_mode(void)
{
    // gpio_put(PROBE_PIN_SWDIO_DIR, SHIFTER_DIRECTION_OUT);
    pio_sm_exec(pio1, PROBE_SM, pio_encode_jmp(probe.offset + probe_offset_out_negedge));
    while (!(pio1->dbg_padoe & (1 << PROBE_PIN_SWDIO)))
        ;
}

void tamarin_start_probe()
{
    // set to output
    // gpio_put(PROBE_PIN_SWCLK_DIR, SHIFTER_DIRECTION_OUT);
    // gpio_put(PROBE_PIN_SWDIO_DIR, SHIFTER_DIRECTION_OUT);
    pio_sm_set_consecutive_pindirs(pio1, PROBE_SM, PROBE_PIN_SWCLK, 1, true);
    pio_sm_set_consecutive_pindirs(pio1, PROBE_SM, PROBE_PIN_SWDIO, 1, true);
    // Enable SM
    pio_sm_set_enabled(pio1, PROBE_SM, 1);

    // Jump to write program
    // probe_write_mode();
    tamarin_probe_read_mode();
    // probe_enabled = true;
}

void tamarin_probe_init()
{
    // Funcsel pins
    pio_gpio_init(pio1, PROBE_PIN_SWCLK);
    pio_gpio_init(pio1, PROBE_PIN_SWDIO);

    #if TAMARIN_RESET_IMPLEMENTED
    gpio_init(PROBE_PIN_SWRST);
    gpio_put(PROBE_PIN_SWRST, 1);
    gpio_set_dir(PROBE_PIN_SWRST, true);
    #endif

    // Make sure SWDIO has a pullup on it. Idle state is high
    gpio_pull_up(PROBE_PIN_SWDIO);

    // // Target reset pin: pull up, input to emulate open drain pin
    // gpio_pull_up(PROBE_PIN_RESET);
    // // gpio_init will leave the pin cleared and set as input
    // gpio_init(PROBE_PIN_RESET);

    uint offset = pio_add_program(pio1, &probe_program);
    probe.offset = offset;

    pio_sm_config sm_config = probe_program_get_default_config(offset);

    // Set SWCLK as a sideset pin
    sm_config_set_sideset_pins(&sm_config, PROBE_PIN_SWCLK);

    // Set SWDIO offset
    sm_config_set_out_pins(&sm_config, PROBE_PIN_SWDIO, 1);
    sm_config_set_set_pins(&sm_config, PROBE_PIN_SWDIO, 1);
    sm_config_set_in_pins(&sm_config, PROBE_PIN_SWDIO);

    // Set pins to input to ensure we don't pull the line low unnecessarily
    // gpio_put(PROBE_PIN_SWCLK_DIR, SHIFTER_DIRECTION_IN);
    // gpio_put(PROBE_PIN_SWDIO_DIR, SHIFTER_DIRECTION_IN);
    pio_sm_set_consecutive_pindirs(pio1, PROBE_SM, PROBE_PIN_SWCLK, 1, false);
    pio_sm_set_consecutive_pindirs(pio1, PROBE_SM, PROBE_PIN_SWDIO, 1, false);

    // shift output right, autopull off, autopull threshold
    sm_config_set_out_shift(&sm_config, true, false, 0);
    // shift input right as swd data is lsb first, autopush off
    sm_config_set_in_shift(&sm_config, true, false, 0);

    // Init SM with config
    pio_sm_init(pio1, PROBE_SM, offset, &sm_config);

    // Set up divisor
    probe_set_swclk_freq(1000);

    tamarin_start_probe();
}

void tamarin_probe_deinit()
{
    pio_sm_set_enabled(pio1, PROBE_SM, false);
    pio_clear_instruction_memory(pio1);
    gpio_disable_pulls(PROBE_PIN_SWDIO);
    #if TAMARIN_RESET_IMPLEMENTED
    gpio_set_dir(PROBE_PIN_SWRST, false);
    #endif
    
}

int __not_in_flash_func(tamarin_tx_read_bare)(uint8_t request, uint32_t *value, uint8_t idle_cycles);

// Drive `cycles` idle clocks with SWDIO low. Idle cycles after a transfer give the
// DP time to complete it, which is what keeps a target from answering WAIT to
// everything that follows.
static void __not_in_flash_func(swd_idle_cycles)(uint cycles)
{
    if (cycles == 0)
    {
        return;
    }

    tamarin_probe_write_mode();
    while (cycles > 0)
    {
        uint chunk = (cycles > 32) ? 32 : cycles;
        probe_write_bits(chunk, 0);
        cycles -= chunk;
    }
}

// Send a packet request and sample the acknowledge.
//
// Neither side drives the bus during the turnaround period, so SWDIO is switched to
// an input before the turnaround clock rather than parking it high. A target that
// does not respond at all leaves the line pulled up and shows up as an ACK of 0b111.
static uint32_t __not_in_flash_func(swd_packet_request)(uint8_t request)
{
    tamarin_probe_write_mode();
    swd_idle_cycles(SWD_LEADING_IDLE_CYCLES);
    probe_write_bits(8, request);

    tamarin_probe_read_mode();
    tamarin_probe_read_bits(SWD_TURNAROUND_CYCLES);
    return tamarin_probe_read_bits(3);
}

int tamarin_line_reset(uint32_t *idcode)
{
    tamarin_probe_write_mode();
    // At least 50 clocks with SWDIO high, the JTAG-to-SWD select sequence, then
    // another line reset so the DP ends up in a known state either way.
    probe_write_bits(32, 0xFFFFFFFF);
    probe_write_bits(32, 0xFFFFFFFF);
    probe_write_bits(16, 0xe79e);
    probe_write_bits(32, 0xFFFFFFFF);
    probe_write_bits(32, 0xFFFFFFFF);

    // A line reset must be followed by a read of DP IDCODE before any other
    // transfer, otherwise the DP ignores everything that comes next.
    uint32_t value = 0;
    int status = tamarin_tx_read_bare(0xa5, &value, SWD_LEADING_IDLE_CYCLES);
    if (idcode != NULL)
    {
        *idcode = value;
    }
    tamarin_probe_read_mode();
    return status;
}

void tamarin_reset()
{
    #if TAMARIN_RESET_IMPLEMENTED
    tamarin_debug("Resetting\r\n");
    gpio_put(PROBE_PIN_SWRST, 0);
    sleep_ms(100);
    gpio_put(PROBE_PIN_SWRST, 1);
    #else
    tamarin_debug("Reset not implemented.\r\n");
    #endif
}

// Read an AP or DP register.
//
// On the wire: request (8) | turnaround | ACK (3) | data (32) | parity (1) | turnaround.
// A WAIT or FAULT acknowledge aborts the transfer straight after the ACK: there is no
// data phase unless overrun detection is enabled in DP CTRL/STAT, which the host does
// not do. Clocking a data phase anyway - as this used to - leaves the probe and the
// target disagreeing about where the next packet request starts.
int __not_in_flash_func(tamarin_tx_read_bare)(uint8_t request, uint32_t *value, uint8_t idle_cycles)
{
    *value = 0;

    for (unsigned attempt = 0; attempt < SWD_MAX_WAIT_RETRIES; attempt++)
    {
        uint32_t ack = swd_packet_request(request);

        if (ack == TAMARIN_STATUS_OK)
        {
            uint32_t read_data = tamarin_probe_read_bits(32);
            uint32_t read_parity = tamarin_probe_read_bits(1);
            tamarin_probe_read_bits(SWD_TURNAROUND_CYCLES);
            swd_idle_cycles(idle_cycles);

            *value = read_data;
            serprint("READ - OK Data: %08X (%d)\r\n", read_data, read_parity);
            if (read_parity != (uint32_t)__builtin_parity(read_data))
            {
                serprint("READ - parity error\r\n");
                return TAMARIN_STATUS_PARITY_ERROR;
            }
            return TAMARIN_STATUS_OK;
        }

        // No data phase follows a WAIT or FAULT, only the turnaround back to us.
        tamarin_probe_read_bits(SWD_TURNAROUND_CYCLES);
        swd_idle_cycles(idle_cycles);

        if (ack != TAMARIN_STATUS_WAIT)
        {
            serprint("READ - ack %d\r\n", ack);
            return (int)ack;
        }
        serprint("READ - WAIT, retry %d\r\n", attempt);
    }

    return TAMARIN_STATUS_WAIT;
}

// Write an AP or DP register.
//
// On the wire: request (8) | turnaround | ACK (3) | turnaround | data (32) | parity (1).
// As for reads, a WAIT or FAULT acknowledge means the data phase is skipped entirely.
int __not_in_flash_func(tamarin_tx_write_bare)(uint8_t request, uint32_t value, uint8_t idle_cycles)
{
    uint32_t value_parity = __builtin_parity(value);

    for (unsigned attempt = 0; attempt < SWD_MAX_WAIT_RETRIES; attempt++)
    {
        uint32_t ack = swd_packet_request(request);

        // The target releases the bus after the ACK whether or not a data phase follows.
        tamarin_probe_read_bits(SWD_TURNAROUND_CYCLES);

        if (ack == TAMARIN_STATUS_OK)
        {
            tamarin_probe_write_mode();
            probe_write_bits(32, value);
            probe_write_bits(1, value_parity);
            swd_idle_cycles(idle_cycles);

            serprint("WRITE - OK Data: %08X\r\n", value);
            return TAMARIN_STATUS_OK;
        }

        swd_idle_cycles(idle_cycles);

        if (ack != TAMARIN_STATUS_WAIT)
        {
            serprint("WRITE - ack %d\r\n", ack);
            return (int)ack;
        }
        serprint("WRITE - WAIT, retry %d\r\n", attempt);
    }

    return TAMARIN_STATUS_WAIT;
}

bool probe_enabled = false;
void probe_handle_pkt(void)
{
    struct tamarin_cmd_hdr *cmd = &probe.probe_cmd;

    tamarin_debug("Processing packet: ID: %u Command: %u Request: 0x%02X Data: 0x%08X Idle: %d\r\n", cmd->id, cmd->cmd, cmd->request, cmd->data, cmd->idle_cycles);
    int result = TAMARIN_STATUS_OK;
    uint32_t data = 0;
    switch (cmd->cmd)
    {
    case TAMARIN_READ:
        tamarin_debug("Executing read\r\n");

        result = tamarin_tx_read_bare(cmd->request, &data, cmd->idle_cycles);
        tamarin_debug("Read: %d 0x%08X\r\n", result, data);
        break;
    case TAMARIN_WRITE:
        tamarin_debug("Executing write\r\n");
        result = tamarin_tx_write_bare(cmd->request, cmd->data, cmd->idle_cycles);
        tamarin_debug("Write: %d 0x%08X\r\n", result, cmd->data);
        break;
    case TAMARIN_LINE_RESET:
        tamarin_debug("Executing line reset\r\n");
        // The status is that of the DP IDCODE read the reset ends with, and the
        // IDCODE itself is handed back in the data field.
        result = tamarin_line_reset(&data);
        tamarin_debug("Line reset: %d IDCODE 0x%08X\r\n", result, data);
        break;
    case TAMARIN_SET_FREQ:
        tamarin_debug("Executing set frequency\r\n");
        if (!probe_set_swclk_freq(cmd->data))
        {
            result = TAMARIN_STATUS_BAD_ARGUMENT;
        }
        break;
    case TAMARIN_RESET:
        tamarin_debug("Executing RESET\r\n");
        tamarin_reset();
        break;
    default:
        tamarin_debug("UNKNOWN COMMAND!\r\n");
        result = TAMARIN_STATUS_UNKNOWN_COMMAND;
        break;
    }

    struct tamarin_res_hdr res;
    res.id = cmd->id;
    res.data = data;
    res.res = (uint8_t)result;

    tud_vendor_n_write(1, (char *)&res, sizeof(res));
    tud_vendor_n_flush(1);
}

// USB bits
void tamarin_probe_task(void)
{
    // Commands are fixed size, so consume them one at a time and leave a partially
    // received command in the FIFO until the rest of it arrives. Reading a whole USB
    // packet and only looking at the first command would drop the others on the floor
    // and leave the host waiting for responses that never come.
    while (tud_vendor_n_available(1) >= sizeof(struct tamarin_cmd_hdr))
    {
        uint8_t tmp_buf[sizeof(struct tamarin_cmd_hdr)];
        uint32_t count = tud_vendor_n_read(1, tmp_buf, sizeof(tmp_buf));
        if (count != sizeof(struct tamarin_cmd_hdr))
        {
            return;
        }

        memcpy(&probe.probe_cmd, tmp_buf, sizeof(struct tamarin_cmd_hdr));
        probe_handle_pkt();
    }
}

void tamarin_probe_handle_write(uint8_t *data, uint total_bits) {
    tamarin_info("Write %d bits\n", total_bits);

    // led_signal_activity(total_bits);

    tamarin_probe_write_mode();

    uint chunk;
    uint bits = total_bits;
    while (bits > 0) {
        if (bits > 8) {
            chunk = 8;
        } else {
            chunk = bits;
        }

        probe_write_bits(chunk, *data++);
        bits -= chunk;
    }
}
