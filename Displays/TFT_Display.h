
#include <stdint.h>
#include "pico/stdlib.h"
#include "../PicoHardware/spi.h"
#include "GFX.h"

#ifndef _TFT_DISPLAY_H_
#define _TFT_DISPLAY_H_




class TFTDisplay : public GFX {

public:

	TFTDisplay(int16_t w, int16_t h, SPI* spi, uint8_t CS, uint8_t DC, uint8_t RST = 255);


	// Over-ride GFX TRANSACTION API / CORE DRAW API
	virtual void startWrite(void);
	virtual void writePixel(int16_t x, int16_t y, uint16_t color);
	virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
	virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
	virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	virtual void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
	virtual void endWrite(void);

	// Over-ride BASIC DRAW API
	virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
	virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	
	// Optional and probably not necessary to change
	virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
   virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
   virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

	// Over-ride any others where we can optimise by setting address and shovelling pixels thereafter.
    virtual void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
    virtual void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);
    virtual void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w,  int16_t h);

	virtual void drawRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	virtual void drawRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	virtual void drawRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	virtual void drawRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );

	virtual void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);
	virtual void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);


void setBitrate(uint32_t n);
void reset();

  // Allow direct access to pushing pixels into framebuffer.  
  // Call setAddrWindow, appropriate number of pushColor then closeAddrWindow in sequence
  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void pushColor(uint16_t color);
  void closeAddrWindow();

protected:

 

void sendCommand(uint8_t commandByte, const uint8_t *dataBytes, uint8_t numDataBytes);
void writecommand(uint8_t c);
void writedata(uint8_t d);
void writedata16(uint16_t d);

//void commandList(const uint8_t *addr);
//void commonInit(const uint8_t *cmdList, uint8_t mode=SPI_MODE0);
   //uint8_t  spiread(void);

void waitTransmitComplete(void);
void beginTransaction(uint32_t clock=20000000); //  Asserts ~SS and sets speed for this device.
void endTransaction();
void set8Bit();
void set16Bit();
void delay(uint ms);

 // ST7735 Specific - may be used by any other driver where there's 
 // an offset mapping from RAM to display locations
void     setRowColStart(uint16_t x, uint16_t y);
uint16_t  rowStart() {return _rowstart;}
uint16_t  colStart() {return _colstart;}



	// Data for managing pixel window below
uint16_t old_x0=-1, old_x1, old_y0=-1, old_y1;

inline void setAddr(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)	  {
   if (x0 != old_x0 || x1 != old_x1) {
      writecommand(CASET); // Column addr set
      writedata16(x0);   // XSTART
      writedata16(x1);   // XEND
      old_x0 = x0; old_x1 = x1;
   }
   if (y0 != old_y0 || y1 != old_y1) {
      writecommand(PASET); // Row addr set
      writedata16(y0);   // YSTART
      writedata16(y1);   // YEND
      old_y0 = y0; old_y1 = y1;
   }
}

      // Inline helper methods for efficient horizontal/vertical line & pixel writing
   inline void HLine(int16_t x, int16_t y, int16_t w, uint16_t color)	{
      x+=_originx;
      y+=_originy;
      // Rectangular clipping
      if((y < _displayclipy1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
      if(x<_displayclipx1) { w = w - (_displayclipx1 - x); x = _displayclipx1; }
      if((x+w-1) >= _displayclipx2)  w = _displayclipx2-x;
      if (w<1) return;
      setAddr(x, y, x+w-1, y);
      writecommand(RAMWR);
      do { writedata16(color); } while (--w > 0);  
   }

   inline void VLine(int16_t x, int16_t y, int16_t h, uint16_t color)	{
     x+=_originx;
     y+=_originy;
     // Rectangular clipping
     if((x < _displayclipx1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
     if(y < _displayclipy1) { h = h - (_displayclipy1 - y); y = _displayclipy1;}
     if((y+h-1) >= _displayclipy2) h = _displayclipy2-y;
     if(h<1) return;
     setAddr(x, y, x, y+h-1);
     writecommand(RAMWR);
     do { writedata16(color); } while (--h > 0);
   }

   inline void Pixel(int16_t x, int16_t y, uint16_t color) {
    x+=_originx;
    y+=_originy;
    if((x < _displayclipx1) ||(x >= _displayclipx2) || (y < _displayclipy1) || (y >= _displayclipy2)) return;
    setAddr(x, y, x, y);
    writecommand(RAMWR);
    writedata16(color);
   }


// Comms
SPI* _spi;
uint8_t  _rst;
uint8_t _cs, _dc;
bool is16Bit;  // true if set for 16 bit transfers
bool isData; // true if set for data, false if command

unsigned long _clock; // SPI clock (if used)

// Common controller opcodes. May be modified in subclasses
uint8_t NOP   = 0x00;
uint8_t CASET = 0x2A;
uint8_t PASET = 0x2B;
uint8_t RAMWR = 0x2C;
uint8_t RAMRD = 0x2E;

uint16_t _colstart, _rowstart;  // Offsets into display RAM to align with physical display
uint16_t _xstart, _ystart;      // _colstart and _rowstart allowing for rotation.


};


#endif // _TFT_DISPLAY_H_

