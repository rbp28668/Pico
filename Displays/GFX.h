#ifndef _GFX_H
#define _GFX_H

#include "print.h"
#include "gfxfont.h"
#include "fonts.h"

/// A generic graphics superclass that can handle all sorts of drawing. At a
/// minimum you can subclass and provide drawPixel(). At a maximum you can do a
/// ton of overriding to optimize. Used for any/all Adafruit displays!
class GFX : public Print {

protected:

template<class T>  const T& min(const T& a, const T& b) { return (b < a) ? b : a; }
template<class T>  const T& max(const T& a, const T& b) { return (b > a) ? b : a; }
template<class T>  const T abs(const T& a) { return (a < 0) ? -a : a;  }
template<class T>  void swap( T& a, T& b) { T temp = a; a = b; b = temp;  }


public:

	static const int16_t CENTER = 9998; // "magic value" to auto-centre in setCursor()


// Color definitions
enum Color {
  BLACK        = 0x0000,      /*   0,   0,   0 */
  NAVY         = 0x000F,      /*   0,   0, 128 */
  DARKGREEN    = 0x03E0,      /*   0, 128,   0 */
  DARKCYAN     = 0x03EF,      /*   0, 128, 128 */
  MAROON       = 0x7800,      /* 128,   0,   0 */
  PURPLE       = 0x780F,      /* 128,   0, 128 */
  OLIVE        = 0x7BE0,      /* 128, 128,   0 */
  LIGHTGREY    = 0xC618,      /* 192, 192, 192 */
  DARKGREY     = 0x7BEF,      /* 128, 128, 128 */
  BLUE         = 0x001F,      /*   0,   0, 255 */
  GREEN        = 0x07E0,      /*   0, 255,   0 */
  CYAN         = 0x07FF,      /*   0, 255, 255 */
  RED          = 0xF800,      /* 255,   0,   0 */
  MAGENTA      = 0xF81F,      /* 255,   0, 255 */
  YELLOW       = 0xFFE0,      /* 255, 255,   0 */
  WHITE        = 0xFFFF,      /* 255, 255, 255 */
  ORANGE       = 0xFD20,      /* 255, 165,   0 */
  GREENYELLOW  = 0xAFE5,      /* 173, 255,  47 */
  PINK         = 0xF81F
};

//These enumerate the text plotting alignment (reference datum point)
enum TextDatum {
  TL_DATUM = 0, // Top left (default)
  TC_DATUM = 1, // Top centre
  TR_DATUM = 2, // Top right
  ML_DATUM = 3, // Middle left 
  CL_DATUM = 3, // Centre left, same as above
  MC_DATUM = 4, // Middle centre
  CC_DATUM = 4, // Centre centre, same as above
  MR_DATUM = 5, // Middle right
  CR_DATUM = 5, // Centre right, same as above
  BL_DATUM = 6, // Bottom left
  BC_DATUM = 7, // Bottom centre
  BR_DATUM = 8, // Bottom right
  //L_BASELINE  9 // Left character baseline (Line the 'A' character would sit on)
  //C_BASELINE 10 // Centre character baseline
  //R_BASELINE 11 // Right character baseline
};

  GFX(int16_t w, int16_t h); // Constructor


  // TRANSACTION API / CORE DRAW API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void startWrite(void);
  virtual void writePixel(int16_t x, int16_t y, uint16_t color);
  virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  virtual void endWrite(void);

  // CONTROL API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void setRotation(uint8_t r);
  virtual void invertDisplay(bool i);

  // BASIC DRAW API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.

  // It's good to implement those, even if using transaction API
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  virtual void fillScreen(uint16_t color);
 
  // Optional and probably not necessary to change
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
  virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

