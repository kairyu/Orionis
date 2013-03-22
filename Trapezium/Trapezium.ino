#include <Wire.h>

unsigned char scancodes[] = {
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0, 0, 0,
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
};

enum {
  COMMAND_NONE = 0,
  COMMAND_SET_ADDR,
  COMMAND_SET_MODE,
  COMMAND_SET_BRIGHTNESS
};

enum {
  MODE_OFF = 0,
  MODE_ON,
  MODE_FADE,
  MODE_KEYDOWN
};

const int led = 13;
int count;
uint8_t keys[128];

void setup() {
  
  // Reset
  pinMode(6, INPUT);
  //digitalWrite(6, LOW);
  //delay(10);
  //digitalWrite(6, HIGH);
  
  // Setup
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
        keys[addr] = 0;
      }
    }
    else {
      keys[addr] = 0;
    }
  }
  Serial.println("Detect Addr OK");
  
  //sendCommand(0x00, COMMAND_SET_BRIGHTNESS, 32);
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
          }
          else {
            Serial.println(" Down");
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
}

void sendCommand(unsigned char addr, unsigned char command, unsigned char param) {
  Wire.beginTransmission(addr);
  Wire.write(command);
  Wire.write(param);
  Wire.endTransmission();
}


