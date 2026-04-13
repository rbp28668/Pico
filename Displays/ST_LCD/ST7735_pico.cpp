/*************************************************** 
  This is a library for the Adafruit 1.8" SPI display.
  This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
  as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618
 
  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
  Taken from Paul Stoffregen's Teensy port at https://github.com/PaulStoffregen/ST7735_t3
  Modified for Raspberry Pi Pico by Bruce Porteous
 ****************************************************/

#include "ST7735_pico.h"
#include "ST7789_pico.h"
#include <limits.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstdint>
#include "hardware/gpio.h"


// Command codes - see data sheet
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

//#include "glcdfont.c"
extern "C" const unsigned char glcdfont[];


// Constructor when using hardware SPI.  Faster, but must use dedicated SPI pins
ST7735_pico::ST7735_pico(SPI* pspi,uint8_t cs, uint8_t rs, uint8_t rst) 
: TFTDisplay(ST7735_TFTWIDTH, ST7735_TFTHEIGHT_160,pspi, cs, rs, rst)
{
}

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
static const uint8_t //PROGMEM
  Bcmd[] = {                  // Initialization commands for 7735B screens
    18,                       // 18 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, no args, w/delay
      50,                     //     50 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, no args, w/delay
      255,                    //     255 = 500 ms delay
    ST7735_COLMOD , 1+DELAY,  //  3: Set color mode, 1 arg + delay:
      0x05,                   //     16-bit color
      10,                     //     10 ms delay
    ST7735_FRMCTR1, 3+DELAY,  //  4: Frame rate control, 3 args + delay:
      0x00,                   //     fastest refresh
      0x06,                   //     6 lines front porch
      0x03,                   //     3 lines back porch
      10,                     //     10 ms delay
    ST7735_MADCTL , 1      ,  //  5: Memory access ctrl (directions), 1 arg:
      0x08,                   //     Row addr/col addr, bottom to top refresh
    ST7735_DISSET5, 2      ,  //  6: Display settings #5, 2 args, no delay:
      0x15,                   //     1 clk cycle nonoverlap, 2 cycle gate
                              //     rise, 3 cycle osc equalize
      0x02,                   //     Fix on VTL
    ST7735_INVCTR , 1      ,  //  7: Display inversion control, 1 arg:
      0x0,                    //     Line inversion
    ST7735_PWCTR1 , 2+DELAY,  //  8: Power control, 2 args + delay:
      0x02,                   //     GVDD = 4.7V
      0x70,                   //     1.0uA
      10,                     //     10 ms delay
    ST7735_PWCTR2 , 1      ,  //  9: Power control, 1 arg, no delay:
      0x05,                   //     VGH = 14.7V, VGL = -7.35V
    ST7735_PWCTR3 , 2      ,  // 10: Power control, 2 args, no delay:
      0x01,                   //     Opamp current small
      0x02,                   //     Boost frequency
    ST7735_VMCTR1 , 2+DELAY,  // 11: Power control, 2 args + delay:
      0x3C,                   //     VCOMH = 4V
      0x38,                   //     VCOML = -1.1V
      10,                     //     10 ms delay
    ST7735_PWCTR6 , 2      ,  // 12: Power control, 2 args, no delay:
      0x11, 0x15,
    ST7735_GMCTRP1,16      ,  // 13: Magical unicorn dust, 16 args, no delay:
      0x09, 0x16, 0x09, 0x20, //     (seriously though, not sure what
      0x21, 0x1B, 0x13, 0x19, //      these config values represent)
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    ST7735_GMCTRN1,16+DELAY,  // 14: Sparkles and rainbows, 16 args + delay:
      0x0B, 0x14, 0x08, 0x1E, //     (ditto)
      0x22, 0x1D, 0x18, 0x1E,
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,                     //     10 ms delay
    ST7735_CASET  , 4      ,  // 15: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 2
      0x00, 0x81,             //     XEND = 129
    ST7735_RASET  , 4      ,  // 16: Row addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 1
      0x00, 0x81,             //     XEND = 160
    ST7735_NORON  ,   DELAY,  // 17: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,   DELAY,  // 18: Main screen turn on, no args, w/delay
      255 },                  //     255 = 500 ms delay

  Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

  Rcmd2green[] = {            // Init for 7735R, part 2 (green tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 0
      0x00, 0x7F+0x02,        //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x01,             //     XSTART = 0
      0x00, 0x9F+0x01 },      //     XEND = 159
  Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F },           //     XEND = 159

  Rcmd2green144[] = {         // Init for 7735R, part 2 (green 1.44 tab)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127

  Rcmd2green160x80[] = {            // 7735R init, part 2 (mini 160x80)
    2,                              //  2 commands in list:
    ST7735_CASET,   4,              //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x4F,                   //     XEND = 79
    ST7735_RASET,   4,              //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x9F},                 //     XEND = 159
  Rcmd2minist7735s[] = {
    3,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00+26,             //     XSTART = 0
      0x00, 0x7F+26,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00+1,             //     XSTART = 0
      0x00, 0x4F+1,           //     XEND = 79
    ST7735_INVON,  0},			// these displays need colors inversed

  Rcmd3[] = {                 // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay


// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in byte array.
void ST7735_pico::commandList(const uint8_t *addr)
{
	uint8_t  numCommands, numArgs;
	uint16_t ms;

	beginTransaction();
	numCommands = *(addr++);		// Number of commands to follow
	//Serial.printf("CommandList: numCmds:%d\n", numCommands); Serial.flush();
	while(numCommands--) {				// For each command...
		writecommand(*(addr++));	//   Read, issue command
		numArgs  = *(addr++);	//   Number of args to follow
		ms       = numArgs & DELAY;		//   If hibit set, delay follows args
		numArgs &= ~DELAY;			//   Mask out delay bit
		while(numArgs > 1) {			//   For each argument...
			writedata(*(addr++)); //   Read, issue argument
			numArgs--;
		}

		if (numArgs) writedata(*(addr++)); //   Read, issue argument - wait until this one completes
		if(ms) {
			ms = *(addr++);	// Read post-command delay time (ms)
			if(ms == 255) ms = 500;		// If 255, delay for 500 ms
			//Serial.printf("delay %d\n", ms); Serial.flush();
			endTransaction();
			delay(ms);
			beginTransaction();
		}
	}
	endTransaction();
}


// Initialization code common to both 'B' and 'R' type displays
// Mode redundant as sets SPI mode
void ST7735_pico::commonInit(const uint8_t *cmdList, uint8_t mode)
{
	_colstart  = _rowstart = 0; // May be overridden in init func
  	_ystart = _xstart = 0;

	reset();
	if(cmdList) commandList(cmdList);
}


// Initialization for ST7735B screens
void ST7735_pico::initB(void)
{
	commonInit(Bcmd);
}


// Initialization for ST7735R screens (green or red tabs)
void ST7735_pico::initR(uint8_t options)
{
	commonInit(Rcmd1);
	if (options == INITR_GREENTAB) {
		commandList(Rcmd2green);
		_colstart = 2;
		_rowstart = 1;
	} else if(options == INITR_144GREENTAB) {
		setScreenSize(ST7735_TFTWIDTH,ST7735_TFTHEIGHT_144);
		commandList(Rcmd2green144);
		_colstart = 2;
		_rowstart = 3;
	} else if(options == INITR_144GREENTAB_OFFSET) {
		setScreenSize(ST7735_TFTWIDTH,ST7735_TFTHEIGHT_144);
		commandList(Rcmd2green144);
		_colstart = 0;
		_rowstart = 32;
	} else if(options == INITR_MINI160x80) {
	    setScreenSize(ST7735_TFTWIDTH_80, ST7735_TFTHEIGHT_160);
	    commandList(Rcmd2green160x80);
	    _colstart = 24;
	    _rowstart = 0;
	} else if (options == INITR_MINI160x80_ST7735S) {
	    setScreenSize(ST7735_TFTWIDTH_80, ST7735_TFTHEIGHT_160);
	    commandList(Rcmd2minist7735s);
	    _colstart = 26;
	    _rowstart = 1;
	} else {
		// _colstart, _rowstart left at default '0' values
		commandList(Rcmd2red);
	}
	commandList(Rcmd3);

	// if black or mini, change MADCTL color filter
	if ((options == INITR_BLACKTAB)  || (options == INITR_MINI160x80)){
		writecommand(ST7735_MADCTL);
		writedata(0xC0);
	}

	tabcolor = options;
	setRotation(0);	

}






#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

void ST7735_pico::setRotation(uint8_t m)
{
	//Serial.printf("Setting Rotation to %d\n", m);
	beginTransaction();
	writecommand(ST7735_MADCTL);
	rotation = m % 4; // can't be higher than 3
	switch (rotation) {
	case 0:
     	if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
			writedata(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
		} else {
			writedata(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
		}
		_width  = WIDTH;
		_height = HEIGHT;
	    _xstart = _colstart;
	    _ystart = _rowstart;
		break;
	case 1:
     	if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
			writedata(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
		} else {
			writedata(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
		}
		_height = WIDTH;
		_width  = HEIGHT;
     	_ystart = _colstart;
     	_xstart = _rowstart;
		break;
	case 2:
     	if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
			writedata(MADCTL_RGB);
		} else {
			writedata(MADCTL_BGR);
		}
		_width  = WIDTH;
		_height = HEIGHT;
     	_xstart = _colstart;
     	// hack to make work on a couple different displays
     	_ystart = (_rowstart==0 || _rowstart==32)? 0 : 1;//_rowstart;
		break;
	case 3:
     	if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
			writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
		} else {
			writedata(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
		}
		_width = HEIGHT;
		_height = WIDTH;
     	_ystart = _colstart;
     	// hack to make work on a couple different displays
     	_xstart = (_rowstart==0 || _rowstart==32)? 0 : 1;//_rowstart;
		break;
	}
	this->rotation = rotation;	// remember the rotation... 
	endTransaction();

	setClipRect();
	setOrigin();
	
	cursor_x = 0;
	cursor_y = 0;
}



void ST7735_pico::invertDisplay(bool i)
{
	beginTransaction();
	writecommand(i ? ST7735_INVON : ST7735_INVOFF);
	endTransaction();

}


// Now lets see if we can writemultiple pixels
void ST7735_pico::writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
{
	x+=_originx;
	y+=_originy;

	uint16_t x_clip_left = 0;  // How many entries at start of colors to skip at start of row
	uint16_t x_clip_right = 0;    // how many color entries to skip at end of row for clipping
	// Rectangular clipping 

	// See if the whole thing out of bounds...
	if((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x+w) <= _displayclipx1) || ((y+h) <= _displayclipy1)) return;

	// In these cases you can not do simple clipping, as we need to synchronize the colors array with the
	// We can clip the height as when we get to the last visible we don't have to go any farther. 
	// also maybe starting y as we will advance the color array. 
 	if(y < _displayclipy1) {
 		int dy = (_displayclipy1 - y);
 		h -= dy; 
 		pcolors += (dy*w); // Advance color array to 
 		y = _displayclipy1; 	
 	}
	if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;
	// For X see how many items in color array to skip at start of row and likewise end of row 
	if(x < _displayclipx1) {
		x_clip_left = _displayclipx1-x; 
		w -= x_clip_left; 
		x = _displayclipx1; 	
	}

	if((x + w - 1) >= _displayclipx2) {
		x_clip_right = w;
		w = _displayclipx2  - x;
		x_clip_right -= w; 
	} 

   	beginTransaction();
	setAddr(x, y, x+w-1, y+h-1);
	writecommand(ST7735_RAMWR);
	for(y=h; y>0; y--) {
		pcolors += x_clip_left;
		for(x=w; x>1; x--) {
			writedata16(*pcolors++);
		}
		writedata16(*pcolors++);
		pcolors += x_clip_right;
	}
	endTransaction();
}


/*

size_t ST7735_pico::write(const uint8_t *buffer, size_t size)
{
	// Lets try to handle some of the special font centering code that was done for default fonts.
	if (_center_x_text || _center_y_text ) {
		int16_t x, y;
	  	uint16_t strngWidth, strngHeight;
	  	getTextBounds(buffer, size, 0, 0, &x, &y, &strngWidth, &strngHeight);
	  	//Serial.printf("_fontwrite bounds: %d %d %u %u\n", x, y, strngWidth, strngHeight);
	  	// Note we may want to play with the x ane y returned if they offset some
		if (_center_x_text && strngWidth > 0){//Avoid operations for strngWidth = 0
			cursor_x -= ((x + strngWidth) / 2);
		}
		if (_center_y_text && strngHeight > 0){//Avoid operations for strngWidth = 0
			cursor_y -= ((y + strngHeight) / 2);
		}
		_center_x_text = false;
		_center_y_text = false;
	}

	size_t cb = size;
	while (cb) {
		uint8_t c = *buffer++;
		cb--;

		if (font) {
			if (c == '\n') {
				cursor_y += font->line_space;
				if(scrollEnable && isWritingScrollArea){
					cursor_x  = scroll_x;
				}else{
					cursor_x  = 0;
				}
			} else {
				drawFontChar(c);
			}
		} else if (gfxFont)  {
			if (c == '\n') {
	            cursor_y += (int16_t)textsize_y * gfxFont->yAdvance;
				if(scrollEnable && isWritingScrollArea){
					cursor_x  = scroll_x;
				}else{
					cursor_x  = 0;
				}
			} else {
				drawGFXFontChar(c);
			}
		} else {
			if (c == '\n') {
				cursor_y += textsize_y*8;
				if(scrollEnable && isWritingScrollArea){
					cursor_x  = scroll_x;
				}else{
					cursor_x  = 0;
				}
			} else if (c == '\r') {
				// skip em
			} else {
				if(scrollEnable && isWritingScrollArea && (cursor_y > (scroll_y+scroll_height - textsize_y*8))){
					scrollTextArea(textsize_y*8);
					cursor_y -= textsize_y*8;
					cursor_x = scroll_x;
				}
				drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
				cursor_x += textsize_x*6;
				if(wrap && scrollEnable && isWritingScrollArea && (cursor_x > (scroll_x+scroll_width - textsize_x*6))){
					cursor_y += textsize_y*8;
					cursor_x = scroll_x;
				}
				else if (wrap && (cursor_x > (_width - textsize_x*6))) {
					cursor_y += textsize_y*8;
					cursor_x = 0;
				}
			}
		}
	}
	return size;
}
*/
/*
// Draw a character
void ST7735_pico::drawChar(int16_t x, int16_t y, unsigned char c,
			    uint16_t fgcolor, uint16_t bgcolor, uint8_t size_x, uint8_t size_y)
{
	if((x >= _width)            || // Clip right
	   (y >= _height)           || // Clip bottom
	   ((x + 6 * size_x - 1) < 0) || // Clip left  TODO: is this correct?
	   ((y + 8 * size_y - 1) < 0))   // Clip top   TODO: is this correct?
		return;

	if (fgcolor == bgcolor) {
		// This transparent approach is only about 20% faster
		if ((size_x == 1) && (size_y == 1)) {
			uint8_t mask = 0x01;
			int16_t xoff, yoff;
			for (yoff=0; yoff < 8; yoff++) {
				uint8_t line = 0;
				for (xoff=0; xoff < 5; xoff++) {
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
			for (yoff=0; yoff < 8; yoff++) {
				uint8_t line = 0;
				for (xoff=0; xoff < 5; xoff++) {
					if (glcdfont[c * 5 + xoff] & mask) line |= 1;
					line <<= 1;
				}
				line >>= 1;
				xoff = 0;
				while (line) {
					if (line == 0x1F) {
						fillRect(x + xoff * size_x, y + yoff * size_y,
							5 * size_x, size_y, fgcolor);
						break;
					} else if (line == 0x1E) {
						fillRect(x + xoff * size_x, y + yoff * size_y,
							4 * size_x, size_y, fgcolor);
						break;
					} else if ((line & 0x1C) == 0x1C) {
						fillRect(x + xoff * size_x, y + yoff * size_y,
							3 * size_x, size_y, fgcolor);
						line <<= 4;
						xoff += 4;
					} else if ((line & 0x18) == 0x18) {
						fillRect(x + xoff * size_x, y + yoff * size_y,
							2 * size_x, size_y, fgcolor);
						line <<= 3;
						xoff += 3;
					} else if ((line & 0x10) == 0x10) {
						fillRect(x + xoff * size_x, y + yoff * size_y,
							size_x, size_y, fgcolor);
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
		// This solid background approach is about 5 time faster
		uint8_t xc, yc;
		uint8_t xr, yr;
		uint8_t mask = 0x01;
		uint16_t color;

		// We need to offset by the origin.
		x+=_originx;
		y+=_originy;
		int16_t x_char_start = x;  // remember our X where we start outputting...

		if((x >= _displayclipx2)            || // Clip right
			 (y >= _displayclipy2)           || // Clip bottom
			 ((x + 6 * size_x - 1) < _displayclipx1) || // Clip left  TODO: this is not correct
			 ((y + 8 * size_y - 1) < _displayclipy1))   // Clip top   TODO: this is not correct
			return;


		// need to build actual pixel rectangle we will output into.
		int16_t y_char_top = y;	// remember the y
		int16_t w =  6 * size_x;
		int16_t h = 8 * size_y;

		if(x < _displayclipx1) {	w -= (_displayclipx1-x); x = _displayclipx1; 	}
		if((x + w - 1) >= _displayclipx2)  w = _displayclipx2  - x;
		if(y < _displayclipy1) {	h -= (_displayclipy1 - y); y = _displayclipy1; 	}
		if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

		beginTransaction();
		setAddr(x, y, x + w -1, y + h - 1);

		y = y_char_top;	// restore the actual y.
		writecommand(ST7735_RAMWR);
		for (yc=0; (yc < 8) && (y < _displayclipy2); yc++) {
			for (yr=0; (yr < size_y) && (y < _displayclipy2); yr++) {
				x = x_char_start; 		// get our first x position...
				if (y >= _displayclipy1) {
					for (xc=0; xc < 5; xc++) {
						if (glcdfont[c * 5 + xc] & mask) {
							color = fgcolor;
						} else {
							color = bgcolor;
						}
						for (xr=0; xr < size_x; xr++) {
							if ((x >= _displayclipx1) && (x < _displayclipx2)) {
								writedata16(color);
							}
							x++;
						}
					}
					for (xr=0; xr < size_x; xr++) {
						if ((x >= _displayclipx1) && (x < _displayclipx2)) {
							writedata16(bgcolor);
						}
						x++;
					}
				}
				y++;
			}
			mask = mask << 1;
		}
		writecommand(ST7735_NOP);
		endTransaction();
	}
}

void ST7735_pico::setFont(const ILI9341_t3_font_t &f) {
	font = &f;
	_gfx_last_char_x_write = 0;	// Don't use cached data here
	if (gfxFont) {
        cursor_y -= 6;
		gfxFont = NULL;
	}
	fontbpp = 1;
	// Calculate additional metrics for Anti-Aliased font support (BDF extn v2.3)
	if (font && font->version==23){
		fontbpp = (font->reserved & 0b000011)+1;
		fontbppindex = (fontbpp >> 2)+1;
		fontbppmask = (1 << (fontbppindex+1))-1;
		fontppb = 8/fontbpp;
		fontalphamx = 31/((1<<fontbpp)-1);
		// Ensure text and bg color are different. Note: use setTextColor to set actual bg color
		if (textcolor == textbgcolor) textbgcolor = (textcolor==0x0000)?0xFFFF:0x0000;
	}
}

// Maybe support GFX Fonts as well?
void ST7735_pico::setFont(const GFXfont *f) {
	font = NULL;	// turn off the other font... 
	_gfx_last_char_x_write = 0;	// Don't use cached data here
	if (f == gfxFont) return;	// same font or lack of so can bail.

    if(f) {            // Font struct pointer passed in?
        if(!gfxFont) { // And no current font struct?
            // Switching from classic to new font behavior.
            // Move cursor pos down 6 pixels so it's on baseline.
            cursor_y += 6;
        }

        // Test wondering high and low of Ys here... 
        int8_t miny_offset = 0;
#if 1
        for (uint8_t i=0; i <= (f->last - f->first); i++) {
        	if (f->glyph[i].yOffset < miny_offset) {
        		miny_offset = f->glyph[i].yOffset;
        	}
        }
#else        
        int max_delta = 0;
        uint8_t index_min = 0;
        uint8_t index_max = 0;

        int8_t minx_offset = 127;
        int8_t maxx_overlap = 0;
        uint8_t indexx_min = 0;
        uint8_t indexx_max = 0;
        for (uint8_t i=0; i <= (f->last - f->first); i++) {
        	if (f->glyph[i].yOffset < miny_offset) {
        		miny_offset = f->glyph[i].yOffset;
        		index_min = i;
        	}

        	if (f->glyph[i].xOffset < minx_offset) {
        		minx_offset = f->glyph[i].xOffset;
        		indexx_min = i;
        	}
        	if ( (f->glyph[i].yOffset + f->glyph[i].height) > max_delta) {
        		max_delta = (f->glyph[i].yOffset + f->glyph[i].height);
        		index_max = i;
        	}
        	int8_t x_overlap = f->glyph[i].xOffset + f->glyph[i].width - f->glyph[i].xAdvance;
        	if (x_overlap > maxx_overlap) {
        		maxx_overlap = x_overlap;
        		indexx_max = i;
        	}
        }
        Serial.printf("Set GFX Font(%x): Y: %d %d(%c) %d(%c) X: %d(%c) %d(%c)\n", (uint32_t)f, f->yAdvance, 
        	miny_offset, index_min + f->first, max_delta, index_max + f->first,
        	minx_offset, indexx_min + f->first, maxx_overlap, indexx_max + f->first);
#endif
        _gfxFont_min_yOffset = miny_offset;	// Probably only thing we need... May cache? 

    } else if(gfxFont) { // NULL passed.  Current font struct defined?
        // Switching from new to classic font behavior.
        // Move cursor pos up 6 pixels so it's at top-left of char.
        cursor_y -= 6;
    }
    gfxFont = f;
}
*/


/*

static uint32_t fetchbit(const uint8_t *p, uint32_t index)
{
	if (p[index >> 3] & (1 << (7 - (index & 7)))) return 1;
	return 0;
}

static uint32_t fetchbits_unsigned(const uint8_t *p, uint32_t index, uint32_t required)
{
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

static uint32_t fetchbits_signed(const uint8_t *p, uint32_t index, uint32_t required)
{
	uint32_t val = fetchbits_unsigned(p, index, required);
	if (val & (1 << (required - 1))) {
		return (int32_t)val - (1 << required);
	}
	return (int32_t)val;
}

uint32_t ST7735_pico::fetchpixel(const uint8_t *p, uint32_t index, uint32_t x)
{
	// The byte
	uint8_t b = p[index >> 3];
	// Shift to LSB position and mask to get value
	uint8_t s = ((fontppb-(x % fontppb)-1)*fontbpp);
	// Mask and return
	return (b >> s) & fontbppmask;
}

*/
/* Now in GFX
void ST7735_pico::charBounds(char c, int16_t *x, int16_t *y,
  int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy) {

	// BUGBUG:: Not handling offset/clip
    if (font) {
        if(c == '\n') { // Newline?
            *x  = 0;    // Reset x to zero, advance y by one line
            *y += font->line_space;
        } else if(c != '\r') { // Not a carriage return; is normal char
			uint32_t bitoffset;
			const uint8_t *data;
			if (c >= font->index1_first && c <= font->index1_last) {
				bitoffset = c - font->index1_first;
				bitoffset *= font->bits_index;
			} else if (c >= font->index2_first && c <= font->index2_last) {
				bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
				bitoffset *= font->bits_index;
			} else if (font->unicode) {
				return; // TODO: implement sparse unicode
			} else {
				return;
			}
			//Serial.printf("  index =  %d\n", fetchbits_unsigned(font->index, bitoffset, font->bits_index));
			data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);

			uint32_t encoding = fetchbits_unsigned(data, 0, 3);
			if (encoding != 0) return;
			uint32_t width = fetchbits_unsigned(data, 3, font->bits_width);
			bitoffset = font->bits_width + 3;
			uint32_t height = fetchbits_unsigned(data, bitoffset, font->bits_height);
			bitoffset += font->bits_height;
			//Serial.printf("  size =   %d,%d\n", width, height);
			//Serial.printf("  line space = %d\n", font->line_space);

			int32_t xoffset = fetchbits_signed(data, bitoffset, font->bits_xoffset);
			bitoffset += font->bits_xoffset;
			int32_t yoffset = fetchbits_signed(data, bitoffset, font->bits_yoffset);
			bitoffset += font->bits_yoffset;

			uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
			bitoffset += font->bits_delta;

            int16_t
                    x1 = *x + xoffset,
                    y1 = *y + yoffset,
                    x2 = x1 + width,
                    y2 = y1 + height;

		    if(wrap && (x2 > _width)) {
	            *x  = 0; // Reset x to zero, advance y by one line
	            *y += font->line_space;
	            x1 = *x + xoffset,
	            y1 = *y + yoffset,
	            x2 = x1 + width,
	            y2 = y1 + height;
        	}
            if(x1 < *minx) *minx = x1;
            if(y1 < *miny) *miny = y1;
            if(x2 > *maxx) *maxx = x2;
            if(y2 > *maxy) *maxy = y2;
            *x += delta;	// ? guessing here...
        }
    } 

    else if(gfxFont) {

        if(c == '\n') { // Newline?
            *x  = 0;    // Reset x to zero, advance y by one line
            *y += textsize_y * gfxFont->yAdvance;
        } else if(c != '\r') { // Not a carriage return; is normal char
            uint8_t first = gfxFont->first,
                    last  = gfxFont->last;
            if((c >= first) && (c <= last)) { // Char present in this font?
    			GFXglyph *glyph  = gfxFont->glyph + (c - first);
                uint8_t gw = glyph->width,
                        gh = glyph->height,
                        xa = glyph->xAdvance;
                int8_t  xo = glyph->xOffset,
                        yo = glyph->yOffset + gfxFont->yAdvance/2;
                if(wrap && ((*x+(((int16_t)xo+gw)*textsize_x)) > _width)) {
                    *x  = 0; // Reset x to zero, advance y by one line
                    *y += textsize_y * gfxFont->yAdvance;
                }
                int16_t tsx = (int16_t)textsize_x,
                        tsy = (int16_t)textsize_y,
                        x1 = *x + xo * tsx,
                        y1 = *y + yo * tsy,
                        x2 = x1 + gw * tsx - 1,
                        y2 = y1 + gh * tsy - 1;
                if(x1 < *minx) *minx = x1;
                if(y1 < *miny) *miny = y1;
                if(x2 > *maxx) *maxx = x2;
                if(y2 > *maxy) *maxy = y2;
                *x += xa * tsx;
            }
        }

    } else { // Default font

        if(c == '\n') {                     // Newline?
            *x  = 0;                        // Reset x to zero,
            *y += textsize_y * 8;           // advance y one line
            // min/max x/y unchaged -- that waits for next 'normal' character
        } else if(c != '\r') {  // Normal char; ignore carriage returns
            if(wrap && ((*x + textsize_x * 6) > _width)) { // Off right?
                *x  = 0;                    // Reset x to zero,
                *y += textsize_y * 8;       // advance y one line
            }
            int x2 = *x + textsize_x * 6 - 1, // Lower-right pixel of char
                y2 = *y + textsize_y * 8 - 1;
            if(x2 > *maxx) *maxx = x2;      // Track max x, y
            if(y2 > *maxy) *maxy = y2;
            if(*x < *minx) *minx = *x;      // Track min x, y
            if(*y < *miny) *miny = *y;
            *x += textsize_x * 6;             // Advance x one char
        }
    }
}
*/
/*
// Add in Adafruit versions of text bounds calculations. 
void ST7735_pico::getTextBounds(const uint8_t *buffer, uint16_t len, int16_t x, int16_t y,
      int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

    while(len--)
        charBounds(*buffer++, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}

void ST7735_pico::getTextBounds(const char *str, int16_t x, int16_t y,
        int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint8_t c; // Current character

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

    while((c = *str++))
        charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}


void ST7735_pico::drawFontPixel( uint8_t alpha, uint32_t x, uint32_t y ){
	// Adjust alpha based on the number of alpha levels supported by the font (based on bpp)
	// Note: Implemented look-up table for alpha, but made absolutely no difference in speed (T3.6)
	alpha = (uint8_t)(alpha * fontalphamx);
	uint32_t result = ((((textcolorPrexpanded - textbgcolorPrexpanded) * alpha) >> 5) + textbgcolorPrexpanded) & 0b00000111111000001111100000011111;
	Pixel(x,y,(uint16_t)((result >> 16) | result));
}

void ST7735_pico::drawFontBits(bool opaque, uint32_t bits, uint32_t numbits, int32_t x, int32_t y, uint32_t repeat)
{
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
				if (bgw>0) {
					if (opaque) {
						fillRect(x1 - bgw, y, bgw, repeat, textbgcolor);
					}
					bgw=0;
				}
				w++;
			} else {
				if (w>0) {
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

void ST7735_pico::drawGFXFontChar(unsigned int c) {
	// Lets do Adafruit GFX character output here as well
    if(c == '\r') 	 return;

    // Some quick and dirty tests to see if we can
    uint8_t first = gfxFont->first;
    if((c < first) || (c > gfxFont->last)) return; 

    GFXglyph *glyph  = gfxFont->glyph + (c - first);
    uint8_t   w     = glyph->width,
              h     = glyph->height;
			  
    // wonder if we should look at xo, yo instead?         
    if((w == 0 ||  h == 0)  && (c != 32))   return;  // Is there an associated bitmap?
	
    int16_t xo = glyph->xOffset; // sic
    int16_t yo = glyph->yOffset + gfxFont->yAdvance/2;

    if(wrap && ((cursor_x + textsize_x * (xo + w)) > _width)) {
        cursor_x  = 0;
        cursor_y += (int16_t)textsize_y * gfxFont->yAdvance;
    }

    // Lets do the work to output the font character
    uint8_t  *bitmap = gfxFont->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  xx, yy, bits = 0, bit = 0;
    //Serial.printf("DGFX_char: %c (%d,%d) : %u %u %u %u %d %d %x %x %d\n", c, cursor_x, cursor_y, w, h,  
    //			glyph->xAdvance, gfxFont->yAdvance, xo, yo, textcolor, textbgcolor, _use_fbtft);Serial.flush();

    if (textcolor == textbgcolor) {

	     //Serial.printf("DGFXChar: %c %u, %u, wh:%d %d o:%d %d\n", c, cursor_x, cursor_y, w, h, xo, yo);

    	// NOTE: Adafruit GFX does not support Opaque font output as there
    	// are issues with proportionally spaced characters that may overlap
    	// So the below is not perfect as we may overwrite a small portion 
    	// of a letter with the next one, when we blank out... 
    	// But: I prefer to let each of us decide if the limitations are
    	// worth it or not.  If Not you still have the option to not
    	// Do transparent mode and instead blank out and blink...
	    for(yy=0; yy<h; yy++) {
	    	uint8_t w_left = w;
	    	xx = 0;
	        while (w_left) {
	            if(!(bit & 7)) {
	                bits = bitmap[bo++];
	            }
	            // Could try up to 8 bits at time, but start off trying up to 4
	            uint8_t xCount;
	            if ((w_left >= 8) && ((bits & 0xff) == 0xff)) {
	            	xCount = 8;
	            	//Serial.print("8");
	                fillRect(cursor_x+(xo+xx)*textsize_x, cursor_y+(yo+yy)*textsize_y,
	                     xCount * textsize_x, textsize_y, textcolor);
	            } else if ((w_left >= 4) && ((bits & 0xf0) == 0xf0)) {
	            	xCount = 4;
	            	//Serial.print("4");
	                fillRect(cursor_x+(xo+xx)*textsize_x, cursor_y+(yo+yy)*textsize_y,
	                      xCount * textsize_x, textsize_y, textcolor);
	            } else if ((w_left >= 3) && ((bits & 0xe0) == 0xe0)) {
	            	//Serial.print("3");
	            	xCount = 3;
	                fillRect(cursor_x+(xo+xx)*textsize_x, cursor_y+(yo+yy)*textsize_y,
	                      xCount * textsize_x, textsize_y, textcolor);
	            } else if ((w_left >= 2) && ((bits & 0xc0) == 0xc0)) {
	            	//Serial.print("2");
	            	xCount = 2;
	                fillRect(cursor_x+(xo+xx)*textsize_x, cursor_y+(yo+yy)*textsize_y,
	                      xCount * textsize_x, textsize_y, textcolor);
	            } else {
	            	xCount = 1;
	            	if(bits & 0x80) {
		                if((textsize_x == 1) && (textsize_y == 1)){
		                    drawPixel(cursor_x+xo+xx, cursor_y+yo+yy, textcolor);
		                } else {
		                fillRect(cursor_x+(xo+xx)*textsize_x, cursor_y+(yo+yy)*textsize_y,
		                      textsize_x, textsize_y, textcolor);
		                }
		            }
	            }
	            xx += xCount;
	            w_left -= xCount;
	            bit += xCount;
	            bits <<= xCount;
	        }

	    }
    	_gfx_last_char_x_write = 0;
	} else {
		// To Do, properly clipping and offsetting...
		// This solid background approach is about 5 time faster
		// Lets calculate bounding rectangle that we will update
		// We need to offset by the origin.

		// We are going direct so do some offsets and clipping
		int16_t x_offset_cursor = cursor_x + _originx;	// This is where the offseted cursor is.
		int16_t x_start = x_offset_cursor;  // I am assuming no negative x offsets.
		int16_t x_end = x_offset_cursor + (glyph->xAdvance * textsize_x);
		if (glyph->xAdvance < (xo + w)) x_end = x_offset_cursor + ((xo + w)* textsize_x);  // BUGBUG Overlflows into next char position.
		int16_t x_left_fill = x_offset_cursor + xo * textsize_x;
		int16_t x;

		if (xo < 0) { 
			// Unusual character that goes back into previous character
			//Serial.printf("GFX Font char XO < 0: %c %d %d %u %u %u\n", c, xo, yo, w, h, glyph->xAdvance );
			x_start += xo * textsize_x;
			x_left_fill = 0;	// Don't need to fill anything here... 
		}

		int16_t y_start = cursor_y + _originy + (_gfxFont_min_yOffset * textsize_y)+ gfxFont->yAdvance*textsize_y/2;  // UP to most negative value.
		int16_t y_end = y_start +  gfxFont->yAdvance * textsize_y;  // how far we will update
		int16_t y = y_start;
		//int8_t y_top_fill = (yo - _gfxFont_min_yOffset) * textsize_y;	 // both negative like -10 - -16 = 6...
		int8_t y_top_fill = (yo - gfxFont->yAdvance/2 - _gfxFont_min_yOffset) * textsize_y;

		// See if anything is within clip rectangle, if not bail
		if((x_start >= _displayclipx2)   || // Clip right
			 (y_start >= _displayclipy2) || // Clip bottom
			 (x_end < _displayclipx1)    || // Clip left
			 (y_end < _displayclipy1))  	// Clip top 
		{
			// But remember to first update the cursor position
			cursor_x += glyph->xAdvance * (int16_t)textsize_x;
			return;
		}

		// If our y_end > _displayclipy2 set it to _displayclipy2 as to not have to test both  Likewise X
		if (y_end > _displayclipy2) y_end = _displayclipy2;
		if (x_end > _displayclipx2) x_end = _displayclipx2;

		// If we get here and 
		if (_gfx_last_cursor_y != (cursor_y + _originy))  _gfx_last_char_x_write = 0;

		{
			// lets try to output text in one output rectangle
			//Serial.printf("    SPI (%d %d) (%d %d)\n", x_start, y_start, x_end, y_end);Serial.flush();
			// compute the actual region we will output given 
			beginTransaction();
		
			setAddr((x_start >= _displayclipx1) ? x_start : _displayclipx1, 
					(y_start >= _displayclipy1) ? y_start : _displayclipy1, 
					x_end  - 1,  y_end - 1); 
			writecommand(ST7735_RAMWR);
			//Serial.printf("SetAddr: %u %u %u %u\n", (x_start >= _displayclipx1) ? x_start : _displayclipx1, 
			//		(y_start >= _displayclipy1) ? y_start : _displayclipy1, 
			//		x_end  - 1,  y_end - 1); 
			// First lets fill in the top parts above the actual rectangle...
			//Serial.printf("    y_top_fill %d x_left_fill %d\n", y_top_fill, x_left_fill);
			while (y_top_fill--) {
				if ( (y >= _displayclipy1) && (y < _displayclipy2)) {
					for (int16_t xx = x_start; xx < x_end; xx++) {
						if (xx >= _displayclipx1) {
							writedata16(gfxFontLastCharPosFG(xx,y)? _gfx_last_char_textcolor : (xx < x_offset_cursor)? _gfx_last_char_textbgcolor : textbgcolor);
						}
					}					
				}
				y++;
			}
			//Serial.println("    After top fill"); Serial.flush();
			// Now lets output all of the pixels for each of the rows.. 
			for(yy=0; (yy<h) && (y < _displayclipy2); yy++) {
				uint16_t bo_save = bo;
				uint8_t bit_save = bit;
				uint8_t bits_save = bits;
				for (uint8_t yts = 0; (yts < textsize_y) && (y < _displayclipy2); yts++) {
					// need to repeat the stuff for each row...
					bo = bo_save;
					bit = bit_save;
					bits = bits_save;
					x = x_start;
					if (y >= _displayclipy1) {
						while (x < x_left_fill) {
							if ( (x >= _displayclipx1) && (x < _displayclipx2) ) {
								// Don't need to check if we are in previous char as in this case x_left_fill is set to 0...
								writedata16(gfxFontLastCharPosFG(x,y)? _gfx_last_char_textcolor :  textbgcolor);
							}
							x++;
						}
				        for(xx=0; xx<w; xx++) {
				            if(!(bit++ & 7)) {
				                bits = bitmap[bo++];
				            }
				            for (uint8_t xts = 0; xts < textsize_x; xts++) {
								if ( (x >= _displayclipx1) && (x < _displayclipx2)) {
				            		if (bits & 0x80)
										writedata16(textcolor);
									else 
										writedata16(gfxFontLastCharPosFG(x,y)? _gfx_last_char_textcolor : (x < x_offset_cursor)? _gfx_last_char_textbgcolor : textbgcolor);
								}
				            	x++;	// remember our logical position...
				            }
				            bits <<= 1;
				        }
				        // Fill in any additional bg colors to right of our output
				        while (x < x_end) {
							if (x >= _displayclipx1) {
								writedata16(gfxFontLastCharPosFG(x,y)? _gfx_last_char_textcolor : (x < x_offset_cursor)? _gfx_last_char_textbgcolor : textbgcolor);
				        	}
				        	x++;
				        }
			    	}
		        	y++;	// remember which row we just output
			    }
		    }
		    // And output any more rows below us...
		    //Serial.println("    Bottom fill"); Serial.flush();
			while (y < y_end) {
				if (y >= _displayclipy1) {
					for (int16_t xx = x_start; xx < x_end; xx++) {
						if (xx >= _displayclipx1 ) {
							writedata16(gfxFontLastCharPosFG(xx,y)? _gfx_last_char_textcolor : (xx < x_offset_cursor)? _gfx_last_char_textbgcolor : textbgcolor);
						}
					}
				}
				y++;
			}
			writecommand(ST7735_NOP);
			endTransaction();
		}
		// Save away info about this last char
		_gfx_c_last = c;
		_gfx_last_cursor_x = cursor_x + _originx; 
		_gfx_last_cursor_y = cursor_y + _originy;
		_gfx_last_char_x_write = x_end;
		_gfx_last_char_textcolor = textcolor;
		_gfx_last_char_textbgcolor = textbgcolor;
	}

    cursor_x += glyph->xAdvance * (int16_t)textsize_x;
}

// Some fonts overlap characters if we detect that the previous 
// character wrote out more width than they advanced in X direction
// we may want to know if the last character output a FG or BG at a position. 
	// Opaque font chracter overlap?
//	unsigned int _gfx_c_last;
//	int16_t   _gfx_last_cursor_x, _gfx_last_cursor_y;
//	int16_t	 _gfx_last_x_overlap = 0;
	
bool ST7735_pico::gfxFontLastCharPosFG(int16_t x, int16_t y) {
    GFXglyph *glyph  = gfxFont->glyph + (_gfx_c_last -  gfxFont->first);

    uint8_t   w     = glyph->width,
              h     = glyph->height;


    int16_t xo = glyph->xOffset; // sic
    int16_t yo = glyph->yOffset + gfxFont->yAdvance/2;
    if (x >= _gfx_last_char_x_write) return false; 	// we did not update here...
    if (y < (_gfx_last_cursor_y + (yo*textsize_y)))  return false;  // above
    if (y >= (_gfx_last_cursor_y + (yo+h)*textsize_y)) return false; // below


    // Lets compute which Row this y is in the bitmap
    int16_t y_bitmap = (y - ((_gfx_last_cursor_y + (yo*textsize_y))) + textsize_y - 1) / textsize_y;
    int16_t x_bitmap = (x - ((_gfx_last_cursor_x + (xo*textsize_x))) + textsize_x - 1) / textsize_x;
    uint16_t  pixel_bit_offset = y_bitmap * w + x_bitmap;

    return ((gfxFont->bitmap[glyph->bitmapOffset + (pixel_bit_offset >> 3)]) & (0x80 >> (pixel_bit_offset & 0x7)));
}

*/