#include <Arduino.h>
#include <Trace.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Trace::Trace()
{
    _currentNeoPixelColor = RED;
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
            traceToDisplay("wait for Serial");
            delay(250);
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
    if (!_useDisplay)
        return;
    byte textSize = msg.length() <= 5 ? 4 : msg.length() <= 8 ? 3
                                        : msg.length() <= 11  ? 2
                                                              : 1;
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
    if (!_useNeoPixel)
        return;
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
    _useNeoPixel = true;
}

void Trace::setNeoPixelColor(uint32_t color)
{
    if (!_useNeoPixel)
        return;
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
    if (!_useNeoPixel)
        return;
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