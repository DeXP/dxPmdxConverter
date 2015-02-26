#include "pmdx2obj.h"

int PmdObjFree(TPMDObj* p){
    if( p->v != NULL ) dxMemFree(p->v);
	if( p->ind != NULL ) dxMemFree(p->ind);
	if( p->mat != NULL ) dxMemFree(p->mat);
	if( dxInFileCheck(p->in) ) dxCloseInFile(p->in);
	if( dxOutFileCheck(p->out) ) dxCloseOutFile(p->out);
	return 0;
}

int PmdObjInit(TPMDObj* p, int format){
	p->format = format;
	p->precision = 6;
	if( format == FORMAT_MQO ) p->precision = 3;
	return 0;
}

char* PmdFloatFormat(TPMDObj* p, const char* format, char* buf){
    long i, j, n;

    n = dxStrLen(format);
    for(i=0,j=0; i<=n; i++,j++){
        if( (i>0) && (format[i]=='f') && (format[i-1]=='%') ){
            buf[j++] = '.';
            buf[j++] = '0' + p->precision;
        }
        buf[j] = format[i];
    }
    return buf;
}

int PmdReadFile(TPMDObj* p, const char* fileName){
    char buf[BUF_LEN + 1];
    unsigned long i, j, n;
    dxFileSize res;

    /* Fill file names structures */
    n = dxStrLen(fileName);
    while( (n>0) && (fileName[n]!='/') && (fileName[n]!='\\') ) n--;
    for(i=0; i<n; i++)
        p->outBase[i] = fileName[i];
    p->outBase[n] = 0;

    n = dxStrLen(fileName);
    while( (n>0) && (fileName[n]!='.') ) n--;
    for(i=0; i<=n; i++)
        p->outFile[i] = fileName[i];
    if( p->format == FORMAT_MQO ){
        p->outFile[i++] = 'm';
        p->outFile[i++] = 'q';
        p->outFile[i++] = 'o';
        p->outFile[i++] = 0;
    } else {
        p->outFile[i++] = 'o';
        p->outFile[i++] = 'b';
        p->outFile[i++] = 'j';
        p->outFile[i++] = 0;
    }
    for(i=0; i<=n+4; i++)
        p->mtlFile[i] = p->outFile[i];
    if( p->format == FORMAT_OBJ ){
        i = n+1;
        p->mtlFile[i++] = 'm';
        p->mtlFile[i++] = 't';
        p->mtlFile[i++] = 'l';
        p->mtlFile[i++] = 0;
    }

    n = dxStrLen(p->mtlFile);
    i=n;
    while( (i>0) && (p->mtlFile[i-1]!='/') && (p->mtlFile[i-1]!='\\') ) i--;
    for(j=0; i<=n; i++, j++)
        p->mtlBase[j] = p->mtlFile[i];


    /* Actual file reading */
    p->in = dxOpenRead(fileName);
	if( ! dxInFileCheck(p->in) ){
		/* dxPrintf("Can't open file: %s\n", inputFileName); */
		return -1;
	}

	dxRead(p->in, buf, sizeof(char), 3, res);
	if( (buf[0]!='P') || (buf[1]!='m') || (buf[2]!='d') )
		return -2;

    dxRead(p->in, &p->version, sizeof(p->version), 1, res);
    if( ( p->version < 0 ) || ( 1.0- p->version > 0.0001 ) ) return -4;
    dxRead(p->in, p->name, sizeof(char), NAME_LEN, res);
	dxRead(p->in, p->comment, sizeof(char), COMMENT_LEN, res);
	if( res != COMMENT_LEN ) return -4;

	dxRead(p->in, &p->vertexCount, sizeof(p->vertexCount), 1, res);
	p->v = (TVertex*) dxMemAlloc(p->vertexCount, sizeof(TVertex) );
	if( p->v == NULL ){
        PmdObjFree(p);
        return -3;
	}

	for(i=0; i< p->vertexCount; i++){
		dxRead(p->in, &p->v[i], sizeof(TVertex), 1, res);
		/*if( (i==0) || (i==1) ) dxPrintf("vx = %d; vy = %d; b0id='%hu', b1id=%hu, res = %d\n",
			(int)v[i].vx, (int)v[i].vy, v[i].bone0id, v[i].bone1id, res);*/
	}


	dxRead(p->in, &p->indexThreeCount, sizeof(p->indexThreeCount), 1, res);
	p->indexCount = p->indexThreeCount / 3;
	p->ind = (TIndex*) dxMemAlloc(p->indexCount, sizeof(TIndex) );
	if( p->ind == NULL ){
        PmdObjFree(p);
        return -3;
	}

	for(i=0; i< p->indexCount; i++)
		dxRead(p->in, &p->ind[i], sizeof(TIndex), 1, res);


	dxRead(p->in, &p->materialCount, sizeof(p->materialCount), 1, res);
	p->mat = (TMaterial*) dxMemAlloc(p->materialCount, sizeof(TMaterial) );
	if( p->mat == NULL ){
        PmdObjFree(p);
        return -3;
	}
	p->fm = (TFixedMaterial*) dxMemAlloc(p->materialCount, sizeof(TFixedMaterial) );
	if( p->fm == NULL ){
        PmdObjFree(p);
        return -3;
	}

	for(i=0; i< p->materialCount; i++){
		dxRead(p->in, &p->mat[i], sizeof(TMaterial), 1, res);
		/* dxPrintf("%s\n", p->mat[i].fileName); */
	}
	dxCloseInFile(p->in);

	return 0;
}


