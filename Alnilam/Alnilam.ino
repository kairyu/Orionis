#include <EEPROM.h>
#include <TinyWireS.h>

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

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

enum {
  EEPROM_I2CADDR = 0,
  EEPROM_DEBUG
};

const int KEY_PIN = 3;
const int LED_PIN = 4;
const int ATTN_PIN = 1;
unsigned char addr = 0;
unsigned char key = 0;
unsigned char state = 0;
unsigned char mode = MODE_KEYDOWN;
unsigned char brightness = 255;

void setup() {
  pinMode(KEY_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // I2C
  addr = EEPROM.read(EEPROM_I2CADDR);
  TinyWireS.begin(addr);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);

  // PWM
  TCCR1 = (0<<COM1A1)|(0<<COM1A0)|(0<<PWM1A)|(1<<CS10);
  GTCCR = (1<<PWM1B)|(1<<COM0B1)|(0<<COM0B0);
  
  pinMode(ATTN_PIN, OUTPUT);
  digitalWrite(ATTN_PIN, HIGH);
}

void loop() {
  key = 1 - digitalRead(KEY_PIN);
  if (mode == MODE_KEYDOWN) {
    if (key) {
      OCR1B = brightness;
    }
    else {
      OCR1B = 0;
    }
  }
}

void receiveEvent(uint8_t howMany) {
  if (howMany < 1) {
    return;
  }
  if (howMany > TWI_RX_BUFFER_SIZE) {
    return;
  }
  unsigned char command = TinyWireS.receive();
  if (howMany > 1) {
    unsigned char param = TinyWireS.receive();
    switch (command) {
      case COMMAND_SET_ADDR:
        setAddr(param);
        break;
      case COMMAND_SET_MODE:
        setMode(param);
        break;
      case COMMAND_SET_BRIGHTNESS:
        setBrightness(param);
        break;
    }
  }
}

void requestEvent() {
  TinyWireS.send((addr << 1) | key);
}

void setAddr(unsigned char a) {
  addr = a;
  EEPROM.write(EEPROM_I2CADDR, a);
  TinyWireS.begin(a);
}

void setMode(unsigned char m) {
  mode = m;
  switch (mode) {
    case MODE_OFF:
      OCR1B = 0;
      break;
    case MODE_ON:
      OCR1B = brightness;
      break;
    case MODE_FADE:
      break;
    case MODE_KEYDOWN:
      if (key) {
        OCR1B = brightness;
      }
      break;
  }
}

void setBrightness(unsigned char b) {
  brightness = b;
  switch (mode) {
    case MODE_OFF:
      break;
    case MODE_ON:
      OCR1B = b;
      break;
    case MODE_FADE:
      break;
    case MODE_KEYDOWN:
      if (key) {
        OCR1B = b;
      }
      break;
  }
}
