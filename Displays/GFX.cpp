/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

// TODO - use of textsize_x/textsize_y in conjunction with GFX font is
// inconsistent. Review and pick one!

#include "GFX.h"

#include <stdlib.h>
#include <string.h>

#include <cstdint>

#include "glcdfont.c"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif

/**************************************************************************/
/*!
   @brief    Instatiate a GFX context for graphics! Can only be done by a
   subclass
   @param    w   Display width, in pixels
   @param    h   Display height, in pixels
*/
/**************************************************************************/
GFX::GFX(int16_t w, int16_t h) : WIDTH(w), HEIGHT(h) {
  _width = WIDTH;
  _height = HEIGHT;
  rotation = 0;
  cursor_y = cursor_x = 0;
  textsize_x = textsize_y = 1;
  textcolor = textbgcolor = 0xFFFF;
  wrap = true;
  gfxFont = NULL;
  font = NULL;
  scroll_x = scroll_y = 0;
  scroll_width = _width;
  scroll_height = _height;
	scrollEnable = false;
  isWritingScrollArea= false;
  scrollbgcolor = 0xFFFF;

  setClipRect();
  setOrigin();
}

/**************************************************************************/
/*!
   @brief    Write a line.  Bresenham's algorithm - thx wikpedia.
   Optimised to use HLine or VLine segments where possible to minimise
   comms overhead on any serial display.

    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                    uint16_t color) {
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  int16_t xbegin = x0;
  if (steep) {
    for (; x0 <= x1; x0++) {
      err -= dy;
      if (err < 0) {
        int16_t len = x0 - xbegin;
        if (len) {
          writeFastVLine(y0, xbegin, len + 1, color);
        } else {
          writePixel(y0, x0, color);
        }
        xbegin = x0 + 1;
        y0 += ystep;
        err += dx;
      }
    }
    if (x0 > xbegin + 1) {
      writeFastVLine(y0, xbegin, x0 - xbegin, color);
    }
  } else {
    for (; x0 <= x1; x0++) {
      err -= dy;
      if (err < 0) {
        int16_t len = x0 - xbegin;
        if (len) {
          writeFastHLine(xbegin, y0, len + 1, color);
        } else {
          writePixel(x0, y0, color);
        }
        xbegin = x0 + 1;
        y0 += ystep;
        err += dx;
      }
    }
    if (x0 > xbegin + 1) {
      writeFastHLine(xbegin, y0, x0 - xbegin, color);
    }
  }
}

/**************************************************************************/
/*!
   @brief    Start a display-writing routine, overwrite in subclasses.
*/
/**************************************************************************/
void GFX::startWrite() {}

/**************************************************************************/
/*!
   @brief    Write a pixel, overwrite in subclasses if startWrite is defined!
    @param   x   x coordinate
    @param   y   y coordinate
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::writePixel(int16_t x, int16_t y, uint16_t color) {}

/**************************************************************************/
/*!
   @brief    Write a perfectly vertical line, overwrite in subclasses if
   startWrite is defined!
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  // Rudimentary clipping
  if ((x >= _width) || (x < 0) || (y >= _height)) return;
  if (y < 0) {
    h += y;
    y = 0;
  }
  if ((y + h - 1) >= _height) h = _height - y;
  while (h-- > 0) {
    writePixel(x, y + h, color);
  }
}

/**************************************************************************/
/*!
   @brief    Write a perfectly horizontal line, overwrite in subclasses if
   startWrite is defined!
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  // Rudimentary clipping
  if ((x >= _width) || (y >= _height) || (y < 0)) return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if ((x + w - 1) >= _width) w = _width - x;
  while (w-- > 0) {
    writePixel(x + w, y, color);
  }
}

/**************************************************************************/
/*!
   @brief    Write a rectangle completely with one color, overwrite in
   subclasses if startWrite is defined!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                        uint16_t color) {
  // Rudimentary clipping
  if ((x >= _width) || (x < 0) || (y >= _height)) return;
  if (y < 0) {
    h += y;
    y = 0;
  }
  if ((y + h - 1) >= _height) h = _height - y;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if ((x + w - 1) >= _width) w = _width - x;

  while (h-- > 0) {
    writeFastHLine(x, y + h, w, color);
  }
}

/**************************************************************************/
/*!
   @brief    End a display-writing routine, overwrite in subclasses if
   startWrite is defined!
*/
/**************************************************************************/
void GFX::endWrite() {}

//=========================================================================
// CONTROL API
// These MAY be overridden by the subclass to provide device-specific
// optimized code.  Otherwise 'generic' versions are used.
//=========================================================================

/**************************************************************************/
/*!
    @brief      Set rotation setting for display
    @param  x   0 thru 3 corresponding to 4 cardinal rotations
*/
/**************************************************************************/
void GFX::setRotation(uint8_t x) {
  rotation = (x & 3);
  switch (rotation) {
    case 0:
    case 2:
      _width = WIDTH;
      _height = HEIGHT;
      break;
    case 1:
    case 3:
      _width = HEIGHT;
      _height = WIDTH;
      break;
  }
  setCursor(0, 0);
  setClipRect();
  setOrigin();
}

/**************************************************************************/
/*!
    @brief      Invert the display (ideally using built-in hardware command)
    @param   i  True if you want to invert, false to make 'normal'
*/
/**************************************************************************/
void GFX::invertDisplay(bool i) {
  // Do nothing, must be subclassed if supported by hardware
  (void)i;  // disable -Wunused-parameter warning
}

//=========================================================================
// BASIC DRAW API
// These MAY be overridden by the subclass to provide device-specific
// optimized code.  Otherwise 'generic' versions are used.
//=========================================================================

