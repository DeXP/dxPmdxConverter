/*
#define WINAPIONLY 1
*/
#include "pmdx2obj.h"

/*#ifdef WINAPIONLY*/
#include "wingui.h"
/*#endif*/

int smain(){
    /*dxPrintf("sizeof(float) = %d\n", sizeof(float));
    dxPrintf("sizeof(unsigned long) = %d\n", sizeof(unsigned long));
    dxPrintf("sizeof(unsigned short) = %d\n", sizeof(unsigned short));
    dxPrintf("sizeof(TVertex) = %d\n", sizeof(TVertex));
    dxPrintf("sizeof(TIndex) = %d\n", sizeof(TIndex));
    dxPrintf("sizeof(TMaterial) = %d\n", sizeof(TMaterial));
    dxPrintf("Opening file '%s'\n", inputFileName);*/

    /*return Pmd2Obj("DragonTail.pmd", FORMAT_OBJ);*/
    /* return Pmd2Obj("e:\\OneMangaDay\\test\\loud gumi.pmd", FORMAT_MQO); */
    GuiCreateWindow();
    /*TPMDObj p;
    char buf[200];
    PmdObjInit(&p, FORMAT_MQO);
    dxPrintf("Format = '%s'", PmdFloatFormat(&p, "\t\"m%lu\" col(%f %f %f %f) dif(%f) amb(%f) \
emi(%f) spc(%f) power(%f)", buf));*/
    return 0;
}


