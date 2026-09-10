# STM32 HC-SR04 Hardware Telemetry Node

## Overview
This repository contains a bare-metal embedded C application for an STM32 ARM Cortex-M microcontroller, designed to interface with an HC-SR04 ultrasonic distance sensor[cite: 2]. Built using the STM32 Hardware Abstraction Layer (HAL), this project acts as a low-latency obstacle detection node[cite: 2]. It acquires raw acoustic time-of-flight data, processes it via hardware timers, and drives a local Human-Machine Interface (HMI) LED based on proximity thresholds[cite: 2]. 

This architecture is ideal for the foundational hardware layer of an autonomous robot, securely handling sensor polling before handing off telemetry to high-level perception systems (like ROS2).

## Hardware Architecture & Pin Mapping
The system utilizes standard GPIO polling and a dedicated hardware timer to process the ultrasonic signals[cite: 2].

*   **MCU Core Clock:** HSI (Internal Oscillator)[cite: 2].
*   **HC-SR04 Trigger (Output):** `GPIOA_PIN_1`[cite: 2].
*   **HC-SR04 Echo (Input):** `GPIOA_PIN_0`[cite: 2].
*   **Status / Warning LED:** `GPIOB_PIN_11`[cite: 2].
*   **Hardware Timer:** `TIM2` (Configured as a microsecond counter for precise pulse measurement)[cite: 2].

## Signal Processing & Core Logic
The primary data acquisition loop operates as follows:

1.  **Signal Generation:** The MCU drives the `TRIG` pin `HIGH` for exactly 10 microseconds to initialize the sensor's acoustic burst, managed by a custom `delay_us` function utilizing `TIM2`[cite: 2].
2.  **Echo Measurement:** The system waits for the `ECHO` pin to transition to `HIGH`[cite: 2]. Once `HIGH`, `TIM2` is reset to 0, and the MCU counts the exact duration the pin remains active[cite: 2].
3.  **Time-out Safeguard:** Both the wait-for-high and wait-for-low loops include a 30 ms timeout using `HAL_GetTick()` to prevent the MCU from hanging if the sensor disconnects or the signal is lost[cite: 2].
4.  **Distance Conversion:** The microsecond pulse duration is converted into centimeters using the standard acoustic velocity formula[cite: 2]:
    `distance_cm = pulse_time / 58`

## System States & HMI Behavior
The local LED serves as a real-time debugging and proximity warning interface, reacting dynamically to a hardcoded 120 cm threshold (`DISTANCE_THRESHOLD_CM`)[cite: 2]:

*   **Clear Path (> 120 cm):** The LED remains explicitly driven to `LOW` (OFF)[cite: 2].
*   **Obstacle Detected (<= 120 cm):** The LED toggles rapidly with a 100 ms delay, indicating a proximity breach[cite: 2].
*   **Sensor Error / Timeout:** If the reading function returns the `999` error code, the LED blinks slowly with a 700 ms delay to indicate hardware failure or a missing echo[cite: 2].

## Deployment
This project is structured for compilation using STM32CubeIDE. 
1. Flash the compiled binary to the STM32 target.
2. Ensure the HC-SR04 is powered appropriately (typically 5V, requiring level-shifting on the Echo pin if connecting to a strictly 3.3V-tolerant STM32 pin).