/**************************************************************************/
/*!
   @brief    Draw a perfectly vertical line (this is often optimized in a
   subclass!)
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  startWrite();
  writeFastVLine(x, y, h, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly horizontal line (this is often optimized in a
   subclass!)
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  startWrite();
  writeFastHLine(x, y, w, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if
   desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  // rudimentary clipping (drawChar w/big text requires this)
  if ((x >= _width) || (y >= _height)) return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if ((x + w - 1) >= _width) w = _width - x;
  if ((y + h - 1) >= _height) h = _height - y;
  startWrite();
  writeFillRect(x, y, w, h, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Fill the screen completely with one color. Update in subclasses if
   desired!
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::fillScreen(uint16_t color) { fillRect(0, 0, _width, _height, color); }

/**************************************************************************/
/*!
   @brief    Draws a pixel.
    @param    x   point x coordinate
    @param    y   point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawPixel(int16_t x, int16_t y, uint16_t color) {
  startWrite();
  writePixel(x, y, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Draw a line
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                   uint16_t color) {
  // Update in subclasses if desired!
  if (x0 == x1) {
    if (y0 > y1) _swap_int16_t(y0, y1);
    drawFastVLine(x0, y0, y1 - y0 + 1, color);
  } else if (y0 == y1) {
    if (x0 > x1) _swap_int16_t(x0, x1);
    drawFastHLine(x0, y0, x1 - x0 + 1, color);
  } else {
    startWrite();
    writeLine(x0, y0, x1, y1, color);
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  startWrite();
  writeFastHLine(x, y, w, color);
  writeFastHLine(x, y + h - 1, w, color);
  writeFastVLine(x, y, h, color);
  writeFastVLine(x + w - 1, y, h, color);
  endWrite();
}

//=========================================================================
// CORE GFX Routines.
// May be over-ridden.
//=========================================================================

/**************************************************************************/
/*!
   @brief    Draw a circle outline
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  startWrite();
  writePixel(x0, y0 + r, color);
  writePixel(x0, y0 - r, color);
  writePixel(x0 + r, y0, color);
  writePixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    writePixel(x0 + x, y0 + y, color);
    writePixel(x0 - x, y0 + y, color);
    writePixel(x0 + x, y0 - y, color);
    writePixel(x0 - x, y0 - y, color);
    writePixel(x0 + y, y0 + x, color);
    writePixel(x0 - y, y0 + x, color);
    writePixel(x0 + y, y0 - x, color);
    writePixel(x0 - y, y0 - x, color);
  }
  endWrite();
}

/**************************************************************************/
/*!
    @brief    Quarter-circle drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    cornername  Mask bit #1 or bit #2 to indicate which quarters of
   the circle we're doing
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
                           uint8_t cornername, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      writePixel(x0 + x, y0 + y, color);
      writePixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      writePixel(x0 + x, y0 - y, color);
      writePixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      writePixel(x0 - y, y0 + x, color);
      writePixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      writePixel(x0 - y, y0 - x, color);
      writePixel(x0 - x, y0 - y, color);
    }
  }
}

/**************************************************************************/
/*!
   @brief    Draw a circle with filled color
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  startWrite();
  writeFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
  endWrite();
}

/**************************************************************************/
/*!
    @brief  Quarter-circle drawer with fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void GFX::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners,
                           int16_t delta, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++;  // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1) writeFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2) writeFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1) writeFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2) writeFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

/**************************************************************************/
/*!
   @brief   Draw a triangle with no fill color
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t x2, int16_t y2, uint16_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

/**************************************************************************/
/*!
   @brief     Draw a triangle with color-fill
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to fill/draw with
*/
/**************************************************************************/
void GFX::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t x2, int16_t y2, uint16_t color) {
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }
  if (y1 > y2) {
    _swap_int16_t(y2, y1);
    _swap_int16_t(x2, x1);
  }
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }

  startWrite();
  if (y0 == y2) {  // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    writeFastHLine(a, y0, b - a + 1, color);
    endWrite();
    return;
  }

  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
          dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
    last = y1;  // Include y1 scanline
  else
    last = y1 - 1;  // Skip it

  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b) _swap_int16_t(a, b);
    writeFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b) _swap_int16_t(a, b);
    writeFastHLine(a, y, b - a + 1, color);
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                        uint16_t color) {
  int16_t max_radius = ((w < h) ? w : h) / 2;  // 1/2 minor axis
  if (r > max_radius) r = max_radius;
  // smarter version
  startWrite();
  writeFastHLine(x + r, y, w - 2 * r, color);          // Top
  writeFastHLine(x + r, y + h - 1, w - 2 * r, color);  // Bottom
  writeFastVLine(x, y + r, h - 2 * r, color);          // Left
  writeFastVLine(x + w - 1, y + r, h - 2 * r, color);  // Right
  // draw four corners
  drawCircleHelper(x + r, y + r, r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw/fill with
*/
/**************************************************************************/
void GFX::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                        uint16_t color) {
  int16_t max_radius = ((w < h) ? w : h) / 2;  // 1/2 minor axis
  if (r > max_radius) r = max_radius;
  // smarter version
  startWrite();
  writeFillRect(x + r, y, w - 2 * r, h, color);
  // draw four corners
  fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
  endWrite();
}

// BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

/**************************************************************************/
/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position,
   using the specified foreground color (unset bits are transparent).
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void GFX::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w,
                     int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8;  // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = bitmap[j * byteWidth + i / 8];
      if (byte & 0x80) writePixel(x + i, y, color);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position,
   using the specified foreground (for set bits) and background (unset bits)
   colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
/**************************************************************************/
void GFX::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w,
                     int16_t h, uint16_t color, uint16_t bg) {
  int16_t byteWidth = (w + 7) / 8;  // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = bitmap[j * byteWidth + i / 8];
      writePixel(x + i, y, (byte & 0x80) ? color : bg);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw XBitMap Files (*.xbm), exported from GIMP.
   Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
   C Array can be directly used with this function.
   There is no RAM-resident version of this function; if generating bitmaps
   in RAM, use the format defined by drawBitmap() and call that instead.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
*/
/**************************************************************************/
void GFX::drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w,
                      int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8;  // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte >>= 1;
      else
        byte = *(&bitmap[j * byteWidth + i / 8]);
      // Nearly identical to drawBitmap(), only the bit order
      // is reversed here (left-to-right = LSB to MSB):
      if (byte & 0x01) writePixel(x + i, y, color);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 8-bit image (grayscale) at the specified (x,y)
   pos. Specifically for 8-bit display devices such as IS31FL3731; no color
   reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void GFX::drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
                              int16_t w, int16_t h) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      writePixel(x + i, y, bitmap[j * w + i]);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 8-bit image (grayscale) with a 1-bit mask
   (set bits = opaque, unset bits = clear) at the specified (x,y) position.
   BOTH buffers (grayscale and mask) must be RAM-residentt, no mix-and-match
   Specifically for 8-bit display devices such as IS31FL3731; no color
   reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    mask  byte array with mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void GFX::drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
                              const uint8_t *mask, int16_t w, int16_t h) {
  int16_t bw = (w + 7) / 8;  // Bitmask scanline pad = whole byte
  uint8_t byte = 0;
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = mask[j * bw + i / 8];
      if (byte & 0x80) {
        writePixel(x + i, y, bitmap[j * w + i]);
      }
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y)
   position. For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void GFX::drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w,
                        int16_t h) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      writePixel(x + i, y, bitmap[j * w + i]);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask (set
   bits = opaque, unset bits = clear) at the specified (x,y) position. BOTH
   buffers (color and mask) must be RAM-resident. For 16-bit display devices; no
   color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    mask  byte array with monochrome mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void GFX::drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap,
                        const uint8_t *mask, int16_t w, int16_t h) {
  int16_t bw = (w + 7) / 8;  // Bitmask scanline pad = whole byte
  uint8_t byte = 0;
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = mask[j * bw + i / 8];
      if (byte & 0x80) {
        writePixel(x + i, y, bitmap[j * w + i]);
      }
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   write 8 bit per pixel paletted bitmap
                                        bitmap data in array at pixels, one byte
   per pixel color palette data in array at palette
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    pixels is the array of 8 bit pixels
    @param    palette is the look up table for the colours
*/
/**************************************************************************/
void GFX::drawRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h,
                       const uint8_t *pixels, const uint16_t *palette) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      writePixel(x + i, y, palette[*pixels++]);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   write 4 bit per pixel paletted bitmap
                                        bitmap data in array at pixels, 4 bits
   per pixel, 2 pixels per byte color palette data in array at palette. width
   must be at least 2 pixels and a multiple of 2
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    pixels is the array of 8 bit pixels
    @param    palette is the look up table for the colours
*/
/**************************************************************************/
void GFX::drawRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h,
                       const uint8_t *pixels, const uint16_t *palette) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i += 2) {
      writePixel(x + i, y, palette[((*pixels) >> 4) & 0xF]);
      writePixel(x + i + 1, y, palette[(*pixels++) & 0xF]);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   write 2 bit per pixel paletted bitmap
                                        bitmap data in array at pixels, 4 pixels
   per byte color palette data in array at palette width must be at least 4
   pixels and a multiple of 4.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    pixels is the array of 8 bit pixels
    @param    palette is the look up table for the colours
*/
/**************************************************************************/
void GFX::drawRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h,
                       const uint8_t *pixels, const uint16_t *palette) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i += 4) {
      writePixel(x + i, y, palette[((*pixels) >> 6) & 0x3]);
      writePixel(x + i + 1, y, palette[((*pixels) >> 4) & 0x3]);
      writePixel(x + i + 2, y, palette[((*pixels) >> 2) & 0x3]);
      writePixel(x + i + 3, y, palette[(*pixels++) & 0x3]);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   write 1 bit per pixel paletted bitmap
                                        bitmap data in array at pixels, one byte
   per 8 pixels color palette data in array at palette width must be at least 8
   pixels and in a multiple of 8 pixels
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    pixels is the array of 8 bit pixels
    @param    palette is the look up table for the colours
*/
/**************************************************************************/
void GFX::drawRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h,
                       const uint8_t *pixels, const uint16_t *palette) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i += 8) {
      // unrolled loop might be faster?
      writePixel(x + i + 0, y, palette[((*pixels) >> 7) & 0x1]);
      writePixel(x + i + 1, y, palette[((*pixels) >> 6) & 0x1]);
      writePixel(x + i + 2, y, palette[((*pixels) >> 5) & 0x1]);
      writePixel(x + i + 3, y, palette[((*pixels) >> 4) & 0x1]);
      writePixel(x + i + 4, y, palette[((*pixels) >> 3) & 0x1]);
      writePixel(x + i + 5, y, palette[((*pixels) >> 2) & 0x1]);
      writePixel(x + i + 6, y, palette[((*pixels) >> 1) & 0x1]);
      writePixel(x + i + 7, y, palette[(*pixels++) & 0x1]);
    }
  }
  endWrite();
}

// fillRectVGradient	- fills area with vertical gradient
void GFX::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color1, uint16_t color2) {
  // rudimentary clipping (drawChar w/big text requires this)
  if ((x >= _width) || (y >= _height)) return;
  if ((x + w - 1) >= _width) w = _width - x;
  if ((y + h - 1) >= _height) h = _height - y;

  int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
  color565toRGB14(color1, r1, g1, b1);
  color565toRGB14(color2, r2, g2, b2);
  dr = (r2 - r1) / h;
  dg = (g2 - g1) / h;
  db = (b2 - b1) / h;
  r = r1;
  g = g1;
  b = b1;

  startWrite();
  for (y = h; y > 0; y--) {
    uint16_t color = RGB14tocolor565(r, g, b);

    for (x = w; x > 0; x--) {
      writePixel(x, y, color);
    }
    r += dr;
    g += dg;
    b += db;
  }
  endWrite();
}

// fillRectHGradient	- fills area with horizontal gradient
void GFX::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color1, uint16_t color2) {
  // rudimentary clipping (drawChar w/big text requires this)
  if ((x >= _width) || (y >= _height)) return;
  if ((x + w - 1) >= _width) w = _width - x;
  if ((y + h - 1) >= _height) h = _height - y;

  int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
  color565toRGB14(color1, r1, g1, b1);
  color565toRGB14(color2, r2, g2, b2);
  dr = (r2 - r1) / h;
  dg = (g2 - g1) / h;
  db = (b2 - b1) / h;
  r = r1;
  g = g1;
  b = b1;

  startWrite();
  for (y = h; y > 0; y--) {
    uint16_t color;
    for (x = w; x > 0; x--) {
      color = RGB14tocolor565(r, g, b);
      writePixel(x, y, color);
      r += dr;
      g += dg;
      b += db;
    }
    r = r1;
    g = g1;
    b = b1;
  }
  endWrite();
}

// fillScreenVGradient - fills screen with vertical gradient
void GFX::fillScreenVGradient(uint16_t color1, uint16_t color2) {
  fillRectVGradient(0, 0, _width, _height, color1, color2);
}

