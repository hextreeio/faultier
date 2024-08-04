#include "pdnd_display.h"

#include <stdlib.h>
#include <stdio.h>

#include "display/font_renderer.h"
#include "display/ssd1306.h"
#include "bootup2.h"

#define PIN_SDA 18
#define PIN_SCL 19
#define I2C_INSTANCE i2c1

pdnd_display *pdnd_global_display = NULL;

void pdnd_display_initialize() {
    pdnd_global_display = malloc(sizeof(pdnd_display));
    if(pdnd_global_display == NULL) {
        return;
    }

    pdnd_display_create(pdnd_global_display);
}

void pdnd_display_create(pdnd_display *display) {
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    i2c_init(I2C_INSTANCE, 1000000);

    ssd1306_context ctx;
    ctx.i2c = I2C_INSTANCE;

    ssd1306_begin(&ctx, SSD1306_SWITCHCAPVCC);
    ssd1306_clear_display(&ctx);
    ssd1306_display(&ctx);

    display->ctx = ctx;
}

void pdnd_display_screen(pdnd_display *d, pdnd_screen *s) {
    if(d == NULL) {
        return;
    }
    if(s == NULL) {
        return;
    }
    ssd1306_clear_display(&d->ctx);
    void ssd1306_draw_sceen(ssd1306_context *ctx, int16_t x, int16_t y, pdnd_screen *screen);
    ssd1306_draw_bitmap(&d->ctx, 0, 0, s->data, s->width, s->height, 1);
    ssd1306_display(&d->ctx);
}

void pdnd_display_printf(pdnd_display *d, uint8_t x, uint8_t y, const char *format, va_list args) {
    if(d == NULL) {
        return;
    }
    char print_buffer[128];
    vsnprintf(print_buffer, 128, format, args);
    font_render_string(&d->ctx, x, y, &DEFAULT_FONT, print_buffer);
}

// Global helper functions
void cls(bool display) {
    ssd1306_clear_display(&pdnd_global_display->ctx);
    if(display) {
        ssd1306_display(&pdnd_global_display->ctx);
    }
}

void ft_cls(bool display) {
    pdnd_screen *s = &bootup2;
    ssd1306_clear_display(&pdnd_global_display->ctx);
    ssd1306_draw_bitmap(&pdnd_global_display->ctx, 0, 0, s->data, s->width, s->height, 1);


    // pdnd_display_screen(pdnd_global_display, &bootup2);
    // ssd1306_clear_display(&pdnd_global_display->ctx);
    if(display) {
        ssd1306_display(&pdnd_global_display->ctx);
    }
}

void pprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    pdnd_display_printf(pdnd_global_display, 0, DEFAULT_FONT_Y, format, args);
    va_end(args);
    
    ssd1306_display(&pdnd_global_display->ctx);
}

void pprintfxy(uint8_t x, uint8_t y, const char *format, ...) {
    va_list args;
    va_start(args, format);
    pdnd_display_printf(pdnd_global_display, x, y, format, args);
    va_end(args);
    ssd1306_display(&pdnd_global_display->ctx);
}
