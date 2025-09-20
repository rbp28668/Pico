
#include <time.h>
#include "display.hpp"

LedDisplay::LedDisplay()
    : tick(0)
    , chase(true)
    , rainbow(true)
    , sparkle(false)
    , delta_v(0.01f)
{
    const float a_nice_blue = 0.6652892561983471f;
    const float pink = 0.8636363636363636f;
    const float red = 1.0f;
    const float yellow = 0.16942148760330578f;
    const float green = 0.3415977961432507f;

    h = a_nice_blue;
    s = 1.0f;
    v = 1.0f;

    // 0x110000 RED
    // 0x001100 GREEN
    // 0x000011 BLUE

    
    uint32_t p = Neopixels::hsvToRgb(red,1.0f,0.2f);
    for(int i=0; i<LENGTH; ++i) colours[i] = p;
    
}




/// @brief Updates the display to display the given time
/// @param t  time to display, treated as utc.
void LedDisplay::update()
{
    shiftOut();

    h += rainbow ? delta_v : 0;

    if(h > 1.0f) h -= 1.0f;

    if(chase && ((tick % 20) == 0)) { 
        colours[0]  = Neopixels::hsvToRgb(h,s,v);
    } else {
        colours[0] = Neopixels::hsvToRgb(h,s,v*0.25f);
    }
    ++tick;

    for(int i=0; i<LENGTH; ++i)   neopixels.setPixel(i, colours[i]);  // single pixel in Neopixel format
 
    neopixels.send();
}




/// @brief Redraws the display - use if colours are updated.
void LedDisplay::redraw()
{
    update();
}