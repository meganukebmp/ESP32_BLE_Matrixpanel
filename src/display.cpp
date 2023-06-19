#include <Arduino.h>
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>
#include <SPIFFS.h>

#include <display.h>

MatrixPanel_I2S_DMA *dma_display = nullptr;
VirtualMatrixPanel  *BigMatrix = nullptr;

AnimatedGIF gif;
File f;

void display_setup() {
    HUB75_I2S_CFG mxconfig(
        PANEL_RES_X,   // module width
        PANEL_RES_Y,   // module height
        PANEL_CHAIN    // Chain length
    );

    // Display Setup
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(128); //0-255 64 good
    dma_display->clearScreen();
    dma_display->fillScreen(dma_display->color565(255, 255, 255));

    // Create a virtual panel to properly compose image coordinates
    BigMatrix = new VirtualMatrixPanel((*dma_display), NUM_ROWS, NUM_COLS, PANEL_RES_X, PANEL_RES_Y, CHAIN_TYPE);

    // Initialize display
    Serial.println("Starting display...");
    if (!dma_display->begin()) {
        // Failed, just hang the program
        Serial.println("FAILED TO ALLOCATE I2S MEMORY!!!");
        while(true){};
    }

    // All other pixel drawing functions can only be called after .begin()
    BigMatrix->fillScreen(dma_display->color565(0, 0, 0));
    gif.begin(LITTLE_ENDIAN_PIXELS);
}


// Draw a line of image directly on the LED Matrix
void GIFDraw(GIFDRAW *pDraw) {
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[320];
    int x, y, iWidth;

    iWidth = pDraw->iWidth;
    if (iWidth > MATRIX_WIDTH)
        iWidth = MATRIX_WIDTH;

    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line

    s = pDraw->pPixels;

     // restore to background color
    if (pDraw->ucDisposalMethod == 2) {
        for (x=0; x<iWidth; x++) {
            if (s[x] == pDraw->ucTransparent)
                s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }

    // Apply the new pixels to the main image
    // if transparency used
    if (pDraw->ucHasTransparency) {
        uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
        int x, iCount;
        pEnd = s + pDraw->iWidth;
        x = 0;
        // count non-transparent pixels
        iCount = 0;
        while(x < pDraw->iWidth) {
            c = ucTransparent-1;
            d = usTemp;
            // while looking for opaque pixels
            while (c != ucTransparent && s < pEnd) {
                c = *s++;
                // done, stop
                if (c == ucTransparent) {
                    // back up to treat it like transparent
                    s--;
                }
                // opaque
                else {
                    *d++ = usPalette[c];
                    iCount++;
                }
            }

             // any opaque pixels?
            if (iCount) {
                for(int xOffset = 0; xOffset < iCount; xOffset++ ) {
                    BigMatrix->drawPixel(x + xOffset, y, usTemp[xOffset]); // 565 Color Format
                }
                x += iCount;
                iCount = 0;
            }

            // no, look for a run of transparent pixels
            c = ucTransparent;
            while (c == ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent)
                    iCount++;
                else
                    s--; 
            }

            if (iCount) {
                // skip these
                x += iCount;
                iCount = 0;
            }
        }
    }
    // does not have transparency
    else {
        s = pDraw->pPixels;

        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x=0; x<pDraw->iWidth; x++) {
             // color 565
            BigMatrix->drawPixel(x, y, usPalette[*s++]);
        }
    }
}

// Open File callback
void * GIFOpenFile(const char *fname, int32_t *pSize) {
    Serial.print("Opening gif: ");
    Serial.println(fname);
    f = SPIFFS.open(fname);
    if (f) {
        *pSize = f.size();
        return (void *)&f;
    }
    return NULL;
}


// Close file callback
void GIFCloseFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL)
        f->close();
}


// Read file callback
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);

    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
        iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around

    if (iBytesRead <= 0)
        return 0;

    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
}

// Seek file callback
int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) { 
    int i = micros();
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();
    i = micros() - i;
    return pFile->iPos;
}

// TODO: Rework this to have frame advancer inbetween wireless stuff
unsigned long start_tick = 0;
bool gifOpened = false;

void display_load_gif(char *name) {
    // start_tick = millis();

    // if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    //     Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    //     Serial.flush();
    //     while (1) {
    //         gif.playFrame(true, NULL);
    //         // we'll get bored after about 10 seconds of the same looping gif
    //         if ( (millis() - start_tick) > 10000) {
    //             break;
    //         }
    //     }
    //     gif.close();
    // }

    if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
        Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        Serial.flush();
        gifOpened = true;
    }
}

void display_advance_frame() {
    if (!gifOpened)
        return;
    gif.playFrame(true, NULL);
}

void display_close_gif() {
    gif.close();
}