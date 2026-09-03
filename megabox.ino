#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#define HOSTNAME "megabox"
#define KERNEL_VER "6.8.0-mega-full"
#define MAX_CMD_LEN 160
#define MAX_FILE_SIZE 512

struct FileHeader {
  char name[16];
  uint16_t address;
  bool used;
};

#define MAX_FILES 7
FileHeader fileTable[MAX_FILES] = {
  {"gpio.c",   100,  true},
  {"pwm.c",    650,  true},
  {"sensor.c", 1200, true},
  {"serial.c", 1750, true},
  {"shift.c",  2300, true},
  {"math.c",   2850, true},
  {"logic.c",  3400, true}
};

char inputBuffer[MAX_CMD_LEN];
uint8_t bufferIndex = 0;
bool isEditing = false;
int editingFileIdx = -1;

int getFreeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void showPrompt() {
  Serial.print("\nroot@");
  Serial.print(HOSTNAME);
  Serial.print(":~# ");
}

int findFile(const char* name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (fileTable[i].used && strcmp(fileTable[i].name, name) == 0) return i;
  }
  return -1;
}

char* trim(char* str) {
  while (*str == ' ' || *str == '\t') str++;
  if (*str == 0) return str;
  char* end = str + strlen(str) - 1;
  while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n' || *end == ';')) {
    *end = 0;
    end--;
  }
  return str;
}

int parsePinOrValue(char* arg) {
  arg = trim(arg);
  if (strcmp(arg, "HIGH") == 0 || strcmp(arg, "OUTPUT") == 0 || strcmp(arg, "true") == 0) return 1;
  if (strcmp(arg, "LOW") == 0 || strcmp(arg, "INPUT") == 0 || strcmp(arg, "false") == 0) return 0;
  if (strcmp(arg, "INPUT_PULLUP") == 0) return INPUT_PULLUP;
  if (strcmp(arg, "LED_BUILTIN") == 0) return LED_BUILTIN;
  if (strcmp(arg, "MSBFIRST") == 0) return MSBFIRST;
  if (strcmp(arg, "LSBFIRST") == 0) return LSBFIRST;

  if (arg[0] == 'A' || arg[0] == 'a') {
    int aNum = atoi(arg + 1);
    if (aNum >= 0 && aNum <= 15) return A0 + aNum;
  }
  return atoi(arg);
}

// -------------------------------------------------------------------
// C-SCRIPT INTERPRETER ENGINE (EXTENDED FOR MEGA2560)
// -------------------------------------------------------------------

int evaluateExpression(char* expr) {
  expr = trim(expr);
  
  // รองรับการอ่านค่าในนิพจน์ เช่น digitalRead(2) หรือ analogRead(A0)
  if (strncmp(expr, "digitalRead", 11) == 0) {
    char* p = strchr(expr, '(');
    if (p) return digitalRead(parsePinOrValue(p + 1));
  } else if (strncmp(expr, "analogRead", 10) == 0) {
    char* p = strchr(expr, '(');
    if (p) return analogRead(parsePinOrValue(p + 1));
  } else if (strcmp(expr, "millis()") == 0) {
    return millis();
  }
  
  return parsePinOrValue(expr);
}

