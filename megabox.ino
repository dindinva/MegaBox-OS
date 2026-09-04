#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#define HOSTNAME "megabox"
#define KERNEL_VER "6.9.0-mega-full-linux"
#define MAX_CMD_LEN 160
#define MAX_FILE_SIZE 512

struct FileHeader {
  char name[16];
  uint16_t address;
  bool used;
};

#define MAX_FILES 8
FileHeader fileTable[MAX_FILES] = {
  {"gpio.c",   100,  true},
  {"pwm.c",    650,  true},
  {"sensor.c", 1200, true},
  {"serial.c", 1750, true},
  {"i2c.c",    2300, true},
  {"math.c",   2850, true},
  {"test.c",   3400, false},
  {"test2.c",  3900, false}
};

char inputBuffer[MAX_CMD_LEN];
uint8_t bufferIndex = 0;
bool isEditing = false;
int editingFileIdx = -1;

void (*resetFunc)(void) = 0; // Soft reset pointer

int getFreeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void showPrompt() {
  Serial.print(F("\nroot@"));
  Serial.print(HOSTNAME);
  Serial.print(F(":~# "));
}

int findFile(const char* name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (fileTable[i].used && strcmp(fileTable[i].name, name) == 0) return i;
  }
  return -1;
}

// ปรับปรุง trim() ให้ลบเฉพาะ ; ปิดท้ายคำสั่งเดี่ยว ไม่ลบทั้งสายคำสั่ง
char* trim(char* str) {
  while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
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
  if (strcmp(arg, "DEFAULT") == 0) return DEFAULT;
  if (strcmp(arg, "INTERNAL1V1") == 0) return INTERNAL1V1;
  if (strcmp(arg, "INTERNAL2V56") == 0) return INTERNAL2V56;
  if (strcmp(arg, "EXTERNAL") == 0) return EXTERNAL;

  if (arg[0] == 'A' || arg[0] == 'a') {
    int aNum = atoi(arg + 1);
    if (aNum >= 0 && aNum <= 15) return A0 + aNum;
  }
  return atoi(arg);
}

// -------------------------------------------------------------------
// EXTENDED ARDUINO C-SCRIPT INTERPRETER ENGINE
// -------------------------------------------------------------------

int evaluateExpression(char* expr) {
  expr = trim(expr);

  if (strncmp(expr, "digitalRead", 11) == 0) {
    char* p = strchr(expr, '(');
    if (p) return digitalRead(parsePinOrValue(p + 1));
  } 
  else if (strncmp(expr, "analogRead", 10) == 0) {
    char* p = strchr(expr, '(');
    if (p) return analogRead(parsePinOrValue(p + 1));
  } 
  else if (strcmp(expr, "millis()") == 0) {
    return millis();
  } 
  else if (strcmp(expr, "micros()") == 0) {
    return micros();
  }
  else if (strncmp(expr, "min", 3) == 0) {
    char* p = strchr(expr, '(');
    if (p) {
      char* arg1 = strtok(p + 1, ",");
      char* arg2 = strtok(NULL, ")");
      if (arg1 && arg2) return min(parsePinOrValue(arg1), parsePinOrValue(arg2));
    }
  }
  else if (strncmp(expr, "max", 3) == 0) {
    char* p = strchr(expr, '(');
    if (p) {
      char* arg1 = strtok(p + 1, ",");
      char* arg2 = strtok(NULL, ")");
      if (arg1 && arg2) return max(parsePinOrValue(arg1), parsePinOrValue(arg2));
    }
  }
  else if (strncmp(expr, "abs", 3) == 0) {
    char* p = strchr(expr, '(');
    if (p) return abs(parsePinOrValue(p + 1));
  }
  else if (strncmp(expr, "sqrt", 4) == 0) {
    char* p = strchr(expr, '(');
    if (p) return (int)sqrt(parsePinOrValue(p + 1));
  }

  return parsePinOrValue(expr);
}

