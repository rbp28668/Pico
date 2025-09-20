
#include "TFT_Display.h"


TFTDisplay::TFTDisplay(int16_t w, int16_t h, SPI* spi, uint8_t cs, uint8_t dc, uint8_t rst)
: GFX(w,h)
, _spi(spi)
, _cs(cs)
, _dc(dc)
, _rst(rst)
, _rowstart(0), _colstart(0)
, _xstart(0), _ystart(0)
{
	isData = false;
	gpio_init(_dc);
	gpio_set_dir(_dc, GPIO_OUT);
	gpio_put(_dc, isData);

	if(! spi->isDedicated()){
		gpio_init(_cs);
		gpio_set_dir(_cs, GPIO_OUT);
		gpio_put(_cs, 1); // deselected
	}
	if(_rst != 0xff){
	  gpio_init(_rst);
	  gpio_set_dir(_rst, GPIO_OUT);
	  gpio_put(_rst, 1);
	}

}

void TFTDisplay::reset(){
	// Reset the screen
	if (_rst != 0xff) {
    	gpio_put(_rst, 1);
		sleep_ms(100);
		gpio_put(_rst, 0);
		sleep_ms(100);
		gpio_put(_rst, 1);
		sleep_ms(200);
	}

}

/*!
 @brief   Send Command handles complete sending of commands and const data
 @param   commandByte       The Command Byte
 @param   dataBytes         A pointer to the Data bytes to send
 @param   numDataBytes      The number of bytes we should send
 */
void TFTDisplay::sendCommand(uint8_t commandByte, const uint8_t *dataBytes, uint8_t numDataBytes) {
    beginTransaction();

    writecommand(commandByte); // Send the command byte
  
    while (numDataBytes > 1) {
	  writedata(*dataBytes++); // Send the data bytes
	  numDataBytes--;
    }
    if (numDataBytes) writedata(*dataBytes);
  
    endTransaction();
}


inline void TFTDisplay::waitTransmitComplete(void)  {
	while (_spi->busy() ){
		tight_loop_contents();
	}
}

void TFTDisplay::beginTransaction(uint32_t clock){
	// TODO - something creative with clock. Currently set in SPI constructor
	if(!_spi->isDedicated()){
		gpio_put(_cs,0); // enable active low slave select
	}
}

void TFTDisplay::endTransaction(){
	if(!_spi->isDedicated()){
    	waitTransmitComplete();
		gpio_put(_cs,1); // disable active low slave select
	}
}

inline void TFTDisplay::set8Bit(){
	waitTransmitComplete();
	_spi->set_format (8, SPI_CPOL_0 , SPI_CPHA_0, SPI_MSB_FIRST);
	is16Bit = false;
}

inline void TFTDisplay::set16Bit(){
	waitTransmitComplete();
	_spi->set_format (16, SPI_CPOL_0 , SPI_CPHA_0, SPI_MSB_FIRST);
	is16Bit = true;
}
 
// Delay times from end of transmission.
void TFTDisplay::delay(uint ms) {
	waitTransmitComplete();
	sleep_ms(ms);
}


void TFTDisplay::writecommand(uint8_t c)
{
	if(isData) waitTransmitComplete();
	if(is16Bit) set8Bit();
	gpio_put(_dc,0); 
	isData = false;
	_spi->write(c);
}

void TFTDisplay::writedata(uint8_t c)
{
	if(!isData) waitTransmitComplete();
	if(is16Bit) set8Bit();
	gpio_put(_dc,1); 
	isData = true;
	_spi->write(c);
}

void TFTDisplay::writedata16(uint16_t d)
{
	if(!isData) waitTransmitComplete();
	if(!is16Bit) set16Bit();
	gpio_put(_dc,1); 
	isData = true;
	_spi->write16(d);
}


// =================================================================================
// Over-ride GFX TRANSACTION API / CORE DRAW API
// =================================================================================


void TFTDisplay::startWrite(void) { 
	beginTransaction();
}

void TFTDisplay::writePixel(int16_t x, int16_t y, uint16_t color){
	setAddr(x, y, x, y);
	writecommand(RAMWR);
	writedata16(color);
}

void TFTDisplay::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if(x < 0) {	w += x; x = 0; 	}
	if(y < 0) {	h += y; y = 0; 	}
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;

	setAddr(x, y, x+w-1, y+h-1);
	writecommand(RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>0; x--) {
			writedata16(color);
		}
	}
}

void TFTDisplay::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color){
	// Rudimentary clipping
	if((x >= _width) || (x < 0) || (y >= _height)) return;
	if(y < 0) {	h += y; y = 0; 	}
	if((y+h-1) >= _height) h = _height-y;
	setAddr(x, y, x, y+h-1);
	writecommand(RAMWR);
	while (h-- > 0) {
		writedata16(color);
	}
}