int ObjWriteMaterial(TPMDObj* p){
    /*char fileName[BUF_LEN + 1];*/
    unsigned long i, j;

    p->out = dxOpenWrite(p->mtlFile);
	if( ! dxOutFileCheck(p->out) ){
		return -5;
	}
	if( p->format == FORMAT_MQO ){
        dxFprintf(p->out, "Metasequoia Document\nFormat Text Ver 1.0\nMaterial %lu {\n", p->materialCount);
        PmdFloatFormat(p, "\t\"m%lu\" col(%f %f %f %f) dif(%f) amb(%f) emi(%f) spc(%f) power(%f)", p->buf);
    }
    p->matErrCnt = 0;
	for(i=0; i< p->materialCount; i++){
		dxStrCpy(p->fm[i].fileName, p->mat[i].fileName);
		p->fm[i].errId = 0;
		j=0;
		while( (p->fm[i].fileName[j] != 0) && (p->fm[i].fileName[j] != '*' ) ) j++;
		if( p->fm[i].fileName[j] == '*' ){
            p->fm[i].errId = 1;
            p->matErrCnt++;
		}
		p->fm[i].fileName[j] = 0;

        if( p->format == FORMAT_MQO ){
            dxFprintf(p->out, p->buf,
                i, p->mat[i].difR, p->mat[i].difG, p->mat[i].difB, p->mat[i].difA,
                1.0, /* dif */
                (p->mat[i].ambR + p->mat[i].ambG + p->mat[i].difB)/3.0, /*amb */
                0.4, /* emi */
                (p->mat[i].specR + p->mat[i].specG + p->mat[i].specB)/3.0,
                p->mat[i].specularity
            );
            if( dxStrLen(p->fm[i].fileName) > 0 ) dxFprintf(p->out, " tex(\"%s\")", p->fm[i].fileName);
            dxFprintf(p->out, "\n");
        } else {
            dxFprintf(p->out, "newmtl m%lu\n", i);
            if( dxStrLen(p->fm[i].fileName) > 0 ) dxFprintf(p->out, "map_Kd %s\n", p->fm[i].fileName);
            dxFprintf(p->out, PmdFloatFormat(p, "Ka %f %f %f\n", p->buf), p->mat[i].ambR, p->mat[i].ambG, p->mat[i].ambB);
            dxFprintf(p->out, PmdFloatFormat(p, "Kd %f %f %f\n", p->buf), p->mat[i].difR, p->mat[i].difG, p->mat[i].difB);
            dxFprintf(p->out, PmdFloatFormat(p, "Ks %f %f %f\n", p->buf), p->mat[i].specR, p->mat[i].specG, p->mat[i].specB);
            dxFprintf(p->out, PmdFloatFormat(p, "Ns %f\n", p->buf), p->mat[i].specularity);
            dxFprintf(p->out, PmdFloatFormat(p, "d %f\n\n", p->buf), p->mat[i].difA);
        }
	}
	if( p->format == FORMAT_MQO ) dxFprintf(p->out, "}");
	if( p->format == FORMAT_OBJ ) dxCloseOutFile(p->out);
	if( p->matErrCnt > 0 ) return 1;
	return 0;
}


