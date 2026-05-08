
#include "TFT_Display.h"

// Size of on-stack buffer for bulk SPI fills (in pixels)
static const int FILL_BUF_SIZE = 32;

/// @brief Creates the base TFT display, setting up the IO pins.
TFTDisplay::TFTDisplay(int16_t w, int16_t h, SPI* spi, uint8_t cs, uint8_t dc, uint8_t rst)
: GFX(w,h)
, _spi(spi)
, _cs(cs)
, _dc(dc)
, _rst(rst)
, _rowstart(0), _colstart(0)
, _xstart(0), _ystart(0)
{
	spi->set_format( 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

	isData = false;
	gpio_init(_dc);
	gpio_set_dir(_dc, GPIO_OUT);
	gpio_put(_dc, isData);

	if(! spi->isDedicated()){
		gpio_init(_cs);
		gpio_set_dir(_cs, GPIO_OUT);
		gpio_put(_cs, 1);
	}
	if(_rst != 0xff){
	  gpio_init(_rst);
	  gpio_set_dir(_rst, GPIO_OUT);
	  gpio_put(_rst, 1);
	}
}

void TFTDisplay::reset(){
	if (_rst != 0xff) {
    	gpio_put(_rst, 1);
		sleep_ms(100);
		gpio_put(_rst, 0);
		sleep_ms(100);
		gpio_put(_rst, 1);
		sleep_ms(200);
	}
}

// =================================================================================
// Low-level SPI communication
// =================================================================================

inline void TFTDisplay::waitTransmitComplete(void) {
	while (_spi->busy()) {
		tight_loop_contents();
	}
}

void TFTDisplay::beginTransaction(uint32_t clock){
	if(!_spi->isDedicated()){
		gpio_put(_cs, 0);
	}
}

void TFTDisplay::endTransaction(){
	if(!_spi->isDedicated()){
    	waitTransmitComplete();
		gpio_put(_cs, 1);
	}
}

inline void TFTDisplay::set8Bit(){
	waitTransmitComplete();
	_spi->set_format(8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
	is16Bit = false;
}

inline void TFTDisplay::set16Bit(){
	waitTransmitComplete();
	_spi->set_format(16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
	is16Bit = true;
}

void TFTDisplay::delay(uint ms) {
	waitTransmitComplete();
	sleep_ms(ms);
}

void TFTDisplay::writecommand(uint8_t c) {
	if(isData) waitTransmitComplete();
	if(is16Bit) set8Bit();
	gpio_put(_dc, 0);
	isData = false;
	_spi->write(c);
}

void TFTDisplay::writedata(uint8_t c) {
	if(!isData) waitTransmitComplete();
	if(is16Bit) set8Bit();
	gpio_put(_dc, 1);
	isData = true;
	_spi->write(c);
}

void TFTDisplay::writedata16(uint16_t d) {
	if(!isData) waitTransmitComplete();
	if(!is16Bit) set16Bit();
	gpio_put(_dc, 1);
	isData = true;
	_spi->write16(d);
}

// =================================================================================
// Bulk pixel helpers
// =================================================================================

void TFTDisplay::fillColor(uint16_t color, uint32_t count) {
	uint16_t buf[FILL_BUF_SIZE];
	for (int i = 0; i < FILL_BUF_SIZE; i++) buf[i] = color;

	while (count >= FILL_BUF_SIZE) {
		_spi->write16(buf, FILL_BUF_SIZE);
		count -= FILL_BUF_SIZE;
	}
	if (count > 0) {
		_spi->write16(buf, count);
	}
}

void TFTDisplay::writePixels(const uint16_t *data, uint32_t count) {
	_spi->write16(data, count);
}

// =================================================================================
// Command list processing (used by subclass init sequences)
// =================================================================================

void TFTDisplay::commandList(const uint8_t *addr) {
	uint8_t numCommands, numArgs;
	uint16_t ms;

	beginTransaction();
	numCommands = *(addr++);
	while(numCommands--) {
		writecommand(*(addr++));
		numArgs = *(addr++);
		ms = numArgs & DELAY;
		numArgs &= ~DELAY;
		while(numArgs > 1) {
			writedata(*(addr++));
			numArgs--;
		}
		if (numArgs) writedata(*(addr++));
		if(ms) {
			ms = *(addr++);
			if(ms == 255) ms = 500;
			endTransaction();
			delay(ms);
			beginTransaction();
		}
	}
	endTransaction();
}

// =================================================================================
// GFX TRANSACTION API overrides
// =================================================================================

void TFTDisplay::startWrite(void) {
	beginTransaction();
}

void TFTDisplay::writePixel(int16_t x, int16_t y, uint16_t color) {
	Pixel(x, y, color);
}

void TFTDisplay::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
	x += _originx;
	y += _originy;

	if (x >= _displayclipx2 || y >= _displayclipy2) return;
	if (x < _displayclipx1) { w -= (_displayclipx1 - x); x = _displayclipx1; }
	if (y < _displayclipy1) { h -= (_displayclipy1 - y); y = _displayclipy1; }
	if ((x + w) > _displayclipx2) w = _displayclipx2 - x;
	if ((y + h) > _displayclipy2) h = _displayclipy2 - y;
	if (w < 1 || h < 1) return;

	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();
	fillColor(color, (uint32_t)w * h);
}

void TFTDisplay::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
	VLine(x, y, h, color);
}

void TFTDisplay::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
	HLine(x, y, w, color);
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

	int16_t dx = x1 - x0;
	int16_t dy = abs(y1 - y0);
	int16_t err = dx / 2;
	int16_t ystep = (y0 < y1) ? 1 : -1;

	int16_t xbegin = x0;
	if (steep) {
		for (; x0 <= x1; x0++) {
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
		for (; x0 <= x1; x0++) {
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
// Bitmap and palette methods (optimised with bulk SPI writes)
// =================================================================================

void TFTDisplay::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
	int16_t byteWidth = (w + 7) / 8;
	uint8_t byte = 0;

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t j = 0; j < h; j++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				byte <<= 1;
			else
				byte = bitmap[j * byteWidth + i / 8];
			_spi->write16((byte & 0x80) ? color : bg);
		}
	}
	endTransaction();
}

void TFTDisplay::drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h) {
	uint16_t rowbuf[320];

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t j = 0; j < h; j++) {
		int16_t rw = (w <= 320) ? w : 320;
		for (int16_t i = 0; i < rw; i++) {
			uint8_t pixel = bitmap[j * w + i];
			rowbuf[i] = color565(pixel, pixel, pixel);
		}
		_spi->write16(rowbuf, rw);
	}
	endTransaction();
}