void executeCLine(char* line) {
  line = trim(line);
  if (strlen(line) == 0 || line[0] == '/' || line[0] == '#') return;

  // --- Inline Conditional IF Statement ---
  if (strncmp(line, "if", 2) == 0) {
    char* openParen = strchr(line, '(');
    char* closeParen = strchr(line, ')');
    if (openParen && closeParen && closeParen > openParen) {
      char condStr[60];
      uint8_t cLen = closeParen - openParen - 1;
      strncpy(condStr, openParen + 1, cLen);
      condStr[cLen] = '\0';

      char* body = closeParen + 1;
      body = trim(body);
      if (body[0] == '{') body++;
      char* closeBrace = strrchr(body, '}');
      if (closeBrace) *closeBrace = '\0';

      bool conditionMet = false;
      if (strstr(condStr, "==")) {
        char* left = strtok(condStr, "==");
        char* right = strtok(NULL, "==");
        if (left && right) conditionMet = (evaluateExpression(left) == evaluateExpression(right));
      } else if (strstr(condStr, "!=")) {
        char* left = strtok(condStr, "!=");
        char* right = strtok(NULL, "!=");
        if (left && right) conditionMet = (evaluateExpression(left) != evaluateExpression(right));
      } else if (strstr(condStr, ">=")) {
        char* left = strtok(condStr, ">=");
        char* right = strtok(NULL, ">=");
        if (left && right) conditionMet = (evaluateExpression(left) >= evaluateExpression(right));
      } else if (strstr(condStr, "<=")) {
        char* left = strtok(condStr, "<=");
        char* right = strtok(NULL, "<=");
        if (left && right) conditionMet = (evaluateExpression(left) <= evaluateExpression(right));
      } else if (strstr(condStr, ">")) {
        char* left = strtok(condStr, ">");
        char* right = strtok(NULL, ">");
        if (left && right) conditionMet = (evaluateExpression(left) > evaluateExpression(right));
      } else if (strstr(condStr, "<")) {
        char* left = strtok(condStr, "<");
        char* right = strtok(NULL, "<");
        if (left && right) conditionMet = (evaluateExpression(left) < evaluateExpression(right));
      }

      if (conditionMet) {
        executeCLine(body);
      } else {
        Serial.println(" [C Skip] IF Condition False");
      }
      return;
    }
  }

  // --- Normal Function Parser ---
  char funcName[24];
  char args[100];

  char* openParen = strchr(line, '(');
  char* closeParen = strrchr(line, ')');

  if (openParen && closeParen && closeParen > openParen) {
    uint8_t funcLen = openParen - line;
    strncpy(funcName, line, funcLen);
    funcName[funcLen] = '\0';
    trim(funcName);

    uint8_t argsLen = closeParen - openParen - 1;
    strncpy(args, openParen + 1, argsLen);
    args[argsLen] = '\0';

    char* argList[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    uint8_t argCount = 0;
    char* token = strtok(args, ",");
    while (token != NULL && argCount < 6) {
      argList[argCount++] = token;
      token = strtok(NULL, ",");
    }

    // --- Digital I/O ---
    if (strcmp(funcName, "pinMode") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int mode = parsePinOrValue(argList[1]);
      pinMode(pin, mode);
      Serial.print(" [C Exec] pinMode("); Serial.print(pin); Serial.print(", "); Serial.print(mode); Serial.println(")");
    } 
    else if (strcmp(funcName, "digitalWrite") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int val = parsePinOrValue(argList[1]);
      digitalWrite(pin, val);
      Serial.print(" [C Exec] digitalWrite("); Serial.print(pin); Serial.print(", "); Serial.print(val ? "HIGH" : "LOW"); Serial.println(")");
    } 
    else if (strcmp(funcName, "digitalRead") == 0 && argCount >= 1) {
      int pin = parsePinOrValue(argList[0]);
      int val = digitalRead(pin);
      Serial.print(" [C Read] digitalRead("); Serial.print(pin); Serial.print(") => "); Serial.println(val ? "HIGH" : "LOW");
    } 

    // --- Analog I/O ---
    else if (strcmp(funcName, "analogRead") == 0 && argCount >= 1) {
      int pin = parsePinOrValue(argList[0]);
      int val = analogRead(pin);
      Serial.print(" [C Read] analogRead("); Serial.print(argList[0]); Serial.print(") => "); Serial.print(val); Serial.print(" ("); Serial.print((val * 5.0) / 1023.0, 2); Serial.println("V)");
    } 
    else if (strcmp(funcName, "analogWrite") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int val = parsePinOrValue(argList[1]);
      analogWrite(pin, val);
      Serial.print(" [C Exec] analogWrite("); Serial.print(pin); Serial.print(", "); Serial.print(val); Serial.println(")");
    } 

    // --- Multi-Hardware Serial Control (Serial, Serial1, Serial2, Serial3) ---
    else if (strncmp(funcName, "Serial", 6) == 0) {
      HardwareSerial* targetSerial = &Serial;
      if (strncmp(funcName, "Serial1", 7) == 0) targetSerial = &Serial1;
      else if (strncmp(funcName, "Serial2", 7) == 0) targetSerial = &Serial2;
      else if (strncmp(funcName, "Serial3", 7) == 0) targetSerial = &Serial3;

      if (strstr(funcName, ".begin") && argCount >= 1) {
        long baud = atol(argList[0]);
        targetSerial->begin(baud);
        Serial.print(" [C Exec] "); Serial.print(funcName); Serial.print("("); Serial.print(baud); Serial.println(")");
      } else if (strstr(funcName, ".println") && argCount >= 1) {
        char* printStr = trim(argList[0]);
        if (printStr[0] == '"') printStr++;
        if (printStr[strlen(printStr) - 1] == '"') printStr[strlen(printStr) - 1] = '\0';
        targetSerial->println(printStr);
      }
    }

    // --- Advanced Shift Register I/O ---
    else if (strcmp(funcName, "shiftOut") == 0 && argCount >= 4) {
      int dataPin = parsePinOrValue(argList[0]);
      int clockPin = parsePinOrValue(argList[1]);
      int bitOrder = parsePinOrValue(argList[2]);
      byte val = parsePinOrValue(argList[3]);
      shiftOut(dataPin, clockPin, bitOrder, val);
      Serial.print(" [C Exec] shiftOut val="); Serial.println(val);
    }
    else if (strcmp(funcName, "shiftIn") == 0 && argCount >= 3) {
      int dataPin = parsePinOrValue(argList[0]);
      int clockPin = parsePinOrValue(argList[1]);
      int bitOrder = parsePinOrValue(argList[2]);
      byte val = shiftIn(dataPin, clockPin, bitOrder);
      Serial.print(" [C Read] shiftIn => "); Serial.println(val);
    }

    // --- Math & Mapping Functions ---
    else if (strcmp(funcName, "map") == 0 && argCount >= 5) {
      long val = parsePinOrValue(argList[0]);
      long in_min = parsePinOrValue(argList[1]);
      long in_max = parsePinOrValue(argList[2]);
      long out_min = parsePinOrValue(argList[3]);
      long out_max = parsePinOrValue(argList[4]);
      long result = map(val, in_min, in_max, out_min, out_max);
      Serial.print(" [C Math] map => "); Serial.println(result);
    }
    else if (strcmp(funcName, "constrain") == 0 && argCount >= 3) {
      long val = parsePinOrValue(argList[0]);
      long a = parsePinOrValue(argList[1]);
      long b = parsePinOrValue(argList[2]);
      long result = constrain(val, a, b);
      Serial.print(" [C Math] constrain => "); Serial.println(result);
    }

    // --- Sound Functions ---
    else if (strcmp(funcName, "tone") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      unsigned int freq = parsePinOrValue(argList[1]);
      if (argCount >= 3) tone(pin, freq, parsePinOrValue(argList[2]));
      else tone(pin, freq);
      Serial.print(" [C Exec] tone("); Serial.print(pin); Serial.print(", "); Serial.print(freq); Serial.println("Hz)");
    } 
    else if (strcmp(funcName, "noTone") == 0 && argCount >= 1) {
      noTone(parsePinOrValue(argList[0]));
      Serial.println(" [C Exec] noTone()");
    } 

    // --- Delays & Time ---
    else if (strcmp(funcName, "delay") == 0 && argCount >= 1) {
      delay(parsePinOrValue(argList[0]));
    } 
    else if (strcmp(funcName, "delayMicroseconds") == 0 && argCount >= 1) {
      delayMicroseconds(parsePinOrValue(argList[0]));
    } 
    else if (strcmp(funcName, "millis") == 0) {
      Serial.print(" [C Time] millis => "); Serial.println(millis());
    }
    else {
      Serial.print(" [C Error] Unknown function: "); Serial.println(funcName);
    }
  }
}

void runCScript(const char* filename) {
  int idx = findFile(filename);
  if (idx == -1) {
    Serial.print("\nrun: "); Serial.print(filename); Serial.println(": File not found");
    return;
  }

  uint16_t addr = fileTable[idx].address;
  uint16_t len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  if (len == 0xFFFF || len == 0) {
    Serial.println("\nrun: Empty program file");
    return;
  }

  Serial.print("\nExecuting C Script '"); Serial.print(filename); Serial.println("' on ATmega2560...\n");

  char lineBuffer[MAX_CMD_LEN];
  uint8_t lineIdx = 0;

  for (uint16_t i = 0; i < len; i++) {
    char c = EEPROM.read(addr + 2 + i);
    if (c == '\n' || c == '\r') {
      lineBuffer[lineIdx] = '\0';
      executeCLine(lineBuffer);
      lineIdx = 0;
    } else {
      if (lineIdx < sizeof(lineBuffer) - 1) lineBuffer[lineIdx++] = c;
    }
  }

  if (lineIdx > 0) {
    lineBuffer[lineIdx] = '\0';
    executeCLine(lineBuffer);
  }

  Serial.println("\n[ Execution Finished ]");
}

// -------------------------------------------------------------------
// SYSTEM SHELL & HARDWARE DIAGNOSTICS
// -------------------------------------------------------------------

void cmdI2cScan() {
  Serial.println("\nScanning I2C bus (SDA: Pin 20, SCL: Pin 21)...");
  byte count = 0;
  Wire.begin();
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print(" - Found I2C device at address 0x");
      if (i < 16) Serial.print("0");
      Serial.println(i, HEX);
      count++;
    }
  }
  if (count == 0) Serial.println("No I2C devices found.");
  else { Serial.print("Total devices: "); Serial.println(count); }
}

