/**
 * ST7789T3 Command Bytes
 * 240RGB x 320 dot, 262K Color TFT Controller/Driver
 * Extracted from ST7789T3_SPEC_V1.0 (2022/05)
 */

#ifndef ST7789T3_COMMANDS_H
#define ST7789T3_COMMANDS_H

/* ============================================================================
 * System Function Command Table 1
 * ============================================================================ */

#define ST7789_NOP          0x00  // No operation
#define ST7789_SWRESET      0x01  // Software reset
#define ST7789_RDDID        0x04  // Read display ID (ID1, ID2, ID3)
#define ST7789_RDDST        0x09  // Read display status
#define ST7789_RDDPM        0x0A  // Read display power mode
#define ST7789_RDDMADCTL    0x0B  // Read display MADCTL
#define ST7789_RDDCOLMOD    0x0C  // Read display pixel format
#define ST7789_RDDIM        0x0D  // Read display image mode
#define ST7789_RDDSM        0x0E  // Read display signal mode
#define ST7789_RDDSDR       0x0F  // Read display self-diagnostic result
#define ST7789_SLPIN        0x10  // Sleep in
#define ST7789_SLPOUT       0x11  // Sleep out
#define ST7789_PTLON        0x12  // Partial display mode on
#define ST7789_NORON        0x13  // Normal display mode on (partial off)
#define ST7789_INVOFF       0x20  // Display inversion off
#define ST7789_INVON        0x21  // Display inversion on
#define ST7789_GAMSET       0x26  // Gamma set
#define ST7789_DISPOFF      0x28  // Display off
#define ST7789_DISPON       0x29  // Display on
#define ST7789_CASET        0x2A  // Column address set (XS, XE)
#define ST7789_RASET        0x2B  // Row address set (YS, YE)
#define ST7789_RAMWR        0x2C  // Memory write
#define ST7789_RAMRD        0x2E  // Memory read
#define ST7789_PTLAR        0x30  // Partial area (start/end address)
#define ST7789_VSCRDEF      0x33  // Vertical scrolling definition (TFA, VSA, BFA)
#define ST7789_TEOFF        0x34  // Tearing effect line off
#define ST7789_TEON         0x35  // Tearing effect line on
#define ST7789_MADCTL       0x36  // Memory data access control (MY, MX, MV, ML, RGB, MH)
#define ST7789_VSCSAD       0x37  // Vertical scroll start address of RAM
#define ST7789_IDMOFF       0x38  // Idle mode off
#define ST7789_IDMON        0x39  // Idle mode on
#define ST7789_COLMOD       0x3A  // Interface pixel format
#define ST7789_WRMEMC       0x3C  // Write memory continue
#define ST7789_RDMEMC       0x3E  // Read memory continue
#define ST7789_STE          0x44  // Set tear scanline
#define ST7789_GSCAN        0x45  // Get scanline
#define ST7789_WRDISBV      0x51  // Write display brightness
#define ST7789_RDDISBV      0x52  // Read display brightness value
#define ST7789_WRCTRLD      0x53  // Write CTRL display (BCTRL, DD, BL)
#define ST7789_RDCTRLD      0x54  // Read CTRL value display
#define ST7789_WRCACE       0x55  // Write content adaptive brightness control and color enhancement
#define ST7789_RDCABC       0x56  // Read content adaptive brightness control
#define ST7789_WRCABCMB     0x5E  // Write CABC minimum brightness
#define ST7789_RDCABCMB     0x5F  // Read CABC minimum brightness
#define ST7789_RDABCSDR     0x68  // Read automatic brightness control self-diagnostic result
#define ST7789_RDID1        0xDA  // Read ID1
#define ST7789_RDID2        0xDB  // Read ID2
#define ST7789_RDID3        0xDC  // Read ID3

/* ============================================================================
 * System Function Command Table 2
 * ============================================================================ */

