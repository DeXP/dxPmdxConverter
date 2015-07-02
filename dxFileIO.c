#include "dxFileIO.h"

#ifdef WINAPIONLY

dxInFileType dxOpenRead(const char* filename){
	dxInFileType ft;
	ft.File = INVALID_HANDLE_VALUE;
	ft.Map = INVALID_HANDLE_VALUE;
	ft.isOpen = FALSE;
	ft.readed = 0;

	if( (ft.File = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL	)) == INVALID_HANDLE_VALUE) return ft;

	ft.sizeHi = 0;
	ft.sizeLo = GetFileSize(ft.File,&ft.sizeHi);
	if( !ft.sizeLo ) return ft;

	ft.Map = CreateFileMapping(ft.File, NULL, PAGE_READONLY, ft.sizeHi, ft.sizeLo, NULL);
	if( !ft.Map ) return ft;

	ft.ptr = MapViewOfFile(ft.Map, FILE_MAP_READ, 0, 0, 0);
	if( !ft.ptr ) return ft;

	ft.isOpen = TRUE;
	return ft;
}

int dxCloseInFile(dxInFileType ft){
	if( !ft.isOpen ) return 1;

	if( ft.ptr ){
		if( !UnmapViewOfFile(ft.ptr) ) return 0;
		ft.ptr = NULL;
	}

	if( ft.Map ){
		if( !CloseHandle(ft.Map) ) return 0;
		ft.Map = NULL;
	}

	if( ft.File != INVALID_HANDLE_VALUE ){
		if( !CloseHandle(ft.File) ) return 0;
		ft.File = INVALID_HANDLE_VALUE;
	}

	ft.isOpen = FALSE;
	return 1;
}


void dxReadInternal(dxInFileType* ft, void* ptr, size_t onesize, size_t count, DWORD* readed){
	/* CopyMemory(ptr, ft->ptr, count*onesize); */
	size_t i, readCount;
	char *d = ptr;
	const char *s = ft->ptr;
	s += ft->readed;

	readCount = count*onesize;
	if( ft->sizeLo - ft->readed < readCount ) readCount = ft->sizeLo - ft->readed;
	for(i=0; i<readCount; i++) {
		d[i] = s[i];
	}
	ft->readed += readCount;
	*readed = readCount;
	/*if( ft->sizeLo < ft->readed ) *readed = 0;*/
}



int dxPrintf(char * format, ...){
	char buf[IOBUF_LEN];
	va_list arglist;
	DWORD dwSize;
	DWORD dwWritten;

	va_start(arglist, format);
	dwSize = wvsprintf(buf, format, arglist);
	va_end(arglist);
	return WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, dwSize, &dwWritten,0);
}


int flushWrite(dxOutFileType file){
	DWORD dwWritten;
	FPRBUF[FPRIND+1] = 0;
	WriteFile(file, FPRBUF, FPRIND, &dwWritten, NULL);
	FPRIND = 0;
	return dwWritten;
}

int dxFprintf(dxOutFileType file, char* format, ...){
	short degree;
	long i, j, frac, mul, dec;
	double fl, div;
	va_list arg_ptr;
	char fmt[7];

	i=0;
	/*FPRIND=0;*/
	va_start(arg_ptr, format);
	while(format[i]!=0){
		if( format[i] == '%' ){
			/* Print formatted output */
			i++;
			switch( format[i] ){
				case 'd':
					FPRIND += wsprintf(FPRBUF+FPRIND, "%d", va_arg(arg_ptr, int) );
				break;
				case 'l':
					FPRIND += wsprintf(FPRBUF+FPRIND, "%ld", va_arg(arg_ptr, long) );
					if( format[i+1] == 'd' ) i++; /*  For "%ld" format - skip "d" */
					if( format[i+1] == 'u' ) i++;
				break;
				case 'c':
					FPRBUF[FPRIND++] = (char)va_arg(arg_ptr, int);
				break;
				case 's':
					FPRIND += wsprintf(FPRBUF+FPRIND, "%s", va_arg(arg_ptr, char*));
				break;
				case '.':
					/* dot. So float */
					i++;
					if( (format[i]>='0') && (format[i]<='9') ){
						degree = format[i]-'0';
						mul = 1;
						div = 1;
						for(j=0; j<degree; j++){
							mul *= 10;
							div /= 10;
						}
						fl = va_arg(arg_ptr, double);
						if( fl < 0 ){
							FPRBUF[FPRIND++] = '-';
							fl = -fl;
						}

#if defined(_MSC_VER)
						dec = (long)fl;
						if( (dec >=0) && (dec > fl) ) dec -= 1;
						if( (dec < 0) && (dec < fl) ) dec += 1;
#else
						if( (fl*mul - (long)(fl*mul)) >=0.5 ) fl += div;
						dec = (long)fl;
#endif
						if( dec < 0 ) dec = -dec;


						fl -= dec;
						fl *= mul;
						frac = (long)fl; /* Visual studio hack  :-( */
						if( frac < 0 ) frac = -frac;

						if( frac >= mul){
							frac -= mul;
							dec += 1;
						}

						FPRIND += wsprintf(FPRBUF+FPRIND, "%ld", dec );
						FPRBUF[FPRIND++] = '.';
						wsprintf(fmt, "%%.%dld", degree);
						FPRIND += wsprintf(FPRBUF+FPRIND, fmt/*"%.6ld"*/, frac);
						i++;
					}
				break;
				default:
					FPRBUF[FPRIND++] = format[i];
				break;
			}
		} else {
			FPRBUF[FPRIND++] = format[i];
		}
		i++;

		if( FPRIND >= FPRBUF_LEN-20 ) flushWrite(file);
	}
	va_end(arg_ptr);
	FPRBUF[FPRIND+1] = 0;

	/* WriteFile(file, FPRBUF, k, &dwWritten, NULL); */
	if( FPRIND >= FPRBUF_LEN-20 ) flushWrite(file);
	return FPRIND;
}