  // Normally these exist only with GFX BUT can be over-ridden if a display architecture
  // gives significant speedup.
  virtual void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  virtual void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
  virtual void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  virtual void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
  virtual void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  virtual void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  virtual void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
  virtual void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
  virtual void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
  virtual void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
  virtual void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
  virtual void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);
  virtual void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t *bitmap, const uint8_t *mask,  int16_t w, int16_t h);
  virtual void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w,  int16_t h);
  virtual void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, const uint8_t *mask, int16_t w, int16_t h);
	virtual void drawRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	virtual void drawRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	virtual void drawRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	virtual void drawRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	virtual void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);
	virtual void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);
	virtual void fillScreenVGradient(uint16_t color1, uint16_t color2);
	virtual void fillScreenHGradient(uint16_t color1, uint16_t color2);

  virtual void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
  virtual void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);


	void measureChar(uint8_t c, uint16_t* w, uint16_t* h);
	uint16_t measureTextWidth(const char* text, int chars = 0);
	uint16_t measureTextHeight(const char* text, int chars = 0);
  void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
  void getTextBounds(const uint8_t *buffer, uint16_t len, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
  void setTextSize(uint8_t s);
  void setTextSize(uint8_t sx, uint8_t sy);
  uint8_t getTextSize();
  uint8_t getTextSizeX();
  uint8_t getTextSizeY();

  void setFont(const GFXfont *f = NULL);
  void setFont(const Font &f);
 	void setFontDefault(void) { setFont(); }

 	uint16_t fontCapHeight();
	uint16_t fontLineSpace();
	uint16_t fontGap();
    
  virtual void setCursor(int16_t x, int16_t y, bool autoCenter = false);
  void getCursor(int16_t *x, int16_t *y);
  void setTextColor(uint16_t c);
  void setTextColor(uint16_t c, uint16_t bg);
  void setTextWrap(bool w);
  bool getTextWrap();

  // Support Print class 
   using Print::write;
  virtual size_t write(uint8_t);

  // Font management and drawing
 	virtual void drawFontChar(unsigned int c, int16_t& cursor_x, int16_t& cursor_y);
	virtual void measureText(const char * str, uint16_t *w, uint16_t *h);

  // added support for drawing strings/numbers/floats with centering
  // modified from tft_ili9341_ESP github library
  // Handle numbers
  virtual int16_t  drawNumber(long long_num,int poX, int poY);
  virtual int16_t  drawFloat(float floatNumber,int decimal,int poX, int poY);   
  virtual int16_t drawString(const char*  string, int poX, int poY);
  virtual int16_t drawString1(const char string[], int16_t len, int poX, int poY);
  virtual void setTextDatum(uint8_t datum);


   // added support for scrolling text area
    // https://github.com/vitormhenrique/ILI9341_t3
    // Discussion regarding this optimized version:
    // http://forum.pjrc.com/threads/26305-Highly-optimized-ILI9341-%28320x240-TFT-color-display%29-library
	//	
	virtual void setScrollTextArea(int16_t x, int16_t y, int16_t w, int16_t h);
	virtual void setScrollBackgroundColor(uint16_t color);
	virtual void enableScroll(void);
	virtual void disableScroll(void);
	virtual void scrollTextArea(uint8_t scrollSize);
	virtual void resetScrollBackgroundColor(uint16_t color);
	virtual uint16_t readPixel(int16_t x, int16_t y);
	virtual void readRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors);
  virtual void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors);

  /************************************************************************/
  /*!
    @brief      Get width of the display, accounting for current rotation
    @returns    Width in pixels
  */
  /************************************************************************/
  int16_t width(void) const { return _width; };

  /************************************************************************/
  /*!
    @brief      Get height of the display, accounting for current rotation
    @returns    Height in pixels
  */
  /************************************************************************/
  int16_t height(void) const { return _height; }

  /************************************************************************/
  /*!
    @brief      Get rotation setting for display
    @returns    0 thru 3 corresponding to 4 cardinal rotations
  */
  /************************************************************************/
  uint8_t getRotation(void) const { return rotation; }

  // get current cursor position (get rotation safe maximum values,
  // using: width() for x, height() for y)
  /************************************************************************/
  /*!
    @brief  Get text cursor X location
    @returns    X coordinate in pixels
  */
  /************************************************************************/
  int16_t getCursorX(void) const { return cursor_x; }

  /************************************************************************/
  /*!
    @brief      Get text cursor Y location
    @returns    Y coordinate in pixels
  */
  /************************************************************************/
  int16_t getCursorY(void) const { return cursor_y; };


  
	// setOrigin sets an offset in display pixels where drawing to (0,0) will appear
	// for example: setOrigin(10,10); drawPixel(5,5); will cause a pixel to be drawn at hardware pixel (15,15)
	void setOrigin(int16_t x = 0, int16_t y = 0) { 
		_originx = x; _originy = y; 
		//if (Serial) Serial.printf("Set Origin %d %d\n", x, y);
		updateDisplayClip();
	}
	void getOrigin(int16_t* x, int16_t* y) { *x = _originx; *y = _originy; }

	// setClipRect() sets a clipping rectangle (relative to any set origin) for drawing to be limited to.
	// Drawing is also restricted to the bounds of the display

	void setClipRect(int16_t x1, int16_t y1, int16_t w, int16_t h) 
		{ _clipx1 = x1; _clipy1 = y1; _clipx2 = x1+w; _clipy2 = y1+h; 
			//if (Serial) Serial.printf("Set clip Rect %d %d %d %d\n", x1, y1, w, h);
			updateDisplayClip();
		}

	void setClipRect() {
			 _clipx1 = 0; _clipy1 = 0; _clipx2 = _width; _clipy2 = _height; 
			//if (Serial) Serial.printf("clear clip Rect\n");
			updateDisplayClip(); 
		}	
