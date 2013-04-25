// Orionis/Trapezium

#include <Wire.h>

unsigned char scancodes[] = {
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0, 0, 0,
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
};

enum {
  COMMAND_NONE = 0,
  COMMAND_SET_ADDR,
  COMMAND_SET_MODE,
  COMMAND_SET_BRIGHTNESS,
  COMMAND_SET_INTERVAL,
  COMMAND_SET_SMOOTH,
};

enum {
  MODE_OFF = 0,
  MODE_ON,
  MODE_FADE,
  MODE_FADE_ONCE,
  MODE_KEYDOWN
};

enum {
  DEMO_OFF = 0,
  DEMO_ON,
  DEMO_KEYDOWN,
  DEMO_ON_SMOOTH,
  DEMO_KEYDOWN_SMOOTH,
  DEMO_FADE,
  DEMO_WAVE,
  DEMO_COUNT
};

const int led = 13;
int count;
unsigned char mode = MODE_OFF;
uint8_t keys[128];
int g_demo = 0;
int g_on_brightness = 0x80;
int g_fade_brightness;
int g_fade_delta;
int g_interval = 0;
int g_fade_interval = 20;
bool g_fade_enabled = false;
int g_wave_interval = 1000;
int g_wave_pos = 0;
int g_wave_delta = 1;
bool g_wave_enabled = false;

long g_time_last = 0;

unsigned char sendCommand(unsigned char addr, unsigned char command, unsigned char param);
void resetAll(int pin);
void blinkAll(int count);
void setBrightnessAll(unsigned char b);
void keyDown(unsigned char key);
void keyUp(unsigned char key);
void demo(int d);
void fade(void);
void fade_proc(void);
void wave(void);
void wave_proc(void);

void setup() {
  
  // Reset
  pinMode(6, OUTPUT);
  resetAll(6);

  // Setup Serial and I2C
  Serial.begin(9600);
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Wire.begin();
  Serial.println("Start I2C");
  
  // Detect slave address
  uint8_t addr = 0x00;
  for (addr = 0; addr < 0x80; addr++) {
    Wire.requestFrom(addr, (uint8_t)1);
    delay(1);
    if (Wire.available()) {
      uint8_t ack = Wire.read();
      if (ack >> 1 == addr) {
        Serial.println(addr, HEX);
        keys[addr] = 1;
      }
      else {
        Serial.print("Fail ");
        Serial.println(addr, HEX);
        keys[addr] = 0;
      }
    }
    else {
      keys[addr] = 0;
    }
  }
  Serial.println("Detect Addr OK");

  sendCommand(0x00, COMMAND_SET_SMOOTH, 0);

  setBrightnessAll(0x80);
  delay(10);
  blinkAll(2);

  demo(g_demo);
}

void loop() {
  /*
  Wire.beginTransmission(0x20);
  Wire.write(COMMAND_IDENTIFY);
  Wire.endTransmission();
  Serial.println("On");
  delay(1000);
  Wire.beginTransmission(0x20);
  Wire.write(COMMAND_REPORT_STATE);
  Wire.endTransmission();
  Serial.println("Off");
  delay(1000);
  */
  /*
  Wire.beginTransmission(0x20);
  Wire.write((byte)COMMAND_REPORT_STATE);
  Wire.endTransmission(true);
  */
  int count = 0;
  for (int i = 0; i < 128; i++) {
    if (keys[i]) {
      Wire.requestFrom((uint8_t)i, (uint8_t)1);
      while (Wire.available()) {
        unsigned char c = Wire.read();
        unsigned char state = keys[i] - 1;
        unsigned char new_state = c & 0x01;
        unsigned char addr = c >> 1;
        if (addr != i) {
          break;
        }
        if (new_state != state) {
          Serial.write(scancodes[i]);
          Serial.print(" Addr: 0x");
          Serial.print(addr, HEX);
          if (new_state == 0) {
            Serial.println(" Up");
            keyUp(addr);
          }
          else {
            Serial.println(" Down");
            keyDown(addr);
          }
        }
        keys[i] = new_state + 1;
      }
      if (keys[i] > 1) {
        count++;
      }
    }
  }
  if (count) {
    digitalWrite(led, HIGH);
  }
  else {
    digitalWrite(led, LOW);
  }
  long time_now = millis();
  if (time_now - g_time_last >= g_interval) {
    g_time_last = time_now;
    if (g_fade_enabled) fade_proc();
    if (g_wave_enabled) wave_proc();
  }
}

unsigned char sendCommand(unsigned char addr, unsigned char command, unsigned char param) {
  Wire.beginTransmission(addr);
  Wire.write(command);
  Wire.write(param);
  return Wire.endTransmission(true);
}

void resetAll(int pin) {
  digitalWrite(pin, LOW);
  delay(10);
  digitalWrite(pin, HIGH);
  delay(100);
}

void blinkAll(int count) {
  while (count--) {
    sendCommand(0x00, COMMAND_SET_MODE, MODE_ON);
    delay(100);
    sendCommand(0x00, COMMAND_SET_MODE, MODE_OFF);
    delay(100);
  }
}

inline void setBrightnessAll(unsigned char b) {
  sendCommand(0x00, COMMAND_SET_BRIGHTNESS, b);
}

