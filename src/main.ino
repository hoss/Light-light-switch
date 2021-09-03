#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Trace.h>
#include <Timer.h>
#include <OneButton.h>

Trace trace = Trace();
Timer timer;

// config
const int lightThreshold = 789;
const String APP_VERSION = "0.2";           // the version of this app
const String APP_NAME = "LightLightSwitch"; // the version of this app
const bool WAIT_FOR_SERIAL = false;
const unsigned int SERIAL_BAUDRATE = 115200;
const unsigned int DEBOUNCE_DURATION = 80;

// config OLED
const bool USE_DISPLAY = true;
const bool UPSIDE_DOWN_DISPLAY = false;
const byte OLED_RESET = 5;
const unsigned int OLED_STARTUP_DELAY = 1000;

// for Feathers with onboard neopixel
const unsigned long ONBOARD_NEOPIXEL_SINE_GLOW_SPEED = 500.0; // LOWER numbers for faster glow speed
const byte NEO_PIXEL_PIN = 8;
const byte NEO_PIXEL_COUNT = 0;
const uint32_t DEFAULT_NEOPIXEL_COLOR = trace.GREEN;

// OLED Featherwing CONFIG
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5

const String DEBUG_RULE = "=====================================================\n";

// PINS
const byte SENSOR_PIN = A4;
const byte LIGHT_SWITCH_PIN = A2;
OneButton buttonA(BUTTON_A, true);

volatile bool interrupted = false;

// State
const byte OFF_STATE = 0;
const byte ON_STATE = 1;
const byte OFF_OVERRIDE_STATE = 2;
const byte ON_OVERRIDE_STATE = 3;
byte state = OFF_STATE;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  trace.initDisplay(UPSIDE_DOWN_DISPLAY, OLED_STARTUP_DELAY, OLED_RESET);
  trace.initSerial(SERIAL_BAUDRATE, WAIT_FOR_SERIAL);
  if (NEO_PIXEL_COUNT > 0)
    trace.initNeoPixel(NEO_PIXEL_COUNT, NEO_PIXEL_PIN);
  trace.setNeoPixelColor(trace.YELLOW);
  showStartUpMessage();
  initPins();
  initButtons();
  initTimers();
  trace.setNeoPixelColor(DEFAULT_NEOPIXEL_COLOR);
}

void initTimers()
{
  trace.trace("initTimers");
  timer.every(1000, checkLightLevels);
}

void checkLightLevels()
{
  int newBrightness = analogRead(SENSOR_PIN);
  trace.trace("B:" + String(newBrightness));
  if (state == OFF_STATE && newBrightness > lightThreshold)
  {
    switchOnLight();
    state = ON_STATE;
  }
  else if (state == ON_STATE && newBrightness < lightThreshold)
  {
    switchOffLight();
    state = OFF_STATE;
  }
}

void switchOnLight()
{
  digitalWrite(LIGHT_SWITCH_PIN, HIGH);
}

void switchOffLight()
{
  digitalWrite(LIGHT_SWITCH_PIN, LOW);
}

void loop()
{
  buttonA.tick();
  timer.update();
  trace.loop();
}

/*
 *  
 *
 *
 *
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 ***************        BUTTON CALLBACKS       ****************
 * 
 */

// This function will be called when the button1 was pressed 1 time (and no 2. button press followed).
void click1()
{
  trace.trace("Button 1 click.");
} // click1

// This function will be called when the button1 was pressed 2 times in a short timeframe.
void doubleclick1()
{
  trace.trace("Button 1 doubleclick.");
} // doubleclick1

// This function will be called once, when the button1 is pressed for a long time.
void longPressStart1()
{
  trace.trace("Button 1 longPress start");
} // longPressStart1

// This function will be called often, while the button1 is pressed for a long time.
void longPress1()
{
  trace.trace("Button 1 longPress...");
} // longPress1

// This function will be called once, when the button1 is released after beeing pressed for a long time.
void longPressStop1()
{
  trace.trace("Button 1 longPress stop");
} // longPressStop1

/*
 *  
 *
 *
 *
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 ***************        BOOTSTRAP UTILITIES       ****************
 * 
 */

void showStartUpMessage()
{
  digitalWrite(LED_BUILTIN, HIGH);
  trace.setNeoPixelColor(trace.MAGENTA);
  trace.trace(APP_NAME);
  delay(900);
  trace.trace("v " + APP_VERSION);
  delay(900);
  digitalWrite(LED_BUILTIN, LOW);
}

void initButtons()
{
  // link the button 1 functions.
  buttonA.attachClick(click1);
  buttonA.attachDoubleClick(doubleclick1);
  buttonA.attachLongPressStart(longPressStart1);
  buttonA.attachLongPressStop(longPressStop1);
  buttonA.attachDuringLongPress(longPress1);
}

void initPins()
{
  setHighOutput(LIGHT_SWITCH_PIN);
}

void setPulledUpInput(byte pin)
{
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
}

void setHighOutput(byte pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
}