void TFTDisplay::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
	// Rudimentary clipping
	if((x >= _width) || (y >= _height) || (y < 0)) return;
	if(x < 0) {	w += x; x = 0; 	}
	if((x+w-1) >= _width)  w = _width-x;
	setAddr(x, y, x+w-1, y);
	writecommand(RAMWR);
	while (w-- > 0) {
		writedata16(color);
	}
}

void TFTDisplay::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
	if (y0 == y1) {
		if (x1 > x0) {
			HLine(x0, y0, x1 - x0 + 1, color);
		} else if (x1 < x0) {
			HLine(x1, y0, x0 - x1 + 1, color);
		} else {
			Pixel(x0, y0, color);
		}
		return;
	} else if (x0 == x1) {
		if (y1 > y0) {
			VLine(x0, y0, y1 - y0 + 1, color);
		} else {
			VLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

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
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					VLine(y0, xbegin, len + 1, color);
				} else {
					Pixel(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			VLine(y0, xbegin, x0 - xbegin, color);
		}

	} else {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					HLine(xbegin, y0, len + 1, color);
				} else {
					Pixel(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			HLine(xbegin, y0, x0 - xbegin, color);
		}
	}
	writecommand(NOP);
}

void TFTDisplay::endWrite(void) {
	endTransaction();
}




// =================================================================================
// Over-ride BASIC DRAW API
// =================================================================================
void TFTDisplay::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	// Rudimentary clipping
	if((x >= _width) || (x < 0) || (y >= _height)) return;
	if(y < 0) {	h += y; y = 0; 	}
	if((y+h-1) >= _height) h = _height-y;
	beginTransaction(_clock);
	setAddr(x, y, x, y+h-1);
	writecommand(RAMWR);
	while (h-- > 0) {
		writedata16(color);
	}
	endTransaction();
}

void TFTDisplay::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	// Rudimentary clipping
	if((x >= _width) || (y >= _height) || (y < 0)) return;
	if(x < 0) {	w += x; x = 0; 	}
	if((x+w-1) >= _width)  w = _width-x;
	beginTransaction(_clock);
	setAddr(x, y, x+w-1, y);
	writecommand(RAMWR);
	while (w-- > 0) {
		writedata16(color);
	}
	endTransaction();
}



// =================================================================================
// Over-ride Probably minimal benefit (if any)
// =================================================================================

void TFTDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {

	if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

	beginTransaction(_clock);
	setAddr(x, y, x, y);
	writecommand(RAMWR);
	writedata16(color);
	endTransaction();
}


///
///
///
// Draw a rectangle
void TFTDisplay::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{

	beginTransaction(_clock);
	HLine(x, y, w, color);
	HLine(x, y+h-1, w, color);
	VLine(x, y, h, color);
	VLine(x+w-1, y, h, color);
	endTransaction();
}




// Bresenham's algorithm - thx wikpedia
void TFTDisplay::drawLine(int16_t x0, int16_t y0,
	int16_t x1, int16_t y1, uint16_t color)
{
	if (y0 == y1) {
		if (x1 > x0) {
			drawFastHLine(x0, y0, x1 - x0 + 1, color);
		} else if (x1 < x0) {
			drawFastHLine(x1, y0, x0 - x1 + 1, color);
		} else {
			drawPixel(x0, y0, color);
		}
		return;
	} else if (x0 == x1) {
		if (y1 > y0) {
			drawFastVLine(x0, y0, y1 - y0 + 1, color);
		} else {
			drawFastVLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

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

	beginTransaction();
	int16_t xbegin = x0;
	if (steep) {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					VLine(y0, xbegin, len + 1, color);
				} else {
					Pixel(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			VLine(y0, xbegin, x0 - xbegin, color);
		}

	} else {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					HLine(xbegin, y0, len + 1, color);
				} else {
					Pixel(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			HLine(xbegin, y0, x0 - xbegin, color);
		}
	}
	writecommand(NOP);
	endTransaction();
}


void TFTDisplay::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg){
  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  beginTransaction(_clock);
  setAddr(x, y, x+w-1, y+h-1);
  writecommand(RAMWR);

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = bitmap[j * byteWidth + i / 8];
      writedata16((byte & 0x80) ? color : bg);
    }
  }
  endTransaction();

}

void TFTDisplay::drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h){
  beginTransaction(_clock);
  setAddr(x, y, x+w-1, y+h-1);
  writecommand(RAMWR);
   for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
	  uint8_t pixel = bitmap[j * w + i];
      writedata16(color565(pixel, pixel, pixel));
    }
  }
  endTransaction();
}

