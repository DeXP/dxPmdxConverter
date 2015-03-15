#ifndef PMDX2OBJ_H
#define PMDX2OBJ_H

#include "dxFileIO.h"


#ifndef MAX_PATH
#define MAX_PATH 255
#endif

#define BUF_LEN 100
#define NAME_LEN 20
#define COMMENT_LEN 256
#define MATNAME_LEN 20


#define FORMAT_OBJ 0
#define FORMAT_MQO 1

#define PMX_BDEF1 0
#define PMX_BDEF2 1
#define PMX_BDEF4 2
#define PMX_SDEF  3


#pragma pack (1)
typedef struct {
	dxFloat4 vx, vy, vz, nx, ny, nz, u, v;
	dxUShort2 bone0id, bone1id;
	dxInt1 bone0weight, edgeFlag;
} TVertex;
/* sizeof(TVertex) = 38 */
#pragma pack()


#pragma pack (1)
typedef struct {
	dxUShort2 face[3];
} TIndex;
/* sizeof(TIndex) = 6 */
#pragma pack()


#pragma pack (1)
typedef struct {
	dxFloat4 difR, difG, difB, difA, specularity, specR, specG, specB, ambR, ambG, ambB;
	dxInt1 toonNumb, edgeFlag;
	dxULong4 indecesNumber;
	char fileName[MATNAME_LEN];
} TMaterial;
/* sizeof(TMaterial) = 70 */
#pragma pack()


#pragma pack (1)
typedef struct {
	dxInt1 face[3];
} TIndex1;
#pragma pack()


#pragma pack (1)
typedef struct {
	dxULong4 face[3];
} TIndex4;
#pragma pack()


#pragma pack (1)
typedef struct {
	dxFloat4 difR, difG, difB, difA, specR, specG, specB, specularity, ambR, ambG, ambB;
	dxInt1 drawingMode;
	dxFloat4 edgeR, edgeG, edgeB, edgeA, edgeSize;
} TPMXMatPart;
#pragma pack()


#pragma pack (1)
typedef struct {
	char fileName[MAX_PATH];
	dxInt1 errId;
} TFixedMaterial;
#pragma pack()


#pragma pack (1)
typedef struct {
	dxFloat4 vx, vy, vz, nx, ny, nz, u, v;
} TPNUV;
#pragma pack()


#pragma pack (1)
typedef struct {
	dxFloat4 x, y, z, w;
} TaddUV;
#pragma pack()


#pragma pack (1)
typedef struct {
	dxInt1 vertex;
	dxInt1 texture;
	dxInt1 material;
	dxInt1 bone;
	dxInt1 morph;
	dxInt1 rigId;
} TPMXIndexSizes;
#pragma pack()


#pragma pack (1)
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

	/* PMX */
	dxInt1 dataCount;
	dxInt1 textEncoding;
	dxInt1 addUVcount;
	TPMXIndexSizes indexSize;
} TPMDObj;
#pragma pack()


/*int Pmd2Obj(const char* fileName, int format, int precision);*/

int Read3dFile(TPMDObj* p, const char* fileName);
char* PmdFloatFormat(TPMDObj* p, const char* format, char* buf);
int PmdObjInit(TPMDObj* p, int format);
int ObjWriteMaterial(TPMDObj* p);
int ObjWriteVertex(TPMDObj* p);
int ObjWriteFaces(TPMDObj* p);
int PmdObjFree(TPMDObj* p);

int PmExtendedReadFile(TPMDObj* p);
int PmdReadFile(TPMDObj* p);

#endif /* PMDX2OBJ_H */
