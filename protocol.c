#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include <stdio.h>
#include <stdint.h>

#include "proto/faultier.pb.h"

void writeout(uint8_t *buf, size_t len);

uint8_t buffer[128];

bool callback(pb_ostream_t * stream, const pb_byte_t * buf, size_t count)
{
    // printf("Protowrite: %d bytes %02X %02X\n", count, buf[0] & 0xFF, buf[1] & 0xFF);
    writeout((uint8_t*)buf, count);
    return 1;
//    FILE *file = (FILE*) stream->state;
//    return fwrite(buf, 1, count, file) == count;
}

char adc_buffer[4096];

struct encode_adc_data {
    uint8_t * adc_buffer;
    size_t adc_buffer_size;
};

static void write_header(size_t size) {
    writeout("FLTR", 4);
    writeout((uint8_t*)&size, 4);
}

bool encode_adc(pb_ostream_t *stream, const pb_field_iter_t *field, void * const *arg) {
    const struct encode_adc_data *d = *((const struct encode_adc_data **) arg);

    // printf("Encode ADC! Len: %d\n", d->adc_buffer_size);
    if (!pb_encode_tag_for_field(stream, field)) {
        // printf("Couldn't encode tag whatever the fuck that means.\n");
        return false;
    }

    return pb_encode_string(stream, d->adc_buffer, d->adc_buffer_size);
}

size_t protocol_response_adc_(pb_ostream_t *stream, uint8_t * adc_buffer, size_t adc_buffer_size) {
    ResponseADC response = ResponseADC_init_zero;
    response.samples.funcs.encode = &encode_adc;
    struct encode_adc_data d = {
        .adc_buffer = adc_buffer,
        .adc_buffer_size = adc_buffer_size
    };
    response.samples.arg = &d;

    Response resp = Response_init_zero;
    resp.which_type = Response_adc_tag;
    resp.type.adc = response;

    pb_encode(stream, Response_fields, &resp);
    return stream->bytes_written;
}

void protocol_response_adc(uint8_t * adc_buffer, size_t adc_buffer_size) {
    pb_ostream_t sizestream = {0};
    write_header(protocol_response_adc_(&sizestream, adc_buffer, adc_buffer_size));
    pb_ostream_t stdoutstream = {&callback, NULL, SIZE_MAX, 0};
    protocol_response_adc_(&stdoutstream, adc_buffer, adc_buffer_size);
}

size_t protocol_response_io_get_state_(pb_ostream_t *stream, IOState state) {
    ResponseIOGetState response = ResponseIOGetState_init_zero;
    response.state = state;
    Response resp = Response_init_zero;
    resp.which_type = Response_io_get_state_tag;
    resp.type.io_get_state = response;
    pb_encode(stream, Response_fields, &resp);
    return stream->bytes_written;

}
void protocol_response_io_get_state(bool state) {
    pb_ostream_t sizestream = {0};
    write_header(protocol_response_io_get_state_(&sizestream, state));
    pb_ostream_t stdoutstream = {&callback, NULL, SIZE_MAX, 0};
    protocol_response_io_get_state_(&stdoutstream, state);
}

size_t protocol_error_(pb_ostream_t *stream, char *message) {
    ResponseError err = ResponseError_init_zero;
    strncpy(err.message, message, 50);
    Response resp = Response_init_zero;
    resp.which_type = Response_error_tag;
    resp.type.error = err;
    pb_encode(stream, Response_fields, &resp);
    return stream->bytes_written;
}

void protocol_error(char *message) {
    pb_ostream_t sizestream = {0};
    write_header(protocol_error_(&sizestream, message));
    pb_ostream_t stdoutstream = {&callback, NULL, SIZE_MAX, 0};
    protocol_error_(&stdoutstream, message);
}

size_t protocol_ok_(pb_ostream_t *stream) {
    ResponseOk ok = ResponseOk_init_zero;
    Response resp = Response_init_zero;
    resp.which_type = Response_ok_tag;
    resp.type.ok = ok;
    
    pb_encode(stream, Response_fields, &resp);
    return stream->bytes_written;
}

void protocol_ok() {
    pb_ostream_t sizestream = {0};
    write_header(protocol_ok_(&sizestream));
    pb_ostream_t stdoutstream = {&callback, NULL, SIZE_MAX, 0};
    protocol_ok_(&stdoutstream);
}

size_t protocol_hello_(pb_ostream_t *stream) {
    ResponseHello hello = ResponseHello_init_zero;
    hello.version = FaultierVersion_FAULTIER_VERSION;
    Response response = Response_init_zero;
    response.which_type = Response_hello_tag;
    response.type.hello = hello;
    pb_encode(stream, Response_fields, &response);
    return stream->bytes_written;
}

void protocol_hello() {
    pb_ostream_t sizestream = {0};
    write_header(protocol_hello_(&sizestream));
    pb_ostream_t stdoutstream = {&callback, NULL, SIZE_MAX, 0};
    protocol_hello_(&stdoutstream);
}

size_t protocol_trigger_timeout_(pb_ostream_t *stream) {
    ResponseTriggerTimeout trigger_timeout = ResponseTriggerTimeout_init_zero;
    Response resp = Response_init_zero;
    resp.which_type = Response_trigger_timeout_tag;
    resp.type.trigger_timeout = trigger_timeout;
    
    pb_encode(stream, Response_fields, &resp);
    return stream->bytes_written;
}

void protocol_trigger_timeout() {
    pb_ostream_t sizestream = {0};
    write_header(protocol_trigger_timeout_(&sizestream));
    pb_ostream_t stdoutstream = {&callback, NULL, SIZE_MAX, 0};
    protocol_trigger_timeout_(&stdoutstream);
}

size_t protocol_swd_check_(pb_ostream_t *stream, bool success) {
    ResponseSWDCheck check = ResponseSWDCheck_init_zero;
    check.enabled = success;
    Response resp = Response_init_zero;
    resp.which_type = Response_swd_check_tag;
    resp.type.swd_check = check;
    pb_encode(stream, Response_fields, &resp);
    return stream->bytes_written;
}

void protocol_swd_check(bool success) {
pb_ostream_t sizestream = {0};
    write_header(protocol_swd_check_(&sizestream, success));
    pb_ostream_t stdoutstream = {&callback, NULL, SIZE_MAX, 0};
    protocol_swd_check_(&stdoutstream, success);
}