// fillScreenHGradient - fills screen with horizontal gradient
void GFX::fillScreenHGradient(uint16_t color1, uint16_t color2) {
  fillRectHGradient(0, 0, _width, _height, color1, color2);
}

// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color,
   no background)
    @param    size  Font magnification level, 1 is 'original' size
*/
/**************************************************************************/
void GFX::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
                   uint16_t bg, uint8_t size) {
  drawChar(x, y, c, color, bg, size, size);
}

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate (top for LCD)
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    fgcolor 16-bit 5-6-5 Color to draw chraracter with
    @param    bgcolor 16-bit 5-6-5 Color to fill background with (if same as
   color, no background)
    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
/**************************************************************************/
void GFX::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t fgcolor,
                   uint16_t bgcolor, uint8_t size_x, uint8_t size_y) {
  if (font) {
    drawCharILI(x, y, c, fgcolor, bgcolor, size_x, size_y);
  } else if (gfxFont) {
    drawCharGFX(x, y, c, fgcolor, bgcolor, size_x, size_y);
  } else {
    drawCharLCD(x, y, c, fgcolor, bgcolor, size_x, size_y);
  }
}

/*
void GFX::drawCharOriginal(int16_t x, int16_t y, unsigned char c,
                            uint16_t color, uint16_t bg, uint8_t size_x,
                            uint8_t size_y) {

  if (!gfxFont) { // 'Classic' built-in font (glcdfont)

    if ((x >= _width) ||              // Clip right
        (y >= _height) ||             // Clip bottom
        ((x + 6 * size_x - 1) < 0) || // Clip left
        ((y + 8 * size_y - 1) < 0))   // Clip top
      return;

    startWrite();
    for (int8_t i = 0; i < 5; i++) { // Char bitmap = 5 columns
      uint8_t line = glcdfont[c * 5 + i];
      for (int8_t j = 0; j < 8; j++, line >>= 1) {
        if (line & 1) {
          if (size_x == 1 && size_y == 1)
            writePixel(x + i, y + j, color);
          else
            writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y,
                          color);
        } else if (bg != color) {
          if (size_x == 1 && size_y == 1)
            writePixel(x + i, y + j, bg);
          else
            writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
        }
      }
    }
    if (bg != color) { // If opaque, draw vertical line for last column
      if (size_x == 1 && size_y == 1)
        writeFastVLine(x + 5, y, 8, bg);
      else
        writeFillRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
    }
    endWrite();

  } else { // Custom font

    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling
    // drawChar() directly with 'bad' characters of font may cause mayhem!

    c -= (uint8_t)*(&gfxFont->first);
    GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c);
    uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);

    uint16_t bo = *(&glyph->bitmapOffset);
    uint8_t w = *(&glyph->width), h = *(&glyph->height);
    int8_t xo = *(&glyph->xOffset),
           yo = *(&glyph->yOffset);
    uint8_t xx, yy, bits = 0, bit = 0;
    int16_t xo16 = 0, yo16 = 0;

    if (size_x > 1 || size_y > 1) {
      xo16 = xo;
      yo16 = yo;
    }

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL infortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.

    startWrite();
    for (yy = 0; yy < h; yy++) {
      for (xx = 0; xx < w; xx++) {
        if (!(bit++ & 7)) {
          bits = *(&bitmap[bo++]);
        }
        if (bits & 0x80) {
          if (size_x == 1 && size_y == 1) {
            writePixel(x + xo + xx, y + yo + yy, color);
          } else {
            writeFillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y,
                          size_x, size_y, color);
          }
        }
        bits <<= 1;
      }
    }
    endWrite();

  } // End classic vs custom font
}
*/

void GFX::measureCharLCD(unsigned char c, uint16_t *w, uint16_t *h) {
  // Adafruit-GFX default font has fixed 6x8 dimensions
  *w = 6 * textsize_x;
  *h = 8 * textsize_y;
}

void GFX::measureCharGFX(unsigned char c, uint16_t *w, uint16_t *h) {
  if (c < gfxFont->first || c > gfxFont->last) return;  //
  c -= gfxFont->first;
  GFXglyph *glyph = gfxFont->glyph + c;
  *w = glyph->width * textsize_x;
  *h = glyph->height * textsize_y;
}

void GFX::measureCharILI(unsigned char c, uint16_t *w, uint16_t *h) {
  // ILI9341_pico font
  *h = font->cap_height;
  *w = 0;

  uint32_t bitoffset;
  const uint8_t *data;

  if (c >= font->index1_first && c <= font->index1_last) {
    bitoffset = c - font->index1_first;
    bitoffset *= font->bits_index;
  } else if (c >= font->index2_first && c <= font->index2_last) {
    bitoffset =
        c - font->index2_first + font->index1_last - font->index1_first + 1;
    bitoffset *= font->bits_index;
  } else if (font->unicode) {
    return;  // TODO: implement sparse unicode
  } else {
    return;
  }

  data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);

  uint32_t encoding = fetchbits_unsigned(data, 0, 3);

  if (encoding != 0) return;

  // uint32_t width =
  fetchbits_unsigned(data, 3, font->bits_width);
  bitoffset = font->bits_width + 3;

  // uint32_t height =
  fetchbits_unsigned(data, bitoffset, font->bits_height);
  bitoffset += font->bits_height;

  // int32_t xoffset =
  fetchbits_signed(data, bitoffset, font->bits_xoffset);
  bitoffset += font->bits_xoffset;

  // int32_t yoffset =
  fetchbits_signed(data, bitoffset, font->bits_yoffset);
  bitoffset += font->bits_yoffset;

  uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
  *w = delta;
}

// Measure the dimensions for a single character
void GFX::measureChar(unsigned char c, uint16_t *w, uint16_t *h) {
  // Treat non-breaking space as normal space
  if (c == 0xa0) {
    c = ' ';
  }

  if (font) {
    measureCharILI(c, w, h);
  } else if (gfxFont) {
    measureCharGFX(c, w, h);
  } else {
    measureCharLCD(c, w, h);
  }
}

// Return the width of a text string
// - num =  max characters to process, or 0 for entire string (null-terminated)
uint16_t GFX::measureTextWidth(const char *text, int num) {
  uint16_t maxH = 0;
  uint16_t currH = 0;
  uint16_t n = num;

  if (n == 0) {
    n = strlen(text);
  };

  for (int i = 0; i < n; i++) {
    if (text[i] == '\n') {
      // For multi-line strings, retain max width
      if (currH > maxH) maxH = currH;
      currH = 0;
    } else {
      uint16_t h, w;
      measureChar(text[i], &w, &h);
      currH += w;
    }
  }
  uint16_t h = maxH > currH ? maxH : currH;
  return h;
}

// Return the height of a text string
// - num =  max characters to process, or 0 for entire string (null-terminated)
uint16_t GFX::measureTextHeight(const char *text, int num) {
  int lines = 1;
  uint16_t n = num;
  if (n == 0) {
    n = strlen(text);
  };
  for (int i = 0; i < n; i++) {
    if (text[i] == '\n') {
      lines++;
    }
  }
  return ((lines - 1) * fontLineSpace() + fontCapHeight());
}

/**************************************************************************/
/*!
    @brief  Helper to determine size of a string with current font/size.
            Pass string and a cursor position, returns UL corner and W,H.
    @param  str  The ASCII string to measure
    @param  x    The current cursor X
    @param  y    The current cursor Y
    @param  x1   The boundary X coordinate, returned by function
    @param  y1   The boundary Y coordinate, returned by function
    @param  w    The boundary width, returned by function
    @param  h    The boundary height, returned by function
*/
/**************************************************************************/
void GFX::getTextBounds(const char *str, int16_t x, int16_t y, int16_t *x1,
                        int16_t *y1, uint16_t *w, uint16_t *h) {
  uint8_t c;  // Current character
  int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;  // Bound rect
  // Bound rect is intentionally initialized inverted, so 1st char sets it

  *x1 = x;  // Initial position is value passed in
  *y1 = y;
  *w = *h = 0;  // Initial size is zero

  while ((c = *str++)) {
    // charBounds() modifies x/y to advance for each character,
    // and min/max x/y are updated to incrementally build bounding rect.
    charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  }

  if (maxx >= minx) {      // If legit string bounds were found...
    *x1 = minx;            // Update x1 to least X coord,
    *w = maxx - minx + 1;  // And w to bound rect width
  }
  if (maxy >= miny) {  // Same for height
    *y1 = miny;
    *h = maxy - miny + 1;
  }
}

/**************************************************************************/
/*!
    @brief  Helper to determine size of a string with current font/size.
            Pass string and a cursor position, returns UL corner and W,H.
    @param  buffer  The ASCII string to measure
    @param  len  the length of the string
    @param  x    The current cursor X
    @param  y    The current cursor Y
    @param  x1   The boundary X coordinate, returned by function
    @param  y1   The boundary Y coordinate, returned by function
    @param  w    The boundary width, returned by function
    @param  h    The boundary height, returned by function
*/
/**************************************************************************/
void GFX::getTextBounds(const uint8_t *buffer, uint16_t len, int16_t x,
                        int16_t y, int16_t *x1, int16_t *y1, uint16_t *w,
                        uint16_t *h) {
  int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;  // Bound rect
  // Bound rect is intentionally initialized inverted, so 1st char sets it

  *x1 = x;  // Initial position is value passed in
  *y1 = y;
  *w = *h = 0;  // Initial size is zero

  for (int i = 0; i < len; ++i) {
    uint8_t c = buffer[i];  // Current character
    // charBounds() modifies x/y to advance for each character,
    // and min/max x/y are updated to incrementally build bounding rect.
    charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  }

  if (maxx >= minx) {      // If legit string bounds were found...
    *x1 = minx;            // Update x1 to least X coord,
    *w = maxx - minx + 1;  // And w to bound rect width
  }
  if (maxy >= miny) {  // Same for height
    *y1 = miny;
    *h = maxy - miny + 1;
  }
}

