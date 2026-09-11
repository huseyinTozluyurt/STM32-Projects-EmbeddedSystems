# STM32 Quad-Sensor Spatial Awareness Node

## Overview
This repository contains a bare-metal embedded C application for an STM32 ARM Cortex-M microcontroller[cite: 3]. It manages a distributed array of four HC-SR04 ultrasonic sensors (Front, Left, Right, Back) to provide continuous, 360-degree spatial awareness for a robotic platform[cite: 3]. 

The system utilizes the STM32 Hardware Abstraction Layer (HAL) to orchestrate sensor polling, evaluates environmental geometry against predefined collision thresholds, streams real-time diagnostic telemetry over UART, and drives a local binary LED state machine for immediate hardware-level feedback[cite: 3].

## Hardware Architecture & Pin Mapping
The node leverages hardware timers (`TIM2`) for precise acoustic pulse measurement and standard GPIOs with pull-down resistors for signal acquisition[cite: 3].

### Sensor Array
*   **Front Sensor:** `TRIG: GPIOB_PIN_0` | `ECHO: GPIOA_PIN_0`[cite: 3]
*   **Left Sensor:** `TRIG: GPIOB_PIN_1` | `ECHO: GPIOA_PIN_1`[cite: 3]
*   **Right Sensor:** `TRIG: GPIOB_PIN_10` | `ECHO: GPIOA_PIN_2`[cite: 3]
*   **Back Sensor:** `TRIG: GPIOB_PIN_11` | `ECHO: GPIOA_PIN_3`[cite: 3]

### HMI & Telemetry
*   **Binary LED 0 (LSB):** `GPIOA_PIN_5`[cite: 3]
*   **Binary LED 1 (MSB):** `GPIOA_PIN_6`[cite: 3]
*   **UART Telemetry:** `USART1` (115200 Baud, 8N1)[cite: 3]

## Signal Processing & Collision Logic
The system evaluates the environment at a stable 300 ms polling rate[cite: 3]. It reads all four sensors sequentially using a custom `HCSR04_Read_All` function and evaluates the data against a strict 20 cm safety threshold (`OBSTACLE_THRESHOLD_CM`)[cite: 3].

To optimize decision-making for a mobile chassis, the collision logic is grouped into two distinct physical axes:
*   **Longitudinal Axis:** Triggers if either the Front or Back sensor detects an object at $\le$ 20 cm[cite: 3].
*   **Lateral Axis:** Triggers if either the Left or Right sensor detects an object at $\le$ 20 cm[cite: 3].

## System States & Hardware Feedback
Instead of requiring an external monitor, the system outputs its spatial state via a two-bit binary LED interface on the physical board, representing four distinct operational states[cite: 3]:

*   **`00` (LEDs OFF):** Clear path. No obstacles detected within 20 cm on any axis[cite: 3].
*   **`01` (LSB ON):** Longitudinal Obstacle. Object detected in the Front or Back path[cite: 3].
*   **`10` (MSB ON):** Lateral Obstacle. Object detected in the Left or Right path[cite: 3].
*   **`11` (Both ON):** Critical State. Either a hardware/timeout error occurred on one or more sensors, or the robot is trapped with obstacles on both the longitudinal and lateral axes simultaneously[cite: 3].

Simultaneously, the node transmits a formatted string over UART containing the exact distance (in cm) and health status (`OK` or `ERR`) of every sensor for integration with higher-level navigation nodes[cite: 3].

## Deployment
This project is configured for compilation using STM32CubeIDE.
1. Build and flash the binary to the STM32 target.
2. Connect the MCU to a serial terminal at `115200` baud to monitor the raw distance streams[cite: 3].
3. Ensure all HC-SR04 sensors are powered with 5V, utilizing appropriate logic level shifting on the 3.3V STM32 Echo input pins.
