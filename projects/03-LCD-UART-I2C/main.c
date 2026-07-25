#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/i2c.h>
#include <stdbool.h>

#define I2C_ADDR ((uint8_t)0x27) // I2C address of the LCD 

#define USER_BTN (GPIO13)  // PC13
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

static void delay_ms(uint32_t ms)
{
    uint32_t start = system_millis;
    while ((system_millis - start) < ms) {
        __asm__("nop");
    }
}

static void lcd_write_nibble(uint8_t nibble_positioned)
{
    uint8_t data_t[2];
    data_t[0] = nibble_positioned | 0x0C;   // EN=1, RS=0, backlight on
    data_t[1] = nibble_positioned | 0x08;   // EN=0, RS=0, backlight on
    i2c_transfer7(I2C1, I2C_ADDR, data_t, 2, NULL, 0);
}

void lcd_send_command(char cmd)
{
    char data_u = (cmd & 0xF0);             //Upper Nibble
    char data_l = ((cmd << 4)& 0xF0);      //Lower Nibble

    uint8_t data_t[4];
    data_t[0] = data_u | 0x0C;    //EN=1, RS=0
    data_t[1] = data_u | 0x08;    //EN=0, RS=0
    data_t[2] = data_l | 0x0C;    //EN=1, RS=0
    data_t[3] = data_l | 0x08;    //EN=0, RS=0

    i2c_transfer7(I2C1,I2C_ADDR,(uint8_t*)data_t,4,NULL,0);
}

void lcd_send_data(char data)
{
    char data_u = (data & 0xF0);             //Upper Nibble
    char data_l = ((data << 4)& 0xF0);      //Lower Nibble

    uint8_t data_t[4];
    data_t[0] = data_u | 0x0D;    //EN=1, RS=1
    data_t[1] = data_u | 0x09;    //EN=0, RS=1
    data_t[2] = data_l | 0x0D;    //EN=1, RS=1
    data_t[3] = data_l | 0x09;    //EN=0, RS=1

    i2c_transfer7(I2C1,I2C_ADDR,(uint8_t*)data_t,4,NULL,0);
}

void lcd_init(void)
{
    delay_ms(50);         

    lcd_write_nibble(0x30);
    delay_ms(5);             

    lcd_write_nibble(0x30); 
    delay_ms(1);             

    lcd_write_nibble(0x30); 
    delay_ms(1);

    lcd_write_nibble(0x20); 
    delay_ms(1);

    lcd_send_command(0x28); // Function set: 4-bit, 2 lines, 5x8 dots
    delay_ms(1);
    lcd_send_command(0x08); // Clear display
    delay_ms(1);
    lcd_send_command(0x01); 
    delay_ms(2);             
    lcd_send_command(0x06); 
    delay_ms(1);
    lcd_send_command(0x0C); // Display on, cursor off, blink off
    delay_ms(1);
}

void lcd_send_string(char *str)
{
    while (*str) {
        lcd_send_data(*str++);
    }
}

void lcd_put_cur(int row, int col)
{
    switch(row)
    {
        case 0:
            col |= 0x80;
            break;
        case 1:
            col |= 0xC0;
            break;
    }
    lcd_send_command(col);
}

static void i2c_setup(void)
{
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, I2C1_SDA_PIN | I2C1_SCL_PIN);
    gpio_set_output_options(GPIOB,GPIO_OTYPE_OD,GPIO_OSPEED_50MHZ,I2C1_SDA_PIN | I2C1_SCL_PIN);
    gpio_set_af(GPIOB, GPIO_AF4, I2C1_SDA_PIN | I2C1_SCL_PIN);

    uint32_t i2c = I2C1;

    i2c_peripheral_disable(i2c);
    i2c_set_speed(i2c, i2c_speed_fm_400k, rcc_apb1_frequency/1000000); 
    i2c_peripheral_enable(i2c);
}

static void systick_setup(void)
{
    // systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    // systick_set_reload(rcc_ahb_frequency / 1000 - 1); // 1ms tick

    systick_set_frequency(SYSTICK_FREQ,CPU_FREQ);

    systick_counter_enable();
    systick_interrupt_enable();
}

void sys_tick_handler(void) 
{
    system_millis++;
}

void rcc_setup(void)
{
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    // rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

     rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void usart2_setup(void) 
{
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

static void gpio_setup(void) 
{
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_NONE, USER_BTN);
}

void usart2_isr(void)
{
    if(usart_get_flag(USART2,USART_SR_RXNE))  // check SR: has a byte arrived?
    {
        uint16_t byte = usart_recv(USART2); // Read the received byte
        // usart_recv(USART2);   //Consume the bye (clears RXNE)
        lcd_send_data((char)byte); // Send the byte to the LCD via I2C
        gpio_set(GPIOA, GPIO5);    // Turn on the LED on PA5
        led_on_since = system_millis;  // Record the time the LED was turned on
        led_is_on = true;   
    }
}

int main(void) 
{
    rcc_setup();
    systick_setup();
    gpio_setup();
    usart2_setup();
    i2c_setup();
    lcd_init();
    // lcd_send_command(0x01);   // Clear Display
    // delay_ms(1000);               
    // lcd_put_cur(0, 0);         // reset cursor to top-left after clearing

    nvic_enable_irq(NVIC_USART2_IRQ);
    nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    // nvic_enable_irq(NVIC_I2C1_EV_IRQ);
    // i2c_enable_interrupt(I2C1, I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    usart_enable_rx_interrupt(USART2);

    while (1) {
        if (led_is_on && (system_millis - led_on_since >= LED_ON_DURATION_MS)) {
            gpio_clear(GPIOA, GPIO5); // Turn off the LED after the duration
            led_is_on = false;
        }
        if (gpio_get(GPIOC, USER_BTN) == 0) { // Check if the user button is pressed (active low)
            lcd_send_command(0x01);   // Clear Display
            delay_ms(1000);         
            lcd_put_cur(0, 0);         // reset cursor to top-left after clearing
        }
    }
}