
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <stdbool.h>

void task1_function(void);
void task1_stack_init(void);
void task2_stack_init(void);
void context_switch(void);
void task2_function(void);
void gpio_setup(void);
void start_first_task(void);

#define CPU_FREQ (84000000)
#define SYSTICK_FREQ (1000)
#define LED_ON_DURATION_MS (200)   // how long the LED stays lit per byte received

uint32_t task1_stack[64], task2_stack[64];   // Stack for each tasks - 64 words = 256 bytes

uint32_t *task_sp[2];       // saved PSP values for task 0 and task 1
uint32_t current_task = 0;          //current task index - 0 or 1

static volatile uint32_t system_millis=0;

static void systick_setup(void)
{
    systick_set_frequency(SYSTICK_FREQ,CPU_FREQ); 
    systick_counter_enable();
    systick_interrupt_enable();
}

void yield(void){
    context_switch();
}

__attribute__((naked)) void context_switch(void)
{
    __asm__ volatile (
        "mrs r0, psp                \n"                 // Read process stack pointer into r0 -> r0 points to the current task's stack
        "stmdb r0!, {r4-r11, r14}   \n"                 // store r4-r11 AND r14 to [r0]

        "ldr r1, =current_task      \n"                 // Load the address of task
        "ldr r2, [r1]               \n"                 //load stack into r2
        "ldr r3, =task_sp           \n"
        "lsls r2, r2, #2            \n"                 // multiply by 4 to get the correct offset for the stack pointer - each pointer occoupies 4 bytes
        "adds r3, r3, r2            \n"
        "str r0, [r3]               \n"                 // Save the updated stack pointer

        //Choose the next task in a round-robin fashion
        "ldr r2, [r1]               \n"                 //reload current_task into r2
        "eors r2, r2, #1            \n"                 //flip 0<->1
        "str r2, [r1]               \n"                 //update current_task to the next task

        "ldr r3, =task_sp           \n"                 // Load the new task's psp into r3
        "lsls r2, r2, #2            \n"
        "adds r3, r3, r2            \n"
        "ldr r0, [r3]               \n"                //contains the stack pointer of the next task 

        "ldmia r0!,{r4-r11, r14}    \n"                 // Restore registers r4-r11 from the stack
        "msr psp, r0                \n"                 // Update the process stack pointer
        "bx lr                      \n"                 // Return from the function
    );
}

__attribute__((naked)) void start_first_task(void)
{
    __asm__ volatile
    (
        "ldr r0, =task_sp           \n"                  // Load the address of task_sp into r0
        "ldr r0, [r0]               \n"                  // Load the value at the address pointed to by r0 into r0
        "ldmia r0!, {r4-r11, r14}   \n"                  // restore r4-r11 and r14 from task 1's initial stack frame [r0]
        "msr psp, r0                \n"                  // PSP points into task 1's stack
        "mrs r1, control            \n"                  // Read the CONTROL register into r1
        "orr r1, r1, #2             \n"                  // Set the CONTROL register to use PSP as the current stack pointer (bit 1 = 1)
        "msr control, r1            \n"                  // Write the updated value back to the CONTROL register
        "isb                        \n"                  // Instruction Synchronization Barrier 
        "bx lr                      \n"                  // jumps to task1_function

    );
}

void task1_stack_init(void){
    // Create stack frame for task 1
    task1_stack[63] = (uint32_t)task1_function; // r14(LR) - resume or start address
    task1_stack[62] = 0; // R11
    task1_stack[61] = 0; // R10     
    task1_stack[60] = 0; // R9
    task1_stack[59] = 0; // R8
    task1_stack[58] = 0; // R7
    task1_stack[57] = 0; // R6
    task1_stack[56] = 0; // R5
    task1_stack[55] = 0; // R4
    task_sp[0] = &task1_stack[55]; // Set the stack pointer for task 1 to point to the top of its stack frame
}

void task2_stack_init(void){
    //Create stack frame for task 2
    task2_stack[63] = (uint32_t)task2_function; // r14(LR) - resume or start address
    task2_stack[62] = 0; // R11
    task2_stack[61] = 0; // R10     
    task2_stack[60] = 0; // R9
    task2_stack[59] = 0; // R8
    task2_stack[58] = 0; // R7
    task2_stack[57] = 0; // R6
    task2_stack[56] = 0; // R5
    task2_stack[55] = 0; // R4
    task_sp[1] = &task2_stack[55]; // Set the stack pointer for task 2 to point to the top of its stack frame
}

void task1_function(void) {
    while (1) {
        gpio_toggle(GPIOA, GPIO5);
        for (volatile int i = 0; i < 100000; i++);  
        yield();
    }
}

void task2_function(void) {
    while (1) {
        gpio_toggle(GPIOA, GPIO6);
        for (volatile int i = 0; i < 100000; i++);
        yield();
    }
}

void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5 | GPIO6);
}


int main(void){
    gpio_setup();
    task1_stack_init();
    task2_stack_init();

    start_first_task();

    while(1){
        // Should not be here
    }

}