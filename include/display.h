#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <AnimatedGIF.h>

#define PANEL_RES_X 64                  // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32                  // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 2                   // Total number of panels chained one to another
#define NUM_ROWS 2                      // Number of rows of chained INDIVIDUAL PANELS
#define NUM_COLS 1                      // Number of INDIVIDUAL PANELS per ROW
#define CHAIN_TYPE CHAIN_BOTTOM_LEFT_UP // The layout of the panel chain

// Change these to whatever suits
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN -1 // required for 1/32 scan panels, like 64x64px. Any available pin would do, i.e. IO32
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

void display_setup();

void GIFDraw(GIFDRAW *pDraw);
void * GIFOpenFile(const char *fname, int32_t *pSize);
void GIFCloseFile(void *pHandle);
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition);
void display_load_gif(const char *name);
void display_advance_frame();
void display_close_gif();
void display_show_loading_screen();

#endif
