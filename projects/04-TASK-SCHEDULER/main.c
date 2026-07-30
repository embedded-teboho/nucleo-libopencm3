
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <stdbool.h>

void task1_function(void);
void task1_stack_init(void);

#define GPIO_LED (GPIO5)

uint32_t task1_stack[64];   // 64 words = 256 bytes

uint32_t task1_sp;
uint32_t task2_sp;
uint32_t *current_task_sp = &task1_sp;   

uint32_t task_sp[2];       // saved PSP values for task 0 and task 1
uint32_t current_task = 0; 

void context_switch(void);

void yield(void){
    context_switch();
}

__attribute__((naked)) void context_switch(void)
{
    __asm__ volatile (
        "mrs r0, psp                \n"                 // Read process stack pointer into r0
        "stmdb r0!, {r4-r11, r14}   \n"                 // store r4-r11 AND r14 to [r0]

        "ldr r1, =current_task_sp   \n"                 // Load the address of task
        "ldr r2, [r1]               \n"                 //load stack into r2
        "ldr r3, =task_sp           \n"
        "lsls r2, r2, #2            \n"
        "adds r3, r3, r2            \n"
        "str r0, [r3]               \n"                 // Save the updated stack pointer

        "ldr r2, [r1]               \n"
        "eors r2, r2, #1            \n"                 //flip 0<->1
        "str r2, [r1]               \n"

        "ldr r3, =task_sp           \n"
        "lsls r2, r2, #2            \n"
        "adds r3, r3, r2            \n"
        "ldr r0, [r3]               \n"                 

        "ldmia r0!,{r4-r11, r14}    \n"                 // Restore registers r4-r11 from the stack
        "msr psp, r0                \n"                 // Update the process stack pointer
        "bx lr                      \n"                 // Return from the function
    );
}

void task1_stack_init(void){
    // Prefill the stack
    task1_stack[63] = 0x01000000; // Initial xPSR
    task1_stack[62] = (uint32_t)task1_function; // PC
    task1_stack[61] = 0xFFFFFFFD; // LR  The Link Register (LR) is register R14. 
                            // It stores the return information for subroutines, function calls, and exceptions. 
                            // On reset, the processor sets the LR value to 0xFFFFFFFF.         
    task1_stack[60] = 0; // R12 - don't care 
    task1_stack[59] = 0; // R3 - don't care 
    task1_stack[58] = 0; // R2 - don't care 
    task1_stack[57] = 0; // R1 - don't care 
    task1_stack[56] = 0; // R0 - don't care 
}

void task1_function(void) {
    while (1) {
        gpio_toggle(GPIOA, GPIO5);
    }
}

int main(void){

    while(1){
        //return nothing
    }

}