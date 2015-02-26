#ifndef DXFILEIO
#define DXFILEIO

#define IOBUF_LEN 256
/* #define FPRBUF_LEN 16384 */
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
  void dxReadBin(dxInFileType* ft, void * ptr, size_t onesize, size_t count, DWORD* readed);
  #define dxRead(file, ptr, onesize, count, readed) ( dxReadBin(&file, ptr, onesize, count, &readed) )

  #define dxOpenWrite(filename) ( FPRIND=0,CreateFile(filename, GENERIC_WRITE, 0, NULL, \
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) )
  #define dxCloseOutFile(file) ( flushWrite(file),CloseHandle(file) )

  #define dxPuts(str) ( dxPrintf("%s\n", str)
  int dxPrintf(char * format, ...);
  int dxFprintf(dxOutFileType file, char* format, ...);

  #define dxMemAlloc(x, y) ( GlobalAlloc(GMEM_FIXED, (x)*(y)) )
  #define dxMemCpy CopyMemory
  #define dxMemFree GlobalFree

  #define dxStrCpy lstrcpy
  #define dxStrLen lstrlen

  #define dxClockT DWORD
  #define dxClock GetTickCount
  #define DX_CLOCK_PER_MS 1.0
#else
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #define dxInFileType FILE*
  #define dxOutFileType FILE*
  #define dxFileSize size_t
  #define dxOpenRead(filename) ( fopen(filename, "rb") )
  #define dxOpenWrite(filename) ( fopen(filename, "w+") )
  #define dxInFileCheck(file) ( (file != NULL) )
  #define dxOutFileCheck(file) ( (file != NULL) )
  #define dxCloseInFile fclose
  #define dxCloseOutFile fclose
  #define dxRead(file, buf, onesize, count, readed) ( res=fread(buf, onesize, count, file),res*=onesize )
  #define dxPrintBuf printf
  #define dxPuts puts
  #define dxPrintf printf
  #define dxFprintf fprintf

  #define dxMemAlloc calloc
  #define dxMemCpy memcpy
  #define dxMemFree free

  #define dxStrCpy strcpy
  #define dxStrLen strlen

  #define dxClockT clock_t
  #define dxClock clock
  #define DX_CLOCK_PER_MS CLOCKS_PER_SEC*1000.0


  #define smain main
#endif

#endif /* DXFILEIO */
