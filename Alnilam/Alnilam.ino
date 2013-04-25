// Orionis/Alnilam

#include <EEPROM.h>
#include <TinyWireS.h>

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

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
  EEPROM_I2CADDR = 0,
  EEPROM_DEBUG
};

const int KEY_PIN = 3;
const int LED_PIN = 4;
const int ATTN_PIN = 1;
unsigned char g_last_state = 0;
long g_last_time = 0;
long g_debounce = 500;
unsigned char g_addr = 0;
unsigned char g_key = 0;
unsigned char g_mode = MODE_KEYDOWN;
unsigned char g_interval = 128;
unsigned char g_brightness = 255;
unsigned char g_on_brightness = 255;
unsigned char g_target_brightness = 0;
char g_fade_delta = -1;
bool g_fade_once = false;
char g_smooth_delta = -1;
bool g_smooth = false;
bool g_turn_off = false;
bool g_is_changing = false;
unsigned char g_command = COMMAND_NONE;

static void pwmOn(void);
static void pwmOff(void);
static void timerOn(void);
static void timerOff(void);
static void ledOn(void);
static void ledFade(void);
static void ledOff(void);
static void setInterval(unsigned char i);
void setBrightness(unsigned char b);
void changeBrightness(unsigned char b);
void changeBrightness(unsigned char from, unsigned char to);
void receiveEvent(uint8_t howMany);
void requestEvent();
void parseReceiving(uint8_t r);
void setMode(unsigned char m);
void setAddr(unsigned char a);

ISR(TIMER0_COMPA_vect) {
  if (g_mode == MODE_FADE || g_mode == MODE_FADE_ONCE) {
    if (g_brightness > 0xFE) {
      g_fade_delta = -1;
    }
    if (g_brightness < 0x01) {
      g_fade_delta = 1;
    }
    g_brightness += g_fade_delta;
    if (g_fade_once) {
      if (g_brightness == 0) {
        g_mode = MODE_OFF;
        pwmOff();
        timerOff();
      }
    }
    OCR1B = g_brightness;
  }
  else if (g_smooth) {
    if (g_brightness != g_target_brightness) {
      g_brightness += g_smooth_delta;
      OCR1B = g_brightness;
    }
    else {
      timerOff();
      if (g_brightness == 0) {
        if (g_turn_off) {
          g_turn_off = false;
          pwmOff();
        }
      }
      g_is_changing = false;
    }
  }
}

void setup() {
  pinMode(KEY_PIN, INPUT);
  digitalWrite(KEY_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);
  
  // I2C
  g_addr = EEPROM.read(EEPROM_I2CADDR);
  TinyWireS.begin(g_addr);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
  
  // Timer0
  TCCR0A = (1<<WGM01);
  TIMSK = (1<<OCIE0A)|(1<<TOIE1)|(1<<TOIE0);
  setInterval(g_interval);
  sei();

  // PWM
  TCCR1 = (0<<COM1A1)|(0<<COM1A0)|(0<<PWM1A)|(1<<CS10);
  pwmOn();

  pinMode(ATTN_PIN, OUTPUT);
  digitalWrite(ATTN_PIN, HIGH);

  OCR1B = 255;
  delay(1000);
  OCR1B = 0;
  delay(1000);
  OCR1B = 255;
  delay(1000);
  OCR1B = 0;
  delay(1000);

  g_smooth = true;
  setMode(MODE_KEYDOWN);
}

void loop() {
  TinyWireS_stop_check();
  unsigned char state;
  state = 1 - digitalRead(KEY_PIN);
  if (state != g_last_state) {
    g_last_time = millis();
  }
  if ((millis() - g_last_time) > g_debounce) {
    if (state != g_key) {
      g_key = state;
      if (g_mode == MODE_KEYDOWN) {
        if (g_key) {
          ledOn();
        }
        else {
          ledOff();
        }
      }
    }
  }
  g_last_state = state;
}

