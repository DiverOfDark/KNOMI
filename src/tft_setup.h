#ifndef TFT_SETUP_H
#define TFT_SETUP_H

#include "hal/pinout.h"

#define USER_SETUP_LOADED

#define DISABLE_ALL_LIBRARY_WARNINGS

// See SetupX_Template.h for all options available
#define USER_SETUP_ID 46

#define TFT_RGB_ORDER TFT_RGB

#define GC9A01_DRIVER
#define TFT_WIDTH DISPLAY_WIDTH
#define TFT_HEIGHT DISPLAY_HEIGHT

// #define TFT_MISO GC9A01_MISO_PIN
#define TFT_MOSI GC9A01_MOSI_PIN
#define TFT_SCLK GC9A01_SCLK_PIN
#define TFT_CS GC9A01_CS_PIN   // Chip select control pin
#define TFT_DC GC9A01_DC_PIN   // Data Command control pin
#define TFT_RST GC9A01_RST_PIN // Reset pin (could connect to RST pin)
#define TFT_INVERSION_OFF

#define SPI_TOUCH_FREQUENCY 2500000

#define LOAD_GLCD // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// #define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
// #define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// #define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
// #define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
// #define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define SMOOTH_FONT

/*
 * tscycw (Write) min: 10ns, almost 100Mhz
 * tscycr (Read) min: 150ns, almost 6.6Mhz
 */
// #define SUPPORT_TRANSACTIONS

#endif
