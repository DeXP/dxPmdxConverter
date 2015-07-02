#ifndef DXFILEIO
#define DXFILEIO

#if defined(_MSC_VER)
	#define dxULong4 unsigned __int32
	#define dxLong4 __int32
	#define dxUShort2 unsigned __int16
	#define dxPRIu32 "lu"
#else
	/*typedef unsigned uint32_t;
	typedef int  int32_t;
	typedef short  int16_t;
	typedef unsigned short  uint16_t;*/
	#include <inttypes.h>

	#define dxULong4 uint32_t
	#define dxLong4 int32_t
	#define dxUShort2 uint16_t
		#ifdef WINAPIONLY
			#define dxPRIu32 "lu"
		#else
			#define dxPRIu32 PRIu32
		#endif
#endif
#define dxFloat4 float
#define dxInt1 char


#define IOBUF_LEN 512
#define FPRBUF_LEN 8192


#ifdef WINAPIONLY
	#include <windows.h>
	#include <stdarg.h>

	/*
	//Naive WinAPI realization. 2-3 times slower, than libc.
	#define dxFileType HANDLE
	#define dxFileSize DWORD
	#define dxOpenRead(filename) ( CreateFile(filename, GENERIC_READ, 0, NULL, \
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) )
	#define dxOpenWrite(filename) ( CreateFile(filename, GENERIC_WRITE, 0, NULL, \
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) )
	#define dxFileCheck(file) ( (file != INVALID_HANDLE_VALUE) )
	#define dxCloseFile(file) ( CloseHandle(file) )
	#define dxRead(file, buf, onesize, count, readed) \
		( ReadFile(file, buf, onesize*count, &readed, NULL) )*/

	typedef struct {
		HANDLE File;
		HANDLE Map;
		DWORD sizeHi;
		DWORD sizeLo;
		void* ptr;
		BOOL isOpen;
		unsigned long readed;
	} dxInFileType;
	#define dxOutFileType HANDLE
	#define dxFileSize DWORD
	long FPRIND;
	char FPRBUF[FPRBUF_LEN];
	int flushWrite(dxOutFileType file);

	dxInFileType dxOpenRead(const char* filename);
	#define dxInFileCheck(file) ( (file.isOpen == TRUE) )
	#define dxOutFileCheck(file) ( (file != INVALID_HANDLE_VALUE) )
	int dxCloseInFile(dxInFileType ft);
	void dxReadInternal(dxInFileType* ft, void * ptr, size_t onesize, size_t count, DWORD* readed);
	#define dxRead(file, ptr, onesize, count, readed) ( dxReadInternal(&file, ptr, onesize, count, &readed) )

	#define dxOpenWrite(filename) ( FPRIND=0,CreateFile(filename, GENERIC_WRITE, 0, NULL, \
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) )
	#define dxOpenWriteBin dxOpenWrite
	#define dxCloseOutFile(file) ( flushWrite(file),CloseHandle(file) )

	#define dxPuts(str) ( dxPrintf("%s\n", str)
	int dxPrintf(char * format, ...);
	#define dxSprintf wsprintf
	int dxFprintf(dxOutFileType file, char* format, ...);
	#define dxFwrite(file, ptr, oneSize, count, writed) WriteFile(file, ptr, oneSize*count, &writed, NULL);

	/*
	//Global Alloc memory implementation. Does NOT support ReAlloc
	#define dxMemAlloc(x, y) ( GlobalAlloc(GMEM_FIXED, (x)*(y)) )
	#define dxMemMAlloc(x) ( GlobalAlloc(GMEM_MOVEABLE, (x)) )
	#define dxMemFree(ptr) ( GlobalUnlock(ptr),GlobalFree(ptr) )
	#define dxMemReAlloc(dst, cnt) ( GlobalUnlock(ptr),GlobalLock( GlobalReAlloc(dst, cnt, GMEM_MOVEABLE) )  )
	*/
	#define dxBigAlloc(x, y) ( GlobalAlloc(GMEM_FIXED, (x)*(y)) )
	#define dxBigFree(ptr) ( GlobalUnlock(ptr),GlobalFree(ptr) )

	#define dxMemAlloc(x, y) ( HeapAlloc( GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, (x)*(y) ) )
	#define dxMemMAlloc( x ) ( HeapAlloc( GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, (x) ) )
	#define dxMemFree( ptr ) ( HeapFree ( GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ptr ) )
	void* dxMemReAlloc(void* ptr, size_t new_size);

	#define dxMemCpy CopyMemory

	#define dxStrCpy lstrcpy
	#define dxStrCmp lstrcmp
	#define dxStrLen lstrlen
	#define dxWideCharToAscii(dst, src, length) ( WideCharToMultiByte(CP_ACP, 0, src, -1, dst, length, NULL, NULL) )

	#define dxClockT DWORD
	#define dxClock GetTickCount
	#define DX_CLOCK_PER_MS 1
	#define dxDeltaClockMs(end, start) ( end-start )
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <sys/stat.h>

	#define dxInFileType FILE*
	#define dxOutFileType FILE*
	#define dxFileSize size_t
	#define dxOpenRead(filename) ( fopen(filename, "rb") )
	#define dxOpenWrite(filename) ( fopen(filename, "w+") )
	#define dxOpenWriteBin(filename) ( fopen(filename, "wb+") )
	#define dxInFileCheck(file) ( (file != NULL) )
	#define dxOutFileCheck(file) ( (file != NULL) )
	#define dxCloseInFile(file)  ( fclose(file),file=NULL )
	#define dxCloseOutFile(file) ( fclose(file),file=NULL )
	#define dxRead(file, buf, onesize, count, readed) ( res=fread(buf, onesize, count, file),res*=onesize )
	#define dxPuts puts
#ifndef KOLIBRIOS
	#define dxPrintf printf
#else
	#define dxPrintf con_printf
#endif
	#define dxSprintf sprintf
	#define dxFprintf fprintf
	#define dxFwrite(file, ptr, oneSize, count, writed) ( writed = fwrite(ptr, oneSize, count, file) )

	#define dxMemAlloc calloc
	#define dxMemMAlloc malloc
	#define dxMemReAlloc realloc
	#define dxMemCpy memcpy
	#define dxMemFree free

	#define dxBigAlloc dxMemAlloc
	#define dxBigFree dxMemFree

	#define dxStrCpy strcpy
	#define dxStrCmp strcmp
	#define dxStrLen strlen
	#define dxWideCharToAscii(dst, src, length) /*wcstombs(dst, src, length)*/

	#define dxClockT clock_t
	#define dxClock clock
	#define DX_CLOCK_PER_MS CLOCKS_PER_SEC*0.001
	#define dxDeltaClockMs(end, start) (end-start)*1000 /(CLOCKS_PER_SEC)
#endif

#define dxCurDeltaTime(start) dxDeltaClockMs(dxClock(), start)
int dxFileExists(const char* fileName);
dxFileSize dxGetFileSize(const char* fileName);
unsigned long dxFileModTime(const char* fileName);

int ZeroFill(char* buf, int size);

int dx_init_console(const char* title);


#ifdef KOLIBRIOS
void (* _cdecl con_printf)(const char* format,...);
#endif


#endif /* DXFILEIO */
