# ST7789T3 Initialization Guide

## Configuration: RGB565 (16bpp), SPI 4-Wire Mode — Portrait (240x320) and Landscape (320x240)

---

## SPI 4-Wire Interface (IM[3:0] = 0110)

**Pins used:** CSX (chip select, active low), DCX (data/command select), SCL (clock), SDA (data in/out)

**Protocol:**

- DCX = 0: byte on SDA is a **command**
- DCX = 1: byte on SDA is a **parameter or pixel data**
- Data is clocked in MSB-first on the **rising edge** of SCL
- CSX must go low before a transaction and can be toggled high between bytes

For 16bpp over SPI, each pixel is sent as **2 bytes** (high byte first): `[R4:R0 G5:G3] [G2:G0 B4:B0]`.

---

## Power-On Reset Sequence

```
1. Apply VDD (2.4-3.3V) and VDDI (1.65-3.3V) — any order
2. Hold RESX low for >= 10µs, then release high
3. Wait >= 120ms after RESX goes high (for internal reset to complete)
4. Chip is now in Sleep In mode, Normal Display Mode On, Display Off
```

---

## Core Initialization Registers

After HW reset, the chip is in **Sleep In** mode. All commands below use the notation: `CMD(param1, param2, ...)` where CMD is sent with DCX=0 and parameters with DCX=1.

### Step 1: Software Reset (optional, but recommended)

| Command  | Hex  | Parameters | Description |
|----------|------|------------|-------------|
| SWRESET  | 01h  | none       | Software reset; all registers return to defaults. **Wait 5ms minimum** after this before next command. If sent during Sleep In, wait **120ms** before SLPOUT. |

### Step 2: Sleep Out

| Command | Hex  | Parameters | Description |
|---------|------|------------|-------------|
| SLPOUT  | 11h  | none       | Starts DC-DC converter, internal oscillator, panel scanning. **Wait 120ms** before next command (to allow voltages to stabilize). |

### Step 3: Pixel Format — COLMOD (3Ah)

| Command | Hex  | Parameter  | Description |
|---------|------|------------|-------------|
| COLMOD  | 3Ah  | **0x55**   | Sets **both** RGB interface and control interface to 16-bit/pixel (RGB565). Bits [6:4]=101 (RGB I/F 65K), Bits [2:0]=101 (MCU/SPI I/F 16bpp). |

Default after reset is 0x66 (18bpp). You **must** write 0x55 for 565 mode.

### Step 4: Memory Access Control — MADCTL (36h)

Controls scan direction and color order. The ST7789T3 native frame memory is **240 columns × 320 rows**. MADCTL lets you remap this to match your desired orientation.

| Bit | Name | Function |
|-----|------|----------|
| D7  | MY   | Page address order: 0=top-to-bottom, 1=bottom-to-top |
| D6  | MX   | Column address order: 0=left-to-right, 1=right-to-left |
| D5  | MV   | Row/Column exchange: **0** = portrait (240x320), **1** = landscape (320x240) |
| D4  | ML   | Line address order (LCD refresh direction) |
| D3  | RGB  | 0=RGB, 1=BGR |
| D2  | MH   | Display data latch order |

#### Portrait Mode (240x320) — MV=0

This is the **native** orientation. No row/column swap needed.

- **Column (X) range**: 0 to 239 (00EFh)
- **Row (Y) range**: 0 to 319 (013Fh)

**Common MADCTL values for portrait:**

| Value  | MY | MX | MV | RGB | Result |
|--------|----|----|----|-----|--------|
| `0x00` | 0  | 0  | 0  | 0   | Portrait, origin top-left, RGB |
| `0x40` | 0  | 1  | 0  | 0   | Portrait, X mirrored (left-right flip) |
| `0x80` | 1  | 0  | 0  | 0   | Portrait, Y mirrored (upside down) |
| `0xC0` | 1  | 1  | 0  | 0   | Portrait, rotated 180° |
| `0x08` | 0  | 0  | 0  | 1   | Portrait, origin top-left, BGR subpixels |

#### Landscape Mode (320x240) — MV=1

Row/column exchange swaps the X and Y axes so the wider dimension becomes X.

- **Column (X) range**: 0 to 319 (013Fh)
- **Row (Y) range**: 0 to 239 (00EFh)

**Common MADCTL values for landscape:**

| Value  | MY | MX | MV | RGB | Result |
|--------|----|----|----|-----|--------|
| `0x60` | 0  | 1  | 1  | 0   | Landscape, origin top-left, RGB |
| `0xA0` | 1  | 0  | 1  | 0   | Landscape, rotated 180° |
| `0x20` | 0  | 0  | 1  | 0   | Landscape, Y mirrored |
| `0xE0` | 1  | 1  | 1  | 0   | Landscape, X mirrored |
| `0x68` | 0  | 1  | 1  | 1   | Landscape, origin top-left, BGR subpixels |

