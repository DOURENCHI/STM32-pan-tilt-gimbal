# STM32 2-Axis Pan-Tilt Gimbal

A two-degree-of-freedom pan-tilt platform built on **STM32F103C8T6**, controlled over UART.

Two SG90 servos are driven by hardware PWM (TIM2 CH1 / CH2). A 10 ms timer interrupt
advances each axis one degree at a time toward its target, so motion never blocks the
main loop — commands arriving mid-move are applied immediately.

## Hardware

| Part | Spec |
|---|---|
| MCU | STM32F103C8T6 (64 KB Flash / 20 KB RAM) |
| Servos | SG90 × 2 (180° position type) |
| Power | External 5 V / 3 A, common ground with the MCU |
| Debugger | ST-Link V2 (SWD) |

## Wiring

| Servo | Signal pin | Power |
|---|---|---|
| Yaw (horizontal) | PA0 — TIM2_CH1 | external 5 V |
| Pitch (vertical) | PA1 — TIM2_CH2 | external 5 V |

UART1: **PA9 = TX**, **PA10 = RX**. Connect to a USB-TTL adapter crosswise
(adapter TX → PA10, adapter RX → PA9, GND common).

## How it works

**PWM generation.** TIM2 runs at 1 MHz (`PSC = 71`) with `ARR = 19999`, giving a
20 ms period (50 Hz). Because one tick equals 1 µs, the compare register value *is*
the pulse width in microseconds — no extra conversion needed.

**Angle mapping.** `pulse_us = 500 + angle × 2000 / 180`, then clamped to a per-axis
safe range (yaw 10–170°, pitch 20–160°). The intermediate multiply is done in 32-bit
to avoid overflow (`180 × 2000 = 360000` exceeds `uint16_t`).

**Non-blocking motion.** `Servo_MoveTo()` only writes `target`. TIM3 interrupts every
10 ms and moves `current` one step toward it, calling `Servo_SetAngle()` for each axis.
Turning speed is set by the interrupt period, and the CPU stays free.

**UART protocol.**

| Command | Effect |
|---|---|
| `A<angle>` | set yaw angle, e.g. `A90` |
| `B<angle>` | set pitch angle, e.g. `B45` |

Terminate each command with a newline. The board replies `OK` or `ERR`.

## Project layout

```
Start/      startup file, system init, CMSIS core
Library/    STM32F10x standard peripheral library
User/       config.h  servo.c/h  uart.c/h  delay.c/h  main.c
```

`config.h` holds every tunable parameter (pulse range, axis IDs, travel limits,
UART baud rate) so that changing hardware only requires editing one file.

## Build

1. Keil MDK 5 with the **STM32F10x Standard Peripheral Library**
2. Make sure the `Keil.STM32F1xx_DFP` device pack is installed
3. Open `Gimbal.uvprojx`
4. `F7` to build, `F8` to flash (enable *Reset and Run* in the debugger settings)

## Status

Implemented: dual-axis independent control, UART command interface, per-axis software
travel limits, smooth non-blocking motion.

Not implemented: closed-loop position feedback, vision-based tracking.
