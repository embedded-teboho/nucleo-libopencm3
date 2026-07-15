#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/timer.h>
#include <stdint.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/usart.h>

#define UART_BAUDRATE 9600
#define UART_PORT GPIOA
#define UART_TX_PIN GPIO2
#define UART_RX_PIN GPIO3

void usart_setup(void) {
    // Enable the USART2 clock
    rcc_periph_clock_enable(RCC_USART2);    // Enable clock for USART2
    rcc_periph_clock_enable(RCC_GPIOA);  // Enable clock for GPIOA

    // Configure GPIO pins for USART2 TX and RX
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, UART_TX_PIN | UART_RX_PIN);
    gpio_set_af(UART_PORT, GPIO_AF7, UART_TX_PIN | UART_RX_PIN);

    // Set the baud rate
    usart_set_baudrate(USART2, UART_BAUDRATE);

    // Set the data bits, stop bits, and parity
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_parity(USART2, USART_PARITY_NONE);

    // Enable the USART transmitter and receiver
    usart_set_mode(USART2, USART_MODE_TX_RX);

    // Enable the USART
    usart_enable(USART2);               //Sets the CR1's UE bit to 1, enabling the USART peripheral. This allows the USART to start transmitting and receiving data.
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
}

int main(void) {
    usart_setup();
    while (1) {
        // TODO: UART echo code
    }
}