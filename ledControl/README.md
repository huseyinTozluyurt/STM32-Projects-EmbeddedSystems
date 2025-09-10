# STM32F103C8T6 PWM LED Sequencer (STM32CubeIDE Project)

This project demonstrates **PWM generation on STM32F103C8T6 (Blue Pill)** using **TIM2 Channel 2**.  
The LED brightness is controlled in different patterns (short pulses, long fades, and special events) defined in a sequence array.

The project is created in **STM32CubeIDE** and uses **HAL drivers**.

---

## 🔧 STM32CubeIDE Configuration


<img width="1894" height="855" alt="configur" src="https://github.com/user-attachments/assets/50be2a4e-842b-4af7-a41c-4d77dcdeb494" />

### RCC
- **High Speed Clock (HSE):** Crystal / Ceramic Resonator (8 MHz external crystal)  
- **Debug:** Serial Wire (SWD enabled for programming/debugging)  

### TIM2
- **Clock Source:** Internal Clock  
- **Channel 2 Mode:** PWM Generation CH2  
- **Prescaler:** 127  
- **Counter Period (ARR):** 625  

#### ➡️ PWM Frequency Calculation
\[
f_{PWM} = \frac{f_{TIM}}{(PSC+1)\times(ARR+1)}
\]

For an 8 MHz timer clock:

\[
f_{PWM} = \frac{8,000,000}{128 \times 626} \approx 100 \text{ Hz}
\]

100 Hz is suitable for LED dimming and sequencing effects.

**Example (in plain code):**

```c
uint32_t f_tim = 8000000;      // 8 MHz timer clock
uint32_t prescaler = 127;
uint32_t arr = 625;

uint32_t f_pwm = f_tim / ((prescaler + 1) * (arr + 1));
// f_pwm = 8,000,000 / (128 * 626) ≈ 100 Hz
```
---

## 📂 Project Structure

```text
ledButton/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32f1xx_hal_conf.h
│   │   └── stm32f1xx_it.h
│   └── Src/
│       ├── main.c              # Application code (PWM LED sequence)
│       ├── stm32f1xx_hal_msp.c
│       ├── stm32f1xx_it.c
│       ├── syscalls.c
│       ├── sysmem.c
│       └── system_stm32f1xx.c
├── Drivers/                    # HAL drivers
├── Startup/                    # Startup files and vector table
├── ledButton.ioc               # CubeMX configuration file
└── STM32F103C8TX_FLASH.ld      # Linker script
```
## 💡 How It Works

- **TIM2_CH2 (PA1)** outputs PWM.  
- A **sequence array (`Step seq[]`)** defines brightness (CCR), ON time, and OFF time.  
- In the main loop, each step is executed:
  - Set CCR → keep ON for `on_ms`  
  - Reset CCR → keep OFF for `off_ms`  
- Special steps include long ON/OFF durations or very dim LED levels.

![demo](https://github.com/user-attachments/assets/eeca5503-4b42-4887-b467-2c57032c94d8)


### Example Sequence
```c
typedef struct { uint16_t ccr_on; uint32_t on_ms; uint32_t off_ms; } Step;

Step seq[] = {
  {500, 300, 3000}, {20, 3000, 1500}, {300, 300, 3000}, {450, 300, 3000}, {100, 300, 3000},
  {600, 7000, 3000}, // special bright step
  {300, 300, 3000}, {600, 300, 3000},
  {20, 7000, 7000},  // special dim step
  {620, 300, 3000}, {200, 300, 3000}
};


