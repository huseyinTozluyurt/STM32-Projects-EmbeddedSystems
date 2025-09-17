# STM32F103C8T6 — DHT11 → SSD1306 OLED (°C) + Target Setpoint via Buttons + Match LED

This firmware reads temperature from a **DHT11** sensor, shows the **real temperature** (centered, 1-decimal) on a **0.96" 128×64 SSD1306 I²C OLED**, lets you set a **target setpoint** with three buttons, **and turns on an LED (PB8)** when the **real temperature equals the target**.

- **PA3 (–)** : decrement setpoint  
- **PA4 (+)** : increment setpoint  
- **PA5 (RST)** : snap/initialize target to the current real temperature (integer `.0`)  
- **PB8 LED** : **ON** when `realC == targetC`, otherwise **OFF**

The display always shows:
- **Real** temperature (large, centered)  
- **Target:** value at the bottom-left (small). When the setpoint is initialized, it shows an integer with **`.0`**.

![WhatsApp Image 2025-09-17 at 1 28 32 PM(1)](https://github.com/user-attachments/assets/e080687f-63e9-4005-b6b9-019c7c9ec2e9)


---

## ✨ Features

- DHT11 one-wire protocol implemented with microsecond timing (TIM1 @ 1 MHz)
- 1 Hz sensor sampling (throttled)
- Proper rounding to **1 decimal** for the real temperature
- Target setpoint stored as **integer °C** (rendered as `.0` after initialization)
- Button debounce and **press-to-snap** behavior (RST)
- **Match indicator LED on PB8** turns **ON** when `realC == targetC`
- Clean separation of logic:
  - `HandleButtons()` → input handling
  - `CalculateCelsius()` → compute/compare and drive LED
  - `ShowDisplay()` → OLED rendering

---

## 🧰 Toolchain

- **IDE:** STM32CubeIDE  
- **MCU:** STM32F103C8T6 (Blue Pill)  
- **Display:** SSD1306 I²C (128×64, typical addr `0x3C`)  
- **Sensor:** DHT11  
- **HAL:** STM32 HAL drivers  
- **Libraries used in code:**
  - `ssd1306.h/.c` (I²C OLED driver)
  - `fonts.h` (font definitions)

> Ensure the `ssd1306` and `fonts` sources are added to your project.

---

## 🔌 Pinout / Wiring

| Signal                | MCU Pin             | Notes                                              |
|-----------------------|---------------------|----------------------------------------------------|
| **DHT11 data**        | PB9                 | Output during start pulse; then input with pull-up |
| **OLED SDA**          | PB7 (I2C1_SDA)      | I²C @ 400 kHz                                      |
| **OLED SCL**          | PB6 (I2C1_SCL)      | I²C @ 400 kHz                                      |
| **Button –**          | PA3                 | Input **pull-up** (pressed = LOW)                  |
| **Button +**          | PA4                 | Input **pull-up** (pressed = LOW)                  |
| **Button RST**        | PA5                 | Input **pull-up** (pressed = LOW)                  |
| **Match LED**         | PB8                 | **Output push-pull**; ON when `realC==targetC`     |
| **Timing timer**      | TIM1                | 1 MHz base for `microDelay()`                      |
| **VCC**               | 3.3 V               | OLED & DHT11                                       |
| **GND**               | GND                 | Common ground                                      |

> OLED default I²C address is often **0x3C**; adjust in `ssd1306_conf.h`/driver if needed.

---

## ⚙️ CubeMX / Project Configuration

- **RCC**
  - HSE: *Crystal/Ceramic Resonator*
  - SYSCLK: from PLL (HSE ×9 → **72 MHz**)  
  - (Troubleshooting note below: ensure **HCLK = 72 MHz**)
- **I²C1**
  - 400 kHz, 7-bit addressing, no stretch
- **GPIO**
  - PA3, PA4, PA5: *Input with Pull-Up* (pressed = LOW)
  - PB8: *Output push-pull* (LED)
  - PB9: configured as output for DHT start pulse; then switched to input pull-up during read
- **TIM1**
  - Prescaler `71` → **1 MHz** tick (1 µs)
  - Used by `microDelay()` for precise DHT timing

---

## 🧠 How the code works (high level)

- **`microDelay()`** uses TIM1 at 1 MHz to create accurate microsecond waits.
- **`DHT11_Start()` / `DHT11_Read()`**: performs sensor handshake and reads 5 bytes (humidity int/dec, temp int/dec, checksum).
- **`CalculateCelsius()`**:
  - Computes `realC = TCI + TCD/10.0f`.
  - Determines `targetC`: if initialized, uses the integer setpoint (`.0`), otherwise mirrors `realC`.
  - **Drives LED on PB8:**  
    - `HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, SET);` when **`realC == targetC`**  
    - else `RESET`  
  - Calls `ShowDisplay(realC, targetC)`.
- **Buttons**:
  - On first press of `+` or `–` (or `RST`), the setpoint **snaps** to the current real temperature, rounded to nearest integer (`> .5` rounds up).
  - `+ / –` then adjust by ±1 °C.

> **Note:** Exact float equality is strict. Because the target becomes an **integer** and realC has **one decimal**, the LED turns on only when realC is exactly that integer (e.g., **21.0** equals target 21). If you prefer a tolerance (e.g., ±0.2 °C), compare with `fabsf(realC - targetC) < 0.2f`.

---

## 🖥️ UI Behavior

- **Centered:** `23.5 C` (rounded to 1 decimal)  
- **Bottom-left:** `Target: 23.0 C` (once initialized)  
- **LED (PB8):** ON when the displayed real equals the target value (per rule above).

---

## 📦 File/Function Highlights

- `first_setpoint_from(float c)` – snaps real temp to **integer** starting setpoint (`> .5` rounds up).
- `HandleButtons()` – edge-detects button presses, debounces, updates setpoint.
- `CalculateCelsius()` – computes temps **and** controls PB8 LED.
- `ShowDisplay(realC, targetC)` – centers large string, prints target line, updates OLED.
- `DHT11_Start() / DHT11_Read()` – full sensor protocol with timeouts and checksum gate.

---

## 🚀 Build & Run

1. Open in **STM32CubeIDE**.  
2. Ensure `ssd1306`/`fonts` sources and `ssd1306_conf.h` are present.  
3. Wire OLED (I²C1), DHT11 (PB9), and LED (PB8).  
4. Flash the MCU.  
5. Use **PA3/PA4/PA5** to control the target setpoint; watch PB8 LED.

---

## 🧪 Troubleshooting

- **No/garbled OLED**  
  - Check I²C address (`0x3C` vs `0x3D`).
  - Ensure pull-ups present and I²C is at 400 kHz.
- **Timing issues with DHT11**  
  - TIM1 must be **1 MHz**; system clock **HCLK = 72 MHz**.
- **Clock config gotcha**

  <img width="1895" height="828" alt="clock" src="https://github.com/user-attachments/assets/e9e4585e-9e01-4cb2-8db0-406de1135d46" />

  Set **HCLK (MHz) to 72** in Clock Configuration.

---

## 📜 License

This project is based on STM32Cube HAL templates.