////

 


protected:

  // Allow derived classes to set screen size from outside constructor.
  void setScreenSize(int16_t w, int16_t h) {
    WIDTH = _width = w;
    HEIGHT = _height = h;
  }


  void measureCharLCD(unsigned char c, uint16_t* w, uint16_t* h);
  void measureCharGFX(unsigned char c, uint16_t* w, uint16_t* h);
  void measureCharILI(unsigned char c, uint16_t* w, uint16_t* h);

  // Make virtual to allow optimised implementations in subclasses
  virtual int drawCharLCD(int16_t& x, int16_t& y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
  virtual int drawCharGFX(int16_t& x, int16_t& y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
  virtual int drawCharILI(int16_t& x, int16_t& y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);


  // Helper methods for ILI9341 fonts
  static uint32_t fetchbit(const uint8_t *p, uint32_t index);
  static uint32_t fetchbits_unsigned(const uint8_t *p, uint32_t index, uint32_t required);
  static uint32_t fetchbits_signed(const uint8_t *p, uint32_t index, uint32_t required);
  uint32_t fetchpixel(const uint8_t *p, uint32_t index, uint32_t x);
  void drawFontBits(bool opaque, uint32_t bits, uint32_t numbits, int32_t x, int32_t y, uint32_t repeat);


  // Use to build up bounds of a string
  void charBoundsLCD(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);
  void charBoundsGFX(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);
  void charBoundsILI(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);
  void charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);


  
  // Write pixel with offset and clipping.
  void fontPixel(int16_t x, int16_t y, uint16_t color)
	  __attribute__((always_inline)) {
	  x+=_originx;
	  y+=_originy;

	  if((x < _displayclipx1) ||(x >= _displayclipx2) || (y < _displayclipy1) || (y >= _displayclipy2)) return;
    writePixel(x,y,color);
	}


// Updates the clip rectangle	  
  inline void updateDisplayClip() {
    _displayclipx1 = max<int16_t>(0,min<int16_t>( _clipx1+_originx, width()));
    _displayclipx2 = max<int16_t>(0,min<int16_t>( _clipx2+_originx, width()));

   _displayclipy1 = max<int16_t>(0,min<int16_t>(_clipy1+_originy, height()));
   _displayclipy2 = max<int16_t>(0,min<int16_t>(_clipy2+_originy, height()));
   _invisible = (_displayclipx1 == _displayclipx2 || _displayclipy1 == _displayclipy2);
   _standard =  (_displayclipx1 == 0) && (_displayclipx2 == _width) && (_displayclipy1 == 0) && (_displayclipy2 == _height);
  }

  //////// Colour manipulation //////

	// Pass 8-bit (each) R,G,B, get back 16-bit packed color
	static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
		return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
	}

	//color565toRGB		- converts 565 format 16 bit color to RGB
	static inline void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
		r = (color>>8)&0x00F8;
		g = (color>>3)&0x00FC;
		b = (color<<3)&0x00F8;
	}
	
	//color565toRGB14		- converts 16 bit 565 format color to 14 bit RGB (2 bits clear for math and sign)
	//returns 00rrrrr000000000,00gggggg00000000,00bbbbb000000000
	//thus not overloading sign, and allowing up to double for additions for fixed point delta
	static inline void color565toRGB14(uint16_t color, int16_t &r, int16_t &g, int16_t &b) {
		r = (color>>2)&0x3E00;
		g = (color<<3)&0x3F00;
		b = (color<<9)&0x3E00;
	}
	
	//RGB14tocolor565		- converts 14 bit RGB back to 16 bit 565 format color
	static inline uint16_t RGB14tocolor565(int16_t r, int16_t g, int16_t b)
	{
		return (((r & 0x3E00) << 2) | ((g & 0x3F00) >>3) | ((b & 0x3E00) >> 9));
	}


  	/**
	 * Found in a pull request for the Adafruit framebuffer library. Clever!
	 * https://github.com/tricorderproject/arducordermini/pull/1/files#diff-d22a481ade4dbb4e41acc4d7c77f683d
	 * Converts  0000000000000000rrrrrggggggbbbbb
	 *     into  00000gggggg00000rrrrr000000bbbbb
	 * with mask 00000111111000001111100000011111
	 * This is useful because it makes space for a parallel fixed-point multiply
	 * This implements the linear interpolation formula: result = bg * (1.0 - alpha) + fg * alpha
	 * This can be factorized into: result = bg + (fg - bg) * alpha
	 * alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
	 * @param	fg		Color to draw in RGB565 (16bit)
	 * @param	bg		Color to draw over in RGB565 (16bit)
	 * @param	alpha	Alpha in range 0-255
	 **/
	uint16_t alphaBlendRGB565( uint32_t fg, uint32_t bg, uint8_t alpha )
	 __attribute__((always_inline)) {
	 	alpha = ( alpha + 4 ) >> 3; // from 0-255 to 0-31
		bg = (bg | (bg << 16)) & 0b00000111111000001111100000011111;
		fg = (fg | (fg << 16)) & 0b00000111111000001111100000011111;
		uint32_t result = ((((fg - bg) * alpha) >> 5) + bg) & 0b00000111111000001111100000011111;
		return (uint16_t)((result >> 16) | result); // contract result
	}
	/**
	 * Same as above, but fg and bg are premultiplied, and alpah is already in range 0-31
	 */
	uint16_t alphaBlendRGB565Premultiplied( uint32_t fg, uint32_t bg, uint8_t alpha )
	 __attribute__((always_inline)) {
		uint32_t result = ((((fg - bg) * alpha) >> 5) + bg) & 0b00000111111000001111100000011111;
		return (uint16_t)((result >> 16) | result); // contract result
	}

 

  int16_t WIDTH;        ///< This is the 'raw' display width - never changes
  int16_t HEIGHT;       ///< This is the 'raw' display height - never changes
  int16_t _width;       ///< Display width as modified by current rotation
  int16_t _height;      ///< Display height as modified by current rotation
  int16_t cursor_x;     ///< x location to start print()ing text
  int16_t cursor_y;     ///< y location to start print()ing text
  bool _center_x_text = false;  // if set cursor has center flag set 
  bool _center_y_text = false;  // if set cursor has center flag set 
  uint16_t textcolor;   ///< 16-bit background color for print()
  uint16_t textbgcolor; ///< 16-bit text color for print()
  uint32_t textcolorPrexpanded;  ///< Pre expanded for alpha blending
  uint32_t textbgcolorPrexpanded; ///< Pre expanded for alpha blending

  uint8_t textsize_x;   ///< Desired magnification in X-axis of text to print()
  uint8_t textsize_y;   ///< Desired magnification in Y-axis of text to print()
  uint8_t textdatum;    ///< For positioning text

  uint8_t rotation;     ///< Display rotation (0 thru 3)
  bool wrap;            ///< If set, 'wrap' text at right edge of display
  
  int16_t scroll_x;
  int16_t scroll_y;
  int16_t scroll_width;
  int16_t scroll_height;
	bool scrollEnable;
  bool isWritingScrollArea; // If set, 'wrap' text at right edge of display
  uint16_t scrollbgcolor;  // background colour to be used when scrolling

  // Clip rectangle and origin 
  int16_t  _clipx1, _clipy1, _clipx2, _clipy2;
  int16_t  _originx, _originy;
  int16_t  _displayclipx1, _displayclipy1, _displayclipx2, _displayclipy2;
  bool _invisible = false; 
  bool _standard = true; // no bounding rectangle or origin set. 

  // Fonts
  const GFXfont *gfxFont;     ///< Pointer to special font
  const Font* font;           ///< Support for ILI9341  fonts.
 	int16_t	 _last_char_x_write = 0;  // Save RHS of last character written to detect overlap

	// Anti-aliased font support
	uint8_t fontbpp = 1;
	uint8_t fontbppindex = 0;
	uint8_t fontbppmask = 1;
	uint8_t fontppb = 8;
	uint8_t* fontalphalut;
	float fontalphamx = 1;


};


#endif // _GFX_H