void cmdPinState() {
  Serial.println("\n--- Arduino Mega Digital Pin Monitor (0-53) ---");
  for (int i = 0; i <= 53; i++) {
    Serial.print("P"); if (i < 10) Serial.print("0");
    Serial.print(i); Serial.print(": ");
    Serial.print(digitalRead(i) ? "HIGH " : "LOW  ");
    if ((i + 1) % 6 == 0) Serial.println();
    else Serial.print(" | ");
  }
  Serial.println();
}

void cmdLs() {
  Serial.println("\ntotal files");
  for (int i = 0; i < MAX_FILES; i++) {
    if (fileTable[i].used) {
      Serial.print("-rw-r--r-- 1 root root  ");
      uint16_t len = (EEPROM.read(fileTable[i].address) << 8) | EEPROM.read(fileTable[i].address + 1);
      if (len == 0xFFFF) len = 0;
      if (len < 10) Serial.print("   ");
      else if (len < 100) Serial.print("  ");
      else Serial.print(" ");
      Serial.print(len);
      Serial.print(" Sep  3 15:30 ");
      Serial.println(fileTable[i].name);
    }
  }
}

void cmdFree() {
  int freeRam = getFreeRam();
  int usedRam = 8192 - freeRam;
  Serial.println("\n               total        used        free      shared  buff/cache   available");
  Serial.print("Mem:           8.0Ki       ");
  Serial.print((float)usedRam / 1024.0, 1);
  Serial.print("Ki       ");
  Serial.print((float)freeRam / 1024.0, 1);
  Serial.println("Ki       0.0Ki       0.0Ki       ");
  Serial.println("EEPROM:        4.0Ki (Internal)");
}