#define ST7789_RAMCTRL      0xB0  // RAM control (RM, DM, ENDIAN, RIM, MDT)
#define ST7789_RGBCTRL      0xB1  // RGB interface control (WO, RCM, VSPL, HSPL, DPL, EPL, VBP, HBP)
#define ST7789_PORCTRL      0xB2  // Porch setting (BPA, FPA, PSEN, BPB, FPB, BPC, FPC)
#define ST7789_FRCTRL1      0xB3  // Frame rate control 1 - partial mode/idle colors (FRSEN, DIV, NLB, RTNB)
#define ST7789_PARCTRL      0xB5  // Partial mode control (NDL, ISC)
#define ST7789_GCTRL        0xB7  // Gate control (VGHS, VGLS)
#define ST7789_GTADJ        0xB8  // Gate on timing adjustment (GTA, GOFR, GOF)
#define ST7789_DGMEN        0xBA  // Digital gamma enable
#define ST7789_VCOMS        0xBB  // VCOMS setting
#define ST7789_LCMCTRL      0xC0  // LCM control (XMY, XBGR, XINV, XMX, XMH, XMV, XGS)
#define ST7789_IDSET        0xC1  // ID code setting (ID1, ID2, ID3)
#define ST7789_VDVVRHEN     0xC2  // VDV and VRH command enable (CMDEN)
#define ST7789_VRHS         0xC3  // VRH set
#define ST7789_VDVS         0xC4  // VDV set
#define ST7789_VCMOFSET     0xC5  // VCOMS offset set
#define ST7789_FRCTRL2      0xC6  // Frame rate control in normal mode (NLA, RTNA)
#define ST7789_CABCCTRL     0xC7  // CABC control (LEDONREV, DPOFPWM, PWMFIX, PWMPOL)
#define ST7789_REGSEL1      0xC8  // Register value selection 1
#define ST7789_REGSEL2      0xCA  // Register value selection 2
#define ST7789_PWMFRSEL     0xCC  // PWM frequency selection
#define ST7789_PWCTRL1      0xD0  // Power control 1 (AVDD, AVCL, VDS)
#define ST7789_VAPVANEN     0xD2  // Enable VAP/VAN signal output
#define ST7789_CMD2EN       0xDF  // Command 2 enable
#define ST7789_PVGAMCTRL    0xE0  // Positive voltage gamma control (14 params)
#define ST7789_NVGAMCTRL    0xE1  // Negative voltage gamma control (14 params)
#define ST7789_DGMLUTR      0xE2  // Digital gamma look-up table for red (64 entries)
#define ST7789_DGMLUTB      0xE3  // Digital gamma look-up table for blue (64 entries)
#define ST7789_GATECTRL     0xE4  // Gate control (NL, SCN, TMG, SM, GS)
#define ST7789_SPI2EN       0xE7  // SPI2 enable
#define ST7789_PWCTRL2      0xE8  // Power control 2 (SBCLK, STP14CK)
#define ST7789_EQCTRL      0xE9  // Equalize time control (SEQ, SPRET, GEQ)
#define ST7789_PROMCTRL     0xEC  // Program mode control
#define ST7789_PROMEN       0xFA  // Program mode enable
#define ST7789_NVMSET       0xFC  // NVM setting (address + data)
#define ST7789_PROMACT      0xFE  // Program action

/* ============================================================================
 * MADCTL (0x36) bit definitions
 * ============================================================================ */

#define ST7789_MADCTL_MY    0x80  // Row address order (top-bottom mirror)
#define ST7789_MADCTL_MX    0x40  // Column address order (left-right mirror)
#define ST7789_MADCTL_MV    0x20  // Row/column exchange (transpose)
#define ST7789_MADCTL_ML    0x10  // Vertical refresh order
#define ST7789_MADCTL_RGB   0x08  // RGB/BGR order (0=RGB, 1=BGR)
#define ST7789_MADCTL_MH    0x04  // Horizontal refresh order

/* ============================================================================
 * COLMOD (0x3A) pixel format values
 * ============================================================================ */

#define ST7789_COLMOD_RGB_65K   0x50  // RGB interface: 16-bit/pixel (65K colors)
#define ST7789_COLMOD_RGB_262K  0x60  // RGB interface: 18-bit/pixel (262K colors)
#define ST7789_COLMOD_CTL_12BIT 0x03  // Control interface: 12-bit/pixel (4K colors)
#define ST7789_COLMOD_CTL_16BIT 0x05  // Control interface: 16-bit/pixel (65K colors)
#define ST7789_COLMOD_CTL_18BIT 0x06  // Control interface: 18-bit/pixel (262K colors)

#endif /* ST7789T3_COMMANDS_H */
