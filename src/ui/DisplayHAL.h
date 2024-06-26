#pragma once
#include "hal/pinout.h"
#include <AnimatedGIF.h>
#include <TFT_eSPI.h>
#include <Ticker.h>

class DisplayHAL {
public:
  // TODO encapsulate
  TFT_eSPI *tft;

  DisplayHAL() {
    tft = new TFT_eSPI(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    tft->init();
    tft->fillScreen(0);
    tft->invertDisplay(true);
    tft->setRotation(0);

    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);

    pinMode(LCD_BL_PIN, OUTPUT);
    digitalWrite(LCD_BL_PIN, HIGH);
  }

  void setBackgroundColor(uint32_t color) { tft->fillScreen(toSpiColor(color)); }

  // GIFDraw is called by AnimatedGIF library frame to screen

#define BUFFER_SIZE 256 // Optimum is >= GIF width or integral division of width

  uint16_t usTemp[BUFFER_SIZE]; // Global to support DMA use

  // Draw a line of image directly on the LCD
  void GIFDraw(GIFDRAW *pDraw, int xOffset, int yOffset, int imageWidth, int imageHeight) {
    tft->startWrite();

    int xOffsetFromLeft = CENTER_X + xOffset - imageWidth / 2;
    int yOffsetFromTop = CENTER_Y + yOffset - imageHeight / 2;

    uint8_t *s;
    uint16_t *d, *usPalette;
    int x, y, iWidth, iCount;

    // Display bounds check and cropping
    iWidth = pDraw->iWidth;
    if (iWidth + pDraw->iX > DISPLAY_WIDTH)
      iWidth = DISPLAY_WIDTH - pDraw->iX;
    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line
    if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
      return;

    // Old image disposal
    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
      for (x = 0; x < iWidth; x++) {
        if (s[x] == pDraw->ucTransparent)
          s[x] = pDraw->ucBackground;
      }
      pDraw->ucHasTransparency = 0;
    }

    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
      uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
      pEnd = s + iWidth;
      x = 0;
      iCount = 0; // count non-transparent pixels
      while (x < iWidth) {
        c = ucTransparent - 1;
        d = &usTemp[0];
        while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE) {
          c = *s++;
          if (c == ucTransparent) // done, stop
          {
            s--; // back up to treat it like transparent
          } else // opaque
          {
            *d++ = usPalette[c];
            iCount++;
          }
        }           // while looking for opaque pixels
        if (iCount) // any opaque pixels?
        {
          // DMA would degrtade performance here due to short line segments
          tft->setAddrWindow(pDraw->iX + x + xOffsetFromLeft, y + yOffsetFromTop, iCount, 1);
          tft->pushPixels(usTemp, iCount);
          x += iCount;
          iCount = 0;
        }
        // no, look for a run of transparent pixels
        c = ucTransparent;
        while (c == ucTransparent && s < pEnd) {
          c = *s++;
          if (c == ucTransparent)
            x++;
          else
            s--;
        }
      }
    } else {
      s = pDraw->pPixels;

      // Unroll the first pass to boost DMA performance
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++)
          usTemp[iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++)
          usTemp[iCount] = usPalette[*s++];

      tft->setAddrWindow(pDraw->iX + xOffsetFromLeft, y + yOffsetFromTop, iWidth, 1);
      tft->pushPixels(&usTemp[0], iCount);

      iWidth -= iCount;
      // Loop if pixel buffer smaller than width
      while (iWidth > 0) {
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        if (iWidth <= BUFFER_SIZE)
          for (iCount = 0; iCount < iWidth; iCount++)
            usTemp[iCount] = usPalette[*s++];
        else
          for (iCount = 0; iCount < BUFFER_SIZE; iCount++)
            usTemp[iCount] = usPalette[*s++];

        tft->pushPixels(&usTemp[0], iCount);
        iWidth -= iCount;
      }
    }

    tft->endWrite();
  }

  static uint32_t toSpiColor(uint32_t color) {
    byte r = color >> 16 & 0xFF;
    byte g = color >> 8 & 0xFF;
    byte b = color >> 0 & 0xFF;
    return ((r & 0xF8) << 8 | (g & 0xFC) << 3 | (b >> 3));
  }
};