void startNano(const char* filename) {
  int idx = findFile(filename);
  if (idx == -1) {
    for (int i = 0; i < MAX_FILES; i++) {
      if (!fileTable[i].used) {
        strncpy(fileTable[i].name, filename, 15);
        fileTable[i].used = true;
        idx = i;
        break;
      }
    }
  }

  if (idx == -1) {
    Serial.println("\nnano: File system full");
    return;
  }

  editingFileIdx = idx;
  isEditing = true;

  Serial.print("\033[2J\033[H");
  Serial.print("  GNU nano C-Editor (Mega2560)   File: ");
  Serial.println(fileTable[idx].name);
  Serial.println("----------------------------------------------------------------");
  Serial.println("[ Type C Code and press ENTER to save & exit ]\n");
  Serial.print("> ");
}

void saveAndExitNano(char* text) {
  uint16_t addr = fileTable[editingFileIdx].address;
  uint16_t len = strlen(text);
  if (len > MAX_FILE_SIZE - 2) len = MAX_FILE_SIZE - 2;

  EEPROM.write(addr, (len >> 8) & 0xFF);
  EEPROM.write(addr + 1, len & 0xFF);

  for (uint16_t i = 0; i < len; i++) {
    EEPROM.write(addr + 2 + i, text[i]);
  }

  Serial.println("\n[ Script saved to EEPROM ]");
  isEditing = false;
  editingFileIdx = -1;
  showPrompt();
}