void TFTDisplay::drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w,  int16_t h){
  beginTransaction(_clock);
  setAddr(x, y, x+w-1, y+h-1);
  writecommand(RAMWR);
   for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      writedata16(bitmap[j * w + i]);
    }
  }
  endTransaction();
}


// writeRect8BPP - 	write 8 bit per pixel paletted bitmap
//					bitmap data in array at pixels, one byte per pixel
//					color palette data in array at palette
void TFTDisplay::drawRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
   	beginTransaction(_clock);
	setAddr(x, y, x+w-1, y+h-1);
	writecommand(RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>0; x--) {
			writedata16(palette[*pixels++]);
		}
	}
	endTransaction();
}

// writeRect4BPP - 	write 4 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 2 pixels
void TFTDisplay::drawRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
   	beginTransaction(_clock);
	setAddr(x, y, x+w-1, y+h-1);
	writecommand(RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>0; x-=2) {
			writedata16(palette[((*pixels)>>4)&0xF]);
			writedata16(palette[(*pixels++)&0xF]);
		}
	}
	endTransaction();
}

// writeRect2BPP - 	write 2 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 4 pixels
void TFTDisplay::drawRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
   	beginTransaction(_clock);
	setAddr(x, y, x+w-1, y+h-1);
	writecommand(RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>0; x-=4) {
			//unrolled loop might be faster?
			writedata16(palette[((*pixels)>>6)&0x3]);
			writedata16(palette[((*pixels)>>4)&0x3]);
			writedata16(palette[((*pixels)>>2)&0x3]);
			writedata16(palette[(*pixels++)&0x3]);
		}
	}
	endTransaction();
}

void TFTDisplay::drawRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
   	beginTransaction(_clock);
	setAddr(x, y, x+w-1, y+h-1);
	writecommand(RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>0; x-=8) {
			//unrolled loop might be faster?
			writedata16(palette[((*pixels)>>7)&0x1]);
			writedata16(palette[((*pixels)>>6)&0x1]);
			writedata16(palette[((*pixels)>>5)&0x1]);
			writedata16(palette[((*pixels)>>4)&0x1]);
			writedata16(palette[((*pixels)>>3)&0x1]);
			writedata16(palette[((*pixels)>>2)&0x1]);
			writedata16(palette[((*pixels)>>1)&0x1]);
			writedata16(palette[(*pixels++)&0x1]);
		}
	}
	endTransaction();
}

// fillRectVGradient	- fills area with vertical gradient
void TFTDisplay::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
{
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;
	
	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
	color565toRGB14(color1,r1,g1,b1);
	color565toRGB14(color2,r2,g2,b2);
	dr=(r2-r1)/h; dg=(g2-g1)/h; db=(b2-b1)/h;
	r=r1;g=g1;b=b1;	

	beginTransaction(_clock);
	setAddr(x, y, x+w-1, y+h-1);
	writecommand(RAMWR);
	for(y=h; y>0; y--) {
		uint16_t color = RGB14tocolor565(r,g,b);

		for(x=w; x>0; x--) {
			writedata16(color);
		}
		r+=dr;g+=dg; b+=db;
	}
	endTransaction();
}

// fillRectHGradient	- fills area with horizontal gradient
void TFTDisplay::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
{
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;
	
	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
	color565toRGB14(color1,r1,g1,b1);
	color565toRGB14(color2,r2,g2,b2);
	dr=(r2-r1)/h; dg=(g2-g1)/h; db=(b2-b1)/h;
	r=r1;g=g1;b=b1;	

	beginTransaction(_clock);
	setAddr(x, y, x+w-1, y+h-1);
	writecommand(RAMWR);
	for(y=h; y>0; y--) {
		uint16_t color;
		for(x=w; x>0; x--) {
			color = RGB14tocolor565(r,g,b);
			writedata16(color);
			r+=dr;g+=dg; b+=db;
		}
		r=r1;g=g1;b=b1;
	}
	endTransaction();
}


void TFTDisplay::setRowColStart(uint16_t x, uint16_t y) {
	_rowstart = x;
	_colstart = y;
	if (rotation != 0xff) setRotation(rotation);
}


void TFTDisplay::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	beginTransaction();
	setAddr(x0, y0, x1, y1);
	writecommand(RAMWR); // write to RAM
}


void TFTDisplay::pushColor(uint16_t color)
{
	writedata16(color);
}

void TFTDisplay::closeAddrWindow()
{
	writecommand(NOP);
	endTransaction();
}


