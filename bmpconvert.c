#include "bmpconvert.h"


#define LODEPNG_NO_COMPILE_DECODER
#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS

#ifdef USEWINGUI
#define LODEPNG_NO_COMPILE_ERROR_TEXT
#endif

#ifdef WINAPIONLY
#define LODEPNG_NO_COMPILE_ALLOCATORS

static void qswap(void *e1, void *e2, int size){
	char t;
	int x = size;
	char* pi = (char *) ( e1 );
	char* pj = (char *) ( e2 );
	do {
		t = *pi;
		*pi++ = *pj;
		*pj++ = t;
	} while( --x > 0 );
}

void qsort(void* ptr, size_t n, size_t size, int (*compare)(const void*, const void*) ){
    size_t i;
    char* base = (char*)ptr;
    char* midp = base;

    if( n <= 1 ) return;
    for( i=1; i<n; ++i ){
        if( compare(base, base + size*i) > 0 ){
            midp += size;
            if( midp != (base+size*i) ){
				qswap(base+size*i, midp, size);
            }
        }
    }
    qswap(base, midp, size);
    qsort(base, (midp - base)/size, size, compare);
    qsort(midp + size, n - 1 - (midp-base)/size, size, compare);
}

#define strlen lstrlen

#if defined(_MSC_VER)
#pragma function(memset)
void *memset(void *s, int c, size_t n){
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}
#else
void* memset(void * ptr, int value, size_t num){
	/* Actually, memset do NOTHING.
	Used only in zlib in lodepng for fill zeroes.
	Replaced by "HEAP_ZERO_MEMORY" flag...*/
	if( (ptr) && (num < 2) ) FillMemory(ptr, num, value);
	return ptr;
}
#endif


void* lodepng_malloc(size_t size){
	/*return HeapAlloc( GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, size );*/
	return dxMemMAlloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size){
	/*if( !ptr ) return lodepng_malloc(new_size);
	return HeapReAlloc( GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ptr, new_size );*/
	return dxMemReAlloc(ptr, new_size);
}

void lodepng_free(void* ptr){
	/*HeapFree( GetProcessHeap(), HEAP_NO_SERIALIZE, ptr );*/
	dxMemFree(ptr);
}

#endif
#include "lodepng.h"
#include "lodepng.c"


char* getPngName(const char* bmpName, char* resName){
	int i = dxStrLen(bmpName);
	while( ( i > 0 )
		&& ( bmpName[i] != '.' )
		&& ( bmpName[i] !=  '/')
		&& ( bmpName[i] != '\\' )
	) i--;
	i++;
	dxStrCpy(resName, bmpName);
	if( (i <= 1)
		|| ( bmpName[i-1] == '\\' )
		|| ( bmpName[i-1] == '/' )
	){
		i = dxStrLen(bmpName);
		resName[i++] = '.';
	}
	resName[i++] = 'p';
	resName[i++] = 'n';
	resName[i++] = 'g';
	resName[i++] = 0;
	return resName;
}

int isBmpExt(const char* bmpName){
	int i = dxStrLen(bmpName);
	i--;
	if( (bmpName[i]!='p') && (bmpName[i]!='P') ) return 0;
	i--;
	if( (bmpName[i]!='m') && (bmpName[i]!='M') ) return 0;
	i--;
	if( (bmpName[i]!='b') && (bmpName[i]!='B') ) return 0;
	return 1;
}

const char* bmp2pngErrorText(int code){
#ifdef USEWINGUI
	if( code > 0 ) return "PNG encode";
	if( code < 0 ) return "BMP decode";
	return "";
#else
	return lodepng_error_text(code);
#endif
}

