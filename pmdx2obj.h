#ifndef PMDX2OBJ_H
#define PMDX2OBJ_H

#include "dxFileIO.h"

#define dxFloat4 float
#define dxULong4 unsigned long
#define dxUShort2 unsigned short
#define dxInt1 char


#ifndef MAX_PATH
#define MAX_PATH 255
#endif

#define BUF_LEN 100
#define NAME_LEN 20
#define COMMENT_LEN 256
#define MATNAME_LEN 20


#define FORMAT_OBJ 0
#define FORMAT_MQO 1

#pragma pack (1)
typedef struct {
	dxFloat4 vx, vy, vz, nx, ny, nz, u, v;
	dxUShort2 bone0id, bone1id;
	dxInt1 bone0weight, edgeFlag;
} TVertex;
/* sizeof(TVertex) = 38 */


#pragma pack (1)
typedef struct {
    dxUShort2 face[3];
} TIndex;
/* sizeof(TIndex) = 6 */

#pragma pack (1)
typedef struct {
	dxFloat4 difR, difG, difB, difA, specularity, specR, specG, specB, ambR, ambG, ambB;
	dxInt1 toonNumb, edgeFlag;
	dxULong4 indecesNumber;
	char fileName[MATNAME_LEN];
} TMaterial;
/* sizeof(TMaterial) = 70 */

#pragma pack (1)
typedef struct {
    char fileName[MATNAME_LEN];
    dxInt1 errId;
} TFixedMaterial;



typedef struct {
    dxInFileType in;
    dxOutFileType out;
    int precision;
    int format;
    char buf[20];
    char outFile[MAX_PATH];
    char outBase[MAX_PATH];
    char mtlFile[MAX_PATH];
    char mtlBase[BUF_LEN];

	dxFloat4 version;
	char name[NAME_LEN + 1];
	char comment[COMMENT_LEN + 1];
	dxULong4 vertexCount;
	TVertex* v;
	dxULong4 indexThreeCount;
	dxULong4 indexCount;
	TIndex* ind;
	dxULong4 materialCount;
	TMaterial* mat;
	TFixedMaterial* fm;
	dxULong4 matErrCnt;
} TPMDObj;


int Pmd2Obj(const char* fileName, int format);

char* PmdFloatFormat(TPMDObj* p, const char* format, char* buf);
int PmdObjInit(TPMDObj* p, int format);
int PmdReadFile(TPMDObj* p, const char* fileName);
int ObjWriteMaterial(TPMDObj* p);
int ObjWriteVertex(TPMDObj* p);
int ObjWriteFaces(TPMDObj* p);
int PmdObjFree(TPMDObj* p);

#endif /* PMDX2OBJ_H */
