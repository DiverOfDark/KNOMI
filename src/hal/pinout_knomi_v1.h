#pragma once

#define BOOT_PIN 0

// knomi v1
#define GC9A01_MOSI_PIN 23 // In some display driver board, it might be written as "SDA" and so on.
#define GC9A01_SCLK_PIN 18
#define GC9A01_CS_PIN 5  // Chip select control pin
#define GC9A01_DC_PIN 19 // Data Command control pin
#define GC9A01_RST_PIN 4 // Reset pin (could connect to Arduino RESET pin)

// // PWM
#define LCD_BL_PIN 2

#define DSPI_FREQUENCY 27000000
#define DSPI_READ_FREQUENCY 20000000