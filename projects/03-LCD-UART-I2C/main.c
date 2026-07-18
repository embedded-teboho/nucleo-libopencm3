#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/i2c.h>
#include <stdbool.h>

#define I2C_ADDR ((uint8_t)0x27) // I2C address of the LCD 

#define LCD_BACKLIGHT (0x08)
#define LCD_EN        (0x04)
#define LCD_RW        (0x02)
#define LCD_RS        (0x01)

#define UART_TX_PIN (GPIO2)
#define UART_RX_PIN (GPIO3)
#define I2C1_SDA_PIN (GPIO9)              //I2C1_SDA
#define I2C1_SCL_PIN (GPIO8)              //I2C1_SCL

static volatile uint32_t system_millis = 0;
static volatile uint32_t led_on_since = 0;
static volatile bool led_is_on = false;

#define CPU_FREQ (84000000)
#define SYSTICK_FREQ (1000)
#define LED_ON_DURATION_MS (200)   // how long the LED stays lit per byte received

static void i2c_setup(void){
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, I2C1_SDA_PIN | I2C1_SCL_PIN);
    gpio_set_output_options(GPIOB,GPIO_OTYPE_OD,GPIO_OSPEED_50MHZ,I2C1_SDA_PIN | I2C1_SCL_PIN);
    gpio_set_af(GPIOB, GPIO_AF4, I2C1_SDA_PIN | I2C1_SCL_PIN);

    uint32_t i2c = I2C1;

    i2c_peripheral_disable(i2c);
    i2c_set_speed(i2c, i2c_speed_fm_400k, rcc_apb1_frequency/1000000); 
    i2c_peripheral_enable(i2c);
}


static void systick_setup(void){
    // systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    // systick_set_reload(rcc_ahb_frequency / 1000 - 1); // 1ms tick

    systick_set_frequency(SYSTICK_FREQ,CPU_FREQ);

    systick_counter_enable();
    systick_interrupt_enable();
}

void sys_tick_handler(void) {
    system_millis++;
}

void rcc_setup(void){
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    // rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

     rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void usart2_setup(void) {
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, UART_RX_PIN);
    gpio_set_af(GPIOA, GPIO_AF7, UART_RX_PIN);

    usart_set_baudrate(USART2, 9600);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_mode(USART2, USART_MODE_RX);

    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_enable(USART2);

}

static void gpio_setup(void) {
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
}

void usart2_isr(void){
    if(usart_get_flag(USART2,USART_SR_RXNE))  // check SR: has a byte arrived?
    {
        uint16_t byte = usart_recv(USART2); // Read the received byte
        // usart_recv(USART2);   //Consume the bye (clears RXNE)
        i2c_transfer7(I2C1,I2C_ADDR,(uint8_t*)&byte,1,NULL,0); // Send the byte to the LCD via I2C
        gpio_set(GPIOA, GPIO5);    // Turn on the LED on PA5
        led_on_since = system_millis;  // Record the time the LED was turned on
        led_is_on = true;   
    }
}

int main(void) {
    rcc_setup();
    systick_setup();
    gpio_setup();
    usart2_setup();
    i2c_setup();

    nvic_enable_irq(NVIC_USART2_IRQ);
    // nvic_enable_irq(NVIC_I2C1_EV_IRQ);
    // i2c_enable_interrupt(I2C1, I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    usart_enable_rx_interrupt(USART2);

    while (1) {
        if (led_is_on && (system_millis - led_on_since >= LED_ON_DURATION_MS)) {
            gpio_clear(GPIOA, GPIO5); // Turn off the LED after the duration
            led_is_on = false;
        }
    }
}