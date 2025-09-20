# GFX For Rasperry Pi Pico
These libraries are ported for the Raspberry Pi Pico.  They make no attempt to retain compatibility with any of the existing Teensy or orther Arduino hardware although do implement print and the base Adafruit GFX interface to make porting software from Arduino easier.
The aim is to back-port and "regularise" a lot of the Teensy extensions to GFX so that the functionality is universally available as far as is practical.
They generally favour composability and flexibility over absolute performance.
Virtual functions are used more than in the original GFX libraries - the overhead is low and they provide more ability for sub-classes to optimise functions.
The SPI or I2C transfers are likely to dominate the timings.  In particular, TFT controller chips that allow you to set an address window and then shovel in pixels will benefit from over-riding anything that can use this model (any variation of a rectangular fill or writing a character with a solid background).  Other controllers may allow more complex operations

# Over-riding GFX
To implement a library for a given display the minimum you need to over-ride in the TRANSACTION API / CORE DRAW API is writePixel. You'll probably need to over-ride startWrite / endWrite.  The other write methods have default implementations written in terms of writePixel so it is strongly recommended that these are also over-ridden as well.

Other methods may be over-ridden, especially when they can follow the "set address / shovel in pixels" pattern supported by the ST and ILI chipsets.
Note that writing individual pixels involves 3x8 bit commands and 5x16 bits of data (104 bits) which is a lot of overhead for a single 16 bit pixel value. As such,
"block" writes supported by the ST & ILI chipsets are valuable in reducing this overhead.  Paradoxically this quite often means that writing every pixel in
a rectangle (e.g. a character with an opaque background) is faster than just writing the foreground pixels.  This also means that secondary optimisations 
such as inlining, avoiding virtual functions etc are negligible compared to minimising the amount of data sent to the controller.

Note: The coordinate system places the origin (0,0) at the top left corner, with positive X increasing to the right and positive Y increasing downward.

## Variables
_screenWidth, _screenHeight - actuall size of display
_width, _height - current width and height of the display allowing for rotation
_xorigin, _yorigin
_clipx1, _clipx2, _clipy1, _clipy2 - define a clipping box relative to the origin so that to be displayed a pixel x has to be >= _clipx1 and < _clipx2 (similarly for y). 
to the origin.
_displayclipx1..._displayclipy2 - define the clipping box relative to the screen - i.e.  _clipx1 + _xorigin etc. 
clip(x+xorigin)
int16_t scroll_x, scroll_y, scroll_width, scroll_height;

in TFTDisplay
_xstart, _ystart - added to the parameters in setAddr so always offset the written pixel location by these values. These are used to provide fixed offsets
inherent in the hardware.  These do however swap round when display is rotated.
_rowstart, _colstart - set when initialising to a given driver or variation of board - used to set _xstart and _ystart when the display is rotated.


# Fonts
There are 3 types of fonts available and not all the functions work the same for each.  These are the basic LCD font, the GFX 1 bit per pixel fonts and the "ILI3941" fonts which in some cases have more than 1 bits per pixel (anti-aliased).
The basic LCD font is fixed size but you can specify an integer multiplier in x and y directions
The Adafruit fonts also allow the multiplier (although it's usually much better just to use a bigger font)
The ILI fonts ignore the text size - just use a bigger font.

## glcdfont
This is the basic LCD font that's used if no other font is specified.  get/set text size just scale this by an integer multiplier in the x and y directions. 

## ILI9341 fonts
## Adafruit fonts

# Acknowledgments

ILI9341 / ST7735 / ST7738 from Paul Stoffregen's Teensy ports of the Adafruit libraries.
https://github.com/PaulStoffregen/ST7735_t3

GFX and the drivers originally written by Limor Fried/Ladyada for Adafruit Industries.
Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

 