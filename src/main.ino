#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Trace.h>

// config
const String APP_VERSION = "0.2"; // the version of this app
const String APP_NAME = "Hello!"; // the version of this app
const bool WAIT_FOR_SERIAL = false;
// debugging tools
const bool USE_DISPLAY = true;
const bool UPSIDE_DOWN_DISPLAY = true;
const byte OLED_RESET = 5;
const unsigned int OLED_STARTUP_DELAY = 1000;

const bool BLUE_POTS_ENABLED = true;
const bool DISPLAY_BLUE_POT_VALUE_CHANGES = true;
const byte N_BLUE_POTS = 4;
// const unsigned long ONBOARD_NEOPIXEL_GLOW_SPEED = 10;         // LOWER numbers for faster glow speed using LUT
const unsigned long ONBOARD_NEOPIXEL_SINE_GLOW_SPEED = 500.0; // LOWER numbers for faster glow speed
const unsigned int SERIAL_BAUDRATE = 115200;
const unsigned int DEBOUNCE_DURATION = 80;
const byte BLUE_POT_DEADZONE = 22; // 22 for non breadboard, 44 for breadboard

Trace trace = Trace();

// OLED Featherwing CONFIG
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5

const String DEBUG_RULE = "=====================================================\n";

// // pins
const byte MODE_SELECT_BTN_PIN = BUTTON_B;
// // pots
const byte BACK_LEFT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A5 : A2;
const byte BACK_RIGHT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A4 : A3;
const byte FRONT_RIGHT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A2 : A5;
const byte FRONT_LEFT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A3 : A4;
byte potPins[] = {BACK_LEFT_BLUE_POT_PIN, BACK_RIGHT_BLUE_POT_PIN, FRONT_LEFT_BLUE_POT_PIN, FRONT_RIGHT_BLUE_POT_PIN};
int bluePotValues[] = {128, 128, 128, 128};

const byte NEO_PIXEL_PIN = 8;
const byte NEO_PIXEL_COUNT = 1;
const uint32_t DEFAULT_NEOPIXEL_COLOR = trace.GREEN;

volatile bool interrupted = false;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  trace.initSerial(SERIAL_BAUDRATE, WAIT_FOR_SERIAL);
  trace.initNeoPixel(NEO_PIXEL_COUNT, NEO_PIXEL_PIN);
  trace.initDisplay(UPSIDE_DOWN_DISPLAY, OLED_STARTUP_DELAY, OLED_RESET);
  trace.setNeoPixelColor(trace.YELLOW);
  showStartUpMessage();
  initPins();
  trace.setNeoPixelColor(DEFAULT_NEOPIXEL_COLOR);
}

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

void initPins()
{
  for (int i = 0; i < N_BLUE_POTS; i++)
  {
    pinMode(potPins[i], INPUT);
  }
  // ADMIN BUTTONS
  setPulledUpInput(BUTTON_A);
  setPulledUpInput(BUTTON_B);
  setPulledUpInput(BUTTON_C);
  // OPTIONALLY USE INTERRUPT
  // attachInterrupt(digitalPinToInterrupt(MODE_SELECT_BTN_PIN), handleInterrupt, FALLING);
}

void loop()
{
  checkBluePotValueChange(false);
  // checkForAdminButtonsPressed();
  trace.loop();
}

// BUTTON HANDLING

void buttonAHandler()
{
  trace.traceHold("+++ buttonAHandler()");
}

void buttonBHandler()
{
  trace.trace("+++ buttonBHandler()\nhold for\nmode change");
  // wait and then check that mode button is still held down
  // button has to be held to prevent false positives
  delay(2000);
  // if (!modeButtonIsDown())
  if (digitalRead(BUTTON_B))
  {
    // button was not held down
    trace.clearDisplay();
    return;
  }
  // mode button was held down
  // PLACE MODE CHANGE CODE HERE
  trace.traceHold("Mode Changed");
  trace.setNeoPixelColor(trace.BLUE);
  trace.traceToSerial("mode button pressed");
}

void buttonCHandler()
{
  trace.traceHold("+++ buttonCHandler()");
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
 ***************        BOOTSTRAP UTILITIES       ****************
 * 
 */

void displayModeState()
{
  trace.trace("displayModeState");
  delay(500);
}

void handleInterrupt()
{
  interrupted = true;
}

bool modeButtonIsDown()
{
  return digitalRead(MODE_SELECT_BTN_PIN) == LOW;
}

void breakHere(String msg)
{
  trace.trace(msg);
  while (digitalRead(BUTTON_A))
    ;
  while (!digitalRead(BUTTON_A))
    ;
}

void checkBluePotValueChange(bool forceDisplayUpdate)
{
  if (!BLUE_POTS_ENABLED)
    return;
  int newValue;
  bool aValueChanged = forceDisplayUpdate;
  for (int i = 0; i < N_BLUE_POTS; i++)
  {
    newValue = analogRead(potPins[i]);
    if (abs(newValue - bluePotValues[i]) > BLUE_POT_DEADZONE)
    {
      aValueChanged = true;
      bluePotValues[i] = newValue;
    }
  }
  if (aValueChanged && DISPLAY_BLUE_POT_VALUE_CHANGES)
  {
    String msg = "";
    String value = "";
    byte nCharsPerValue = 4;
    for (int i = 0; i < N_BLUE_POTS; i++)
    {
      value = String(bluePotValues[i]);
      // pad left
      while (value.length() < nCharsPerValue)
        value = " " + value;
      msg += value;
      // add space or newline to allow two values per line
      msg += i % 2 == 1 ? "\n" : "  ";
    }
    trace.updateDisplayWithTextSize(msg, 2);
    trace.traceToSerial("bluePot change:\n" + msg);
  }
}

/*
 * HELPER METHODS ARE BELOW 
 *
 *
 *
 */

void pulsePinLow(byte pin)
{
  digitalWrite(pin, LOW);
  delay(300);
  digitalWrite(pin, HIGH);
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

void handleButtonRelease(byte buttonPin, void (*callBack)())
{
  if (!digitalRead(buttonPin))
  {
    // WAIT FOR RELEASE
    while (!digitalRead(buttonPin))
      ;
    (*callBack)();
  }
}

void handleButtonPress(byte buttonPin, void (*callBack)())
{
  if (!digitalRead(buttonPin))
  {
    (*callBack)();
  }
}

void checkForAdminButtonsPressed()
{
  handleButtonRelease(BUTTON_A, buttonAHandler);
  handleButtonPress(BUTTON_B, buttonBHandler);
  handleButtonRelease(BUTTON_C, buttonCHandler);
}
