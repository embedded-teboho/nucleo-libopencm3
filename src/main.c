#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/timer.h>
#include <stdint.h>
#include <libopencm3/stm32/dac.h>

#define I2C_ADDR 0x27

// Define the ports
#define LED_PORTA (GPIOA)
#define LED_PORTB (GPIOB)
#define LED_PORTC (GPIOC)

// Pins used by each LED
#define PC8_PIN  GPIO8
#define PC6_PIN  GPIO6
#define PC5_PIN  GPIO5
#define PC4_PIN  GPIO4
#define PC10_PIN GPIO10

#define PB15_PIN GPIO15
#define PB14_PIN GPIO14
#define PB13_PIN GPIO13
#define PB9_PIN GPIO9               //I2C1_SDA
#define PB8_PIN GPIO8               //I2C1_SCL

#define PA12_PIN GPIO12
#define PA11_PIN GPIO11

#define LCD_BACKLIGHT 0x08
#define LCD_EN        0x04
#define LCD_RW        0x02
#define LCD_RS        0x01

static void lcd_send_byte(uint8_t data, uint8_t rs);
static void lcd_write(uint8_t data);
static void lcd_command(uint8_t cmd);
static void lcd_init(void);
static void i2c_write(uint8_t data);

static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 2000; i++) {
        __asm__("nop");
    }
}

static void i2c_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);

    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, PB8_PIN | PB9_PIN);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, PB8_PIN | PB9_PIN);
    gpio_set_af(GPIOB, GPIO_AF4, PB8_PIN | PB9_PIN);    // AF4 = I2C1

    i2c_peripheral_disable(I2C1);
    i2c_set_speed(I2C1, i2c_speed_sm_100k, rcc_apb1_frequency / 1000000);
    
    i2c_peripheral_enable(I2C1);

}

static void i2c_write(uint8_t data) {

    while (I2C_SR2(I2C1) & I2C_SR2_BUSY);

    i2c_send_start(I2C1);

    while (!(I2C_SR1(I2C1) & I2C_SR1_SB));

    i2c_send_7bit_address(I2C1, I2C_ADDR, I2C_WRITE);

    while (!(I2C_SR1(I2C1) & I2C_SR1_ADDR));

    (void)I2C_SR2(I2C1);

    i2c_send_data(I2C1, data);

    while (!(I2C_SR1(I2C1) & (I2C_SR1_BTF)));

    i2c_send_stop(I2C1);
}

static void lcd_write(uint8_t data) {
    i2c_write(data | LCD_BACKLIGHT); 
}

static void lcd_command(uint8_t cmd) {
    lcd_send_byte(cmd, 0);
}

static void lcd_data(uint8_t data) {
    lcd_send_byte(data, 1);
}

static void lcd_send_byte(uint8_t data, uint8_t rs) {
    uint8_t high = (data & 0xF0);
    uint8_t low  = (data << 4) & 0xF0;

    uint8_t rs_bit = rs ? LCD_RS : 0;

    lcd_write(high | rs_bit);
    lcd_write(high | rs_bit | LCD_EN);
    delay_ms(1);
    lcd_write(high | rs_bit);

    lcd_write(low | rs_bit);
    lcd_write(low | rs_bit | LCD_EN);
    delay_ms(1);
    lcd_write(low | rs_bit);

    delay_ms(2);
}

static void lcd_init(void) {
    delay_ms(50);  

    lcd_write(0x30 | LCD_BACKLIGHT);
    lcd_write(0x30 | LCD_BACKLIGHT | LCD_EN);
    delay_ms(5);
    lcd_write(0x30 | LCD_BACKLIGHT);
    
    delay_ms(5);
    
    lcd_write(0x30 | LCD_BACKLIGHT);
    lcd_write(0x30 | LCD_BACKLIGHT | LCD_EN);
    delay_ms(1);
    lcd_write(0x30 | LCD_BACKLIGHT);
    
    delay_ms(5);
    
    lcd_write(0x20 | LCD_BACKLIGHT);
    lcd_write(0x20 | LCD_BACKLIGHT | LCD_EN);
    delay_ms(1);
    lcd_write(0x20 | LCD_BACKLIGHT);
    
    delay_ms(5);

    lcd_command(0x28);
    delay_ms(5);
    
    lcd_command(0x0C); 
    delay_ms(5);
    
    lcd_command(0x06);
    delay_ms(5);
    
    lcd_command(0x01);
    delay_ms(5);
}


static void rcc_setup(void){
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);  //Enable clock for GPIOA
    rcc_periph_clock_enable(RCC_GPIOB);  //Enable clock for GPIOB
    rcc_periph_clock_enable(RCC_GPIOC);   //Enable clock for GPIOC

    gpio_mode_setup(LED_PORTC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PC8_PIN);
    gpio_mode_setup(LED_PORTC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PC6_PIN);
    gpio_mode_setup(LED_PORTC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PC5_PIN);
    gpio_mode_setup(LED_PORTC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PC4_PIN);
    gpio_mode_setup(LED_PORTC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PC10_PIN);

    gpio_mode_setup(LED_PORTB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PB15_PIN);
    gpio_mode_setup(LED_PORTB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PB14_PIN);
    gpio_mode_setup(LED_PORTB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PB13_PIN);

    gpio_mode_setup(LED_PORTA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PA12_PIN);
    gpio_mode_setup(LED_PORTA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PA11_PIN);

}


int main(void){
    rcc_setup();
    i2c_setup();
    gpio_setup();

    delay_ms(100);
    
    lcd_init();
    
    lcd_command(0x01);
    delay_ms(5);
    lcd_command(0x02);
    delay_ms(5);

    const char *msg = "Hello, STM32!";
    for (int i = 0; msg[i] != '\0'; i++) {
        lcd_data(msg[i]);
    }

    while(1){
        gpio_toggle(LED_PORTC, PC8_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTC, PC6_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTC, PC5_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTC, PC4_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTB, PB15_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTB, PB14_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTB, PB13_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTC, PC10_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTA, PA12_PIN);
         delay_ms(500);

        gpio_toggle(LED_PORTA, PA11_PIN);
         delay_ms(500);
    }

    return 0;
}