void keyDown(unsigned char key) {
  switch (key) {
  }
}

void keyUp(unsigned char key) {
  switch (key) {
    case 0x10:
      g_demo = (g_demo + 1) % DEMO_COUNT;
      demo(g_demo);
      break;
    case 0x11:
      if (g_demo == DEMO_ON | g_demo == DEMO_ON_SMOOTH) {
        g_on_brightness -= 0x20;
        if (g_on_brightness <= 0) g_on_brightness = 0;
        setBrightnessAll(g_on_brightness);
        Serial.print("Brightness: 0x");
        Serial.println(g_on_brightness, HEX);
      }
      else if (g_demo == DEMO_FADE) {
        g_fade_interval -= 5;
        if (g_fade_interval <= 5) g_fade_interval = 5;
        Serial.print("Interval: ");
        Serial.println(g_fade_interval);
        fade();
      }
      else if (g_demo == DEMO_WAVE) {
        g_wave_interval -= 100;
        if (g_wave_interval <= 100) g_wave_interval = 100;
        Serial.print("Interval: ");
        Serial.println(g_wave_interval);
        wave();
      }
      break;
    case 0x12:
      if (g_demo == DEMO_ON | g_demo == DEMO_ON_SMOOTH) {
        g_on_brightness += 0x20;
        if (g_on_brightness >= 0xFF) g_on_brightness = 0xFF;
        setBrightnessAll(g_on_brightness);
        Serial.print("Brightness: 0x");
        Serial.println(g_on_brightness, HEX);
      }
      else if (g_demo == DEMO_FADE) {
        g_fade_interval += 5;
        if (g_fade_interval >= 50) g_fade_interval = 50;
        Serial.print("Interval: ");
        Serial.println(g_fade_interval);
        fade();
      }
      else if (g_demo == DEMO_WAVE) {
        g_wave_interval += 100;
        if (g_wave_interval >= 2000) g_wave_interval = 2000;
        Serial.print("Interval: ");
        Serial.println(g_wave_interval);
        wave();
      }
      break;
  }
}

void demo(int d) {
  switch (d) {
    case DEMO_OFF:
      g_fade_enabled = false;
      g_wave_enabled = false;
      sendCommand(0x00, COMMAND_SET_MODE, MODE_OFF);
      Serial.println("LED OFF");
      break;
    case DEMO_ON:
      sendCommand(0x00, COMMAND_SET_SMOOTH, 0);
      sendCommand(0x00, COMMAND_SET_MODE, MODE_ON);
      g_on_brightness = 0x80;
      setBrightnessAll(g_on_brightness);
      Serial.println("LED ON");
      break;
    case DEMO_KEYDOWN:
      sendCommand(0x00, COMMAND_SET_SMOOTH, 0);
      sendCommand(0x00, COMMAND_SET_MODE, MODE_KEYDOWN);
      g_on_brightness = 0xFF;
      setBrightnessAll(g_on_brightness);
      Serial.println("LED ON WHEN KEYDOWN");
      break;
    case DEMO_ON_SMOOTH:
      sendCommand(0x00, COMMAND_SET_SMOOTH, 1);
      sendCommand(0x00, COMMAND_SET_MODE, MODE_ON);
      g_on_brightness = 0x80;
      setBrightnessAll(g_on_brightness);
      Serial.println("LED ON (SMOOTH)");
      break;
    case DEMO_KEYDOWN_SMOOTH:
      sendCommand(0x00, COMMAND_SET_SMOOTH, 1);
      sendCommand(0x00, COMMAND_SET_MODE, MODE_KEYDOWN);
      g_on_brightness = 0xFF;
      setBrightnessAll(g_on_brightness);
      Serial.println("LED ON WHEN KEYDOWN (SMOOTH)");
      break;
    case DEMO_FADE:
      fade();
      Serial.println("LED FADE");
      break;
    case DEMO_WAVE:
      wave();
      Serial.println("LED WAVE");
      break;
  }
}

void fade(void) {
  sendCommand(0x00, COMMAND_SET_MODE, MODE_ON);
  g_fade_brightness = 0;
  g_fade_delta = -1;
  g_interval = g_fade_interval;
  g_fade_enabled = true;
  g_wave_enabled = false;
}

void fade_proc(void) {
  if (g_fade_brightness > 0x7F) {
    g_fade_delta = -g_fade_delta;
  }
  if (g_fade_brightness < 0x01) {
    g_fade_delta = -g_fade_delta;
  }
  g_fade_brightness += g_fade_delta;
  setBrightnessAll(g_fade_brightness);
}

void wave(void) {
  sendCommand(0x00, COMMAND_SET_MODE, MODE_OFF);
  g_wave_pos = 0;
  g_wave_delta = 1;
  g_interval = g_wave_interval;
  g_fade_enabled = false;
  g_wave_enabled = true;
}

void wave_proc(void) {
  sendCommand(0x10 + g_wave_pos, COMMAND_SET_MODE, MODE_FADE_ONCE);
  /*
  if (g_wave_pos == 0) g_wave_delta = 1;
  if (g_wave_pos == 2) g_wave_delta = -1;
  g_wave_pos += g_wave_delta;
  */
  g_wave_pos = (g_wave_pos + 1) % 3;
}
