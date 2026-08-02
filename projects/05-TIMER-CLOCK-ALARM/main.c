#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/rtc.h>
#include <stdbool.h>

#define GOAL_FREQUENCY (2)                                    //10ms represented as 100Hz                  
#define TIMER_CLOCK (rcc_apb1_frequency * 2)                    //F_timer   (CK_PSC)
#define COUNTER_CLOCK (1000000)                                 //F_counter (CK_CNT)
#define TIMER_PRESCALER (TIMER_CLOCK / COUNTER_CLOCK - 1)       //PSC
#define TIMER_PERIOD (COUNTER_CLOCK / GOAL_FREQUENCY - 1)       //ARR

#define GPIO_LED_PIN GPIO5
#define GPIO_LED_PORT GPIOA


static void rcc_setup(void){
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);              //Enable clock for TIM2
    rcc_periph_reset_pulse(RST_TIM2);              //Reset TIM2 to default values
}

static void rtc_setup(void){
    pwr_disable_backup_domain_write_protect();
    rcc_osc_on(RCC_LSE);
    rcc_wait_for_osc_ready(RCC_LSE);

    //Select LSE as RTC clock source
    RCC_BDCR |= (1<<8);                 //bit 8 = 1
    RCC_BDCR &= ~(1<<9);                //bit 9 = 0. Together with bit 8, this selects LSE as RTC clock source
    RCC_BDCR |= (1<<15);                //Enable RTC clock

    rtc_unlock();                       //Disable write protection for RTC registers
    rtc_wait_for_synchro();             //Wait for RTC registers to synchronize with APB
    rtc_set_init_flag();                    //Set initialization flag to enter initialization mode
    while((RTC_ISR & (1<<6)) != (1<<6));        //Wait for initialization mode to be entered
    rtc_set_prescaler(255, 127);                 //Set RTC prescaler to get 1Hz clock
    rtc_clear_init_flag();                  //Clear initialization flag to exit initialization mode
    while((RTC_ISR & (1<<6)) == (1<<6));        //Wait for initialization mode to be exited
    rtc_lock();                         //Enable write protection for RTC registers
    pwr_disable_backup_domain_write_protect();                       //Disable backup domain write protection
}

static void gpio_setup(void){
    gpio_mode_setup(GPIO_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_LED_PIN);
    gpio_set_output_options(GPIO_LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_LED_PIN);
}

static void timer_setup(void){
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_disable_preload(TIM2);
    timer_continuous_mode(TIM2);
    
    timer_set_prescaler(TIM2, TIMER_PRESCALER);     //setup TIMx_PSC register
    timer_set_period(TIM2, TIMER_PERIOD);           //setup TIMx_ARR register

    // Enable the update interrupt and the NVIC interrupt for TIM2
    timer_enable_irq(TIM2, TIM_DIER_UIE);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    timer_enable_counter(TIM2);
}

static void nvic_setup(void)
{
    nvic_enable_irq(NVIC_RTC_WKUP_IRQ);
    nvic_set_priority(NVIC_RTC_WKUP_IRQ, 0);
}

void tim2_isr(void){
    if(timer_get_flag(TIM2, TIM_SR_UIF)){
        timer_clear_flag(TIM2, TIM_SR_UIF);
        gpio_toggle(GPIO_LED_PORT, GPIO_LED_PIN);
    }
}
int main(void){
    rcc_setup();
    gpio_setup();
    timer_setup();
    nvic_setup();
    rtc_setup();

    while(1){
        // Should not be here
    }
    
}