#ifndef BMPCONVERT_H
#define BMPCONVERT_H

#include "dxFileIO.h"

#define dxWORD dxUShort2
#define dxDWORD dxULong4
#define dxLONG dxLong4
#define dxBYTE char

#pragma pack (1)
typedef struct dxtagBITMAPFILEHEADER {
  dxWORD  bfType;
  dxDWORD bfSize;
  dxWORD  bfReserved1;
  dxWORD  bfReserved2;
  dxDWORD bfOffBits;
} dxBITMAPFILEHEADER, *dxPBITMAPFILEHEADER;
#pragma pack()


#pragma pack (1)
typedef struct dxtagBITMAPINFOHEADER{
  dxDWORD  biSize;
  dxLONG   biWidth;
  dxLONG   biHeight;
  dxWORD   biPlanes;
  dxWORD   biBitCount;
  dxDWORD  biCompression;
  dxDWORD  biSizeImage;
  dxLONG   biXPelsPerMeter;
  dxLONG   biYPelsPerMeter;
  dxDWORD  biClrUsed;
  dxDWORD  biClrImportant;
} dxBITMAPINFOHEADER, *dxPBITMAPINFOHEADER;
#pragma pack()


#pragma pack (1)
typedef struct {
  dxDWORD	biRedMask;
	dxDWORD	biGreenMask;
	dxDWORD	biBlueMask;
	dxDWORD	biAlphaMask;
	dxDWORD	biCSType;
} dxHEADERADD, *dxPHEADERADD;
#pragma pack()


#pragma pack (1)
typedef struct dxtagRGBQUAD {
  dxBYTE	rgbBlue;
  dxBYTE	rgbGreen;
  dxBYTE	rgbRed;
  dxBYTE	rgbReserved;
} dxRGBQUAD;
#pragma pack()

#define dxBM_CHARS_MAGIC 0x4D42

int isBmpExt(const char* bmpName);
char* getPngName(const char* bmpName, char* resName);
int bmp2png(const char* bmpName, const char* pngName);

const char* bmp2pngErrorText(int code);


#endif