void executeCLine(char* line) {
  line = trim(line);
  if (strlen(line) == 0 || line[0] == '/' || line[0] == '#') return;

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

      if (conditionMet) executeCLine(body);
      else Serial.println(F(" [C Skip] IF Condition False"));
      return;
    }
  }

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

    // 1. TIMING & DELAYS
    if (strcmp(funcName, "delay") == 0 && argCount >= 1) {
      int ms = evaluateExpression(argList[0]);
      Serial.print(F(" [C Exec] delay(")); Serial.print(ms); Serial.println(F(" ms)"));
      delay(ms);
    } 
    else if (strcmp(funcName, "delayMicroseconds") == 0 && argCount >= 1) {
      int us = evaluateExpression(argList[0]);
      Serial.print(F(" [C Exec] delayMicroseconds(")); Serial.print(us); Serial.println(F(" us)"));
      delayMicroseconds(us);
    }

    // 2. DIGITAL & ANALOG I/O
    else if (strcmp(funcName, "pinMode") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int mode = parsePinOrValue(argList[1]);
      pinMode(pin, mode);
      Serial.print(F(" [C Exec] pinMode(")); Serial.print(pin); Serial.print(F(", ")); Serial.print(mode); Serial.println(F(")"));
    } 
    else if (strcmp(funcName, "digitalWrite") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int val = parsePinOrValue(argList[1]);
      digitalWrite(pin, val);
      Serial.print(F(" [C Exec] digitalWrite(")); Serial.print(pin); Serial.print(F(", ")); Serial.print(val ? F("HIGH") : F("LOW")); Serial.println(F(")"));
    } 
    else if (strcmp(funcName, "digitalRead") == 0 && argCount >= 1) {
      int pin = parsePinOrValue(argList[0]);
      int val = digitalRead(pin);
      Serial.print(F(" [C Read] digitalRead(")); Serial.print(pin); Serial.print(F(") => ")); Serial.println(val ? F("HIGH") : F("LOW"));
    } 
    else if (strcmp(funcName, "togglePin") == 0 && argCount >= 1) {
      int pin = parsePinOrValue(argList[0]);
      int currentVal = digitalRead(pin);
      digitalWrite(pin, !currentVal);
      Serial.print(F(" [C Exec] togglePin(")); Serial.print(pin); Serial.print(F(") => ")); Serial.println(!currentVal ? F("HIGH") : F("LOW"));
    }
    else if (strcmp(funcName, "analogRead") == 0 && argCount >= 1) {
      int pin = parsePinOrValue(argList[0]);
      int val = analogRead(pin);
      Serial.print(F(" [C Read] analogRead(")); Serial.print(argList[0]); Serial.print(F(") => ")); Serial.print(val); Serial.print(F(" (")); Serial.print((val * 5.0) / 1023.0, 2); Serial.println(F("V)"));
    } 
    else if (strcmp(funcName, "analogWrite") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int val = parsePinOrValue(argList[1]);
      analogWrite(pin, val);
      Serial.print(F(" [C Exec] analogWrite(")); Serial.print(pin); Serial.print(F(", ")); Serial.print(val); Serial.println(F(")"));
    } 
    else if (strcmp(funcName, "analogReference") == 0 && argCount >= 1) {
      int type = parsePinOrValue(argList[0]);
      analogReference(type);
      Serial.print(F(" [C Exec] analogReference(")); Serial.print(type); Serial.println(F(")"));
    }

    // 3. ADVANCED SOUND & PULSE
    else if (strcmp(funcName, "tone") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int freq = parsePinOrValue(argList[1]);
      if (argCount >= 3) {
        int duration = parsePinOrValue(argList[2]);
        tone(pin, freq, duration);
      } else {
        tone(pin, freq);
      }
      Serial.print(F(" [C Exec] tone(")); Serial.print(pin); Serial.print(F(", ")); Serial.print(freq); Serial.println(F("Hz)"));
    }
    else if (strcmp(funcName, "noTone") == 0 && argCount >= 1) {
      int pin = parsePinOrValue(argList[0]);
      noTone(pin);
      Serial.print(F(" [C Exec] noTone(")); Serial.print(pin); Serial.println(F(")"));
    }
    else if (strcmp(funcName, "pulseIn") == 0 && argCount >= 2) {
      int pin = parsePinOrValue(argList[0]);
      int state = parsePinOrValue(argList[1]);
      unsigned long duration = pulseIn(pin, state);
      Serial.print(F(" [C Exec] pulseIn(")); Serial.print(pin); Serial.print(F(") => ")); Serial.print(duration); Serial.println(F(" us"));
    }

    // 4. MATH & BITWISE UTILITIES
    else if (strcmp(funcName, "map") == 0 && argCount >= 5) {
      long val = parsePinOrValue(argList[0]);
      long in_min = parsePinOrValue(argList[1]);
      long in_max = parsePinOrValue(argList[2]);
      long out_min = parsePinOrValue(argList[3]);
      long out_max = parsePinOrValue(argList[4]);
      long result = map(val, in_min, in_max, out_min, out_max);
      Serial.print(F(" [C Math] map => ")); Serial.println(result);
    }
    else if (strcmp(funcName, "constrain") == 0 && argCount >= 3) {
      long val = parsePinOrValue(argList[0]);
      long a = parsePinOrValue(argList[1]);
      long b = parsePinOrValue(argList[2]);
      Serial.print(F(" [C Math] constrain => ")); Serial.println(constrain(val, a, b));
    }
    else if (strcmp(funcName, "bitSet") == 0 && argCount >= 2) {
      int val = parsePinOrValue(argList[0]);
      int bit = parsePinOrValue(argList[1]);
      bitSet(val, bit);
      Serial.print(F(" [C Bit] bitSet => ")); Serial.println(val);
    }
    else if (strcmp(funcName, "bitClear") == 0 && argCount >= 2) {
      int val = parsePinOrValue(argList[0]);
      int bit = parsePinOrValue(argList[1]);
      bitClear(val, bit);
      Serial.print(F(" [C Bit] bitClear => ")); Serial.println(val);
    }

    // 5. I2C HARDWARE BUS
    else if (strcmp(funcName, "Wire.begin") == 0) {
      Wire.begin();
      Serial.println(F(" [C Exec] Wire.begin() (I2C Master Initialized)"));
    }
    else if (strcmp(funcName, "wireScan") == 0) {
      Serial.println(F(" [C Exec] Scanning I2C bus..."));
      Wire.begin();
      byte error, address;
      int nDevices = 0;
      for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
          Serial.print(F("  -> Found I2C device at 0x"));
          if (address < 16) Serial.print("0");
          Serial.println(address, HEX);
          nDevices++;
        }
      }
      if (nDevices == 0) Serial.println(F("  -> No I2C devices found"));
    }

    // 6. SERIAL I/O
    else if (strncmp(funcName, "Serial", 6) == 0) {
      HardwareSerial* targetSerial = &Serial;
      if (strncmp(funcName, "Serial1", 7) == 0) targetSerial = &Serial1;
      else if (strncmp(funcName, "Serial2", 7) == 0) targetSerial = &Serial2;
      else if (strncmp(funcName, "Serial3", 7) == 0) targetSerial = &Serial3;

      if (strstr(funcName, ".begin") && argCount >= 1) {
        long baud = atol(argList[0]);
        targetSerial->begin(baud);
        Serial.print(F(" [C Exec] ")); Serial.print(funcName); Serial.print(F("(")); Serial.print(baud); Serial.println(F(")"));
      } else if (strstr(strstr(funcName, ".print") ? funcName : "", "ln") && argCount >= 1) {
        char* printStr = trim(argList[0]);
        if (printStr[0] == '"') printStr++;
        if (printStr[strlen(printStr) - 1] == '"') printStr[strlen(printStr) - 1] = '\0';
        targetSerial->println(printStr);
      }
    }
    else {
      Serial.print(F(" [C Error] Unknown or unsupported function: ")); Serial.println(funcName);
    }
  }
}