void* dxMemReAlloc(void* ptr, size_t new_size){
	if( !ptr ) return dxMemMAlloc(new_size);
	return HeapReAlloc( GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ptr, new_size );
}


int dxFileExists(const char* fileName){
	return GetFileAttributes(fileName) != INVALID_FILE_ATTRIBUTES;
}

dxFileSize dxGetFileSize(const char* fileName){
	HANDLE file;
	dxFileSize res;
	file = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE){
		CloseHandle(file);
		return 0;
	}
	res = GetFileSize(file, NULL);
	CloseHandle(file);
	return res;
}

unsigned long dxFileModTime(const char* fileName){
	HANDLE file;
	FILETIME modTime;
	ULARGE_INTEGER date, adjust;
	file = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE) return 0;
	if( !GetFileTime(file, NULL, NULL, &modTime) ){
		CloseHandle(file);
		return 0;
	}
	CloseHandle(file);
	date.HighPart = modTime.dwHighDateTime;
	date.LowPart = modTime.dwLowDateTime;
	adjust.QuadPart = 1164447360;
	adjust.QuadPart *= 100000000;
	date.QuadPart -= adjust.QuadPart;
	return (unsigned long)(date.QuadPart / 10000000);
}


int dx_init_console(const char* title){
	SetConsoleTitle(title);
	return 0;
}

#else

int dxFileExists(const char* fileName){
	FILE *file;
	file = fopen(fileName, "r");
	if( file != NULL ){
		fclose(file);
		return 1;
	} else {
		return 0;
	}
}

dxFileSize dxGetFileSize(const char* fileName){
	size_t size = 0;
	FILE *file = NULL;
    file = fopen(fileName,"rb");
    if( file == NULL ) return 0;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
    return size;
}

unsigned long dxFileModTime(const char* fileName){
#ifndef NOSTAT
	struct stat st;
	if( stat(fileName, &st) ) return 0;
	#ifdef __MACH__
	return st.st_mtimespec.tv_sec;
	#else
	return st.st_mtime;
	#endif
#else
	return 0;
#endif
}


#ifdef KOLIBRIOS
#pragma pack(push,1)
typedef struct{
	char *name;
	void *data;
} kol_struct_import;
#pragma pack(pop)
void (* _stdcall con_init)(unsigned w_w, unsigned w_h, unsigned s_w, unsigned s_h, const char* t);

void kol_exit(){
	asm ("int $0x40"::"a"(-1));
}

kol_struct_import* kol_cofflib_load(char *name){
	asm volatile ("int $0x40"::"a"(68), "b"(19), "c"(name));
}

void* kol_cofflib_procload(kol_struct_import *imp, char *name){
	int i;
	for (i=0;;i++)
		if ( NULL == ((imp+i) -> name))
			break;
		else
			if ( 0 == strcmp(name, (imp+i)->name) )
				return (imp+i)->data;
	return NULL;
}

int dx_init_console(const char* title){
	kol_struct_import *imp;

	imp = kol_cofflib_load("/sys/lib/console.obj");
	if( imp == NULL ) kol_exit();

	con_init = ( _stdcall  void (*)(unsigned, unsigned, unsigned, unsigned, const char*))
                kol_cofflib_procload (imp, "con_init");
	if( con_init == NULL ) kol_exit();

	con_printf = ( _cdecl void (*)(const char*,...))
                kol_cofflib_procload (imp, "con_printf");
	if( con_printf == NULL ) kol_exit();
	con_init(-1, -1, -1, -1, title);
	con_printf("Console inited!\n");
	return 0;
}
#else
int dx_init_console(const char* title){
	if( title[0] ) return 1;
	return 0;
}
#endif

#endif /* WINAPIONLY */

int ZeroFill(char* buf, int size){
	int i;
	for(i=0; i<size; i++) buf[i] = 0;
	return 0;
}