void TFTDisplay::drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t j = 0; j < h; j++) {
		_spi->write16(bitmap + j * w, w);
	}
	endTransaction();
}

void TFTDisplay::drawRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette) {
	uint16_t rowbuf[320];

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t row = 0; row < h; row++) {
		int16_t rw = (w <= 320) ? w : 320;
		for (int16_t col = 0; col < rw; col++) {
			rowbuf[col] = palette[*pixels++];
		}
		_spi->write16(rowbuf, rw);
	}
	endTransaction();
}

void TFTDisplay::drawRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette) {
	uint16_t rowbuf[320];

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t row = 0; row < h; row++) {
		int16_t rw = (w <= 320) ? w : 320;
		for (int16_t col = 0; col < rw; col += 2) {
			rowbuf[col] = palette[((*pixels) >> 4) & 0xF];
			rowbuf[col + 1] = palette[(*pixels++) & 0xF];
		}
		_spi->write16(rowbuf, rw);
	}
	endTransaction();
}

void TFTDisplay::drawRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette) {
	uint16_t rowbuf[320];

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t row = 0; row < h; row++) {
		int16_t rw = (w <= 320) ? w : 320;
		for (int16_t col = 0; col < rw; col += 4) {
			rowbuf[col]     = palette[((*pixels) >> 6) & 0x3];
			rowbuf[col + 1] = palette[((*pixels) >> 4) & 0x3];
			rowbuf[col + 2] = palette[((*pixels) >> 2) & 0x3];
			rowbuf[col + 3] = palette[(*pixels++) & 0x3];
		}
		_spi->write16(rowbuf, rw);
	}
	endTransaction();
}

