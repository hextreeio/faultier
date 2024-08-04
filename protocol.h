#include <stdlib.h>

size_t protocol_response_adc(uint8_t * adc_buffer, size_t adc_buffer_size);
void protocol_error(char *message);
void protocol_ok();
void protocol_hello();
void protocol_trigger_timeout();
void protocol_swd_check(bool success);
