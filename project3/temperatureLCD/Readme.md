# STM32F103C8T6 — DHT11 → SSD1306 OLED (°C) + Target Setpoint via Buttons

This firmware reads temperature from a **DHT11** sensor, shows the **real temperature** (centered, 1-decimal) on a **0.96" 128×64 SSD1306 I²C OLED**, and lets you set a **target setpoint** with three buttons:

- **PA3 (–)** : decrement setpoint  
- **PA4 (+)** : increment setpoint  
- **PA5 (RST)** : snap/initialize target to the current real temperature (integer `.0`)

The display always shows:
- **Real** temperature (large, centered)  
- **Target:** value at the bottom-left (small). When the setpoint is initialized, it shows an integer with **`.0`**.

---

## ✨ Features

- DHT11 one-wire protocol implemented with microsecond timing
- 1 Hz sensor sampling (throttled)
- Proper rounding to **1 decimal** on the real temperature
- Setpoint stored as an **integer °C** (shown with `.0`)
- Button debounce and **press-to-snap** behavior (RST)
- Non-blocking UI feel (main loop runs fast; DHT read throttled)
- Clean separation of logic:
  - `HandleButtons()` → input handling
  - `CalculateCelsius()` → value formatting / state
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

> Make sure the `ssd1306` and `fonts` sources are included in your project.

---

## 🔌 Pinout / Wiring

| Signal            | MCU Pin | Notes                                  |
|-------------------|---------|----------------------------------------|
| **DHT11 data**    | PB9     | Pull-up used; driven/inputs per protocol |
| **OLED SDA**      | PB7 (I2C1_SDA) | I²C @ 400 kHz                    |
| **OLED SCL**      | PB6 (I2C1_SCL) | I²C @ 400 kHz                    |
| **Button –**      | PA3     | Input **pull-up** (pressed = LOW)      |
| **Button +**      | PA4     | Input **pull-up** (pressed = LOW)      |
| **Button RST**    | PA5     | Input **pull-up** (pressed = LOW)      |
| **Timing timer**  | TIM1    | 1 MHz base for `microDelay()`          |
| **VCC**           | 3.3 V   | OLED & DHT11                           |
| **GND**           | GND     | Common ground                          |

> DHT11 requires **~20 ms low** start signal and specific microsecond sampling timing.  
> OLED default address is often **0x3C**; adjust in `ssd1306.c` if needed.

---

## ⚙️ CubeMX / Project Configuration

- **RCC**
  - HSE: *Crystal/Ceramic Resonator*
  - SYSCLK: from PLL (HSE ×9 → 72 MHz typical)
- **I²C1**
  - 400 kHz, 7-bit addressing, no stretch
- **GPIO**
  - PA3, PA4, PA5: *Input with Pull-Up* (pressed = LOW)
  - PB9: initially Output (driven in start frame), then **Input Pull-Up** during DHT read
- **TIM1**
  - Prescaler `71` → **1 MHz** tick (1 µs)
  - Used by `microDelay()` for precise DHT timing

---

## 🧠 How the code works (high level)

- **`microDelay()`** uses TIM1 at 1 MHz to create accurate microsecond waits.
- **`DHT11_Start()` / `DHT11_Read()`** perform the sensor handshake and read the 5 bytes (humidity int/dec, temp int/dec, checksum).
- **`CalculateCelsius()`**:
  - Converts DHT11 temperature into `realC` (e.g., 23.4 °C).
  - If the **setpoint** is initialized, we show it as an integer with `.0`; otherwise we mirror the real temp.
  - `format_temp_1dp()` rounds to 1 decimal correctly.
- **Buttons**:
  - On the **first** press of `+` or `–` (or RST), the setpoint **snaps** to the current real temperature, rounded to the nearest integer (20.7→21, 20.3→20).
  - Subsequent `+ / –` adjust by ±1 °C.
- **`ShowDisplay()`** centers the real temperature in a big font and prints the `Target:` line at bottom-left in a small font.

---

## 🖥️ UI Behavior

- **Centered:** `23.5 C` (rounded to 1 decimal)  
- **Bottom-left:** `Target: 23.0 C` (once initialized)  
- Until you initialize the setpoint, `Target` mirrors the real value (with decimals).

---

## 📦 File/Function Highlights

- `first_setpoint_from(float c)` – snap real temp to an **integer** starting setpoint (`> .5` rounds up).
- `HandleButtons()` – edge-detects button presses, debounces, updates setpoint.
- `ShowDisplay(realC, targetC)` – computes text width, centers large string, draws both lines.
- `DHT11_Start() / DHT11_Read()` – full sensor protocol with timeouts and checksum gate.

---

## 🚀 Build & Run

1. Import/open the project in **STM32CubeIDE**.  
2. Ensure **ssd1306**/**fonts** sources are added to your project.  
3. Wire OLED (I²C1) and DHT11 as in the table above.  
4. Flash the MCU.  
5. Use **PA3/PA4/PA5** to control the target setpoint.

---

## 🧪 Troubleshooting

<img width="1895" height="828" alt="clock" src="https://github.com/user-attachments/assets/e9e4585e-9e01-4cb2-8db0-406de1135d46" />

Set HCLK (MHz) as 72 instead of 1 from Clock Configuration window.

---


## 📜 License

This project is based on STM32Cube HAL templates. See the repository’s `LICENSE` if present.
