#include "pmdx2obj.h"
#include "bmpconvert.h"

#ifdef USEWINGUI
	#include "wingui.h"
#endif

/*#ifndef WINAPIONLY
	#define ConsoleMain main
#endif*/


#ifdef USEWINGUI
#ifdef WINAPIONLY
int smain(){
#else
int main(){
#endif
	/*return Pmd2Obj("DragonTail.pmd", FORMAT_OBJ, -1);*/
	/* return Pmd2Obj("e:\\OneMangaDay\\test\\loud gumi.pmd", FORMAT_OBJ, -1); */
	/* return Pmd2Obj("House.pmx", FORMAT_OBJ, -1); */
	return GuiCreateWindow();
}

#else
/* No WIN GUI -- Console application */

#ifdef WINAPIONLY
int smain(){
#else
int main(int argc, char** argv){
#endif
	char fileName[MAX_PATH];
	char tmpPath[MAX_PATH];
	TPMDObj p;
	int format = FORMAT_OBJ;
	int precision = -1;
	int res;
	int i;
	int isBmp = 0;
	int hasFile = 0;
	dxClockT startTime;
#ifdef WINAPIONLY
	int argc;
	char** argv;
	int size;

	LPWSTR* lpArgv = CommandLineToArgvW( GetCommandLineW(), &argc );
	argv = (char**) dxMemAlloc(argc, sizeof(char*) );

	for(i=0; i<argc; i++){
		size = lstrlenW( lpArgv[i] ) + 1;
		argv[i] = dxMemAlloc(size, 1);
		dxWideCharToAscii(argv[i], lpArgv[i], size);
	}
	dxMemFree( lpArgv );
#endif

	fileName[0] = 0;
	for(i=1; i<argc; i++){
		if( argv[i][0] == '-' ){
			switch( argv[i][1] ){
				case 'o':
					format = FORMAT_OBJ;
				break;
				case 'm':
					format = FORMAT_MQO;
				break;
				case 'b':
					isBmp = 1;
				break;
				case 'p':
					precision = argv[i][2] - '0';
				break;
			}
		} else {
			if( dxFileExists(argv[i]) ){
				dxStrCpy(fileName, argv[i]);
				hasFile = 1;
			}
		}
	}

	/*dxPrintf("sizeof(dxFloat4) = %lu\n", sizeof(dxFloat4));
	dxPrintf("sizeof(dxULong4) = %lu\n", sizeof(dxULong4));
	dxPrintf("sizeof(dxUShort2) = %lu\n", sizeof(dxUShort2));
	dxPrintf("sizeof(TVertex) = %lu\n", sizeof(TVertex));
	dxPrintf("sizeof(TIndex) = %lu\n", sizeof(TIndex));
	dxPrintf("sizeof(TMaterial) = %lu\n", sizeof(TMaterial));*/

	res = 0;
	if( isBmp && hasFile ){
		getPngName(fileName, tmpPath);
		dxPrintf("Input file: %s\n", fileName);
		dxPrintf("Output format: PNG\n");
		dxPrintf("Output file: %s\n", tmpPath);
		dxPrintf("==================\n");
		startTime = dxClock();
		res = bmp2png(fileName, tmpPath);
		if( res != 0 ){
			dxPrintf("Error code: %d\n", res);
			if( res < 0 ) dxPrintf("BMP decode error...\n");
				else dxPrintf("PNG encode error: %s\n", bmp2pngErrorText(res) );
		}
		dxPrintf("Done in %ld ms.\n", dxCurDeltaTime(startTime) );
	}

	if( !isBmp && hasFile ){
		PmdObjInit(&p, format);
		if( (precision > 0) && (precision < 8) ) p.precision = precision;

		dxPrintf("Input file: %s\n", fileName);
		dxPrintf("Output format: %s\n", (format == FORMAT_MQO)? "MQO": "OBJ");
		dxPrintf("Precision: %d\n", p.precision);
		dxPrintf("==================\n");

		startTime = dxClock();
		dxPrintf("Reading input file ... ");
		res |= Read3dFile(&p, fileName);
		dxPrintf("done! (%ld ms.)\n", dxCurDeltaTime(startTime) );
		if( res >= 0 ){
			dxPrintf("Writing output: Materials ... ");
			res |= ObjWriteMaterial(&p);
			dxPrintf("done! (%ld ms.)\n", dxCurDeltaTime(startTime) );
			if( res >= 0 ){
				dxPrintf("Writing output: Vertexes ... ");
				res |= ObjWriteVertex(&p);
				dxPrintf("done! (%ld ms.)\n", dxCurDeltaTime(startTime) );
				if( res >= 0 ){
					dxPrintf("Writing output: Faces ... ");
					res |= ObjWriteFaces(&p);
					dxPrintf("done! (%ld ms.)\n", dxCurDeltaTime(startTime) );
				}
			}
		}
		PmdObjFree(&p);
		dxPrintf("==================\n");
		if( res < 0 ) dxPrintf("!!! Convertation error !!!\n");
		if( res > 0 ) dxPrintf("Convert performed with minor errors.\n");
		dxPrintf("Total covertation time: %ld ms.\n", dxCurDeltaTime(startTime) );

	}

	if( !hasFile ){
		dxPrintf("dxPmdxConverter, console version\n\
Usage: dxPmdxConverter [-o][-m][-b][-pN] filename\n\
\t-o - sets output format to OBJ. Default on.\n\
\t-m - sets output format to MQO.\n\
\t-b - bitmap converter mode. Converts 'fileName' BMP to PNG.\n\
\t-pN - float type precision. 1<=N<=7\n\
\n\
Examples:\n\
\tdxPmdxConverter -o -p5 myFile.pmx\n\
\tdxPmdxConverter -b myFile.bmp\n");
	}

#ifdef WINAPIONLY
	for(i=0; i<argc; i++) dxMemFree( argv[i] );
	dxMemFree( argv );
#endif
	return res;
}

#endif
