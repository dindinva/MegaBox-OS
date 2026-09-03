

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


Updated ***3 sep 2026***
## 🚀 Update Notes: Command Reference & Help System Integration

We have updated the project's Help System to include a comprehensive command reference for both **Linux** shell environments and **Arduino C/C++** microcontroller programming. Users can access this documentation interactively via the built-in Serial CLI or Terminal interface using the `help`, `linux`, or `arduino` commands.

---

### 🐧 Linux Command Reference

| Category | Command | Description |
| :--- | :--- | :--- |
| **File & Directory** | `ls -la` | List all files including hidden files with detailed permissions |
| | `cd <path>` | Change working directory (`~` for home, `..` for parent) |
| | `pwd` | Print current working directory path |
| | `mkdir <dir>` | Create a new directory |
| | `rm -rf <path>` | Remove files or directories recursively and forcefully |
| | `cp <src> <dest>` | Copy files or directories |
| | `mv <src> <dest>` | Move or rename files or directories |
| | `cat <file>` | Display content of a file |
| | `nano <file>` | Open text file in Nano editor |
| **System & Hardware**| `sudo <command>` | Execute a command with superuser (root) privileges |
| | `chmod +x <file>` | Grant execution permission to a file |
| | `chown <user>:<group>`| Change file ownership |
| | `dmesg \| tail` | View recent kernel and hardware connection logs |
| | `lsusb` | List connected USB devices |
| | `ls /dev/tty*` | List available serial communication ports |

---

### ⚡ Arduino C/C++ Command Reference

| Category | Function / Command | Description |
| :--- | :--- | :--- |
| **Core Structure** | `void setup()` | Runs once when power is applied or board is reset |
| | `void loop()` | Runs continuously after setup completes |
| **Digital & Analog I/O**| `pinMode(pin, mode)` | Configure pin behavior (`INPUT`, `OUTPUT`, `INPUT_PULLUP`) |
| | `digitalWrite(pin, val)`| Write digital state (`HIGH` or `LOW`) to a pin |
| | `digitalRead(pin)` | Read digital state from a pin |
| | `analogRead(pin)` | Read analog signal value (10-bit resolution: `0 - 1023`) |
| | `analogWrite(pin, val)` | Output PWM signal (8-bit resolution: `0 - 255`) |
| **Time Control** | `delay(ms)` | Pause execution for a given time in milliseconds |
| | `delayMicroseconds(us)`| Pause execution for a given time in microseconds |
| | `millis()` | Returns total runtime in milliseconds since power-on |
| | `micros()` | Returns total runtime in microseconds since power-on |
| **Serial Control** | `Serial.begin(speed)` | Initialize serial communication rate (e.g., `9600`) |
| | `Serial.print(val)` | Print data to the Serial Monitor |
| | `Serial.println(val)` | Print data with an appended newline character |
| | `Serial.available()` | Return number of bytes available for reading in serial buffer |
| | `Serial.read()` | Read the next incoming byte from serial buffer |
| **Math & Advanced** | `map(v, fL, fH, tL, tH)`| Re-maps a number from one range to another |
| | `constrain(amt, low, high)`| Constrain a value within specified lower and upper bounds |
| | `attachInterrupt(...)` | Attach external hardware interrupt function to a pin |

---

### 🛠 How to Access Help in the Program

- **Via Serial Monitor (Arduino):** Open Serial Monitor at **9600 Baud** and send `help`, `linux`, or `arduino`.
- **Via Linux Terminal (Python CLI):** Execute the script and enter `help` at the prompt to print the full reference table.