int bmp2png(const char* bmpName, const char* pngName){
	dxInFileType in;
	dxOutFileType out;
	dxFileSize res;
	dxBITMAPFILEHEADER bmfh;
	dxBITMAPINFOHEADER bmih;
	dxHEADERADD bmha;
	dxRGBQUAD colors[256];
	dxLONG padWidth;
	dxLONG height, width;
	dxWORD bpp;
	dxLONG numColors;
	dxDWORD readCount = 0;
	unsigned long numChannels;
	unsigned char curVal[5];
	unsigned char* image;
	long x, y, newpos, curY;

	dxWORD blueMask  = 0x001F;
	dxWORD blueDiv   = 1;
	dxBYTE blueBitCnt = 0;
	dxWORD greenMask = 0x03E0;
	dxWORD greenDiv  = 1;
	dxWORD greenBitCnt = 0;
	dxWORD redMask   = 0x7C00;
	dxWORD redDiv    = 1;
	dxWORD redBitCnt = 0;
	dxWORD cur16Val;

	unsigned char* png;
	size_t pngsize;
	unsigned long PNGnumChannels;
	unsigned error;


	in = dxOpenRead(bmpName);
	if( ! dxInFileCheck(in) ) return -1;

	dxRead(in, &bmfh, sizeof(bmfh), 1, res);
	readCount += sizeof(bmfh);
	if( bmfh.bfType != dxBM_CHARS_MAGIC ) return -1;
	dxRead(in, &bmih, sizeof(bmih), 1, res);
	readCount += sizeof(bmih);

	bpp = bmih.biBitCount;
	numChannels = bpp / 8;
	numColors = (1 << bpp);
	height = bmih.biHeight;
	width = bmih.biWidth;
	if( height < 0 ) height = -height;
	/*if( bpp < 8 ) return -2;*/

	/* load the palette for 8 bits per pixel */
	if( bpp == 8 ){
		dxRead(in, &colors, sizeof(dxRGBQUAD), numColors, res);
		readCount += sizeof(dxRGBQUAD)*numColors;
	}

	if( readCount < bmfh.bfOffBits ){
		dxRead(in, &bmha, sizeof(bmha), 1, res);
		readCount += sizeof(bmha);

		redMask = bmha.biRedMask;
		if( redMask == 0 ) redMask = 1;
		for(x=0; x<16; x++)
			if( (redMask & (1 << x)) > 0 ) redBitCnt++;
		redDiv = 1;
		while( (redMask % redDiv) == 0 ) redDiv *= 2;
		redDiv /= 2;

		greenMask = bmha.biGreenMask;
		if( greenMask == 0 ) greenMask = 1;
		for(x=0; x<16; x++)
			if( (greenMask & (1 << x)) > 0 ) greenBitCnt++;
		greenDiv = 1;
		while( (greenMask % greenDiv) == 0 ) greenDiv *= 2;
		greenDiv /= 2;

		blueMask = bmha.biBlueMask;
		if( blueMask == 0 ) blueMask = 1;
		for(x=0; x<16; x++)
			if( (blueMask & (1 << x)) > 0 ) blueBitCnt++;
		blueDiv = 1;
		while( (blueMask % blueDiv) == 0 ) blueDiv *= 2;
		blueDiv /= 2;

		/* Skip estimated header */
		while( readCount < bmfh.bfOffBits ){
			dxRead(in, &curVal, sizeof(char), 1, res);
			readCount += sizeof(char);
		}
	}

/*
#ifndef USEWINGUI
	dxPrintf("bfReserved1     = %d\n", bmfh.bfReserved1);
	dxPrintf("bfReserved2     = %d\n", bmfh.bfReserved2);
	dxPrintf("bfOffBits       = %d\n", bmfh.bfOffBits);
	dxPrintf("header1 size    = %lu\n", sizeof(dxBITMAPINFOHEADER)+sizeof(dxBITMAPFILEHEADER) );
	dxPrintf("headerReadCount = %d\n", readCount);

	dxPrintf("biBitCount      = %d\n", bmih.biBitCount);
	dxPrintf("numChannels     = %lu\n", numChannels);
	dxPrintf("biClrImportant  = %d\n", bmih.biClrImportant);
	dxPrintf("biClrUsed       = %d\n", bmih.biClrUsed);
	dxPrintf("biCompression   = %d\n", bmih.biCompression);
	dxPrintf("biHeight        = %d\n", bmih.biHeight);
	dxPrintf("height          = %d\n", height);
	dxPrintf("biPlanes        = %d\n", bmih.biPlanes);
	dxPrintf("biSize          = %d\n", bmih.biSize);
	dxPrintf("calcSize        = %d\n", bmfh.bfSize - bmfh.bfOffBits);
	dxPrintf("biSizeImage     = %d\n", bmih.biSizeImage);
	dxPrintf("biWidth         = %d\n", bmih.biWidth);
	dxPrintf("biXPelsPerMeter = %d\n", bmih.biXPelsPerMeter);
	dxPrintf("biYPelsPerMeter = %d\n", bmih.biYPelsPerMeter);

	dxPrintf("redMask         = 0x%x\n", redMask);
	dxPrintf("redDiv          = %d\n", redDiv);
	dxPrintf("redBitCnt       = %d\n", redBitCnt);
	dxPrintf("greenMask       = 0x%x\n", greenMask);
	dxPrintf("greenDiv        = %d\n", greenDiv);
	dxPrintf("blueMask        = 0x%x\n", blueMask);
	dxPrintf("alphaMask       = 0x%x\n", bmha.biAlphaMask);
	dxPrintf("out File Name   = '%s'\n", pngName);
#endif
*/


	if( (bmih.biCompression != 0) && (bmih.biCompression != 3) ) return -5;
	if( (numChannels < 2) || ( numChannels > 4) ) return -6;




	padWidth = 0;
	while( (width + padWidth) %4 != 0 ){
		padWidth++;
	}
	/* dxPrintf("padWidth = %"dxPRIu32"\n", padWidth); */

	PNGnumChannels = 3;
	if( numChannels == 4 ) PNGnumChannels = 4;
	image = dxMemAlloc(width*height*PNGnumChannels, sizeof(char) );
	if( image == NULL ) return -3;

	for(y = 0; y < height; y++){
		for(x = 0; x < width; x++){
			curY = y;
			if( bmih.biHeight > 0 ) curY = height - y - 1;
			newpos = PNGnumChannels * curY * width + PNGnumChannels * x;
			dxRead(in, &curVal, sizeof(char), numChannels, res);

			if( numChannels == 2 ){
				/* 16 bit loader */
				cur16Val = curVal[0] + curVal[1]*256;

				image[newpos + 0] = (255.0 / ((1<<(redBitCnt+1)) - 1)  ) * (cur16Val & redMask) / redDiv;
				image[newpos + 1] = (255.0 / ((1<<(greenBitCnt+1)) - 1)  ) * (cur16Val & greenMask) / greenDiv;
				image[newpos + 2] = (255.0 / ((1<<(blueBitCnt+1)) - 1)  ) * (cur16Val & blueMask) / blueDiv;
			}

			if( numChannels >= 3 ){
				image[newpos + 0] = curVal[2];
				image[newpos + 1] = curVal[1];
				image[newpos + 2] = curVal[0];
			}
			if( numChannels == 4 )
				image[newpos + 3] = curVal[3]; /* Alpha */
		}
		/* Skip pad */
		if( numChannels == 3 ) dxRead(in, &curVal, sizeof(char), padWidth, res);
	}
	dxCloseInFile(in);

	if( numChannels == 4 ){
		error = lodepng_encode32(&png, &pngsize, image, width, height);
	} else {
		error = lodepng_encode24(&png, &pngsize, image, width, height);
	}

#ifndef USEWINGUI
	dxPrintf("PNG Encode code = %u; pngSize = %"dxPRIu32"\n", error, (dxULong4) pngsize);
#endif

	dxMemFree(image);

	out = dxOpenWriteBin(pngName);
	if( ! dxOutFileCheck(out) ) return -4;
	dxFwrite(out, png, pngsize, 1, res);
	dxCloseOutFile(out);
	return error;
}