/**************************************************************************/
/*!
    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel
   that much bigger. (so big blocky fonts)
   Note - only used for the built in lcd font.
    @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
/**************************************************************************/
void GFX::setTextSize(uint8_t s) { setTextSize(s, s); }

/**************************************************************************/
/*!
    @brief  Sets size multipliers in x and y direction.
            Is only applied to the basic LCD font. For the other fonts-
            just use a bigger font!
    @param  x  is the size multiplier in the x direction.
    @param  y  is the size multiplier in the y direction
*/
/**************************************************************************/
void GFX::setTextSize(uint8_t s_x, uint8_t s_y) {
  textsize_x = (s_x > 0) ? s_x : 1;
  textsize_y = (s_y > 0) ? s_y : 1;
}

/**************************************************************************/
/*!
    @brief  Gets the size multiplier of the basic LCD font.
            Warning - only returns the x direction.
*/
/**************************************************************************/
uint8_t GFX::getTextSize() {
  return textsize_x;  // bug bug 2 values now
}

/**************************************************************************/
/*!
   @brief  Gets the X size multiplier of the basic LCD font.
*/
/**************************************************************************/
uint8_t GFX::getTextSizeX() { return textsize_x; }

/**************************************************************************/
/*!
   @brief  Gets the Y size multiplier of the basic LCD font.
*/
/**************************************************************************/
uint8_t GFX::getTextSizeY() { return textsize_y; }

/**************************************************************************/
/*!
    @brief Set the GFX font to display when print()ing, either custom or default
    @param  f  The GFXfont object, if NULL use built in 6x8 font
*/
/**************************************************************************/
void GFX::setFont(const GFXfont *f) {
  if (f) {                    // Font struct pointer passed in?
    if (!gfxFont && !font) {  // And no current font struct?
      // Switching from classic to new font behavior.
      // Move cursor pos down 6 pixels so it's on baseline.
      cursor_y += 6;
    }
    font = 0;
    gfxFont = (GFXfont *)f;
  } else {  // NULL passed.
    // Current font struct defined?
    if (gfxFont || font) {
      // Switching from new to classic font behavior.
      // Move cursor pos up 6 pixels so it's at top-left of char.
      cursor_y -= 6;
    }
    font = 0;
    gfxFont = 0;
  }
  _last_char_x_write = 0;  // Don't use cached data here
}

/**************************************************************************/
/*!
    @brief Set the ILI9341 font to display when print()ing, either custom or
   default
    @param  f  The ILI9341 font object.
*/
/**************************************************************************/
void GFX::setFont(const Font &f) {
  if (!gfxFont && !font) {
    cursor_y -= 6;  // ???
  }

  gfxFont = NULL;

  font = &f;
  _last_char_x_write = 0;  // Don't use cached data here

  fontbpp = 1;
  // Calculate additional metrics for Anti-Aliased font support (BDF extn v2.3)
  if (font && font->version == 23) {
    fontbpp = (font->reserved & 0b000011) + 1;
    fontbppindex = (fontbpp >> 2) + 1;
    fontbppmask = (1 << (fontbppindex + 1)) - 1;
    fontppb = 8 / fontbpp;
    fontalphamx = 31 / ((1 << fontbpp) - 1);
    // Ensure text and bg color are different. Note: use setTextColor to set
    // actual bg color
    if (textcolor == textbgcolor)
      textbgcolor = (textcolor == 0x0000) ? 0xFFFF : 0x0000;
  }
}

/**********************************************************************/
/*!
    @brief  Set text cursor location.  Note top left for LCD fonts,
    baseline for others.
    @param  x    X coordinate in pixels
    @param  y    Y coordinate in pixels
  */
/**********************************************************************/
void GFX::setCursor(int16_t x, int16_t y, bool autoCenter) {
  _center_x_text = autoCenter;  // remember the state.
  _center_y_text = autoCenter;  // remember the state.
  if (x == CENTER) {
    _center_x_text = true;
    x = _width / 2;
  }
  if (y == CENTER) {
    _center_y_text = true;
    y = _height / 2;
  }
  if (x < 0)
    x = 0;
  else if (x >= _width)
    x = _width - 1;
  cursor_x = x;
  if (y < 0)
    y = 0;
  else if (y >= _height)
    y = _height - 1;
  cursor_y = y;

  if (x >= scroll_x && x <= (scroll_x + scroll_width) && y >= scroll_y &&
      y <= (scroll_y + scroll_height)) {
    isWritingScrollArea = true;
  } else {
    isWritingScrollArea = false;
  }
  _last_char_x_write = 0;  // Don't use cached data here
}

/**********************************************************************/
/*!
    @brief   Gets the current position of the cursor
    @param   x returns the x coordinate of the cursor.
    @param   y returns the y coordinate of the cursor.

  */
/**********************************************************************/
void GFX::getCursor(int16_t *x, int16_t *y) {
  *x = cursor_x;
  *y = cursor_y;
}

/**********************************************************************/
/*!
    @brief   Set text font color with transparant background
    @param   c   16-bit 5-6-5 Color to draw text with
    @note    For 'transparent' background, background and foreground
             are set to same color rather than using a separate flag.
  */
/**********************************************************************/
void GFX::setTextColor(uint16_t c) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  setTextColor(c, c);
}

/**********************************************************************/
/*!
    @brief   Set text font color with custom background color
    @param   c   16-bit 5-6-5 Color to draw text with
    @param   bg  16-bit 5-6-5 Color to draw background/fill with
  */
/**********************************************************************/
void GFX::setTextColor(uint16_t c, uint16_t b) {
  textcolor = c;
  textbgcolor = b;
  // pre-expand colors for fast alpha-blending later
  textcolorPrexpanded =
      (textcolor | (textcolor << 16)) & 0b00000111111000001111100000011111;
  textbgcolorPrexpanded =
      (textbgcolor | (textbgcolor << 16)) & 0b00000111111000001111100000011111;
}

/**********************************************************************/
/*!
  @brief  Set whether text that is too long for the screen width should
          automatically wrap around to the next line (else clip right).
  @param  w  true for wrapping, false for clipping
  */
/**********************************************************************/
void GFX::setTextWrap(bool w) { wrap = w; }

/**********************************************************************/
/*!
  @brief  get whether text that is too long for the screen width should
          automatically wrap around to the next line (else clip right).
  */
/**********************************************************************/
bool GFX::getTextWrap() { return wrap; }

/**************************************************************************/
/*!
    @brief  Print one byte/character of data, used to support print()
    @param  c  The 8-bit ascii character to write
*/
/**************************************************************************/
size_t GFX::write(uint8_t c) {

  if (c == '\n') {               // Newline?
    cursor_x = 0;                // Reset x to zero,
    cursor_y += fontLineSpace();  // advance y one line
  } else if (c != '\r') {        // Ignore carriage returns
    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
    uint16_t w,h;
    measureChar(c,&w,&h);
    cursor_x += w;  // Advance x one char
  }

  return 1;
}

/**************************************************************************/
/*!
    @brief  Gets the nominal height of a capital letter in the current font
*/
/**************************************************************************/
uint16_t GFX::fontCapHeight() {
  if (font) {
    return font->cap_height;
  } else if (gfxFont) {
    return textsize_y * gfxFont->yAdvance;  // as good as we can (easily) manage
                                            // for the font as a whole
  } else {
    return textsize_y * 8;
  }
}

/**************************************************************************/
/*!
    @brief  Gets the amount to advance the y coordinate between lines
    using the current font.
*/
/**************************************************************************/
uint16_t GFX::fontLineSpace() {
  if (font) {
    return font->line_space;
  } else if (gfxFont) {
    return textsize_y * gfxFont->yAdvance;
  } else {
    return textsize_y * 8;
  }
}

/**************************************************************************/
/*!
    @brief  Gets the nominal vertical gap between capital letters in the 
    current font.
*/
/**************************************************************************/
uint16_t GFX::fontGap() {
  if (font) {
    return font->line_space - font->cap_height;
  } else if (gfxFont) {
    return 0;
  } else {
    return 0;
  }
}

