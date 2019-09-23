#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
//
// config
const String APP_VERSION = "0.1"; // the version of this app
const String APP_NAME = "Hello!"; // the version of this app
const bool WAIT_FOR_SERIAL = false;
// debugging tools
const bool USE_OLED = true;
const bool UPSIDE_DOWN_DISPLAY = false;
const bool BLUE_POTS_ENABLED = true;
const bool DISPLAY_BLUE_POT_VALUE_CHANGES = false;
const byte N_BLUE_POTS = 4;
const unsigned long ONBOARD_NEOPIXEL_GLOW_SPEED = 10;        // LOWER numbers for faster glow speed using LUT
const unsigned long ONBOARD_NEOPIXEL_SINE_GLOW_SPEED = 500.0; // LOWER numbers for faster glow speed
const unsigned int SERIAL_BAUDRATE = 9600;
const unsigned int DEBOUNCE_DURATION = 80;
const unsigned int OLED_STARTUP_DELAY = 1000;
const bool SHOW_ADAFRUIT_LOGO_ON_DISPLAY_STARTUP = false;
const byte BLUE_POT_DEADZONE = 22; // 22 for non breadboard, 44 for breadboard
// onboard neoPixel for status visualization
const byte NEO_PIXEL_PIN = 8;
const byte NEO_PIXEL_COUNT = 1;
Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NEO_PIXEL_COUNT, NEO_PIXEL_PIN);
// colors for neoPixel
const uint32_t RED = neoPixel.Color(255, 0, 0);
const uint32_t MAGENTA = neoPixel.Color(255, 0, 255);
const uint32_t YELLOW = neoPixel.Color(255, 255, 0);
const uint32_t ORANGE = neoPixel.Color(255, 95, 0);
const uint32_t GREEN = neoPixel.Color(0, 255, 0);
const uint32_t BLUE = neoPixel.Color(0, 0, 255);
uint32_t DEFAULT_NEOPIXEL_COLOR = GREEN;
uint32_t currentNeoPixelColor = RED;

// OLED Featherwing CONFIG
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5
const byte OLED_RESET = 5;
Adafruit_SSD1306 display(OLED_RESET);
const String DEBUG_RULE = "=====================================================\n";
unsigned long timeDisplayStarted = 0;

// // pins
const byte MODE_SELECT_BTN_PIN = BUTTON_B;
// // pots
const byte BACK_LEFT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A5 : A2;
const byte BACK_RIGHT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A4 : A3;
const byte FRONT_RIGHT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A2 : A5;
const byte FRONT_LEFT_BLUE_POT_PIN = UPSIDE_DOWN_DISPLAY ? A3 : A4;
byte potPins[] = {BACK_LEFT_BLUE_POT_PIN, BACK_RIGHT_BLUE_POT_PIN, FRONT_LEFT_BLUE_POT_PIN, FRONT_RIGHT_BLUE_POT_PIN};

int neoPixelGammaLUTIndex = 0;
int neoPixelGlowDirection = 1;
int sineWaveLUTLength = 255;
byte sineWaveLUT[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

unsigned long timeNextGlowLUTIndexChangeAllowed = 0;
int bluePotValues[] = {128, 128, 128, 128};
unsigned long timeNextOLEDMessageAllowed = 0;
bool manuallyRemoveOLEDMessage = false;
volatile bool interrupted = false;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  initNeoPixel();
  setNeoPixelColor(YELLOW);
  initDisplay();
  showStartUpMessage();
  initSerial();
  initPins();
  setNeoPixelColor(DEFAULT_NEOPIXEL_COLOR);
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
  checkForAdminButtonsPressed();
  glowNeoPixel();
  checkForOLEDTimeout();
}

// BUTTON HANDLING

void buttonAHandler()
{
  traceHold("+++ buttonAHandler()");
}

void buttonBHandler()
{
  trace("+++ buttonBHandler()\nhold for\nmode change");
  // wait and then check that mode button is still held down
  // button has to be held to prevent false positives
  delay(2000);
  // if (!modeButtonIsDown())
  if (digitalRead(BUTTON_B))
  {
    // button was not held down
    clearDisplay();
    return;
  }
  // mode button was held down
  // PLACE MODE CHANGE CODE HERE
  traceHold("Mode Changed");
  setNeoPixelColor(BLUE);
  traceToSerial("mode button pressed");
}

