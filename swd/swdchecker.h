#include <stdint.h>

void init_swd();
bool read_swd(char command, uint32_t *value);
bool write_swd_int(char command, uint32_t data_int);
