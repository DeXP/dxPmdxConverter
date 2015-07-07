#include "pmdx2obj.h"
#include "bmpconvert.h"

#ifdef USEWINGUI
	#include "wingui.h"
#endif


#ifdef USEWINGUI
#ifdef WINAPIONLY
int smain(){
#else
int main(){
#endif
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
/*dxOutFileType outTar;*/
	char fileName[DX_MAX_PATH];
	char tmpPath[DX_MAX_PATH];
	char pngTexName[DX_MAX_PATH];
	char pngFileName[DX_MAX_PATH];
	TPMDObj p;
	int format = FORMAT_OBJ;
	int precision = -1;
	int res, alreadyDone;
	int i;
	unsigned int j, k;
	int isBmp = 0;
	int compressOutput = 0;
	int isGzip = 0;
	int gzipPoint = 0;
	dxFileSize curSize, tarSize;
	char* tarBuf;

	int hasFile = 0;
	int texConvert = 0;
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
	dx_init_console("dxPmdxConverter");

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
				case 't':
					texConvert = 1;
				break;
				case 'c':
					compressOutput = 1;
				break;
				case 'g':
					gzipPoint = i+1;
					isGzip = 1;
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

	if( isBmp && isGzip ) dxPrintf("Sorry, you cannot combine GZIP compression with BMP convertation...\n");

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

	if( !isBmp && isGzip ){
		/* GZip archiever */
		tarSize = 0;
		dxStrCpy(fileName, argv[gzipPoint]);
		for(i=gzipPoint+1; i<argc; i++){
			if( dxFileExists(argv[i]) ){
				tarSize += tarGetSize(argv[i]);
			}
		}
		dxPrintf("Resulting TAR size: %lu\n", (long unsigned)tarSize );
		tarBuf = (char*) dxBigAlloc(tarSize, 1);
		curSize = 0;
		for(i=gzipPoint+1; i<argc; i++){
			if( dxFileExists(argv[i]) ){
				curSize += tarAppend(tarBuf + curSize, argv[i], argv[i]);
			}
		}
		dxPrintf("Real TAR size: %lu\n", (long unsigned)curSize );
		if( tarSize != curSize ) dxPrintf("!!! Something wrong. Sizes are not equal !!!\n");

		gzipToFile(tarBuf, curSize, fileName);
		dxBigFree(tarBuf);
	}

	if( !isBmp && !isGzip && hasFile ){
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

			if( texConvert ){
				dxPrintf("Converting textures from BMP to PNG:\n");
				for(j=0; j< p.materialCount; j++){
					dxSprintf(tmpPath, "%s\\%s", p.outBase, p.fm[j].fileName);
					/*dxPrintf("%s\n", tmpPath);*/
					getPngName(p.fm[j].fileName, pngTexName);
					alreadyDone = 0;
					for(k=0; k<j; k++)
						if( dxStrCmp(pngTexName, p.fm[k].fileName) == 0 )
							alreadyDone = 1;
					if( !alreadyDone && dxFileExists(tmpPath) ){
						getPngName(tmpPath, pngFileName);
						if( isBmpExt(tmpPath) && (bmp2png(tmpPath, pngFileName) == 0) ){
							/* Texture convert success */
							dxStrCpy(p.fm[j].fileName, pngTexName);

							dxPrintf("  %s (%ld ms.)\n", p.fm[j].fileName, dxCurDeltaTime(startTime) );
						} else {
							/* BMP to PNG convert error */
							;
						}
					} else if( alreadyDone ) dxStrCpy(p.fm[j].fileName, pngTexName);
				}

			}

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
		if( compressOutput ){
			dxPrintf("Compressing output and textures into archive...\n");
			Pmd2Gzip(&p);
			dxPrintf("done! (%ld ms.)\n", dxCurDeltaTime(startTime) );
		}

		PmdObjFree(&p);
		dxPrintf("==================\n");
		if( res < 0 ) dxPrintf("!!! Convertation error !!!\n");
		if( res > 0 ) dxPrintf("Convert performed with minor errors.\n");
		dxPrintf("Total covertation time: %ld ms.\n", dxCurDeltaTime(startTime) );

	}

	if( !hasFile ){
		dxPrintf("dxPmdxConverter, console version\n\
Usage: dxPmdxConverter [-o][-m][-b][-g][-t][-c][-pN] filename\n\
\t-o - sets output format to OBJ. Default on.\n\
\t-m - sets output format to MQO.\n\
\t-t - convert textures from BMP to PNG.\n\
\t-pN - float type precision. 1<=N<=7\n\
\t-c - compress output into .tgz\n\
\t-b - bitmap converter mode. Converts 'fileName' BMP to PNG.\n\
\t-g arhiveName.tgz file1 dir1/file1 dir1/file2 - TAR.GZ compressor\n\
\n\
Examples:\n\
\tdxPmdxConverter -o -p5 -t myFile.pmx\n\
\tdxPmdxConverter -b myFile.bmp\n");
	}

#ifdef WINAPIONLY
	for(i=0; i<argc; i++) dxMemFree( argv[i] );
	dxMemFree( argv );
	ExitProcess(res);
#endif
	return res;
}

#endif