> **Note:** The exact value depends on how your panel is physically mounted and its subpixel order. If colours appear swapped (red/blue), toggle the RGB bit (D3).

### Step 5: Column Address Set — CASET (2Ah)

4 parameter bytes defining the X write window. The valid range depends on the MV setting in MADCTL.

**Portrait (MV=0) — full screen:**

```
CASET: 0x2A
  param1: XS[15:8]  = 0x00    (start column high byte)
  param2: XS[7:0]   = 0x00    (start column low byte)
  param3: XE[15:8]  = 0x00    (end column high byte)
  param4: XE[7:0]   = 0xEF    (end column low byte = 239)
```

**Landscape (MV=1) — full screen:**

```
CASET: 0x2A
  param1: XS[15:8]  = 0x00    (start column high byte)
  param2: XS[7:0]   = 0x00    (start column low byte)
  param3: XE[15:8]  = 0x01    (end column high byte)
  param4: XE[7:0]   = 0x3F    (end column low byte = 319)
```

### Step 6: Row Address Set — RASET (2Bh)

4 parameter bytes defining the Y write window.

**Portrait (MV=0) — full screen:**

```
RASET: 0x2B
  param1: YS[15:8]  = 0x00    (start row high byte)
  param2: YS[7:0]   = 0x00    (start row low byte)
  param3: YE[15:8]  = 0x01    (end row high byte)
  param4: YE[7:0]   = 0x3F    (end row low byte = 319)
```

**Landscape (MV=1) — full screen:**

```
RASET: 0x2B
  param1: YS[15:8]  = 0x00    (start row high byte)
  param2: YS[7:0]   = 0x00    (start row low byte)
  param3: YE[15:8]  = 0x00    (end row high byte)
  param4: YE[7:0]   = 0xEF    (end row low byte = 239)
```

**Summary of address ranges:**

| Orientation | MV | CASET (X) range | RASET (Y) range |
|-------------|-----|-----------------|-----------------|
| Portrait    | 0   | 0–239 (00EFh)   | 0–319 (013Fh)   |
| Landscape   | 1   | 0–319 (013Fh)   | 0–239 (00EFh)   |

### Step 7: Display Inversion (panel-dependent)

Many ST7789 panels require inversion ON for correct colors:

| Command | Hex  | Parameters | Description |
|---------|------|------------|-------------|
| INVON   | 21h  | none       | Display inversion on (default is off after reset) |

### Step 8: Display On

| Command | Hex  | Parameters | Description |
|---------|------|------------|-------------|
| DISPON  | 29h  | none       | Enable frame memory output to panel. Default after reset is Display Off. |

---

## Optional But Common Tuning Registers (Command Table 2)

These require that **EXTC pin is high** (to access Command Table 2 registers).

| Register   | Hex  | Default              | Typical Value | Purpose |
|------------|------|----------------------|---------------|---------|
| PORCTRL    | B2h  | 0C/0C/00/33/33       | 0C/0C/00/33/33 | Porch timing (back/front porch). Defaults are usually fine. |
| GCTRL      | B7h  | —                    | 0x35           | Gate voltage control (VGH/VGL). Panel-specific. |
| VCOMS      | BBh  | —                    | 0x19–0x32      | VCOM voltage setting. Panel-specific, affects contrast. |
| LCMCTRL    | C0h  | —                    | 0x2C           | LCM control register |
| VDVVRHEN   | C2h  | —                    | 0x01           | Enable VDV/VRH from command (not NVM) |
| VRHS       | C3h  | —                    | 0x12           | VRH set — controls GVDD (source positive voltage, range 4.45–6.4V) |
| VDVS       | C4h  | —                    | 0x20           | VDV set — offset for VCOM |
| FRCTRL2    | C6h  | —                    | 0x0F           | Frame rate in normal mode (0x0F = 60Hz) |
| PWCTRL1    | D0h  | —                    | 0xA4, 0xA1     | Power control — booster circuit settings |
| PVGAMCTRL  | E0h  | —                    | 14 bytes       | Positive gamma correction curve |
| NVGAMCTRL  | E1h  | —                    | 14 bytes       | Negative gamma correction curve |

---

## Programming Model: The Write Window and Pixel Streaming

### How CASET/RASET Define a Write Window

CASET and RASET do **not** write pixels — they define a rectangular window into the frame memory. The subsequent RAMWR command then fills that window with pixel data, one pixel at a time, using an internal address counter that auto-increments through the window.

```
CASET (2Ah) sets the horizontal bounds:  XS (start column) to XE (end column)
RASET (2Bh) sets the vertical bounds:    YS (start row)    to YE (end row)
```