void processCommand(char* cmd) {
  int len = strlen(cmd);
  while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\r' || cmd[len - 1] == '\n')) {
    cmd[--len] = '\0';
  }

  if (strlen(cmd) == 0) return;

  if (strcmp(cmd, "clear") == 0) Serial.print("\033[2J\033[H");
  else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "ls -l") == 0) cmdLs();
  else if (strcmp(cmd, "free") == 0 || strcmp(cmd, "free -h") == 0) cmdFree();
  else if (strcmp(cmd, "i2cscan") == 0) cmdI2cScan();
  else if (strcmp(cmd, "pinstate") == 0) cmdPinState();
  else if (strncmp(cmd, "nano ", 5) == 0) startNano(cmd + 5);
  else if (strncmp(cmd, "run ", 4) == 0) runCScript(cmd + 4);
  else if (strcmp(cmd, "uname") == 0 || strcmp(cmd, "uname -a") == 0) {
    Serial.print("\nLinux "); Serial.print(HOSTNAME); Serial.print(" "); Serial.print(KERNEL_VER);
    Serial.println(" #1 SMP PREEMPT Thu Sep 03 2026 avr2560 GNU/Linux");
  } else if (strcmp(cmd, "help") == 0) {
    Serial.println("\nSystem Commands:");
    Serial.println("  ls, free, uname, i2cscan, pinstate, nano <file.c>, run <file.c>, clear");
    Serial.println("\nSupported C Functions in Interpreter:");
    Serial.println("  I/O     : pinMode, digitalWrite, digitalRead, analogRead, analogWrite");
    Serial.println("  Shift   : shiftOut, shiftIn");
    Serial.println("  Logic   : if (digitalRead(pin) == HIGH) { ... }");
    Serial.println("  Math    : map, constrain, millis");
    Serial.println("  Sound   : tone, noTone");
    Serial.println("  Serial  : Serial.begin, Serial1.println, Serial2.println, Serial3.println");
  } else {
    Serial.print("\nsh: "); Serial.print(cmd); Serial.println(": command not found");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("\n[    0.000000] Booting MegaBox OS (Full C Engine) on ATmega2560");
  Serial.println("[    0.000084] Linux version " KERNEL_VER);
  Serial.println("[    0.001200] Memory: 8KB SRAM, 256KB Flash, 4KB EEPROM");
  Serial.println("[    0.002000] Initializing Serial1, Serial2, Serial3 interfaces");
  Serial.println("[    0.005400] Mounting internal 4KB EEPROM as /dev/eeprom");
  Serial.println("\nWelcome to MegaBox Linux C-Interpreter OS!");
  Serial.println("Type 'help' for available commands.");
  showPrompt();
}

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (isEditing) {
      if (c != '\r' && c != '\n') Serial.print(c);
      if (c == '\r' || c == '\n') {
        inputBuffer[bufferIndex] = '\0';
        saveAndExitNano(inputBuffer);
        bufferIndex = 0;
      } else if (c == '\b' || c == 127) {
        if (bufferIndex > 0) { bufferIndex--; Serial.print("\b \b"); }
      } else {
        if (bufferIndex < MAX_CMD_LEN - 1) inputBuffer[bufferIndex++] = c;
      }
      continue;
    }

    if (c != '\r' && c != '\n') Serial.print(c);

    if (c == '\r' || c == '\n') {
      inputBuffer[bufferIndex] = '\0';
      processCommand(inputBuffer);
      bufferIndex = 0;
      showPrompt();
    } else if (c == '\b' || c == 127) {
      if (bufferIndex > 0) { bufferIndex--; Serial.print("\b \b"); }
    } else {
      if (bufferIndex < MAX_CMD_LEN - 1) inputBuffer[bufferIndex++] = c;
    }
  }
}