int GFX::drawCharLCD(int16_t& cursor_x, int16_t& cursor_y, unsigned char c, uint16_t fgcolor,
                     uint16_t bgcolor, uint8_t size_x, uint8_t size_y) {
  int16_t x = cursor_x;
  int16_t y = cursor_y;

  // TODO implement wrap, clipping etc.

  if ((x >= _width) ||               // Clip right
      (y >= _height) ||              // Clip bottom
      ((x + 6 * size_x - 1) < 0) ||  // Clip left  TODO: is this correct?
      ((y + 8 * size_y - 1) < 0))    // Clip top   TODO: is this correct?
    return 0;

  if (fgcolor == bgcolor) {
    // This transparent approach is only about 20% faster
    if ((size_x == 1) && (size_y == 1)) {
      uint8_t mask = 0x01;
      int16_t xoff, yoff;
      for (yoff = 0; yoff < 8; yoff++) {
        uint8_t line = 0;
        for (xoff = 0; xoff < 5; xoff++) {
          if (glcdfont[c * 5 + xoff] & mask) line |= 1;
          line <<= 1;
        }
        line >>= 1;
        xoff = 0;
        while (line) {
          if (line == 0x1F) {
            drawFastHLine(x + xoff, y + yoff, 5, fgcolor);
            break;
          } else if (line == 0x1E) {
            drawFastHLine(x + xoff, y + yoff, 4, fgcolor);
            break;
          } else if ((line & 0x1C) == 0x1C) {
            drawFastHLine(x + xoff, y + yoff, 3, fgcolor);
            line <<= 4;
            xoff += 4;
          } else if ((line & 0x18) == 0x18) {
            drawFastHLine(x + xoff, y + yoff, 2, fgcolor);
            line <<= 3;
            xoff += 3;
          } else if ((line & 0x10) == 0x10) {
            drawPixel(x + xoff, y + yoff, fgcolor);
            line <<= 2;
            xoff += 2;
          } else {
            line <<= 1;
            xoff += 1;
          }
        }
        mask = mask << 1;
      }
    } else {
      uint8_t mask = 0x01;
      int16_t xoff, yoff;
      for (yoff = 0; yoff < 8; yoff++) {
        uint8_t line = 0;
        for (xoff = 0; xoff < 5; xoff++) {
          if (glcdfont[c * 5 + xoff] & mask) line |= 1;
          line <<= 1;
        }
        line >>= 1;
        xoff = 0;
        while (line) {
          if (line == 0x1F) {
            fillRect(x + xoff * size_x, y + yoff * size_y, 5 * size_x, size_y,
                     fgcolor);
            break;
          } else if (line == 0x1E) {
            fillRect(x + xoff * size_x, y + yoff * size_y, 4 * size_x, size_y,
                     fgcolor);
            break;
          } else if ((line & 0x1C) == 0x1C) {
            fillRect(x + xoff * size_x, y + yoff * size_y, 3 * size_x, size_y,
                     fgcolor);
            line <<= 4;
            xoff += 4;
          } else if ((line & 0x18) == 0x18) {
            fillRect(x + xoff * size_x, y + yoff * size_y, 2 * size_x, size_y,
                     fgcolor);
            line <<= 3;
            xoff += 3;
          } else if ((line & 0x10) == 0x10) {
            fillRect(x + xoff * size_x, y + yoff * size_y, size_x, size_y,
                     fgcolor);
            line <<= 2;
            xoff += 2;
          } else {
            line <<= 1;
            xoff += 1;
          }
        }
        mask = mask << 1;
      }
    }
  } else {
    // Solid
    uint8_t xc, yc;
    uint8_t xr, yr;
    uint8_t mask = 0x01;
    uint16_t color;

    // We need to offset by the origin.
    x += _originx;
    y += _originy;
    int16_t x_char_start = x;  // remember our X where we start outputting...

    if ((x >= _displayclipx2) ||  // Clip right
        (y >= _displayclipy2) ||  // Clip bottom
        ((x + 6 * size_x - 1) <
         _displayclipx1) ||  // Clip left  TODO: this is not correct
        ((y + 8 * size_y - 1) <
         _displayclipy1))  // Clip top   TODO: this is not correct
      return 0;

    // need to build actual pixel rectangle we will output into.
    int16_t y_char_top = y;  // remember the y
    int16_t w = 6 * size_x;
    int16_t h = 8 * size_y;

    if (x < _displayclipx1) {
      w -= (_displayclipx1 - x);
      x = _displayclipx1;
    }
    if ((x + w - 1) >= _displayclipx2) w = _displayclipx2 - x;
    if (y < _displayclipy1) {
      h -= (_displayclipy1 - y);
      y = _displayclipy1;
    }
    if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

    startWrite();
    y = y_char_top;  // restore the actual y.
    for (yc = 0; (yc < 8) && (y < _displayclipy2); yc++) {
      for (yr = 0; (yr < size_y) && (y < _displayclipy2); yr++) {
        x = x_char_start;  // get our first x position...
        if (y >= _displayclipy1) {
          for (xc = 0; xc < 5; xc++) {
            if (glcdfont[c * 5 + xc] & mask) {
              color = fgcolor;
            } else {
              color = bgcolor;
            }
            for (xr = 0; xr < size_x; xr++) {
              if ((x >= _displayclipx1) && (x < _displayclipx2)) {
                writePixel(x, y, fgcolor);
              }
              x++;
            }
          }
          for (xr = 0; xr < size_x; xr++) {
            if ((x >= _displayclipx1) && (x < _displayclipx2)) {
              writePixel(x, y, bgcolor);
            }
            x++;
          }
        }
        y++;
      }
      mask = mask << 1;
    }
    endWrite();
  }
  cursor_x += 6 * size_x;
  return 6 * size_x;
}

int GFX::drawCharGFX(int16_t& cursor_x, int16_t& cursor_y, unsigned char c, uint16_t color,
                     uint16_t bg, uint8_t size_x, uint8_t size_y) {
  // Character is assumed previously filtered by write() to eliminate
  // newlines, returns, non-printable characters, etc.  Calling
  // drawChar() directly with 'bad' characters of font may cause mayhem!

  c -= (uint8_t) * (&gfxFont->first);
  GFXglyph *glyph = gfxFont->glyph + c;
  uint8_t *bitmap = gfxFont->bitmap;

  uint16_t bo = *(&glyph->bitmapOffset);
  uint8_t w = *(&glyph->width), h = *(&glyph->height);
  int8_t xo = *(&glyph->xOffset), yo = *(&glyph->yOffset);
  uint8_t xx, yy, bits = 0, bit = 0;
  int16_t xo16 = 0, yo16 = 0;

  if (size_x > 1 || size_y > 1) {
    xo16 = xo;
    yo16 = yo;
  }

  // TODO - implement clipping and wrap

  startWrite();
  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = *(&bitmap[bo++]);
      }
      if (bits & 0x80) {
        if (size_x == 1 && size_y == 1) {
          writePixel(cursor_x + xo + xx, cursor_y + yo + yy, color);
        } else {
          writeFillRect(cursor_x + (xo16 + xx) * size_x, cursor_y + (yo16 + yy) * size_y,
                        size_x, size_y, color);
        }
      }
      bits <<= 1;
    }
  }
  endWrite();

  cursor_x += glyph->xAdvance * size_x;
  return glyph->xAdvance * size_x;
}