int ObjWriteVertex(TPMDObj* p){
    unsigned long i;

    if( p->format == FORMAT_OBJ ){
        p->out = dxOpenWrite(p->outFile);
        if( ! dxOutFileCheck(p->out) ){
            return -5;
        }

        dxFprintf(p->out, "mtllib %s\ng Object\n", p->mtlBase);
        PmdFloatFormat(p, "v %f %f %f\n", p->buf);
        for(i=0; i< p->vertexCount; i++)
            dxFprintf(p->out, p->buf,
                p->v[i].vx,
                p->v[i].vy,
                p->v[i].vz);

        PmdFloatFormat(p, "vt %f %f\n", p->buf);
        for(i=0; i< p->vertexCount; i++)
            dxFprintf(p->out, p->buf, p->v[i].u, p->v[i].v);
    } else {
        dxFprintf(p->out, "\nObject \"%s\" {\n\tvertex %lu {\n", p->name, p->vertexCount);
        PmdFloatFormat(p, "\t\t%f %f %f\n", p->buf);
        for(i=0; i< p->vertexCount; i++)
            dxFprintf(p->out, p->buf,
                p->v[i].vx,
                p->v[i].vy,
                - p->v[i].vz);
        dxFprintf(p->out, "\t}\n");
    }


    return 0;
}

int ObjWriteFaces(TPMDObj* p){
    unsigned long i;
    unsigned long curCnt = 0;
    unsigned long curInd = 0;
    dxUShort2 x, y, z;

    if( p->format == FORMAT_OBJ ) dxFprintf(p->out, "usemtl m0\n");
        else dxFprintf(p->out, "\tface %ld {\n", p->indexCount);
    PmdFloatFormat(p, "\t\t3 V(%d %d %d) M(%lu) UV(%f %f %f %f %f %f)\n", p->buf);
	for(i=0; i< p->indexCount; i++){
        if( curCnt >= (p->mat[curInd].indecesNumber/3) ){
            curCnt = 0;
            curInd += 1;
            if( p->format == FORMAT_OBJ ) dxFprintf(p->out, "usemtl m%lu\n", curInd);
        }
        x = p->ind[i].face[0];
        y = p->ind[i].face[1];
        z = p->ind[i].face[2];
		if( p->format == FORMAT_OBJ )
            dxFprintf(p->out, "f %d/%d %d/%d %d/%d\n",
            x+1, x+1, y+1, y+1, z+1, z+1);

        if( p->format == FORMAT_MQO )
            dxFprintf(p->out, p->buf,
                x, y, z, curInd,
                p->v[x].u, p->v[x].v,
                p->v[y].u, p->v[y].v,
                p->v[z].u, p->v[z].v
            );
        curCnt++;
    }
    if( p->format == FORMAT_MQO )
        dxFprintf(p->out, "\t}\n}\nEof");

	dxCloseOutFile(p->out);
	return 0;
}


int Pmd2ObjBin(TPMDObj* p, const char* fileName){
    int res;
    res = PmdReadFile(p, fileName);
    if( res < 0 ) return res;

	res = ObjWriteMaterial(p);
	if( res < 0 ) return res;

    res = ObjWriteVertex(p);
    if( res < 0 ) return res;

    res = ObjWriteFaces(p);
    if( res < 0 ) return res;

	PmdObjFree(p);
	return 0;
}


int Pmd2Obj(const char* fileName, int format){
    TPMDObj p;
    PmdObjInit(&p, format);
    return Pmd2ObjBin(&p, fileName);
}