void TFTDisplay::drawRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette) {
	uint16_t rowbuf[320];

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t row = 0; row < h; row++) {
		int16_t rw = (w <= 320) ? w : 320;
		for (int16_t col = 0; col < rw; col += 8) {
			rowbuf[col]     = palette[((*pixels) >> 7) & 0x1];
			rowbuf[col + 1] = palette[((*pixels) >> 6) & 0x1];
			rowbuf[col + 2] = palette[((*pixels) >> 5) & 0x1];
			rowbuf[col + 3] = palette[((*pixels) >> 4) & 0x1];
			rowbuf[col + 4] = palette[((*pixels) >> 3) & 0x1];
			rowbuf[col + 5] = palette[((*pixels) >> 2) & 0x1];
			rowbuf[col + 6] = palette[((*pixels) >> 1) & 0x1];
			rowbuf[col + 7] = palette[(*pixels++) & 0x1];
		}
		_spi->write16(rowbuf, rw);
	}
	endTransaction();
}

// =================================================================================
// Gradient fills (optimised with row buffer burst writes)
// =================================================================================

void TFTDisplay::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2) {
	if ((x >= _width) || (y >= _height)) return;
	if ((x + w - 1) >= _width)  w = _width - x;
	if ((y + h - 1) >= _height) h = _height - y;

	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
	color565toRGB14(color1, r1, g1, b1);
	color565toRGB14(color2, r2, g2, b2);
	dr = (r2 - r1) / h; dg = (g2 - g1) / h; db = (b2 - b1) / h;
	r = r1; g = g1; b = b1;

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t row = 0; row < h; row++) {
		uint16_t color = RGB14tocolor565(r, g, b);
		fillColor(color, w);
		r += dr; g += dg; b += db;
	}
	endTransaction();
}

void TFTDisplay::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2) {
	if ((x >= _width) || (y >= _height)) return;
	if ((x + w - 1) >= _width)  w = _width - x;
	if ((y + h - 1) >= _height) h = _height - y;

	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db;
	color565toRGB14(color1, r1, g1, b1);
	color565toRGB14(color2, r2, g2, b2);
	dr = (r2 - r1) / w; dg = (g2 - g1) / w; db = (b2 - b1) / w;

	uint16_t rowbuf[320];
	int16_t rw = (w <= 320) ? w : 320;

	// Build the gradient row once
	int16_t r = r1, g = g1, b = b1;
	for (int16_t col = 0; col < rw; col++) {
		rowbuf[col] = RGB14tocolor565(r, g, b);
		r += dr; g += dg; b += db;
	}

	beginTransaction(_clock);
	setAddr(x, y, x + w - 1, y + h - 1);
	beginPixelStream();

	for (int16_t row = 0; row < h; row++) {
		_spi->write16(rowbuf, rw);
	}
	endTransaction();
}

// =================================================================================
// Public utility methods
// =================================================================================

void TFTDisplay::setRowColStart(uint16_t x, uint16_t y) {
	_rowstart = x;
	_colstart = y;
	if (rotation != 0xff) setRotation(rotation);
}

void TFTDisplay::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	beginTransaction();
	setAddr(x0, y0, x1, y1);
	writecommand(RAMWR);
}

void TFTDisplay::pushColor(uint16_t color) {
	writedata16(color);
}

/// @brief Pushes a stream of colours to the display.
/// @param colors The colours to write - MUST BE A STATIC or HEAP BUFFER as DMA may be used.
/// @param count number of words to push.
/// @note setAddrWindow must have been called to set up the destination.
void TFTDisplay::pushColors(uint16_t* colors, size_t count)
{
	#if TFT_USE_DMA

	ensuredata16(); 	 // Ensure spi set up for 16 bit data transfers
	dma.waitForFinish(); // before reconfiguring

	DmaConfig cfg = dma.getConfig();
	cfg.dreq(DREQ_SPI1_TX); // (spi->dreq());  // should be SPI 1 TX
	cfg.transferDataSize(DMA_SIZE_16);
	cfg.readIncrement(true); // Transfer block of data
	cfg.writeIncrement(false); // single register
	cfg.enable(true);
	dma.configure(cfg);
	dma.setReadAddr(colors);
	dma.setWriteAddr(_spi->dma_target());

	dma.setTransCount(dma_encode_transfer_count(count), true);
	#else
	while(count-- > 0){
		writedata16(*colors++);
	}
	#endif

}

void TFTDisplay::closeAddrWindow() {
	writecommand(NOP);
	endTransaction();
}