Together they define the rectangle **(XS, YS) → (XE, YE)** inclusive on both ends. The window width is `(XE - XS + 1)` pixels and the height is `(YE - YS + 1)` pixels.

### Address Counter Behaviour During RAMWR

When RAMWR (2Ch) is issued, the internal column and row pointers reset to **(XS, YS)**. As pixel data bytes arrive:

1. Each pixel is written at the current (column, row) position.
2. The **column pointer increments** after each pixel.
3. When the column pointer exceeds **XE**, it wraps back to **XS** and the **row pointer increments**.
4. When the row pointer exceeds **YE**, it wraps back to **YS** (the window repeats).
5. Any non-data command sent on the bus terminates the write.

This means the pixel data is consumed in **row-major order** within the window — left to right, top to bottom (assuming default MADCTL scan direction).

```
Window defined by CASET=[XS,XE], RASET=[YS,YE]:

  (XS,YS) → pixel 0, pixel 1, pixel 2, ... pixel (XE-XS)     ← row YS
  (XS,YS+1) → next pixel, ...                                  ← row YS+1
  ...
  (XS,YE) → ..., last pixel                                    ← row YE
```

The total number of pixels to fill the window exactly is:

```
N = (XE - XS + 1) × (YE - YS + 1)
```

In RGB565 mode, that's `N × 2` bytes sent after RAMWR.

> **Important:** The window only affects where RAMWR writes. It does **not** clip or clear the rest of the display. Pixels outside the window are untouched.

### WRMEMC (3Ch) — Continue a Previous Write

If you need to pause a write (e.g. to service an interrupt) and resume later, send WRMEMC (3Ch) instead of RAMWR. It continues from the address where the previous RAMWR or WRMEMC left off, without resetting the pointer to (XS, YS). A preceding CASET/RASET is not required — it uses the existing window.

### Example: Writing an Arbitrary Block of Pixels (pseudocode)

This function blits a rectangular buffer to any position on the display. It works identically for portrait or landscape — the caller just needs to respect the coordinate range set by MADCTL.

```c
// Write a (width × height) block of RGB565 pixels to position (x, y).
// pixel_data[] is row-major: pixel_data[0] is top-left of the block.
// Each entry is a 16-bit RGB565 value.
//
void blit_block(uint16_t x, uint16_t y,
                uint16_t width, uint16_t height,
                const uint16_t *pixel_data)
{
    uint16_t xe = x + width  - 1;
    uint16_t ye = y + height - 1;

    // 1. Define the write window
    send_cmd(0x2A);              // CASET
    send_data(x  >> 8);         // XS high byte
    send_data(x  & 0xFF);       // XS low byte
    send_data(xe >> 8);         // XE high byte
    send_data(xe & 0xFF);       // XE low byte

    send_cmd(0x2B);              // RASET
    send_data(y  >> 8);         // YS high byte
    send_data(y  & 0xFF);       // YS low byte
    send_data(ye >> 8);         // YE high byte
    send_data(ye & 0xFF);       // YE low byte

    // 2. Begin memory write — pointer resets to (XS, YS)
    send_cmd(0x2C);              // RAMWR

    // 3. Stream all pixels, row by row within the window.
    //    The controller auto-increments: when column hits XE,
    //    it wraps to XS and advances to the next row.
    uint32_t total_pixels = (uint32_t)width * height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        send_data(pixel_data[i] >> 8);    // high byte: [R4:R0 G5:G3]
        send_data(pixel_data[i] & 0xFF);  // low byte:  [G2:G0 B4:B0]
    }

    // 4. Done — sending any command (or just leaving CS high)
    //    terminates the write. No explicit "end" command needed.
}
```

**Usage examples:**

```c
// Fill a 50×30 red rectangle at position (100, 80)
uint16_t red_buf[50 * 30];
uint16_t red_565 = (0x1F << 11);  // R=31, G=0, B=0
for (int i = 0; i < 50 * 30; i++) red_buf[i] = red_565;
blit_block(100, 80, 50, 30, red_buf);

// Draw a single pixel at (200, 150)
uint16_t white = 0xFFFF;
blit_block(200, 150, 1, 1, &white);

// Full-screen update (landscape 320×240)
blit_block(0, 0, 320, 240, framebuffer);

// Full-screen update (portrait 240×320)
blit_block(0, 0, 240, 320, framebuffer);
```

### Optimising with DMA / Bulk SPI Transfers

In practice, the per-pixel `send_data` loop above is slow. Most MCU SPI peripherals support DMA or bulk transfer. The pattern is the same — set the window, issue RAMWR, then DMA the entire pixel buffer with DCX held high:

```c
void blit_block_dma(uint16_t x, uint16_t y,
                    uint16_t width, uint16_t height,
                    const uint16_t *pixel_data)
{
    uint16_t xe = x + width  - 1;
    uint16_t ye = y + height - 1;

    send_cmd(0x2A);              // CASET
    send_data(x >> 8);  send_data(x & 0xFF);
    send_data(xe >> 8); send_data(xe & 0xFF);

    send_cmd(0x2B);              // RASET
    send_data(y >> 8);  send_data(y & 0xFF);
    send_data(ye >> 8); send_data(ye & 0xFF);

    send_cmd(0x2C);              // RAMWR

    // Hold DCX=1 (data mode), assert CSX=0, fire DMA
    set_dcx_high();
    set_csx_low();
    spi_dma_transfer((const uint8_t *)pixel_data,
                     (uint32_t)width * height * 2);
    // On DMA complete callback: release CSX
}
```

> **Byte order note:** The ST7789T3 expects MSB first (big-endian). If your MCU is little-endian, you may need to byte-swap each 16-bit pixel before transfer, or configure the SPI peripheral for 16-bit word mode if available.

---

## Minimal Init Sequence — Portrait 240x320 (pseudocode)

```c
hw_reset();               // RESX low 10µs, high, wait 120ms

send_cmd(0x01);           // SWRESET
delay_ms(150);

send_cmd(0x11);           // SLPOUT
delay_ms(120);

send_cmd(0x3A);           // COLMOD
send_data(0x55);          // 16-bit/pixel (RGB565)

send_cmd(0x36);           // MADCTL
send_data(0x00);          // MV=0 → native 240x320 portrait, RGB

send_cmd(0x21);           // INVON (if panel needs it)

send_cmd(0x2A);           // CASET
send_data(0x00);          // XS = 0
send_data(0x00);
send_data(0x00);          // XE = 239
send_data(0xEF);

send_cmd(0x2B);           // RASET
send_data(0x00);          // YS = 0
send_data(0x00);
send_data(0x01);          // YE = 319
send_data(0x3F);

send_cmd(0x29);           // DISPON
delay_ms(20);

// Now ready to push pixels with RAMWR (0x2C)
// Full frame = 240 × 320 × 2 = 153,600 bytes
```

## Minimal Init Sequence — Landscape 320x240 (pseudocode)

```c
hw_reset();               // RESX low 10µs, high, wait 120ms

send_cmd(0x01);           // SWRESET
delay_ms(150);

send_cmd(0x11);           // SLPOUT
delay_ms(120);

send_cmd(0x3A);           // COLMOD
send_data(0x55);          // 16-bit/pixel (RGB565)

send_cmd(0x36);           // MADCTL
send_data(0x60);          // MV=1, MX=1 → 320x240 landscape, RGB

send_cmd(0x21);           // INVON (if panel needs it)

send_cmd(0x2A);           // CASET
send_data(0x00);          // XS = 0
send_data(0x00);
send_data(0x01);          // XE = 319
send_data(0x3F);

send_cmd(0x2B);           // RASET
send_data(0x00);          // YS = 0
send_data(0x00);
send_data(0x00);          // YE = 239
send_data(0xEF);

send_cmd(0x29);           // DISPON
delay_ms(20);

// Now ready to push pixels with RAMWR (0x2C)
// Full frame = 320 × 240 × 2 = 153,600 bytes
```

---

## Key Timing Constraints Summary

| Event | Minimum Delay |
|-------|---------------|
| After HW reset (RESX high) | 120ms |
| After SWRESET | 5ms (120ms if transitioning to SLPOUT) |
| After SLPOUT | 120ms |
| SLPIN after SLPOUT | 120ms |
| After SLPIN | 5ms |

---

## SPI Timing (4-line serial, from datasheet §7.4.3)

| Parameter | Symbol | Min | Unit |
|-----------|--------|-----|------|
| Serial clock cycle (write) | TSCYCW | 16 | ns |
| Serial clock cycle (read) | TSCYCR | 150 | ns |
| SCL high duration (write) | TSHW | 6 | ns |
| SCL low duration (write) | TSLW | 6 | ns |
| Chip select setup time | TCSS | 15 | ns |
| Chip select hold time | TCSH | 15 | ns |
| Data setup time | TSDS | 6 | ns |
| Data hold time | TSDH | 6 | ns |

Maximum SPI clock for writes: ~62.5 MHz (1/16ns). Reads are limited to ~6.67 MHz (1/150ns).

---

## Reference: Display ID

Reading command RDDID (04h) returns 3 bytes after a dummy byte:

- ID1 = 0x77 (manufacturer)
- ID2 = 0x89 (driver version)
- ID3 = 0x01 (driver ID)
