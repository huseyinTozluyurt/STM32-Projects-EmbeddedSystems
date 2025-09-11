# STM32F103C8T6 – ADC to PWM LED Control with Button

This project demonstrates how to use the **STM32F103C8T6 (Blue Pill)** microcontroller with **STM32CubeIDE** to:

- Read an **analog value** from a potentiometer (via **ADC1, Channel 2**)
- Generate a **PWM signal** on **TIM2 Channel 2** to control LED brightness
- Use a **button (PA9)** to toggle between ADC-driven control and fixed PWM output

---

## ⚡ Features

- **ADC1** reads analog input (0–3.3V mapped to 0–4095).
- **Timer 2, Channel 2** generates PWM for LED dimming.
- **GPIO Input (PA9)** acts as a button to toggle between modes.
- PWM duty cycle updates according to potentiometer position.
  
![WhatsApp Image 2025-09-11 at 2 38 25 PM](https://github.com/user-attachments/assets/397ce783-3262-4128-9cc9-884bce64417a)

---

## 🛠️ Hardware Setup

- **MCU:** STM32F103C8T6 ("Blue Pill")
- **HSE:** Crystal/Ceramic Resonator
- **SYS Debug:** Serial Wire (SWD)
- **Button:** PA9 connected to GND with pull-up
- **Potentiometer:** connected to ADC1_IN2 (PA2)
- **LED:** connected to TIM2_CH2 pin (usually PA1) with resistor

---

## ⚙️ STM32CubeIDE Configuration

<img width="1920" height="1080" alt="Screenshot (209)" src="https://github.com/user-attachments/assets/bb19150b-8eec-4666-b0b9-d6c7c25141fe" />


- **System Core / RCC**
  - High Speed Clock (HSE): *Crystal/Ceramic Resonator*
  - Debug: *Serial Wire*
- **Timers / TIM2**
  - Clock Source: *Internal Clock*
  - Channel 2: *PWM Generation CH2*
  - Prescaler: `127`
  - Counter Period: `625`
- **ADC1**
  - Channel: `ADC1_IN2`
  - Continuous Conversion: Enabled
- **GPIO**
  - PA9: Input (Button)
  - PA2: Analog (Potentiometer)
  - PA1: PWM Output (LED)

---

## 📄 Code Overview

```c
// Start peripherals
HAL_ADC_Start(&hadc1);
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

while (1)
{
    uint32_t mycounter = 0;

    // Button toggle
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_RESET) {
        mycounter += 1;
    }

    if (mycounter % 2 == 1) {
        // ADC-driven PWM
        HAL_ADC_PollForConversion(&hadc1, 1000);
        uint16_t readValue = HAL_ADC_GetValue(&hadc1);
        uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2); // 625
        uint32_t pwmValue = (readValue * arr) / 4095;
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pwmValue);
        HAL_Delay(400);
    } else {
        // Fixed PWM output
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 80);
        HAL_Delay(400);
    }
}
```

If button not pressed → keeps current PWM output.

If button pressed → toggles between ADC-based LED dimming and fixed brightness.

🚀 How to Run

Open the project in STM32CubeIDE.

Connect the Blue Pill to your PC via ST-Link (or compatible debugger).

Build & flash the project.

Turn the potentiometer → LED brightness changes.

Press the button → switch between ADC mode and fixed PWM mode.

## 📂 Repository Structure


```text
Project2/
 ├── Core/          # Application source & headers
 ├── Drivers/       # HAL drivers
 ├── Startup/       # Startup assembly & linker script
 ├── potProject.ioc # STM32CubeMX configuration
 └── main.c         # Main application logic

```
