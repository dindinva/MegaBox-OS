
**MegaBox OS** is a lightweight, Unix-like embedded terminal and C-script interpreter environment engineered specifically for the **ATmega2560 (Arduino Mega 2560)**. Inspired by Linux terminal utilities (`bash`, `nano`, `free`, `ls`), MegaBox turns your micro-controller into an interactive, standalone programming environment capable of writing, saving, and executing live C-like scripts directly over Serial or dedicated I/O without needing to re-flash the AVR firmware via IDE.

---

## 🌟 Key Features

* **Real-time C-Script Interpreter Engine**:
  * Parse and execute C functions dynamically on the fly from internal EEPROM storage.
  * Supports conditional execution (`if` statements with logic/relational operators).
  * Direct GPIO manipulation, ADC sampling, PWM generation, shift register driving, and math mapping.
* **Unix-like Command Shell (CLI)**:
  * Interactive shell with standard terminal commands (`ls`, `free`, `uname`, `clear`, `help`).
  * Built-in text editor (`nano`) to compose and store C scripts directly to persistent EEPROM memory.
* **Full Hardware Exploitation of ATmega2560**:
  * **Extended I/O**: Access to Digital Pins `0–53` and Analog Pins `A0–A15`.
  * **Multi-Hardware Serial**: Native control over `Serial`, `Serial1`, `Serial2`, and `Serial3`.
  * **Hardware Diagnostics**: Integrated `i2cscan` utility (I2C bus scanner on pins 20/21) and real-time 54-pin GPIO logic monitor (`pinstate`).
* **Resource Optimized**:
  * Low footprint design fits strictly within the 8 KB SRAM and 4 KB internal EEPROM bounds of the ATmega2560.
  * Embedded Memory Allocation Monitor tracking heap/stack usage in real time.

---

## 🛠️ Supported C Interpreter Functions

MegaBox interprets the following syntax inside `.c` scripts saved on the device:

| Category | Supported Functions & Syntax |
| :--- | :--- |
| **Digital I/O** | `pinMode(pin, mode)`, `digitalWrite(pin, val)`, `digitalRead(pin)` |
| **Analog & PWM** | `analogRead(pin)`, `analogWrite(pin, val)` *(Supports A0–A15)* |
| **Multi-Serial** | `Serial.begin(baud)`, `Serial1.println("str")`, `Serial2.println("str")`, `Serial3.println("str")` |
| **Shift Registers** | `shiftOut(dataPin, clockPin, bitOrder, value)`, `shiftIn(dataPin, clockPin, bitOrder)` |
| **Math & Logic** | `map(val, in_min, in_max, out_min, out_max)`, `constrain(val, min, max)` |
| **Sound & Timing** | `tone(pin, freq, duration)`, `noTone(pin)`, `delay(ms)`, `delayMicroseconds(us)`, `millis()` |
| **Control Flow** | `if (digitalRead(pin) == LOW) { digitalWrite(13, HIGH); }` |

---

## ⌨️ Shell Commands

Once connected via Serial Terminal (Baud Rate: `9600`, Line Ending: `Both NL & CR`), you will be greeted by the MegaBox shell prompt: `root@megabox:~#`

* **`ls`** — List all script files saved in the 4 KB EEPROM filesystem along with byte size.
* **`free`** — Display SRAM memory allocation (8 KiB total) and internal EEPROM storage state.
* **`uname -a`** — Print kernel and target MCU architecture information.
* **`i2cscan`** — Scan for attached I2C devices on pins 20 (SDA) and 21 (SCL).
* **`pinstate`** — Display logic state (`HIGH` / `LOW`) for all 54 digital I/O pins.
* **`nano <filename.c>`** — Open the built-in mini text editor to write/edit C scripts.
* **`run <filename.c>`** — Execute the stored C script sequentially line by line.
* **`clear`** — Clear the terminal window.
* **`help`** — Show the help menu and supported functions list.

---

## 🚀 Quick Start & Examples

### 1. Installation
1. Open `MegaBox.ino` in the Arduino IDE.
2. Select **Board**: `Arduino Mega or Mega 2560` and choose the correct serial port.
3. Upload the sketch to your board.
4. Open **Serial Monitor** at **9600 baud** (Set line ending to **Both NL & CR**).

### 2. Creating & Executing C Scripts

#### Example A: Conditional Button LED Trigger (`logic.c`)
Enter `nano logic.c` in the terminal, type the following code, and press **ENTER**:
```c
pinMode(2, INPUT_PULLUP); pinMode(13, OUTPUT); if (digitalRead(2) == LOW) { digitalWrite(13, HIGH); }