void buttonCHandler()
{
  traceHold("+++ buttonCHandler()");
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
  trace("displayModeState");
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

void checkForOLEDTimeout()
{
  if (manuallyRemoveOLEDMessage && millis() > timeNextOLEDMessageAllowed)
  {
    setNeoPixelColor(DEFAULT_NEOPIXEL_COLOR);
    manuallyRemoveOLEDMessage = false;
    clearDisplay();
  }
}

void glowNeoPixel()
{
  if (millis() > timeNextGlowLUTIndexChangeAllowed)
  {
    timeNextGlowLUTIndexChangeAllowed = millis() + ONBOARD_NEOPIXEL_GLOW_SPEED;
    // String msg = String(neoPixelGammaLUTIndex) + ">";
    neoPixelGammaLUTIndex += neoPixelGlowDirection;
    if (neoPixelGammaLUTIndex >= sineWaveLUTLength)
    {
      neoPixelGlowDirection = 0 - neoPixelGlowDirection;
      neoPixelGammaLUTIndex = sineWaveLUTLength;
    }
    else if (neoPixelGammaLUTIndex <= 0)
    {
      neoPixelGlowDirection = 0 - neoPixelGlowDirection;
      neoPixelGammaLUTIndex = 0;
    }
    byte brightness = sineWaveLUT[neoPixelGammaLUTIndex];
    neoPixel.setBrightness(brightness);
    neoPixel.setPixelColor(0, currentNeoPixelColor);
    neoPixel.show();
    // trace(msg + String(neoPixelGammaLUTIndex));
  }
}

void glowSineNeoPixel()
{
  unsigned long currentMillis = millis();
  float time = currentMillis / ONBOARD_NEOPIXEL_SINE_GLOW_SPEED;
  float sinFactor = sin(time);
  sinFactor += 1;
  byte brightness = sinFactor * 128;
  // traceToDisplay(
  //     "currentMillis = " + String(currentMillis) +
  //     "\ntime = " + String(time) +
  //     "\nbrightness = " + String(brightness) +
  //     "\nsinFactor = " + String(sinFactor));
  neoPixel.setBrightness(brightness);
  neoPixel.setPixelColor(0, currentNeoPixelColor);
  neoPixel.show();
}

void breakHere(String msg)
{
  trace(msg);
  while (digitalRead(BUTTON_A))
    ;
  while (!digitalRead(BUTTON_A))
    ;
}

void initNeoPixel()
{
  neoPixel.begin();
  neoPixel.show();
}

void setNeoPixelColor(uint32_t color)
{
  currentNeoPixelColor = color;
  neoPixel.setPixelColor(0, currentNeoPixelColor);
  neoPixel.show();
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
    updateDisplayWithTextSize(msg, 2);
    traceToSerial("bluePot change:\n" + msg);
  }
}

/*
 * HELPER METHODS ARE BELOW 
 *
 *
 *
 */

void flashSOS()
{
  int duration;
  int flashDurations[] = {100, 200, 300, 400, 300, 200};
  for (unsigned int i = 0; i < 6; i++)
  {
    duration = flashDurations[i];
    digitalWrite(LED_BUILTIN, HIGH);
    delay(duration);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void flashSOSNeo()
{
  int duration = 600;
  setNeoPixelColor(BLUE);
  delay(duration);
  setNeoPixelColor(RED);
  delay(duration);
}

void flashLED(int duration)
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(duration);
  digitalWrite(LED_BUILTIN, LOW);
}

void pulsePinLow(byte pin)
{
  digitalWrite(pin, LOW);
  delay(300);
  digitalWrite(pin, HIGH);
}

void traceToDisplay(String msg)
{
  updateDisplay(msg);
}

void traceToSerial(String msg)
{
  Serial.println(String(millis()) + " :: " + msg);
}

void trace(String msg)
{
  traceToSerial(msg);
  updateDisplay(msg);
}

void traceHold(String msg)
{
  traceToSerial(msg);
  updateDisplayWithImportantMessage(msg, 1, 3000, true);
}

void showStartUpMessage()
{
  digitalWrite(LED_BUILTIN, HIGH);
  trace(APP_NAME);
  delay(900);
  trace("v " + APP_VERSION);
  delay(900);
  digitalWrite(LED_BUILTIN, LOW);
}

void initDisplay()
{
  if (!USE_OLED)
    return;
  // artificial delay - OLED driver circuit needs a small amount of time to be ready after initial power up.
  // see https://learn.adafruit.com/adafruit-oled-featherwing/troubleshooting for details.
  delay(OLED_STARTUP_DELAY);
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  if (UPSIDE_DOWN_DISPLAY)
  {
    display.setRotation(2);
  }
  if (SHOW_ADAFRUIT_LOGO_ON_DISPLAY_STARTUP)
  {
    display.display(); // show Adafruit logo
    delay(500);
  }
  // Clear the buffer.
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(4);
  display.display();
}

void updateDisplay(String msg)
{
  if (!USE_OLED)
    return;
  byte textSize = msg.length() <= 5 ? 4 : msg.length() <= 8 ? 3 : msg.length() <= 11 ? 2 : 1;
  updateDisplayWithTextSize(msg, textSize);
}

void updateDisplayWithImportantMessage(String msg, byte textSize, int displayDuration, bool removeAfterWait)
{
  updateDisplayWithTextSize(msg, textSize);
  timeNextOLEDMessageAllowed = millis() + displayDuration;
  manuallyRemoveOLEDMessage = removeAfterWait;
  setNeoPixelColor(MAGENTA);
}

void updateDisplayWithTextSize(String msg, byte textSize)
{
  if (!USE_OLED)
    return;
  if (millis() < timeNextOLEDMessageAllowed)
  {
    traceToSerial("OLED not ready for msg=" + msg);
    return;
  }
  display.clearDisplay();
  display.setTextSize(textSize);
  byte cursorY = 0;
  display.setCursor(0, cursorY);
  display.clearDisplay();
  display.println(msg);
  display.display();
}

void clearDisplay()
{
  if (!USE_OLED)
    return;
  updateDisplay("");
}

void initSerial()
{
  Serial.begin(SERIAL_BAUDRATE);
  long count = 0;
  if (WAIT_FOR_SERIAL)
  {
    while (!Serial)
    {
      traceToDisplay(String(count++) + ": Wait for Serial");
      flashSOS();
    };
    trace("Serial OK");
  }
  else
  {
    delay(500);
  }
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
