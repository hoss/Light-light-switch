#include <Arduino.h>
#include <Trace.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Trace::Trace()
{
    _currentNeoPixelColor = RED;
    // byte _sineWaveLUT[] = {
    //     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    //     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    //     1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    //     2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    //     5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    //     10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    //     17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    //     25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    //     37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    //     51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    //     69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    //     90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    //     115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    //     144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    //     177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    //     215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};
}

void Trace::loop()
{
    checkForOLEDTimeout();
    if (glowHeartBeat)
        glowNeoPixel();
}

void Trace::initSerial(unsigned int serialBaudrate, bool waitForSerial)
{
    Serial.begin(serialBaudrate);
    // long count = 0;
    if (waitForSerial)
    {
        while (!Serial)
        {
            // flashSOS();
        };
    }
    _useSerial = true;
    delay(500);
    Serial.println("Serial initialized with Baudrate = " + String(serialBaudrate));
}

void Trace::traceToSerial(String msg)
{
    if (!_useSerial)
        return;
    Serial.println(String(millis()) + " :: " + msg);
}

void Trace::initDisplay(bool upsideDown, unsigned int startupDelay, byte oledReset)
{
    _display = Adafruit_SSD1306(oledReset);
    // artificial delay - OLED driver circuit needs a small amount of time to be ready after initial power up.
    // see https://learn.adafruit.com/adafruit-oled-featherwing/troubleshooting for details.
    delay(startupDelay);
    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
    _display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
    if (upsideDown)
    {
        _display.setRotation(2);
    }
    // Clear the buffer.
    _display.clearDisplay();
    _display.setTextColor(WHITE);
    _display.setTextSize(4);
    _display.display();
    _useDisplay = true;
}

void Trace::traceToDisplay(String msg)
{
    updateDisplay(msg);
}

void Trace::trace(String msg)
{
    traceToSerial(msg);
    updateDisplay(msg);
}

void Trace::traceHold(String msg)
{
    traceToSerial(msg);
    updateDisplayWithImportantMessage(msg, 1, 3000, true);
}

void Trace::updateDisplay(String msg)
{
    byte textSize = msg.length() <= 5 ? 4 : msg.length() <= 8 ? 3 : msg.length() <= 11 ? 2 : 1;
    updateDisplayWithTextSize(msg, textSize);
}

void Trace::updateDisplayWithImportantMessage(String msg, byte textSize, int displayDuration, bool removeAfterWait)
{
    updateDisplayWithTextSize(msg, textSize);
    _timeNextOLEDMessageAllowed = millis() + displayDuration;
    _manuallyRemoveOLEDMessage = removeAfterWait;
    // setNeoPixelColor(MAGENTA);
}

void Trace::updateDisplayWithTextSize(String msg, byte textSize)
{
    if (!_useDisplay)
        return;
    if (millis() < _timeNextOLEDMessageAllowed)
    {
        traceToSerial("OLED not ready for msg=" + msg);
        return;
    }
    _display.clearDisplay();
    _display.setTextSize(textSize);
    byte cursorY = 0;
    _display.setCursor(0, cursorY);
    _display.clearDisplay();
    _display.println(msg);
    _display.display();
}

void Trace::checkForOLEDTimeout()
{
    if (_manuallyRemoveOLEDMessage && millis() > _timeNextOLEDMessageAllowed)
    {
        setNeoPixelColor(_currentNeoPixelColor);
        _manuallyRemoveOLEDMessage = false;
        //-- clearDisplay();
    }
}

void Trace::clearDisplay()
{
    updateDisplay("");
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
 ***************        ONBOARD NEOPIXEL       ****************
 * 
 */

void Trace::glowNeoPixel()
{
    if (millis() > _timeNextGlowLUTIndexChangeAllowed)
    {
        _timeNextGlowLUTIndexChangeAllowed = millis() + _neoPixelGlowSpeed;
        // String msg = String(_neoPixelGammaLUTIndex) + ">";
        _neoPixelGammaLUTIndex += _neoPixelGlowDirection;
        if (_neoPixelGammaLUTIndex >= _sineWaveLUTLength)
        {
            _neoPixelGlowDirection *= -1;
            _neoPixelGammaLUTIndex = _sineWaveLUTLength;
        }
        else if (_neoPixelGammaLUTIndex <= 0)
        {
            _neoPixelGlowDirection *= -1;
            _neoPixelGammaLUTIndex = 0;
        }
        byte brightness = _sineWaveLUT[_neoPixelGammaLUTIndex];
        _neoPixel.setBrightness(brightness);
        _neoPixel.setPixelColor(0, _currentNeoPixelColor);
        _neoPixel.show();
        // msg += String(_neoPixelGammaLUTIndex);
        // msg += "=" + String(brightness);
        // traceToDisplay(msg);
    }
}

void Trace::initNeoPixel(unsigned int neoPixelCount, byte neoPixelPin)
{
    _neoPixel = Adafruit_NeoPixel(neoPixelCount, neoPixelPin);
    _neoPixel.begin();
    _neoPixel.show();
}

void Trace::setNeoPixelColor(uint32_t color)
{
    _currentNeoPixelColor = color;
    _neoPixel.setPixelColor(0, _currentNeoPixelColor);
    _neoPixel.show();
}

void Trace::flashSOS()
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

void Trace::flashSOSNeo()
{
    int duration = 600;
    setNeoPixelColor(BLUE);
    delay(duration);
    setNeoPixelColor(RED);
    delay(duration);
}

void Trace::flashLED(unsigned int duration)
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(duration);
    digitalWrite(LED_BUILTIN, LOW);
}