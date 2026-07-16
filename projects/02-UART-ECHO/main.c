#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/exti.h>

#define UART_BAUDRATE 9600
#define UART_PORT GPIOA
#define UART_TX_PIN GPIO2
#define UART_RX_PIN GPIO3
#define GPIO_LED_PIN GPIO5

static void usart_setup(void) {
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
    // Enable the USART transmitter and receiver
    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_parity(USART2, USART_PARITY_NONE);

    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    // Enable the USART
    usart_enable(USART2);               //Sets the CR1's UE bit to 1, enabling the USART peripheral. This allows the USART to start transmitting and receiving data.

}

static void led_setup(void) {
    // Enable the GPIOA clock
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure GPIO pin for led (e.g., PA5)
    gpio_mode_setup(UART_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_LED_PIN);
}

static void delay(volatile uint32_t count) {
    for(volatile uint32_t i = 0; i < count; i++) 
    {
        __asm__("nop");  //Do nothing
    }
}

static void usart2_isr(void){
    if(usart_get_flag(USART2, USART_SR_RXNE)) 
    {
        uint16_t byte = usart_recv(USART2); // Read the received byte
        usart_send_blocking(USART2, byte); // Echo the received byte back
        gpio_toggle(GPIOA, GPIO5);
    }

}

int main(void) {
    led_setup();
    usart_setup();
    
    gpio_toggle(GPIOA, GPIO5);
    // Enable the USART2 interrupt in the NVIC
    nvic_enable_irq(NVIC_USART2_IRQ);
    // Enable the USART2 receive interrupt
    usart_enable_rx_interrupt(USART2);
    
    while (1) {
        // Do nothing, wait for interrupts
    }
}