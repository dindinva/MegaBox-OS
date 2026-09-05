

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

📖 Manual & Help Commands
 * help: Displays all available Linux system commands and Arduino C-Script API functions.
 * help linux: Displays only the Linux system commands.
 * help arduino: Displays only the Arduino C-Script API commands and functions.
 * 
🐧 Linux System Commands

| Command | Description / Function|
| ls, ls -l | List all files stored in EEPROM |
| cat <file> | Display the contents of a file |
| touch <file> | Create a new empty file |
| rm <file> | Remove a file from EEPROM |
| cp <src> <dst> | Copy content from a source file to a destination file |
| wc <file> | Count lines, words, and characters in a file |
| pwd | Print the current working directory (/root) |
| echo <text> | Print a string to the terminal output |
| clear | Clear the terminal screen |
| free, free -h | Display RAM and EEPROM memory status |
| uptime | Show system running time since boot |
| ps, top | Show active processes and free SRAM |
| dmesg | Display system boot log |
| whoami | Show current logged-in user (root) |
| hostname | Show the device hostname (megabox) |
| uname, uname -a | Show system and Linux kernel details |
| reboot | Restart the ATmega2560 board |

🛠️ Arduino C-Script Management, Editing & Execution
 * ed <file>: Open the Unix ed line-oriented text editor (full function Unix text editor commands).
 * nano <file.c>: Open the Nano basic text editor to write a C script--(can not edit)
 * run <file.c>: Execute a C script using the built-in interpreter.
 * 
⚡ Supported C-Script API Functions
 * Digital I/O Operations:
   * pinMode(pin, mode): Set pin mode (INPUT, OUTPUT, INPUT_PULLUP).
   * digitalWrite(pin, val): Write a digital value (HIGH, LOW).
   * digitalRead(pin): Read digital signal state from a pin.
   * togglePin(pin): Invert the current output state of a digital pin.
 * Analog & PWM Operations:
   * analogWrite(pin, val): Output PWM signal on a supported pin (range 0–255).
   * analogRead(pin): Read analog input value from a pin (range 0–1023).
   * analogReference(type): Set analog reference voltage (DEFAULT, INTERNAL1V1, INTERNAL2V56, EXTERNAL).
 * System & Timing Functions:
   * delay(ms): Pause execution for specified milliseconds.
   * delayMicroseconds(us): Pause execution for specified microseconds.
   * millis(): Return system uptime in milliseconds.
   * micros(): Return system uptime in microseconds.
 * Audio & Pulse Measurement:
   * tone(pin, freq, [dur]): Generate a square wave of specified frequency on a pin (with optional duration).
   * noTone(pin): Stop tone generation on a pin.
   * pulseIn(pin, state): Measure the pulse width on a pin.
 * Math & Bit Operations:
   * map(v, fL, fH, tL, tH): Map a value from one range to another.
   * constrain(v, min, max): Clamp a value within min and max boundaries.
   * min(a, b), max(a, b), abs(x): Minimum, maximum, and absolute value functions.
   * sqrt(x): Calculate square root.
   * bitSet(x, n), bitClear(x, n): Set or clear a specific bit.
 * Communication & Bus Interfaces:
   * Wire.begin(): Initialize I2C bus as master.
   * wireScan(): Scan for connected devices on the I2C bus.
   * Serial.println("text"): Print text output to Serial port.
 * Control Structures & Loops:
   * if (cond) { body }: Conditional branching logic.
   * while (cond) { body }: Standard while loop structure.
   * do { body } while (cond): Do-while loop structure.
   * for (count) { body }: Count-based loop structure.

---

## 🚀 Quick Start & Examples

### 1. Installation
1. Open `MegaBox.ino` in the Arduino IDE.
2. Select **Board**: `Arduino Mega or Mega 2560` and choose the correct serial port.
3. Upload the sketch to your board.
4. Open **Serial Monitor** at **9600 baud** (Set line ending to 'newline').

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
Updated ***5 sep 2026***
## 🚀 Update Notes: Command Reference & Help System Integration,add ed editor and loop commands version 6.9.1

Updated ***4 sep 2026***
## 🚀 Update Notes: Command Reference & Help System Integration version 6.9.0


---

### 💡 Quick Start Example

```bash
# 1. Create a C script in EEPROM
nano blink.c

# 2. Write code inside editor (1 line), then press ENTER to save
pinMode(13, OUTPUT);digitalWrite(13, HIGH);delay(1000);digitalWrite(13, LOW);

# 3. Execute the script
run blink.c
