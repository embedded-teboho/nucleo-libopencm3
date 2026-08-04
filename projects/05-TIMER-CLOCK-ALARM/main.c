#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/rtc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <stdbool.h>

#define GOAL_FREQUENCY      (2)                                                    
#define TIMER_CLOCK         (rcc_apb1_frequency * 2)                  //F_timer   (CK_PSC)
#define COUNTER_CLOCK       (1000000)                               //F_counter (CK_CNT)
#define TIMER_PRESCALER     (TIMER_CLOCK / COUNTER_CLOCK - 1)     //PSC
#define TIMER_PERIOD        (COUNTER_CLOCK / GOAL_FREQUENCY - 1)     //ARR

#define GPIO_LED_PIN        (GPIO5)
#define GPIO_LED_PORT       (GPIOA)
#define GPIO_BTN_PORT       (GPIOB)
#define UART_TX_PIN         (GPIO2)
#define UART_RX_PIN         (GPIO3)
#define START_BTN           (GPIO5)
#define USER_BTN            (GPIO13)  // Button on PC13
#define EXTI_BUTTON_SOURCE  (EXTI13) // EXTI line for the user button
#define EXTI_ALARM_SOURCE   (EXTI17)  // EXTI line for the alarm button

static volatile bool button_pressed = false;
static volatile bool stopwatch_running = false;

static void rtc_set_alarm_time(uint8_t *hours, uint8_t *minutes)
{
    uint8_t hours_bcd = _rtc_dec_to_bcd(*hours);
    uint8_t minutes_bcd = _rtc_dec_to_bcd(*minutes);

    pwr_disable_backup_domain_write_protect();
    rtc_unlock(); // Disable write protection for RTC registers

    RTC_CR &= ~RTC_CR_ALRAE; // Disable Alarm A
    RTC_CR &= ~RTC_CR_ALRBE; // Disable Alarm B

    RTC_ALRMAR = (1 << 31) | // MSK4=1, ignore date
                 ((uint8_t)hours_bcd << 16) | // Set hours
                 ((uint8_t)minutes_bcd << 8)|   // Set minutes
                 (1 << 7); //MSK1=1, ignore seconds

    while ((RTC_ISR & RTC_ISR_ALRAWF) == 0); // Poll until Alarm A write flag is set

    RTC_CR |= RTC_CR_ALRAE; // Enable Alarm A
    RTC_CR |= RTC_CR_ALRAIE; // Enable Alarm A interrupt

    rtc_lock(); // Enable write protection for RTC registers
    pwr_enable_backup_domain_write_protect();
}

static void rtc_set_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds){

    uint8_t hours_bcd = _rtc_dec_to_bcd(*hours);
    uint8_t minutes_bcd = _rtc_dec_to_bcd(*minutes);
    uint8_t seconds_bcd = _rtc_dec_to_bcd(*seconds);

    pwr_disable_backup_domain_write_protect();
    rcc_osc_on(RCC_LSE);
    rcc_wait_for_osc_ready(RCC_LSE);

    //Select LSE as RTC clock source
    RCC_BDCR |= (1 << 8);                         //bit 8 = 1
    RCC_BDCR &= ~(1 << 9);                        //bit 9 = 0. Together with bit 8, this selects LSE as RTC clock source
    RCC_BDCR |= (1 << 15);                        //Enable RTC clock

    rtc_unlock();                                   //Disable write protection for RTC registers
    // rtc_wait_for_synchro();                      //Wait for RTC registers to synchronize with APB
    rtc_set_init_flag();                            //Set initialization flag to enter initialization mode
    while((RTC_ISR & (1 << 6)) != (1 << 6));            //Wait for initialization mode to be entered
    rtc_set_prescaler(255, 127);                    //Set RTC prescaler to get 1Hz clock

    RTC_TR = (hours_bcd << 16) | (minutes_bcd << 8) | seconds_bcd; //Set time in RTC_TR register

    rtc_clear_init_flag();                          //Clear initialization flag to exit initialization mode
    while((RTC_ISR & (1 << 6)) == (1 << 6));            //Wait for initialization mode to be exited
    rtc_lock();                                     //Enable write protection for RTC registers
    pwr_enable_backup_domain_write_protect();       //Disable backup domain write protection
}

static void rcc_setup(void){
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_TIM2);           //Enable clock for TIM2
    rcc_periph_reset_pulse(RST_TIM2);            //Reset TIM2 to default values
    rcc_periph_clock_enable(RCC_USART2);         //Enable clock for USART2        
    rcc_periph_clock_enable(RCC_PWR);            //Enable clock for PWR
    rcc_periph_clock_enable(RCC_SYSCFG); /* For EXTI. */
}

static void usart2_setup(void) {
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_MODE_RX | USART_MODE_TX);
    gpio_set_af(GPIOA, GPIO_AF7, USART_MODE_RX | USART_MODE_TX);

    usart_set_baudrate(USART2, 9600);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_mode(USART2, USART_MODE_TX_RX);

    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_enable(USART2);
}

void rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds){

    uint32_t tr = RTC_TR; // Read the RTC_TR register

    *hours = ((tr >> 20) & 0x3) * 10 + ((tr >> 16) & 0xF);          //Extract hours from RTC_TR register
    *minutes = ((tr >> 12) & 0x7) * 10 + ((tr >> 8) & 0xF);         //Extract minutes from RTC_TR register
    *seconds = ((tr >> 4) & 0x7) * 10 + ((tr >> 0) & 0xF);          //Extract seconds from RTC_TR register
}

