#include "faultier.h"

#include "pico/stdlib.h"

void faultier_init() {
    gpio_init(PIN_LED0);
    gpio_init(PIN_LED1);
    gpio_init(PIN_LED2);
    gpio_init(PIN_MUX0);
    gpio_init(PIN_MUX1);
    gpio_init(PIN_MUX2);
    gpio_init(PIN_GATE);
    
    gpio_set_dir(PIN_LED0, 1);
    gpio_set_dir(PIN_LED1, 1);
    gpio_set_dir(PIN_LED2, 1);
    gpio_set_dir(PIN_MUX0, 1);
    gpio_set_dir(PIN_MUX1, 1);
    gpio_set_dir(PIN_MUX2, 1);
    gpio_set_dir(PIN_GATE, 1);
    
    gpio_put(PIN_LED0, 0);
    gpio_put(PIN_LED1, 0);
    gpio_put(PIN_LED2, 0);
    gpio_put(PIN_MUX0, 0);
    gpio_put(PIN_MUX1, 0);
    gpio_put(PIN_MUX2, 0);
    gpio_put(PIN_GATE, 0);
}