
#include <time.h>
#include "display.hpp"

BcdDisplay::BcdDisplay()
    : lastTime(0)
{
    const float a_nice_blue = 0.6652892561983471f;
    const float pink = 0.8636363636363636f;
    const float red = 1.0f;
    const float yellow = 0.16942148760330578f;
    const float green = 0.3415977961432507f;

    setDigitColour(a_nice_blue, 1.0f, 1.0f);
    setColonColour(green,1.0f, 0.5f);

}

/// @brief Sets the digit colour using RGB value
/// @param rgb
void BcdDisplay::setDigitColour(uint32_t rgb)
{
    rgb = convertToRaw(rgb);

    for (int i = 0; i < 30; ++i)
    {
        switch (i)
        {
        case 8:
        case 9:
        case 18:
        case 19:
            break;

        default:
            colours[i] = rgb;
        }
    }
}

/// @brief Sets the colon colour using RGB value
/// @param rgb
void BcdDisplay::setColonColour(uint32_t rgb)
{
    rgb = convertToRaw(rgb);
    colours[8] = rgb;
    colours[9] = rgb;

    colours[18] = rgb;
    colours[19] = rgb;
}

/// @brief Sets the digit colour using HSV colour space
/// @param h - hue, 0..1
/// @param s - saturation, 0..1
/// @param v - value (brightness), 0..1
void BcdDisplay::setDigitColour(float h, float s, float v)
{
    dh = h;
    ds = s;
    dv = v;
    uint32_t rgb = Neopixels::hsvToRgb(h, s, v);
    setDigitColour(rgb);
}

/// @brief Sets the colon colour using HSV colour space
/// @param h - hue, 0..1
/// @param s - saturation, 0..1
/// @param v - value (brightness), 0..1
void BcdDisplay::setColonColour(float h, float s, float v)
{
    ch = h;
    cs = s;
    cv = v;
    uint32_t rgb = Neopixels::hsvToRgb(h, s, v);
    setColonColour(rgb);
}

/// @brief Sets the overall brightness of the digit and colon as a fraction of the brightness set.
/// @note this uses the colour (hue and saturation) set by the HSV variants of setDigitColour and setColonColour.
/// @param v is the overall brightness multiplier.
void BcdDisplay::setBrightness( float v){
    uint32_t rgb = Neopixels::hsvToRgb(dh, ds, v*dv);
    setDigitColour(rgb);
    rgb = Neopixels::hsvToRgb(ch,cs, v*cv);
    setColonColour(rgb);
}

/// @brief Sets the pixel values for an individual digit using current colour map.
/// @param idx is the index of the neopixel to set.
/// @param digit is the numerical value to set the digit to.
/// @param up determines whether the string of pixels is aligned up or down (it snakes)
/// @return new value of idx to use for next digit/colon.
int BcdDisplay::setDigit(int idx, int digit, bool up)
{
    if (up)
    {
        neopixels.setPixelRaw(idx, (digit & 0x01) ? colours[idx] : 0);
        ++idx;
        neopixels.setPixelRaw(idx, (digit & 0x02) ? colours[idx] : 0);
        ++idx;
        neopixels.setPixelRaw(idx, (digit & 0x04) ? colours[idx] : 0);
        ++idx;
        neopixels.setPixelRaw(idx, (digit & 0x08) ? colours[idx] : 0);
        ++idx;
    }
    else
    {
        neopixels.setPixelRaw(idx, (digit & 0x08) ? colours[idx] : 0);
        ++idx;
        neopixels.setPixelRaw(idx, (digit & 0x04) ? colours[idx] : 0);
        ++idx;
        neopixels.setPixelRaw(idx, (digit & 0x02) ? colours[idx] : 0);
        ++idx;
        neopixels.setPixelRaw(idx, (digit & 0x01) ? colours[idx] : 0);
        ++idx;
    }

    return idx;
}

/// @brief Sets the RGB value of the first notification light.
/// @param rgb colour to set.
void BcdDisplay::setNotify1(uint32_t rgb)
{
    neopixels.setPixelRaw(28, convertToRaw(rgb));
}

/// @brief Sets the RGB value of the second notification light.
/// @param rgb colour to set.
void BcdDisplay::setNotify2(uint32_t rgb)
{
    neopixels.setPixelRaw(29, convertToRaw(rgb));
}


/// @brief Updates the display to display the given time
/// @param t  time to display, treated as utc.
void BcdDisplay::update(time_t t)
{
    lastTime = t;

    struct tm *utc = gmtime(&t);

    int idx = 0;

    // Hours
    int h = utc->tm_hour;
    idx = setDigit(idx, h / 10, true);
    idx = setDigit(idx, h % 10, false);

    // Colon (up)
    neopixels.setPixelRaw(idx, colours[idx]);
    ++idx;
    neopixels.setPixelRaw(idx, colours[idx]);
    ++idx;

    // Minutes
    int m = utc->tm_min;
    idx = setDigit(idx, m / 10, false);
    idx = setDigit(idx, m % 10, true);

    // Colon (down)
    neopixels.setPixelRaw(idx, colours[idx]);
    ++idx;
    neopixels.setPixelRaw(idx, colours[idx]);
    ++idx;

    // Seconds
    int s = utc->tm_sec;
    idx = setDigit(idx, s / 10, true);
    idx = setDigit(idx, s % 10, false);

    neopixels.send();
}

/// @brief Enables or disables auto-brightness
/// @param on 
void BcdDisplay::setAutoBrightness(bool on)
{
    autoBrightness = on;
}

/// @brief Sets the light level used for auto-brightness.
/// @param light level from the ADC
void BcdDisplay::setLightLevel(uint32_t light)
{
    const uint32_t top = 3800;     // max brightness above this.
    const uint32_t bottom = 700;   // min brightness below this
    const float base = 0.1f;  // don't go darker than this.
    float val;
    if( light > top){
        val = 1.0f;
    } else if(light < bottom){
        val = base;
    } else {
        val = base + (light - bottom) * (1.0f-base) / (top - bottom);
    }

    // recursive averaging - track light over time.
    const float smooth = 5.0f;
    brightness = (brightness * smooth + val)/(smooth + 1);


    // only actually use brightness if enabled.
    if(autoBrightness){
        setBrightness(brightness);
    }
}

/// @brief Redraws the display - use if colours are updated.
void BcdDisplay::redraw()
{
    update(lastTime);
}