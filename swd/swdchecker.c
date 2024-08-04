#include "tamarin_probe.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

void init_swd(void) {
    // probe_write_mode();
    // probe_handle_write("\xff\xff\xff\xff", 32);
    // probe_handle_write("\xff\xff\xff\xff", 32);
    // probe_handle_write("\xff\xff\xff\xff", 32);
    // probe_handle_write("\x9e\xe7", 16);

    // probe_handle_write("\xff\xff\xff\xff", 32);
    // probe_handle_write("\xff\xff\xff\xff", 32);
    // probe_handle_write("\x00", 8);
       tamarin_probe_write_mode();
       tamarin_probe_handle_write("\xff\xff\xff\xff", 32);
       tamarin_probe_handle_write("\xff\xff\xff\xff", 32);
       tamarin_probe_handle_write("\xff\xff\xff\xff", 32);
       tamarin_probe_handle_write("\x9e\xe7", 16);
       tamarin_probe_handle_write("\xff\xff\xff\xff", 32);
       tamarin_probe_handle_write("\xff\xff\xff\xff", 32);
       tamarin_probe_handle_write("\x00", 8);
}


int count_set_bits(unsigned char byte) {
	byte = byte - ((byte >> 1) & 0x55);
	byte = (byte & 0x33) + ((byte >> 2) & 0x33);
	return ((byte + (byte >> 4)) & 0x0F);
}

uint8_t calculate_parity_bit(uint8_t *data) {
	int set_bits = count_set_bits(data[0]);
	set_bits += count_set_bits(data[1]);
	set_bits += count_set_bits(data[2]);
	set_bits += count_set_bits(data[3]);
	if((set_bits % 2) == 0) {
		return 0x0;
	} else {
		return 0x1;
	}
}
bool write_swd(char command, uint8_t *data);
bool write_swd_int(char command, uint32_t data_int) {
	// can be replaced by just &data...
	char data[4];
	data[0] = data_int & 0xFF;
	data[1] = (data_int >> 8) & 0xFF;
	data[2] = (data_int >> 16) & 0xFF;
	data[3] = (data_int >> 24) & 0xFF;
	
	return write_swd(command, data);
}
bool write_swd(char command, uint8_t *data) {
	uint8_t parity_byte = calculate_parity_bit(data);
		// new shit
		// Request debugport write select
		tamarin_probe_handle_write(&command, 8);
		tamarin_probe_read_mode();
		// turn (yes, slow, for debugging)
		tamarin_probe_read_bits(1);

		uint8_t res = tamarin_probe_read_bits(3);
		// printf("RES: %d\n", res);
		if(res == 1) {
			// putchar('A');
		} else {
			// putchar('WRITE B1 B');
			// printf("Write SWD failed, command: %d", command);
			return 0;
		}

		// // turn again
		tamarin_probe_read_bits(1);

		
		tamarin_probe_write_mode();
		tamarin_probe_handle_write(data, 32);
		// probe_handle_write("\x00", 8);
		// trailing bits
		tamarin_probe_handle_write(&parity_byte, 8);
		return 1;
}

bool read_swd(char command, uint32_t *value) {
		tamarin_probe_handle_write(&command, 8);
		tamarin_probe_read_mode();
		// turn (yes, slow, for debugging)
		tamarin_probe_read_bits(1);

		uint32_t res = tamarin_probe_read_bits(3);
		if(res == 1) {
			// all good
		} else {
			// printf("Read error for command: %d %d\n", command & 0xff, res);
			return 0;
		}
		// printf("RES: %d\n", res);
		// if(res == 1) {
		//     putchar('A');
		// } else {
		//     putchar('B');
		// }


		uint32_t id = 0;
		res = tamarin_probe_read_bits(8);
		id |= res;
		// printf("RES1: %d\n", res);
		res = tamarin_probe_read_bits(8);
		id |= res << 8;
		// printf("RES2: %d\n", res);
		res = tamarin_probe_read_bits(8);
		id |= res << 16;
		// printf("RES3: %d\n", res);
		res = tamarin_probe_read_bits(8);
		id |= res << 24;
		// printf("RES4: %d\n", res);
		// printf("ID: %08X", id);
		tamarin_probe_read_bits(1);
		tamarin_probe_write_mode();
		// trailing bits
		tamarin_probe_handle_write("\x00", 8);
		*value = id;
		return 1;
}
