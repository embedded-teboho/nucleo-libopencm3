#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>

static void delay(volatile uint32_t n) {
    while (n--) __asm__("nop");
}

int main(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);

    while (1) {
        gpio_toggle(GPIOA, GPIO5);
        delay(1000000);
    }
}