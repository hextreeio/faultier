#pragma once


#define DISPLAY_SDA 18
#define DISPLAY_SCL 19


#define PIN_GATE 0
#define PIN_MUX0 1
#define PIN_MUX1 2
#define PIN_MUX2 3
#define PIN_EXT0 4
#define PIN_EXT1 27

#define PIN_MUX_MEASURE 26
#define PIN_CB_MEASURE 28

#define PIN_LED0 22
#define PIN_LED1 21
#define PIN_LED2 20

#define PIN_IO_BASE 5

// ADC channels
#define ADC_MUX_PIN 26
#define ADC_EXT_PIN 27
#define ADC_CB_PIN 28
#define ADC_MUX 0
#define ADC_EXT 1
#define ADC_CB 2


#define SWD_CLK 15
#define SWD_IO 16
#define SWD_RST 17

#define PIO_IRQ_TRIGGERED 0
#define PIO_IRQ_GLITCHED 1

struct faultier_configuration {
    void (*trigger_generator)(void*);
};

void faultier_init();