void rtc_get_alarm_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {

    uint32_t alrmar = RTC_ALRMAR; // Read the RTC_ALRMAR register

    *hours = ((alrmar >> 20) & 0x3) * 10 + ((alrmar >> 16) & 0xF);          //Extract hours from RTC_ALRMAR register
    *minutes = ((alrmar >> 12) & 0x7) * 10 + ((alrmar >> 8) & 0xF);         //Extract minutes from RTC_ALRMAR register
    *seconds = ((alrmar >> 4) & 0x7) * 10 + ((alrmar >> 0) & 0xF);          //Extract seconds from RTC_ALRMAR register
}

void decimal_to_digits(uint8_t decimal, char *digits) {
    digits[0] = (decimal / 10) + '0'; // Tens place
    digits[1] = (decimal % 10) + '0'; // Units place
}

static void gpio_setup(void)
{
    gpio_mode_setup(GPIO_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_LED_PIN);
    gpio_mode_setup(GPIO_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
    gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_NONE, USER_BTN);

    gpio_set_output_options(GPIO_LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_LED_PIN);
    gpio_set_output_options(GPIO_LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO6);
}

static void timer_setup(void){
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_disable_preload(TIM2);
    timer_continuous_mode(TIM2);
    
    timer_set_prescaler(TIM2, TIMER_PRESCALER);     //setup TIMx_PSC register
    // timer_set_period(TIM2, TIMER_PERIOD);           //setup TIMx_ARR register
    timer_set_period(TIM2, 0xFFFFFFFF);  
    
}

void exti15_10_isr(void)
{
    if(exti_get_flag_status(EXTI_BUTTON_SOURCE)) {
        exti_reset_request(EXTI_BUTTON_SOURCE); // Clear the EXTI flag
        button_pressed = true; 
    }
}

void tim2_isr(void){
    if(timer_get_flag(TIM2, TIM_SR_UIF)){
        timer_clear_flag(TIM2, TIM_SR_UIF);
        gpio_toggle(GPIO_LED_PORT, GPIO6);
    }
}

void rtc_alarm_isr(void)
{
    if (RTC_ISR & RTC_ISR_ALRAF) { // Check if Alarm A flag is set
        RTC_ISR &= ~RTC_ISR_ALRAF; // Clear Alarm A flag
        gpio_toggle(GPIO_LED_PORT, GPIO8); // Toggle LED
    }
}


int main(void){
    rcc_setup();
    gpio_setup();
    timer_setup();
    gpio_set(GPIO_LED_PORT, GPIO_LED_PIN);   
    usart2_setup();
    
    // Trigger selection

    // timer_enable_irq(TIM2, TIM_DIER_UIE);
    // nvic_enable_irq(NVIC_TIM2_IRQ);

    nvic_enable_irq(NVIC_RTC_ALARM_IRQ);
    exti_set_trigger(EXTI_ALARM_SOURCE, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI_ALARM_SOURCE);
    
    nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    exti_select_source(EXTI_BUTTON_SOURCE, GPIOC);

    exti_set_trigger(EXTI_BUTTON_SOURCE, EXTI_TRIGGER_FALLING);
    exti_enable_request(EXTI_BUTTON_SOURCE);

    uint8_t last_seconds = 0xFF;   
    char time_str[9] = {"00:00:12"}; 
    rtc_set_time(&time_str[0], &time_str[3], &time_str[6]); // Set the time to 00:00:12

    char counter_str[9] = {0}; 

    char set_alarm_str[6] = {"00:03"};   
    rtc_set_alarm_time(&set_alarm_str[0], &set_alarm_str[3]); // Set the alarm to 00:03

    while(1){

        uint8_t alarm_hours, alarm_minutes, alarm_seconds;
        rtc_get_alarm_time(&alarm_hours, &alarm_minutes, &alarm_seconds);

        uint8_t time_hours, time_minutes, time_seconds;
        rtc_get_time(&time_hours, &time_minutes, &time_seconds);
        
        if(time_seconds != last_seconds){
            last_seconds = time_seconds;

            decimal_to_digits(time_hours, &time_str[0]);
            decimal_to_digits(time_minutes, &time_str[3]);
            decimal_to_digits(time_seconds, &time_str[6]);

            for(int i = 0; i < 8; i++) 
            {
                usart_send_blocking(USART2, time_str[i]);
            }
            usart_send_blocking(USART2, '\r'); // Carriage return to overwrite the previous line
        }

        if(button_pressed)
        {
            button_pressed = false; // Reset the flag
            
            stopwatch_running = !stopwatch_running; // Toggle the stopwatch state

            if(stopwatch_running){

                timer_enable_counter(TIM2); // Start the timer
            }
            else
            {
                timer_disable_counter(TIM2); // Stop the timer
                TIM2_CNT = 0; // Reset the timer counter
            }
        }

        if(stopwatch_running)
        {
            uint32_t elapsed_us = timer_get_counter(TIM2);
            uint32_t elapsed_cs = elapsed_us / 10000; // Convert microseconds to centiseconds
            uint32_t time_minutes = elapsed_cs / 6000; 
            uint32_t time_seconds = (elapsed_cs / 100) % 60; 
            uint32_t centis = elapsed_cs % 100; 

            decimal_to_digits(time_minutes, &counter_str[0]);
            counter_str[2] = ':'; // Add colon between minutes and seconds
            decimal_to_digits(time_seconds, &counter_str[3]);
            counter_str[5] = ':'; // Add colon between seconds and centiseconds
            decimal_to_digits(centis, &counter_str[6]);
            counter_str[8] = '\0'; // Null-terminate the string

            for(int i = 0; i < 8; i++) 
            {
                usart_send_blocking(USART2, counter_str[i]);
            }
            usart_send_blocking(USART2, ' ');
            usart_send_blocking(USART2, ' ');
            usart_send_blocking(USART2, ' ');
            
            usart_send_blocking(USART2, '\r'); // Carriage return to overwrite the previous line
        }

    }
}