static inline void pwmOn(void) {
  //TCCR1 = (0<<COM1A1)|(0<<COM1A0)|(0<<PWM1A)|(1<<CS10);
  GTCCR = (1<<PWM1B)|(1<<COM0B1)|(0<<COM0B0);
}

static inline void pwmOff(void) {
  //TCCR1 = 0;
  GTCCR = 0;
}

static inline void timerOn(void) {
  TCCR0B = (1<<CS02)|(0<<CS01)|(0<<CS00);
}

static inline void timerOff(void) {
  TCCR0B = 0;
}

static inline void ledOn(void) {
  changeBrightness(0, g_on_brightness);
  pwmOn();
}

static inline void ledOff(void) {
  if (g_smooth) {
    g_turn_off = true;
    changeBrightness(0);
  }
  else {
    pwmOff();
    timerOff();
  }
}

static inline void ledFade(void) {
  //g_brightness = 0;
  g_fade_delta = 1;
  changeBrightness(g_brightness);
  pwmOn();
  timerOn();
}

void setBrightness(unsigned char b) {
  g_on_brightness = b;
  if (g_mode == MODE_ON) {
    changeBrightness(b);
  }
  else if (g_mode == MODE_KEYDOWN) {
    if (g_key) {
      changeBrightness(b);
    }
  }
}

void changeBrightness(unsigned char b) {
  if (g_mode == MODE_FADE) {
    g_brightness = b;
    OCR1B = b;
  }
  else {
    changeBrightness(g_brightness, b);
  }
}

void changeBrightness(unsigned char from, unsigned char to) {
  if (g_mode == MODE_KEYDOWN) {
    if (g_key) {
    }
  }
  if (g_smooth) {
    OCR1B = from;
    g_brightness = from;
    g_target_brightness = to;
    if (from != to) {
      g_smooth_delta = (to > from) ? 1 : -1;
      g_is_changing = true;
      timerOn();
    }
  }
  else {
    g_brightness = to;
    OCR1B = to;
  }
}

static inline void setInterval(unsigned char i) {
  OCR0A = i;
}

void receiveEvent(uint8_t howMany) {
  if (howMany < 1) {
    return;
  }
  if (howMany > TWI_RX_BUFFER_SIZE) {
    return;
  }
  while (howMany--) {
    parseReceiving(TinyWireS.receive());
  }
}

void parseReceiving(uint8_t r) {
  if (g_command == COMMAND_NONE) {
    g_command = r;
    return;
  }
  else {
    unsigned char param = r;
    switch (g_command) {
      case COMMAND_SET_ADDR:
        setAddr(param);
        break;
      case COMMAND_SET_MODE:
        setMode(param);
        break;
      case COMMAND_SET_BRIGHTNESS:
        g_on_brightness = param;
        setBrightness(param);
        break;
      case COMMAND_SET_INTERVAL:
        setInterval(param);
        break;
      case COMMAND_SET_SMOOTH:
        g_smooth = (param != 0);
        break;
    }
    g_command = COMMAND_NONE;
  }
}

void requestEvent() {
  TinyWireS.send((g_addr << 1) | g_key);
}

void setAddr(unsigned char a) {
  g_addr = a;
  EEPROM.write(EEPROM_I2CADDR, a);
  TinyWireS.begin(a);
}

void setMode(unsigned char m) {
  g_mode = m;
  g_turn_off = false;
  switch (g_mode) {
    case MODE_OFF:
      ledOff();
      break;
    case MODE_ON:
      ledOn();
      break;
    case MODE_FADE:
      g_fade_once = false;
      ledFade();
      break;
    case MODE_FADE_ONCE:
      g_fade_once = true;
      ledFade();
      break;
    case MODE_KEYDOWN:
      if (g_key) {
        ledOn();
      }
      else {
        ledOff();
      }
      break;
  }
}