int GFX::drawCharILI(int16_t& cursor_x, int16_t& cursor_y, unsigned char c,
                     uint16_t color, uint16_t bg, uint8_t size_x,
                     uint8_t size_y) {
  uint32_t bitoffset;
  const uint8_t *data;

  // Serial.printf("drawFontChar(%c) %d\n", c, c);

  if (c >= font->index1_first && c <= font->index1_last) {
    bitoffset = c - font->index1_first;
    bitoffset *= font->bits_index;
  } else if (c >= font->index2_first && c <= font->index2_last) {
    bitoffset =
        c - font->index2_first + font->index1_last - font->index1_first + 1;
    bitoffset *= font->bits_index;
  } else if (font->unicode) {
    return 0;  // TODO: implement sparse unicode
  } else {
    return 0;  // Can't print this character in this font.
  }

  // bitoffset now has the bit offset into the index.  The value from the index offsets
  // into the font data to point to the first byte storying the actual glyph.
  data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);

  // Start running through the glyph data unpacking it as we go.

  uint32_t encoding = fetchbits_unsigned(data, 0, 3);
  if (encoding != 0) return 0;  // some new encoding we can't cope with

  // Skipping initial 3 encoding bits...
  uint32_t width = fetchbits_unsigned(data, 3, font->bits_width);
  bitoffset = font->bits_width + 3;  
  uint32_t height = fetchbits_unsigned(data, bitoffset, font->bits_height);
  bitoffset += font->bits_height;

  int32_t xoffset = fetchbits_signed(data, bitoffset, font->bits_xoffset);
  bitoffset += font->bits_xoffset;
  int32_t yoffset = fetchbits_signed(data, bitoffset, font->bits_yoffset);
  bitoffset += font->bits_yoffset;

  uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
  bitoffset += font->bits_delta;

  // horizontally, we draw every pixel, or none at all
  if (cursor_x < 0) cursor_x = 0;
  int32_t origin_x = cursor_x + xoffset;
  if (origin_x < 0) {
    cursor_x -= xoffset;
    origin_x = 0;
  }
  if (origin_x + (int)width > _width) {
    if (!wrap) return 0;
    origin_x = 0;
    if (xoffset >= 0) {
      cursor_x = 0;
    } else {
      cursor_x = -xoffset;
    }
    cursor_y += font->line_space;
  }
  if (wrap && scrollEnable && isWritingScrollArea && ((origin_x + (int)width) > (scroll_x + scroll_width))) {
    origin_x = 0;
    if (xoffset >= 0) {
      cursor_x = scroll_x;
    } else {
      cursor_x = -xoffset;
    }
    cursor_y += font->line_space;
  }

  if (scrollEnable && isWritingScrollArea &&  (cursor_y > (scroll_y + scroll_height - font->cap_height))) {
    scrollTextArea(font->line_space);
    cursor_y -= font->line_space;
    cursor_x = scroll_x;
  }
  if (cursor_y >= _height) return 0;

  // vertically, the top and/or bottom can be clipped
  int32_t origin_y = cursor_y + font->cap_height - height - yoffset;
  // Serial.printf("  origin = %d,%d\n", origin_x, origin_y);

  // TODO: compute top skip and number of lines
  int32_t linecount = height;
  // uint32_t loopcount = 0;
  int32_t y = origin_y;
  bool opaque = (textbgcolor != textcolor);

  // Going to try a fast Opaque method which works similar to drawChar, which is
  // near the speed of writerect
  if (!opaque) {
    // Anti-alias support
    if (fontbpp > 1) {
      // This branch should, in most cases, never happen. This is because if an
      // anti-aliased font is being used, textcolor and textbgcolor should
      // always be different. Even though an anti-alised font is being used,
      // pixels in this case will all be solid because pixels are rendered on
      // same colour as themselves! This won't look very good.
      bitoffset = ((bitoffset + 7) & (-8));  // byte-boundary
      uint32_t xp = 0;
      uint8_t halfalpha = 1 << (fontbpp - 1);
      while (linecount) {
        uint32_t x = 0;
        while (x < width) {
          // One pixel at a time, either on (if alpha > 0.5) or off
          if (fetchpixel(data, bitoffset, xp) >= halfalpha) {
            fontPixel(origin_x + x, y, textcolor);
          }
          bitoffset += fontbpp;
          x++;
          xp++;
        }
        y++;
        linecount--;
      }
    }
    // Solid pixels
    else {
      while (linecount > 0) {
        // Serial.printf("    linecount = %d\n", linecount);
        uint32_t n = 1;
        if (fetchbit(data, bitoffset++) != 0) {
          n = fetchbits_unsigned(data, bitoffset, 3) + 2;
          bitoffset += 3;
        }
        uint32_t x = 0;
        do {
          int32_t xsize = width - x;
          if (xsize > 32) xsize = 32;
          uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
          // Serial.printf("    multi line %d %d %x\n", n, x, bits);
          drawFontBits(opaque, bits, xsize, origin_x + x, y, n);
          bitoffset += xsize;
          x += xsize;
        } while (x < width);

        y += n;
        linecount -= n;
        // if (++loopcount > 100) {
        // Serial.println("     abort draw loop");
        // break;
        //}
      }
    }  // 1bpp
  }

  // opaque
  else {
    // Now opaque mode...
    // Now write out background color for the number of rows above the above the
    // character figure out bounding rectangle... In this mode we need to update
    // to use the offset and bounding rectangles as we are doing it it direct.
    // also update the Origin
    int cursor_x_origin = cursor_x + _originx;
    int cursor_y_origin = cursor_y + _originy;
    origin_x += _originx;
    origin_y += _originy;

    int start_x = (origin_x < cursor_x_origin) ? origin_x : cursor_x_origin;
    if (start_x < 0) start_x = 0;

    int start_y = (origin_y < cursor_y_origin) ? origin_y : cursor_y_origin;
    if (start_y < 0) start_y = 0;
    int end_x = cursor_x_origin + delta;
    if ((origin_x + (int)width) > end_x) end_x = origin_x + (int)width;
    if (end_x >= _displayclipx2) end_x = _displayclipx2;
    int end_y = cursor_y_origin + font->line_space;
    if ((origin_y + (int)height) > end_y) end_y = origin_y + (int)height;
    if (end_y >= _displayclipy2) end_y = _displayclipy2;
    end_x--;  // setup to last one we draw
    end_y--;
    int start_x_min = (start_x >= _displayclipx1) ? start_x : _displayclipx1;
    int start_y_min = (start_y >= _displayclipy1) ? start_y : _displayclipy1;

    // See if anything is in the display area.
    if ((end_x < _displayclipx1) || (start_x >= _displayclipx2) ||
        (end_y < _displayclipy1) || (start_y >= _displayclipy2)) {
      cursor_x += delta;  // could use goto or another indent level...
      return 0;
    }

    startWrite();
    // Serial.printf("SetAddr %d %d %d %d\n", start_x_min, start_y_min, end_x,
    // end_y);
    // output rectangle we are updating... We have already clipped end_x/y, but
    // not yet start_x/y
    int screen_y = start_y_min;
    int screen_x;

    // Clear above character
    while (screen_y < origin_y) {
      for (screen_x = start_x_min; screen_x <= end_x; screen_x++) {
        fontPixel(screen_x, screen_y, textbgcolor);
      }
      screen_y++;
    }

    // Anti-aliased font
    if (fontbpp > 1) {
      screen_y = origin_y;
      bitoffset = ((bitoffset + 7) & (-8));  // byte-boundary
      int glyphend_x = origin_x + width;
      uint32_t xp = 0;
      while (linecount) {
        screen_x = start_x;
        while (screen_x <= end_x) {
          // XXX: I'm sure clipping could be done way more efficiently than just
          // checking every single pixel, but let's just get this going
          if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2) &&
              (screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
            // Clear before or after pixel
            if ((screen_x < origin_x) || (screen_x >= glyphend_x)) {
              fontPixel(screen_x, screen_y, textbgcolor);
            }
            // Draw alpha-blended character
            else {
              uint8_t alpha = fetchpixel(data, bitoffset, xp);
              fontPixel(screen_x, screen_y,
                alphaBlendRGB565Premultiplied( textcolorPrexpanded, textbgcolorPrexpanded, (uint8_t)(alpha * fontalphamx))
                );
              bitoffset += fontbpp;
              xp++;
            }
          }  // clip
          screen_x++;
        }
        screen_y++;
        linecount--;
      }

    }  // anti-aliased

    // 1bpp
    else {
      // Now lets process each of the data lines.
      screen_y = origin_y;
      while (linecount > 0) {
        // Serial.printf("    linecount = %d\n", linecount);
        uint32_t b = fetchbit(data, bitoffset++);
        uint32_t n;
        if (b == 0) {
          // Serial.println("    Single");
          n = 1;
        } else {
          // Serial.println("    Multi");
          n = fetchbits_unsigned(data, bitoffset, 3) + 2;
          bitoffset += 3;
        }
        uint32_t bitoffset_row_start = bitoffset;
        while (n--)  // one or more copies of the line.
        {
          // do some clipping here.
          bitoffset = bitoffset_row_start;  // we will work through these bits
                                            // maybe multiple times
          // We need to handle case where some of the bits may not be visible,
          // but we still need to read through them
          // Serial.printf("y:%d  %d %d %d %d\n", screen_y, start_x, origin_x,
          // _displayclipx1, _displayclipx2);
          if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
            for (screen_x = start_x; screen_x < origin_x; screen_x++) {
              if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2)) {
                // Serial.write('-');
                fontPixel(screen_x, screen_y, textbgcolor);
              }
            }
          }
          uint32_t x = 0;
          screen_x = origin_x;
          do {
            uint32_t xsize = width - x;
            if (xsize > 32) xsize = 32;
            uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
            uint32_t bit_mask = 1 << (xsize - 1);
            // Serial.printf("     %d %d %x %x - ", x, xsize, bits, bit_mask);
            if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
              while (bit_mask) {
                if ((screen_x >= _displayclipx1) &&
                    (screen_x < _displayclipx2)) {
                  fontPixel(screen_x, screen_y,
                            (bits & bit_mask) ? textcolor : textbgcolor);
                  // Serial.write((bits & bit_mask) ? '*' : '.');
                }
                bit_mask = bit_mask >> 1;
                screen_x++;  // Current actual screen X
              }
              // Serial.println();
              bitoffset += xsize;
            }
            x += xsize;
          } while (x < width);
          if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
            // output bg color and right hand side
            while (screen_x++ <= end_x) {
              fontPixel(screen_x, screen_y, textbgcolor);
              // Serial.write('+');
            }
            // Serial.println();
          }
          screen_y++;
          linecount--;
        }
      }
    }  // 1bpp

    // clear below character - note reusing xcreen_x for this
    screen_x =
        (end_y + 1 - screen_y) *
        (end_x + 1 - start_x_min);  // How many bytes we need to still output
    // Serial.printf("Clear Below: %d\n", screen_x);
    while (screen_x-- > 0) {
      fontPixel(screen_x, screen_y, textbgcolor);
    }
    endWrite();
  }

  cursor_x += delta;  // Update
  
  // Increment to setup for the next character.
  return delta;
}

