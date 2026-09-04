

# MegaBox OS — Embedded C-Interpreter & Terminal OS for Arduino Mega 2560

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
Usage example:
https://youtu.be/_VCTRxD8m0M?si=hv8ieFO4rIRtqwD9
-----
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

pinMode(2, INPUT_PULLUP); pinMode(13, OUTPUT); if (digitalRead(2) == LOW) { digitalWrite(13, HIGH); }
Then run the script:
root@megabox:~# run logic.c
Example B: Reading ADC & Sending Data over Secondary Hardware Serial (sensor.c)
Serial1.begin(9600); map(analogRead(A15), 0, 1023, 0, 255); Serial1.println("Data processed");
📐 Hardware Requirements
* Board: Arduino Mega 2560 (ATmega2560)
* USB Cable: Type-A to Type-B USB Cable
* Terminal Emulator: Arduino Serial Monitor, PuTTY, Tera Term, or Serial/Minicom on Linux/macOS.


Updated ***4 sep 2026***
## 🚀 Update Notes: Command Reference & Help System Integration

We have updated the project's Help System to include a comprehensive command reference for both **Linux** shell environments and **Arduino C/C++** microcontroller programming. Users can access this documentation interactively via the built-in Serial CLI or Terminal interface using the `help`, `linux`, or `arduino` commands.

## 🚀 Update Notes: Micro-Shell CLI & Arduino C-Script Interpreter

We have updated the project documentation to reflect the latest embedded CLI system running on the **ATmega2560**. The environment now features a simulated **Linux-like file system stored in EEPROM** alongside an **on-board C-Script Interpreter**.

---

### 🐧 Linux System Commands (EEPROM & ATmega2560 Shell)

Commands executed directly in the embedded terminal to interact with EEPROM storage, monitor system memory (SRAM), and manage the ATmega2560 hardware.

| Command | Usage | Description |
| :--- | :--- | :--- |
| **`ls`**, **`ls -l`** | `ls [-l]` | List files stored in EEPROM |
| **`cat`** | `cat <file>` | Display content of a file |
| **`touch`** | `touch <file>` | Create a new empty file in EEPROM |
| **`rm`** | `rm <file>` | Remove a file from EEPROM storage |
| **`cp`** | `cp <src> <dst>` | Copy file content to a new location |
| **`wc`** | `wc <file>` | Count lines, words, and characters in a file |
| **`pwd`** | `pwd` | Show current working directory |
| **`echo`** | `echo <text>` | Print text string to terminal output |
| **`clear`** | `clear` | Clear the terminal screen |
| **`free`**, **`free -h`** | `free [-h]` | Display RAM (SRAM) and EEPROM usage status |
| **`uptime`** | `uptime` | Show total system running time |
| **`ps`**, **`top`** | `ps` / `top` | Display running tasks and active SRAM allocation |
| **`dmesg`** | `dmesg` | Show system boot and initialization log |
| **`whoami`** | `whoami` | Show current active user (`root`) |
| **`hostname`** | `hostname` | Display device hostname |
| **`uname -a`** | `uname -a` | Print Linux kernel details |
| **`reboot`** | `reboot` | Restart the ATmega2560 board |

---

### ⚡ Arduino C-Script API Commands & Interpreter Functions

Use built-in utility commands to write or execute lightweight C scripts directly on the board.

#### 🛠 CLI Script Management
* **`nano <file.c>`** : Open the built-in text editor to create/edit a C script.
* **`run <file.c>`** : Execute the C script using the embedded interpreter.

#### 📜 Supported Functions inside C Scripts

| Category | API Function / Syntax | Description |
| :--- | :--- | :--- |
| **Digital I/O** | `pinMode(pin, mode)` | Configure pin state (`INPUT`, `OUTPUT`, `INPUT_PULLUP`) |
| | `digitalWrite(pin, val)` | Set digital output state (`HIGH`, `LOW`) |
| | `digitalRead(pin)` | Read digital state from a pin |
| | `togglePin(pin)` | Invert state of specified digital pin |
| **Analog I/O** | `analogWrite(pin, val)` | Output PWM signal (`0 - 255`) |
| | `analogRead(pin)` | Read analog input voltage (`0 - 1023`) |
| | `analogReference(type)` | Set analog reference voltage (`DEFAULT`, `INTERNAL1V1`, etc.) |
| **Time & Delays** | `delay(ms)` | Pause script execution in milliseconds |
| | `delayMicroseconds(us)` | Pause script execution in microseconds |
| | `millis()`, `micros()` | Return uptime timers (ms / us) |
| **Audio & Signals** | `tone(pin, freq, [dur])` | Generate sound frequency on a pin |
| | `noTone(pin)` | Stop active tone generation |
| | `pulseIn(pin, state)` | Measure pulse width duration |
| **Math & Utils** | `map(v, fL, fH, tL, tH)` | Map value from one range to another |
| | `constrain(v, min, max)` | Clamp value bounds within min and max |
| | `min(a,b)`, `max(a,b)`, `abs(x)` | Basic math operations |
| | `sqrt(x)` | Calculate square root |
| | `bitSet(x,n)`, `bitClear(x,n)` | Perform bitwise manipulation |
| **I2C Communication**| `Wire.begin()` | Initialize I2C bus |
| | `wireScan()` | Scan I2C bus for connected device addresses |
| **Serial & Control** | `Serial.println("text")` | Output text to serial console |
| | `if (cond) { body }` | Standard C conditional logic branching support |

---

### 💡 Quick Start Example

```bash
# 1. Create a C script in EEPROM
nano blink.c

# 2. Write code inside editor (1 line), then press ENTER to save
pinMode(13, OUTPUT);digitalWrite(13, HIGH);delay(1000);digitalWrite(13, LOW);

# 3. Execute the script
run blink.c
