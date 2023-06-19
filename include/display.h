#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <AnimatedGIF.h>

#define PANEL_RES_X 64                  // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32                  // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 2                   // Total number of panels chained one to another
#define NUM_ROWS 2                      // Number of rows of chained INDIVIDUAL PANELS
#define NUM_COLS 1                      // Number of INDIVIDUAL PANELS per ROW
#define CHAIN_TYPE CHAIN_BOTTOM_LEFT_UP // The layout of the panel chain

void display_setup();

void GIFDraw(GIFDRAW *pDraw);
void * GIFOpenFile(const char *fname, int32_t *pSize);
void GIFCloseFile(void *pHandle);
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition);
void display_play_gif(char *name);

#endif