/**************************************************************************/
/*!
    @brief  Print one byte/character of data using whatever font is
    selected.
    @param  c  The 8-bit ascii character to write
*/
/**************************************************************************/
void GFX::drawFontChar(unsigned int c, int16_t& cursor_x, int16_t& cursor_y) {
  if (font) {
    drawCharILI(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
  } else if (gfxFont) {
    drawCharGFX(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
  } else {  // revert to LCD font
    drawCharLCD(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
  }
}

/**************************************************************************/
/*!
   @brief gets pixel length of a given ASCII string.  Note that this
   takes account of newlines as line breaks.  The length of the longest
   line is returned. Works for all font types.  This is independent of
   where the string is printed - it assumes no wrapping unlike the
   textBounds methods.
    @param    str is the string to measure.
    @param    w is used to return the width of the string
    @param    h is used to return the height of the string
*/
/**************************************************************************/

void GFX::measureText(const char * str, uint16_t *w, uint16_t *h){
  *w = *h = 0;
  if (!str) return;

  uint16_t len = 0, maxlen = 0, height = 0;
  while (*str++) {
    if (*str == '\n') {
      if (len > maxlen) {
        maxlen = len;
        len = 0;
        *h += height;
        height = 0;
      }
    } else {  // "Proper" character to measure lenth;

      // Figure out what to do with the different fonts.
      if (font) {  // ILI font in use?

        uint32_t bitoffset;
        const uint8_t *data;
        uint16_t c = *str;

        if (c >= font->index1_first && c <= font->index1_last) {
          bitoffset = c - font->index1_first;
          bitoffset *= font->bits_index;
        } else if (c >= font->index2_first && c <= font->index2_last) {
          bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
          bitoffset *= font->bits_index;
        } else if (font->unicode) {
          continue; // can't handle unicode
        } else {
          continue; // char not in this font
        }
  
        height = font->line_space;

        data = font->data +    fetchbits_unsigned(font->index, bitoffset, font->bits_index);

        uint32_t encoding = fetchbits_unsigned(data, 0, 3);
        if (encoding != 0) continue;
  
        bitoffset = font->bits_width + 3;
        bitoffset += font->bits_height;

        bitoffset += font->bits_xoffset;
        bitoffset += font->bits_yoffset;

        uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
        bitoffset += font->bits_delta;
  
        len += delta;  

      } else if (gfxFont) { // GFX Font
        uint16_t c = *str;
        if (c < gfxFont->first || c > gfxFont->last) continue;  //
        c -= gfxFont->first;
        GFXglyph *glyph = gfxFont->glyph + c;
        len += glyph->width * textsize_x;
        height = gfxFont->yAdvance * textsize_y;
      } else {  // LCD font
        len += textsize_x * 6;
        height = textsize_y * 8;
      }

    }  // endif "proper char"
  }    // end while *str++

  if (len > maxlen) {
    maxlen = len;
  }

  *w = maxlen;
  *h += height;
}

/***************************************************************************************
** Function name:           drawNumber
** Description:             draw a long integer
***************************************************************************************/
int16_t GFX::drawNumber(long long_num, int poX, int poY) {
  char str[14];
  utoa(long_num, str, 10);  // was ltoa(long_num, str, 10);
  return drawString(str, poX, poY);
}

int16_t GFX::drawFloat(float floatNumber, int dp, int poX, int poY) {
  char str[14];          // Array to contain decimal string
  uint8_t ptr = 0;       // Initialise pointer for array
  int8_t digits = 1;     // Count the digits to avoid array overflow
  float rounding = 0.5;  // Round up down delta

  if (dp > 7) dp = 7;  // Limit the size of decimal portion

  // Adjust the rounding value
  for (uint8_t i = 0; i < dp; ++i) rounding /= 10.0f;

  if (floatNumber < -rounding)  // add sign, avoid adding - sign to 0.0!
  {
    str[ptr++] = '-';  // Negative number
    str[ptr] = 0;      // Put a null in the array as a precaution
    digits =
        0;  // Set digits to 0 to compensate so pointer value can be used later
    floatNumber = -floatNumber;  // Make positive
  }

  floatNumber += rounding;  // Round up or down

  // For error put ... in string and return (all TFT_ILI9341_ESP library fonts
  // contain . character)
  if (floatNumber >= 2147483647) {
    strcpy(str, "...");
    // return drawString(str, poX, poY);
  }
  // No chance of overflow from here on

  // Get integer part
  unsigned long temp = (unsigned long)floatNumber;

  // Put integer part into array
  utoa(temp, str + ptr, 10);  // was ltoa(temp, str + ptr, 10);

  // Find out where the null is to get the digit count loaded
  while ((uint8_t)str[ptr] != 0) ptr++;  // Move the pointer along
  digits += ptr;                         // Count the digits

  str[ptr++] = '.';  // Add decimal point
  str[ptr] = '0';    // Add a dummy zero
  str[ptr + 1] =
      0;  // Add a null but don't increment pointer so it can be overwritten

  // Get the decimal portion
  floatNumber = floatNumber - temp;

  // Get decimal digits one by one and put in array
  // Limit digit count so we don't get a false sense of resolution
  uint8_t i = 0;
  while ((i < dp) &&
         (digits <
          9))  // while (i < dp) for no limit but array size must be increased
  {
    i++;
    floatNumber *= 10;          // for the next decimal
    temp = floatNumber;         // get the decimal
    utoa(temp, str + ptr, 10);  // was ltoa(temp, str + ptr, 10);
    ptr++;
    digits++;             // Increment pointer and digits count
    floatNumber -= temp;  // Remove that digit
  }

  // Finally we can plot the string and return pixel length
  return drawString(str, poX, poY);
}

/***************************************************************************************
** Function name:           drawString (with or without user defined font)
** Description :            draw string with padding if it is defined
***************************************************************************************/
// Without font number, uses font set by setTextFont()
int16_t GFX::drawString(const char *string, int poX, int poY) {
  int16_t len = strlen(string);
  return drawString1(string, len, poX, poY);
}

/***************************************************************************************
** Function name:           drawString1 (with or without user defined font)
** Description :            draw string as array of char with padding if it is
*defined
***************************************************************************************/
int16_t GFX::drawString1(const char string[], int16_t len, int poX, int poY) {
  uint8_t padding = 1 /*, baseline = 0*/;

  uint16_t cwidth; 
  uint16_t cheight;
  measureText(string, &cwidth, &cheight);

  if (textdatum) {
    switch (textdatum) {
      case TC_DATUM:
        poX -= cwidth / 2;
        padding += 1;
        break;
      case TR_DATUM:
        poX -= cwidth;
        padding += 2;
        break;
      case ML_DATUM:
        poY -= cheight / 2;
        // padding += 0;
        break;
      case MC_DATUM:
        poX -= cwidth / 2;
        poY -= cheight / 2;
        padding += 1;
        break;
      case MR_DATUM:
        poX -= cwidth;
        poY -= cheight / 2;
        padding += 2;
        break;
      case BL_DATUM:
        poY -= cheight;
        // padding += 0;
        break;
      case BC_DATUM:
        poX -= cwidth / 2;
        poY -= cheight;
        padding += 1;
        break;
      case BR_DATUM:
        poX -= cwidth;
        poY -= cheight;
        padding += 2;
        break;
        /*
        case L_BASELINE:
          poY -= baseline;
          //padding += 0;
          break;
        case C_BASELINE:
          poX -= cwidth/2;
          poY -= baseline;
          //padding += 1;
          break;
        case R_BASELINE:
          poX -= cwidth;
          poY -= baseline;
          padding += 2;
          break;
          */
    }
    // Check coordinates are OK, adjust if not
    if (poX < 0) poX = 0;
    if (poX + cwidth > width()) poX = width() - cwidth;
    if (poY < 0) poY = 0;
    // if (poY+cheight-baseline >_height) poY = _height - cheight;
  }
  setCursor(poX, poY);
  for (uint8_t i = 0; i < len; i++) {
    drawFontChar(string[i], cursor_x, cursor_y);
  }
  return cwidth;
}

/***************************************************************************************
** Function name:           setTextDatum
** Description:             Set the text position reference datum
***************************************************************************************/
void GFX::setTextDatum(uint8_t d) { textdatum = d; }

void GFX::scrollTextArea(uint8_t scrollSize) {
  uint16_t awColors[scroll_width];
  for (int y = scroll_y + scrollSize; y < (scroll_y + scroll_height); y++) {
    readRect(scroll_x, y, scroll_width, 1, awColors);
    writeRect(scroll_x, y - scrollSize, scroll_width, 1, awColors);
  }
  fillRect(scroll_x, (scroll_y + scroll_height) - scrollSize, scroll_width, scrollSize, scrollbgcolor);
}

void GFX::setScrollTextArea(int16_t x, int16_t y, int16_t w, int16_t h) {
  scroll_x = x;
  scroll_y = y;
  scroll_width = w;
  scroll_height = h;
}

void GFX::setScrollBackgroundColor(uint16_t color) {
  scrollbgcolor = color;
  fillRect(scroll_x, scroll_y, scroll_width, scroll_height, scrollbgcolor);
}

void GFX::enableScroll(void) { scrollEnable = true; }

void GFX::disableScroll(void) { scrollEnable = false; }

void GFX::resetScrollBackgroundColor(uint16_t color) { scrollbgcolor = color; }

// Read pixel & read/write rect to enable scrolling.  Subclass MUST over-ride
// one of the read methods.

uint16_t GFX::readPixel(int16_t x, int16_t y) {
  uint16_t colors = 0;
  readRect(x, y, 1, 1, &colors);
  return colors;
}

// Now lets see if we can read in multiple pixels
void GFX::readRect(int16_t x, int16_t y, int16_t w, int16_t h,
                   uint16_t *pcolors) {
  // Use our Origin.
  x += _originx;
  y += _originy;
  // TODO validate parameters
  for (int16_t iy = 0; iy < h; ++iy) {
    for (int16_t ix = 0; ix < w; ++ix) {
      *pcolors++ = readPixel(x + ix, y + iy);
    }
  }
}

// Write multiple pixels
void GFX::writeRect(int16_t x, int16_t y, int16_t w, int16_t h,
                    uint16_t *pcolors) {
  // Use our Origin.
  x += _originx;
  y += _originy;
  // TODO validate parameters
  for (int16_t iy = 0; iy < h; ++iy) {
    for (int16_t ix = 0; ix < w; ++ix) {
      writePixel(x + ix, y + iy, *pcolors);
    }
  }
}

void GFX::charBoundsLCD(unsigned char c, int16_t *x, int16_t *y, int16_t *minx,
                        int16_t *miny, int16_t *maxx, int16_t *maxy) {
  if (c == '\n') {         // Newline?
    *x = 0;                // Reset x to zero,
    *y += textsize_y * 8;  // advance y one line
    // min/max x/y unchaged -- that waits for next 'normal' character
  } else if (c != '\r') {  // Normal char; ignore carriage returns
    if (wrap && ((*x + textsize_x * 6) > _width)) {  // Off right?
      *x = 0;                                        // Reset x to zero,
      *y += textsize_y * 8;                          // advance y one line
    }
    int x2 = *x + textsize_x * 6 - 1,  // Lower-right pixel of char
        y2 = *y + textsize_y * 8 - 1;
    if (x2 > *maxx) *maxx = x2;  // Track max x, y
    if (y2 > *maxy) *maxy = y2;
    if (*x < *minx) *minx = *x;  // Track min x, y
    if (*y < *miny) *miny = *y;
    *x += textsize_x * 6;  // Advance x one char
  }
}

void GFX::charBoundsGFX(unsigned char c, int16_t *x, int16_t *y, int16_t *minx,
                        int16_t *miny, int16_t *maxx, int16_t *maxy) {
  if (c == '\n') {  // Newline?
    *x = 0;         // Reset x to zero, advance y by one line
    *y += textsize_y * (uint8_t) * (&gfxFont->yAdvance);
  } else if (c != '\r') {  // Not a carriage return; is normal char
    uint8_t first = *(&gfxFont->first), last = *(&gfxFont->last);
    if ((c >= first) && (c <= last)) {  // Char present in this font?
      GFXglyph *glyph = gfxFont->glyph + c - first;
      uint8_t gw = *(&glyph->width), gh = *(&glyph->height),
              xa = *(&glyph->xAdvance);
      int8_t xo = *(&glyph->xOffset), yo = *(&glyph->yOffset);
      if (wrap && ((*x + (((int16_t)xo + gw) * textsize_x)) > _width)) {
        *x = 0;  // Reset x to zero, advance y by one line
        *y += textsize_y * (uint8_t) * (&gfxFont->yAdvance);
      }
      int16_t tsx = (int16_t)textsize_x, tsy = (int16_t)textsize_y,
              x1 = *x + xo * tsx, y1 = *y + yo * tsy, x2 = x1 + gw * tsx - 1,
              y2 = y1 + gh * tsy - 1;
      if (x1 < *minx) *minx = x1;
      if (y1 < *miny) *miny = y1;
      if (x2 > *maxx) *maxx = x2;
      if (y2 > *maxy) *maxy = y2;
      *x += xa * tsx;
    }
  }
}

void GFX::charBoundsILI(unsigned char c, int16_t *x, int16_t *y, int16_t *minx,
                        int16_t *miny, int16_t *maxx, int16_t *maxy) {
  if (c == '\n') {  // Newline?
    *x = 0;         // Reset x to zero, advance y by one line
    *y += font->line_space;
  } else if (c != '\r') {  // Not a carriage return; is normal char
    uint32_t bitoffset;

    // Convert c into a bit index into the index array (or bail).
    if (c >= font->index1_first && c <= font->index1_last) {
      bitoffset = c - font->index1_first;
      bitoffset *= font->bits_index;
    } else if (c >= font->index2_first && c <= font->index2_last) {
      bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
      bitoffset *= font->bits_index;
    } else if (font->unicode) {
      return;
    } else {
      return;
    }

    // Now use the offset value from the index to find where this character is
    // stored in the data.
    const uint8_t *data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);
    bitoffset = 0;  // Now relative to the start of the data.

    // First 3 bits are the encoding. If it's not 0 this doesn't know to to
    // decode.
    uint32_t encoding = fetchbits_unsigned(data, bitoffset, 3);
    if (encoding != 0) return;
    bitoffset += 3;

    // Get metadata.
    uint16_t width = fetchbits_unsigned(font->data, bitoffset, font->bits_width);
    bitoffset += font->bits_width;

    uint16_t height = fetchbits_unsigned(font->data, bitoffset, font->bits_height);
    bitoffset += font->bits_height;

    int16_t xo = fetchbits_signed(font->data, bitoffset, font->bits_xoffset);
    bitoffset += font->bits_xoffset;

    int16_t yo = fetchbits_signed(font->data, bitoffset, font->bits_yoffset);
    bitoffset += font->bits_yoffset;

    uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
    bitoffset += font->bits_delta;

    // bitoffset now points to the bitmap itself for this char - not needed
    // here.

    // If this would clip then wrap if enabled.
    if (wrap && (*x + xo + width) > _width) {
      *x = 0;  // Reset x to zero, advance y by one line
      *y += font->line_space;
    }
    int16_t x1 = *x + xo;
    int16_t y1 = *y + yo;
    int16_t x2 = x1 + width;
    int16_t y2 = y1 + height;
    if (x1 < *minx) *minx = x1;
    if (y1 < *miny) *miny = y1;
    if (x2 > *maxx) *maxx = x2;
    if (y2 > *maxy) *maxy = y2;
    *x += delta;
  }
}

/**************************************************************************/
/*!
    @brief  Helper to determine size of a character with current font/size.
            Used by getTextBounds() functions.
    @param  c     The ASCII character in question
    @param  x     Pointer to x location of character. Value is modified by
                  this function to advance to next character.
    @param  y     Pointer to y location of character. Value is modified by
                  this function to advance to next character.
    @param  minx  Pointer to minimum X coordinate, passed in to AND returned
                  by this function -- this is used to incrementally build a
                  bounding rectangle for a string.
    @param  miny  Pointer to minimum Y coord, passed in AND returned.
    @param  maxx  Pointer to maximum X coord, passed in AND returned.
    @param  maxy  Pointer to maximum Y coord, passed in AND returned.
*/
/**************************************************************************/
void GFX::charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx,
                     int16_t *miny, int16_t *maxx, int16_t *maxy) {
  if (font) {
    charBoundsILI(c, x, y, minx, miny, maxx, maxy);
  } else if (gfxFont) {
    charBoundsGFX(c, x, y, minx, miny, maxx, maxy);
  } else {  // Default font
    charBoundsLCD(c, x, y, minx, miny, maxx, maxy);
  }
}

// Various static methods for font wrangling.  The ILI9341 fonts
// are highly compressed.

// Fetch a given bit at at bit offset from the start of the array
// ILI9341 fonts
uint32_t GFX::fetchbit(const uint8_t *p, uint32_t index) {
  if (p[index >> 3] & (1 << (7 - (index & 7)))) return 1;
  return 0;
}

// Fetch a given number of bits at at bit offset from the start of the array
// ILI9341 fonts
uint32_t GFX::fetchbits_unsigned(const uint8_t *p, uint32_t index,
                                 uint32_t required) {
  uint32_t val = 0;
  do {
    uint8_t b = p[index >> 3];
    uint32_t avail = 8 - (index & 7);
    if (avail <= required) {
      val <<= avail;
      val |= b & ((1 << avail) - 1);
      index += avail;
      required -= avail;
    } else {
      b >>= avail - required;
      val <<= required;
      val |= b & ((1 << required) - 1);
      break;
    }
  } while (required);
  return val;
}

// Fetch a range of bits as an unsigned number from a bit offset from the start
// of the array ILI9341 fonts
uint32_t GFX::fetchbits_signed(const uint8_t *p, uint32_t index,
                               uint32_t required) {
  uint32_t val = fetchbits_unsigned(p, index, required);
  if (val & (1 << (required - 1))) {
    return (int32_t)val - (1 << required);
  }
  return (int32_t)val;
}

uint32_t GFX::fetchpixel(const uint8_t *p, uint32_t index, uint32_t x) {
  // The byte
  uint8_t b = p[index >> 3];
  // Shift to LSB position and mask to get value
  uint8_t s = ((fontppb - (x % fontppb) - 1) * fontbpp);
  // Mask and return
  return (b >> s) & fontbppmask;
}

void GFX::drawFontBits(bool opaque, uint32_t bits, uint32_t numbits, int32_t x,
                       int32_t y, uint32_t repeat) {
  if (bits == 0) {
    if (opaque) {
      fillRect(x, y, numbits, repeat, textbgcolor);
    }
  } else {
    int32_t x1 = x;
    uint32_t n = numbits;
    int w;
    int bgw;

    w = 0;
    bgw = 0;

    do {
      n--;
      if (bits & (1 << n)) {
        if (bgw > 0) {
          if (opaque) {
            fillRect(x1 - bgw, y, bgw, repeat, textbgcolor);
          }
          bgw = 0;
        }
        w++;
      } else {
        if (w > 0) {
          fillRect(x1 - w, y, w, repeat, textcolor);
          w = 0;
        }
        bgw++;
      }
      x1++;
    } while (n > 0);

    if (w > 0) {
      fillRect(x1 - w, y, w, repeat, textcolor);
    }

    if (bgw > 0) {
      if (opaque) {
        fillRect(x1 - bgw, y, bgw, repeat, textbgcolor);
      }
    }
  }
}