// แก้ไขฟังก์ชันรันไฟล์ให้อ่านและคั่นด้วยทั้ง \n, \r และ ;
void runCScript(const char* filename) {
  int idx = findFile(filename);
  if (idx == -1) {
    Serial.print(F("run: ")); Serial.print(filename); Serial.println(F(": File not found"));
    return;
  }

  uint16_t addr = fileTable[idx].address;
  uint16_t len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  if (len == 0xFFFF || len == 0) {
    Serial.println(F("run: Empty program file"));
    return;
  }

  Serial.print(F("Executing C Script '")); Serial.print(filename); Serial.println(F("'...\n"));

  char lineBuffer[MAX_CMD_LEN];
  uint8_t lineIdx = 0;
  bool inQuotes = false;

  for (uint16_t i = 0; i < len; i++) {
    char c = EEPROM.read(addr + 2 + i);

    if (c == '"') {
      inQuotes = !inQuotes;
    }

    // หากเจอ \n, \r หรือ ; (ขณะไม่ได้อยู่ในเครื่องหมายคำพูด ") ให้รันคำสั่งทันที
    if ((c == '\n' || c == '\r' || (c == ';' && !inQuotes))) {
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

  Serial.println(F("\n[ Execution Finished ]"));
}

// -------------------------------------------------------------------
// LINUX COMMAND HANDLERS
// -------------------------------------------------------------------

void cmdCat(const char* filename) {
  int idx = findFile(filename);
  if (idx == -1) {
    Serial.print(F("cat: ")); Serial.print(filename); Serial.println(F(": No such file"));
    return;
  }
  uint16_t addr = fileTable[idx].address;
  uint16_t len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  if (len == 0xFFFF || len == 0) return;

  for (uint16_t i = 0; i < len; i++) {
    Serial.print((char)EEPROM.read(addr + 2 + i));
  }
  Serial.println();
}

void cmdRm(const char* filename) {
  int idx = findFile(filename);
  if (idx == -1) {
    Serial.print(F("rm: cannot remove '")); Serial.print(filename); Serial.println(F("': No such file"));
    return;
  }
  fileTable[idx].used = false;
  uint16_t addr = fileTable[idx].address;
  EEPROM.write(addr, 0);
  EEPROM.write(addr + 1, 0);
  Serial.print(F("removed '")); Serial.print(filename); Serial.println(F("'"));
}

void cmdTouch(const char* filename) {
  int idx = findFile(filename);
  if (idx != -1) return;

  for (int i = 0; i < MAX_FILES; i++) {
    if (!fileTable[i].used) {
      strncpy(fileTable[i].name, filename, 15);
      fileTable[i].used = true;
      uint16_t addr = fileTable[i].address;
      EEPROM.write(addr, 0);
      EEPROM.write(addr + 1, 0);
      return;
    }
  }
  Serial.println(F("touch: File system full"));
}

void cmdCp(char* args) {
  char* src = strtok(args, " ");
  char* dst = strtok(NULL, " ");
  if (!src || !dst) {
    Serial.println(F("cp: missing file operand"));
    return;
  }
  int sIdx = findFile(src);
  if (sIdx == -1) {
    Serial.print(F("cp: cannot stat '")); Serial.print(src); Serial.println(F("': No such file"));
    return;
  }
  cmdTouch(dst);
  int dIdx = findFile(dst);
  if (dIdx == -1) return;

  uint16_t sAddr = fileTable[sIdx].address;
  uint16_t dAddr = fileTable[dIdx].address;
  uint16_t len = (EEPROM.read(sAddr) << 8) | EEPROM.read(sAddr + 1);

  for (uint16_t i = 0; i < len + 2; i++) {
    EEPROM.write(dAddr + i, EEPROM.read(sAddr + i));
  }
}

void cmdWc(const char* filename) {
  int idx = findFile(filename);
  if (idx == -1) {
    Serial.print(F("wc: ")); Serial.print(filename); Serial.println(F(": No such file"));
    return;
  }
  uint16_t addr = fileTable[idx].address;
  uint16_t len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  if (len == 0xFFFF) len = 0;

  int lines = 0, words = 0;
  bool inWord = false;

  for (uint16_t i = 0; i < len; i++) {
    char c = EEPROM.read(addr + 2 + i);
    if (c == '\n') lines++;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      inWord = false;
    } else if (!inWord) {
      inWord = true;
      words++;
    }
  }
  Serial.print(lines); Serial.print(F("  "));
  Serial.print(words); Serial.print(F("  "));
  Serial.print(len); Serial.print(F(" "));
  Serial.println(fileTable[idx].name);
}

void cmdUptime() {
  unsigned long sec = millis() / 1000;
  unsigned long min = sec / 60;
  unsigned long hr = min / 60;
  unsigned long days = hr / 24;

  Serial.print(F("up "));
  if (days > 0) { Serial.print(days); Serial.print(F(" days, ")); }
  Serial.print(hr % 24); Serial.print(F(" hours, "));
  Serial.print(min % 60); Serial.print(F(" minutes, "));
  Serial.print(sec % 60); Serial.println(F(" seconds"));
}

void cmdPsTop() {
  Serial.println(F("\nPID TTY      TIME CMD"));
  Serial.println(F("  1 pts/0    00:00 init [systemd]"));
  Serial.println(F("  2 pts/0    00:00 kthreadd"));
  Serial.println(F("  7 pts/0    00:01 mega-interpreter"));
  Serial.print(F(" 42 pts/0    00:00 bash (sh)"));
  Serial.print(F(" [Free SRAM: ")); Serial.print(getFreeRam()); Serial.println(F(" Bytes]"));
}

void cmdDmesg() {
  Serial.println(F("[    0.000000] Booting MegaBox OS (Linux Subsystem) on ATmega2560"));
  Serial.println(F("[    0.000084] Linux version " KERNEL_VER));
  Serial.println(F("[    0.001200] Memory: 8KB SRAM, 256KB Flash, 4KB EEPROM"));
  Serial.println(F("[    0.002000] Initializing Serial, Serial1, Serial2, Serial3 interfaces"));
  Serial.println(F("[    0.005400] Mounting internal 4KB EEPROM as /dev/eeprom"));
}

void cmdLs() {
  Serial.println(F("\ntotal files"));
  for (int i = 0; i < MAX_FILES; i++) {
    if (fileTable[i].used) {
      Serial.print(F("-rw-r--r-- 1 root root  "));
      uint16_t len = (EEPROM.read(fileTable[i].address) << 8) | EEPROM.read(fileTable[i].address + 1);
      if (len == 0xFFFF) len = 0;
      if (len < 10) Serial.print(F("   "));
      else if (len < 100) Serial.print(F("  "));
      else Serial.print(F(" "));
      Serial.print(len);
      Serial.print(F(" Sep  3 2026 "));
      Serial.println(fileTable[i].name);
    }
  }
}

void cmdFree() {
  int freeRam = getFreeRam();
  int usedRam = 8192 - freeRam;
  Serial.println(F("\n               total        used        free      shared  buff/cache   available"));
  Serial.print(F("Mem:           8.0Ki       "));
  Serial.print((float)usedRam / 1024.0, 1);
  Serial.print(F("Ki       "));
  Serial.print((float)freeRam / 1024.0, 1);
  Serial.println(F("Ki       0.0Ki       0.0Ki       "));
  Serial.println(F("EEPROM:        4.0Ki (Internal)"));
}

void printLinuxHelp() {
  Serial.println(F("\n=========================================="));
  Serial.println(F("         LINUX SYSTEM COMMANDS            "));
  Serial.println(F("=========================================="));
  Serial.println(F("  ls, ls -l       : List files in EEPROM"));
  Serial.println(F("  cat <file>      : Display content of a file"));
  Serial.println(F("  touch <file>    : Create a new empty file"));
  Serial.println(F("  rm <file>       : Remove file from EEPROM"));
  Serial.println(F("  cp <src> <dst>  : Copy file content"));
  Serial.println(F("  wc <file>       : Count lines, words, chars"));
  Serial.println(F("  pwd             : Show current working directory"));
  Serial.println(F("  echo <text>     : Print string to terminal"));
  Serial.println(F("  clear           : Clear terminal screen"));
  Serial.println(F("  free, free -h   : Display RAM and EEPROM status"));
  Serial.println(F("  uptime          : Show system running time"));
  Serial.println(F("  ps, top         : Show running tasks & SRAM"));
  Serial.println(F("  dmesg           : Show system boot log"));
  Serial.println(F("  whoami          : Show current user (root)"));
  Serial.println(F("  hostname        : Show device hostname"));
  Serial.println(F("  uname -a        : Show Linux kernel details"));
  Serial.println(F("  reboot          : Restart ATmega2560 board"));
}

void printArduinoHelp() {
  Serial.println(F("\n=========================================="));
  Serial.println(F("      ARDUINO C-SCRIPT API COMMANDS       "));
  Serial.println(F("=========================================="));
  Serial.println(F("  nano <file.c>   : Open editor to edit C script"));
  Serial.println(F("  run <file.c>    : Execute C script interpreter"));
  Serial.println(F("\n[ Supported Functions inside C Scripts ]"));
  Serial.println(F("  pinMode(pin, mode)         : INPUT, OUTPUT, INPUT_PULLUP"));
  Serial.println(F("  digitalWrite(pin, val)     : HIGH, LOW"));
  Serial.println(F("  digitalRead(pin)           : Read digital pin"));
  Serial.println(F("  togglePin(pin)             : Invert pin state"));
  Serial.println(F("  analogWrite(pin, val)      : PWM output (0-255)"));
  Serial.println(F("  analogRead(pin)            : Read pin (0-1023)"));
  Serial.println(F("  analogReference(type)      : DEFAULT, INTERNAL1V1..."));
  Serial.println(F("  delay(ms)                  : Delay execution (ms)"));
  Serial.println(F("  delayMicroseconds(us)      : Delay execution (us)"));
  Serial.println(F("  millis(), micros()         : System uptime timers"));
  Serial.println(F("  tone(pin, freq, [dur])     : Generate sound frequency"));
  Serial.println(F("  noTone(pin)                : Stop tone frequency"));
  Serial.println(F("  pulseIn(pin, state)        : Measure pulse width"));
  Serial.println(F("  map(v, fL, fH, tL, tH)     : Map value range"));
  Serial.println(F("  constrain(v, min, max)     : Clamp value bounds"));
  Serial.println(F("  min(a,b), max(a,b), abs(x) : Math operations"));
  Serial.println(F("  sqrt(x)                    : Square root math"));
  Serial.println(F("  bitSet(x,n), bitClear(x,n) : Bit manipulation"));
  Serial.println(F("  Wire.begin()               : Initialize I2C bus"));
  Serial.println(F("  wireScan()                 : Scan for I2C addresses"));
  Serial.println(F("  Serial.println(\"text\")     : Serial output"));
  Serial.println(F("  if (cond) { body }         : Logic branch support"));
  Serial.println(F("==========================================\n"));
}

void startNano(const char* filename) {
  cmdTouch(filename);
  int idx = findFile(filename);
  if (idx == -1) return;

  editingFileIdx = idx;
  isEditing = true;

  Serial.print(F("\033[2J\033[H"));
  Serial.print(F("  GNU nano C-Editor (Mega2560)   File: "));
  Serial.println(fileTable[idx].name);
  Serial.println(F("----------------------------------------------------------------"));
  Serial.println(F("[ Type C Code and press ENTER to save & exit ]\n"));
  Serial.print(F("> "));
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

  Serial.println(F("\n[ Script saved to EEPROM ]"));
  isEditing = false;
  editingFileIdx = -1;
  showPrompt();
}

void processCommand(char* cmd) {
  cmd = trim(cmd);
  if (strlen(cmd) == 0) return;

  if (strcmp(cmd, "clear") == 0) Serial.print(F("\033[2J\033[H"));
  else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "ls -l") == 0) cmdLs();
  else if (strcmp(cmd, "free") == 0 || strcmp(cmd, "free -h") == 0) cmdFree();
  else if (strcmp(cmd, "pwd") == 0) Serial.println(F("/root"));
  else if (strcmp(cmd, "whoami") == 0) Serial.println(F("root"));
  else if (strcmp(cmd, "hostname") == 0) Serial.println(HOSTNAME);
  else if (strcmp(cmd, "uptime") == 0) cmdUptime();
  else if (strcmp(cmd, "ps") == 0 || strcmp(cmd, "top") == 0) cmdPsTop();
  else if (strcmp(cmd, "dmesg") == 0) cmdDmesg();
  else if (strcmp(cmd, "reboot") == 0) { Serial.println(F("Rebooting system...")); delay(500); resetFunc(); }
  else if (strncmp(cmd, "echo ", 5) == 0) Serial.println(cmd + 5);
  else if (strncmp(cmd, "cat ", 4) == 0) cmdCat(cmd + 4);
  else if (strncmp(cmd, "rm ", 3) == 0) cmdRm(cmd + 3);
  else if (strncmp(cmd, "touch ", 6) == 0) cmdTouch(cmd + 6);
  else if (strncmp(cmd, "wc ", 3) == 0) cmdWc(cmd + 3);
  else if (strncmp(cmd, "cp ", 3) == 0) cmdCp(cmd + 3);
  else if (strncmp(cmd, "nano ", 5) == 0) startNano(cmd + 5);
  else if (strncmp(cmd, "run ", 4) == 0) runCScript(cmd + 4);
  else if (strcmp(cmd, "uname") == 0 || strcmp(cmd, "uname -a") == 0) {
    Serial.print(F("Linux ")); Serial.print(HOSTNAME); Serial.print(F(" ")); Serial.print(KERNEL_VER);
    Serial.println(F(" #1 SMP PREEMPT Thu Sep 03 2026 avr2560 GNU/Linux"));
  } 
  else if (strcmp(cmd, "help") == 0) {
    printLinuxHelp();
    printArduinoHelp();
  } 
  else if (strcmp(cmd, "help linux") == 0) {
    printLinuxHelp();
  } 
  else if (strcmp(cmd, "help arduino") == 0) {
    printArduinoHelp();
  } 
  else {
    Serial.print(F("sh: ")); Serial.print(cmd); Serial.println(F(": command not found"));
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  cmdDmesg();
  Serial.println(F("\nWelcome to MegaBox Linux C-Interpreter OS!"));
  Serial.println(F("Type 'help', 'help linux', or 'help arduino' for available commands."));
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
        if (bufferIndex > 0) { bufferIndex--; Serial.print(F("\b \b")); }
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
      if (bufferIndex > 0) { bufferIndex--; Serial.print(F("\b \b")); }
    } else {
      if (bufferIndex < MAX_CMD_LEN - 1) inputBuffer[bufferIndex++] = c;
    }
  }
}
