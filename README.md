# nucleo-libopencm3

A collection of projects built on an **STM32 Nucleo-F401RE**, using **[libopencm3](https://github.com/libopencm3/libopencm3)**. The goal throughout has been to actually understand the hardwareby reading the reference manual (RM0368) and actively going through the register level code in the library.

Everything here is built with the **Arm GNU Toolchain** (`arm-none-eabi-gcc`), flashed via **OpenOCD** over the Nucleo's onboard ST-Link, and developed in **VS Code**.

## Repo structure

```
nucleo-libopencm3/
├── libopencm3/           # git submodule — the peripheral library itself
├── projects/             # one folder per project (see below)
├── Makefile              # shared build system — pass PROJECT=<folder-name>
├── openocd.cfg           # ST-Link + STM32F4 target config, shared across projects
├── requirements.txt      # Python deps for any PC-side scripts (e.g. pyserial)
└── .vscode/              # build/debug tasks, shared across all projects
```

## Getting started

```bash
git clone --recurse-submodules https://github.com/embedded-teboho/nucleo-libopencm3
cd nucleo-libopencm3/libopencm3
make          
cd ..
make PROJECT=<project-folder-name>
make flash PROJECT=<project-folder-name>
```

## Projects

### `01-LED-BLINK-I2C`
The first program — GPIO output configuration from scratch, toggling the different leds and making sense of the STM32 I2C. Starting point for understanding `MODER`, `OTYPER`, `OSPEEDR`, and `PUPDR`.

### `02-UART-ECHO`
Interrupt-driven USART2 echo, using the ST-Link's virtual COM port. Covers `CR1`/`CR2`/`CR3` configuration and NVIC interrupt enabling.

### `03-LCD-UART-I2C`
Receives characters over UART and displays them on a **16x2 HD44780 LCD** via a PCF8574 I2C backpack. The real substance of this project was writing the LCD driver itself from the HD44780 datasheet: 4-bit nibble mode, the RS/RW/EN control-line bit-packing, and the datasheet's "initialization by instruction" reset sequence — needed because resetting the STM32 alone doesn't reset the LCD controller's internal state. Also includes a button-triggered screen clear via EXTI.

### `04TASK-SCHEDULER`
A hand-written **cooperative task scheduler**, written largely in inline ARM assembly. No RTOS, no HAL — just the Process Stack Pointer (PSP), the AAPCS calling convention, and a manually constructed initial stack frame per task. Two tasks blink independent LEDs, switching control via a directly-called `context_switch()` function (not yet interrupt-driven — PendSV-based preemption is a planned follow-up). This project is the closest thing to "how does FreeRTOS actually work under the hood."

### `05-TIMER-CLOCK-ALARM`
A multi-mode digital watch: stopwatch (hardware timer, TIM2), a real-time clock (RTC, clocked from the onboard 32.768kHz LSE crystal), and eventually an alarm — all displayed on the LCD from project 3, with physical button controls for mode-switching, start/stop, and reset. In progress.

## Toolchain

- **Arm GNU Toolchain** (`arm-none-eabi-gcc`) — [Arm's GitLab releases](https://gitlab.arm.com/tooling/gnu-toolchains-for-arm)
- **OpenOCD** — flashing/debugging via the Nucleo's onboard ST-Link
- **VS Code** + Cortex-Debug extension
- **libopencm3** — peripheral library, included as a submodule
