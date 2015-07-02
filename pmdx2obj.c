#include "pmdx2obj.h"

int PmdObjFree(TPMDObj* p){
	if( p->v != NULL ) dxMemFree(p->v);
	if( p->ind != NULL ) dxMemFree(p->ind);
	if( p->mat != NULL ) dxMemFree(p->mat);
	if( p->fm != NULL ) dxMemFree(p->fm);
	if( dxInFileCheck(p->in) ) dxCloseInFile(p->in);
	if( dxOutFileCheck(p->out) ) dxCloseOutFile(p->out);
	return 0;
}

int PmdObjInit(TPMDObj* p, int format){
	p->format = format;
	p->precision = 6;
	if( format == FORMAT_MQO ) p->precision = 3;
	p->materialCount = 0;
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


int Read3dFile(TPMDObj* p, const char* fileName){
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
	/*for(i=0; i<dxStrLen(p->outFile)+1; i++)
		p->resultFile[i] = p->outFile[i];*/
	for(i=0; i<=n+4; i++)
		p->mtlFile[i] = p->outFile[i];
	if( p->format == FORMAT_OBJ ){
		i = n+1;
		p->mtlFile[i++] = 'm';
		p->mtlFile[i++] = 't';
		p->mtlFile[i++] = 'l';
		p->mtlFile[i++] = 0;
	}

	for(i=0; i<=n+4; i++)
		p->gzipFile[i] = p->outFile[i];
	{
		i = n+1;
		p->gzipFile[i++] = 't';
		p->gzipFile[i++] = 'g';
		p->gzipFile[i++] = 'z';
		p->gzipFile[i++] = 0;
	}

	n = dxStrLen(p->mtlFile);
	i=n;
	while( (i>0) && (p->mtlFile[i-1]!='/') && (p->mtlFile[i-1]!='\\') ) i--;
	for(j=0; i<=n; i++, j++){
		p->mtlBase[j] = p->mtlFile[i];
		p->resultFile[j] = p->outFile[i];
	}


	/* Actual file reading */
	p->in = dxOpenRead(fileName);
	if( ! dxInFileCheck(p->in) ){
		/* dxPrintf("Can't open file: %s\n", inputFileName); */
		return -1;
	}

	dxRead(p->in, buf, sizeof(char), 3, res);
	if(buf[0]!='P' ) return -2;
	if( (buf[1]!='m') && (buf[1] != 'M') ) return -2;
	if( buf[2] == 'd' ) return PmdReadFile(p);
		else if( buf[2] == 'X' ) return PmExtendedReadFile(p);
	return -2;
}

int PmdReadSkipBytes(TPMDObj* p, unsigned int byteCount){
	dxFileSize res;
	unsigned int i;
	char c;

	for(i=0; i<byteCount; i++)
		dxRead(p->in, &c, sizeof(c), 1, res);
	return byteCount;
}

int PmExtendedReadFile(TPMDObj* p){
	unsigned int i, j;
	dxInt1 weightType;
	dxULong4 tmpCount;
	dxFileSize res;
	TIndex1 ti1;
	TIndex4 ti4;
	dxULong4 TexCount;
	dxULong4 TexLen[300];
	char Tex[300][DX_MAX_PATH];
	char tmpFile[DX_MAX_PATH];
	TPMXMatPart mp;
	dxULong4 textureIndex;
	dxInt1 toonFlag;

	PmdReadSkipBytes(p, 1);
	dxRead(p->in, &p->version, sizeof(p->version), 1, res);
	if( ( p->version < 0 ) || ( 2.0- p->version > 0.0001 ) ) return -4;
	dxRead(p->in, &p->dataCount, sizeof(p->dataCount), 1, res);
	dxRead(p->in, &p->textEncoding, sizeof(p->textEncoding), 1, res);
	/* if( p->textEncoding == 0 ) dxPrintf("utf16\n"); else dxPrintf("utf8\n"); */
	dxRead(p->in, &p->addUVcount, sizeof(p->addUVcount), 1, res);
	dxRead(p->in, &p->indexSize, sizeof(p->indexSize), 1, res);

	p->name[0] = 'd';
	p->name[1] = 'x';
	p->name[2] = 0;

	for(i=0; i<4; i++){
		/* Skip Local/Global name and comment */
		dxRead(p->in, &tmpCount, sizeof(tmpCount), 1, res);
		PmdReadSkipBytes(p, tmpCount);
	}

	dxRead(p->in, &p->vertexCount, sizeof(p->vertexCount), 1, res);
	if( p->vertexCount == 0 ) return -4;
	p->v = (TVertex*) dxMemAlloc(p->vertexCount, sizeof(TVertex) );
	if( p->v == NULL ){
		PmdObjFree(p);
		return -3;
	}
	for(i=0; i< p->vertexCount; i++){
		dxRead(p->in, &p->v[i], sizeof(TPNUV), 1, res);
		PmdReadSkipBytes(p, (p->addUVcount)*sizeof(TaddUV) );
		dxRead(p->in, &weightType, sizeof(weightType), 1, res);
		switch( weightType ){
			case PMX_BDEF1:
				PmdReadSkipBytes(p, p->indexSize.bone);
			break;
			case PMX_BDEF2:
				PmdReadSkipBytes(p, (p->indexSize.bone)*2 + 4);
			break;
			case PMX_BDEF4:
				PmdReadSkipBytes(p, (p->indexSize.bone)*4 + 4*4);
			break;
			case PMX_SDEF:
				PmdReadSkipBytes(p, (p->indexSize.bone)*2 + 4 + 12*3);
			break;
		}
		PmdReadSkipBytes(p, sizeof(dxFloat4) ); /* Edge Scale */
	}


	dxRead(p->in, &p->indexThreeCount, sizeof(p->indexThreeCount), 1, res);
	p->indexCount = p->indexThreeCount / 3;
	p->ind = (TIndex*) dxMemAlloc(p->indexCount, sizeof(TIndex) );
	if( p->ind == NULL ){
		PmdObjFree(p);
		return -3;
	}
	for(i=0; i< p->indexCount; i++){
		switch(p->indexSize.vertex){
			case 1:
				dxRead(p->in, &ti1, sizeof(TIndex1), 1, res);
				for(j=0; j<3; j++)
					p->ind[i].face[j] = ti1.face[j];
			break;
			case 2:
				dxRead(p->in, &p->ind[i], sizeof(TIndex), 1, res);
			break;
			case 4:
				dxRead(p->in, &ti4, sizeof(TIndex4), 1, res);
				for(j=0; j<3; j++)
					p->ind[i].face[j] = (dxUShort2) ti4.face[j];
			break;
		}
	}

	dxRead(p->in, &TexCount, sizeof(dxULong4), 1, res);
	for(j=0; j<DX_MAX_PATH; j++)
		tmpFile[j] = 0;
	for(i=0; i<TexCount; i++){
		dxRead(p->in, &TexLen[i], sizeof(dxULong4), 1, res);
		dxRead(p->in, &tmpFile, sizeof(char), TexLen[i], res);
		dxWideCharToAscii(Tex[i], (wchar_t*)tmpFile, DX_MAX_PATH);

		/* dxPrintf("Tex[%d](%lu) = '%s'", i, TexLen[i], Tex[i]); */
		for(j=0; j<TexLen[i]; j++) tmpFile[j] = 0;
	}

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
	p->matErrCnt = 0;

	for(i=0; i< p->materialCount; i++){
		/* Skip material Local name */
		dxRead(p->in, &tmpCount, sizeof(tmpCount), 1, res);
		PmdReadSkipBytes(p, tmpCount);

		/* Skip material Global name */
		dxRead(p->in, &tmpCount, sizeof(tmpCount), 1, res);
		PmdReadSkipBytes(p, tmpCount);

		dxRead(p->in, &mp, sizeof(mp), 1, res);
		p->mat[i].difR = mp.difR;
		p->mat[i].difG = mp.difG;
		p->mat[i].difB = mp.difB;
		p->mat[i].difA = mp.difA;
		p->mat[i].specularity = mp.specularity;
		p->mat[i].specR = mp.specR;
		p->mat[i].specG = mp.specG;
		p->mat[i].specB = mp.specB;
		p->mat[i].ambR = mp.ambR;
		p->mat[i].ambG = mp.ambG;
		p->mat[i].ambB = mp.ambB;
		p->mat[i].edgeFlag = ( mp.drawingMode & 8 );

		textureIndex = 0;
		dxRead(p->in, &textureIndex, p->indexSize.texture, 1, res);
		dxStrCpy(p->fm[i].fileName, Tex[textureIndex]);
		/* dxPrintf("[%d] = '%s' = '%s'\n", i, p->fm[i].fileName, Tex[textureIndex]); */
		/* p->fm[i].fileName[ dxStrLen(Tex[textureIndex]) ] = 0; */
		if( dxStrLen(Tex[textureIndex]) < NAME_LEN ) dxStrCpy(p->fm[i].fileName, Tex[textureIndex]);

		PmdReadSkipBytes(p, p->indexSize.texture + 1); /* Skip Environment */

		dxRead(p->in, &toonFlag, sizeof(toonFlag), 1, res);
		if( toonFlag ){
			/* Inbuilt */
			dxRead(p->in, &p->mat[i].toonNumb, sizeof(p->mat[i].toonNumb), 1, res);
		} else {
			/* Texture index */
			dxRead(p->in, &tmpCount, p->indexSize.texture, 1, res);
			p->mat[i].toonNumb = (dxInt1)tmpCount;
		}

		/* Skip material Memo */
		dxRead(p->in, &tmpCount, sizeof(tmpCount), 1, res);
		PmdReadSkipBytes(p, tmpCount);

		dxRead(p->in, &p->mat[i].indecesNumber, sizeof(p->mat[i].indecesNumber), 1, res);
	}
	dxCloseInFile(p->in);

	return 0;
}


int PmdReadFile(TPMDObj* p){
	unsigned long i, j;
	dxFileSize res;

	dxRead(p->in, &p->version, sizeof(p->version), 1, res);
	if( ( p->version < 0 ) || ( 1.0- p->version > 0.0001 ) ) return -4;
	dxRead(p->in, p->name, sizeof(char), NAME_LEN, res);
	dxRead(p->in, p->comment, sizeof(char), COMMENT_LEN, res);
	if( res != COMMENT_LEN ) return -4;

	dxRead(p->in, &p->vertexCount, sizeof(p->vertexCount), 1, res);
	if( p->vertexCount == 0 ) return -4;
	p->v = (TVertex*) dxMemAlloc(p->vertexCount, sizeof(TVertex) );
	if( p->v == NULL ){
		PmdObjFree(p);
		return -3;
	}

	for(i=0; i< p->vertexCount; i++){
		dxRead(p->in, &p->v[i], sizeof(TVertex), 1, res);
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

	p->matErrCnt = 0;
	for(i=0; i< p->materialCount; i++){
		dxRead(p->in, &p->mat[i], sizeof(TMaterial), 1, res);

		dxStrCpy(p->fm[i].fileName, p->mat[i].fileName);
		p->fm[i].errId = 0;
		j=0;
		while( (p->fm[i].fileName[j] != 0) && (p->fm[i].fileName[j] != '*' ) ) j++;
		if( p->fm[i].fileName[j] == '*' ){
			p->fm[i].errId = 1;
			p->matErrCnt++;
		}
		p->fm[i].fileName[j] = 0;

		if( p->mat[i].difA >= 0.98 ) p->mat[i].difA = 1.0;
	}
	dxCloseInFile(p->in);

	if( p->matErrCnt > 0 ) return 1;
	return 0;
}


int ObjWriteMaterial(TPMDObj* p){
	unsigned long i;

	p->out = dxOpenWriteBin(p->mtlFile);
	if( ! dxOutFileCheck(p->out) ){
		return -5;
	}
	if( p->format == FORMAT_MQO ){
		dxFprintf(p->out, "Metasequoia Document\nFormat Text Ver 1.0\nMaterial %"dxPRIu32" {\n", p->materialCount);
		PmdFloatFormat(p, "\t\"m%lu\" col(%f %f %f %f) dif(%f) amb(%f) emi(%f) spc(%f) power(%f)", p->buf);
	}
	for(i=0; i< p->materialCount; i++){

		if( p->format == FORMAT_MQO ){
			dxFprintf(p->out, p->buf,
				i, p->mat[i].difR, p->mat[i].difG, p->mat[i].difB, p->mat[i].difA,
				1.0, /* dif */
				(p->mat[i].ambR + p->mat[i].ambG + p->mat[i].ambB)/3.0, /*amb */
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
			dxFprintf(p->out, "#Toon %d; EdgeFlag %d\n", p->mat[i].toonNumb, p->mat[i].edgeFlag);
			dxFprintf(p->out, PmdFloatFormat(p, "d %f\n\n", p->buf), p->mat[i].difA);
		}
	}
	if( p->format == FORMAT_MQO ) dxFprintf(p->out, "}");
	if( p->format == FORMAT_OBJ ) dxCloseOutFile(p->out);
	return 0;
}

int ObjWriteVertex(TPMDObj* p){
	unsigned long i;

	if( p->format == FORMAT_OBJ ){
		p->out = dxOpenWriteBin(p->outFile);
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
			dxFprintf(p->out, p->buf, p->v[i].u, - p->v[i].v);

		/*PmdFloatFormat(p, "vn %f %f %f\n", p->buf);
		for(i=0; i< p->vertexCount; i++)
			dxFprintf(p->out, p->buf,
				p->v[i].nx,
				p->v[i].ny,
				p->v[i].nz);*/
	} else {
		dxFprintf(p->out, "\nObject \"%s\" {\n\tvertex %"dxPRIu32" {\n", p->name, p->vertexCount);
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
		else dxFprintf(p->out, "\tface %"dxPRIu32" {\n", p->indexCount);
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


int Pmd2Gzip(TPMDObj* p){
	#define GZIP_DEBUG 0
	unsigned j, k, ti;
	char tmpPath[DX_MAX_PATH];
	int alreadyDone, res;
	dxFileSize curSize, tarSize;
	char* tar = NULL;
	#if GZIP_DEBUG
	unsigned curCount = 0;
	dxPrintf("Archive content: \n");
	#endif

	tarSize = 0;
	curSize = 0;
	for(ti=0; ti<2; ti++){
		if( ti == 1 ) tar = (char*)dxBigAlloc(tarSize, 1);

		ZeroFill(tmpPath, DX_MAX_PATH);
		dxSprintf(tmpPath, "%s\\%s", p->outBase, p->resultFile);
		if( ti == 0 ) tarSize += tarGetSize( tmpPath );
				else curSize += tarAppend(tar + curSize, tmpPath, p->resultFile);

		if( p->format == FORMAT_OBJ ){
			ZeroFill(tmpPath, DX_MAX_PATH);
			dxSprintf(tmpPath, "%s\\%s", p->outBase, p->mtlBase);
			if( ti == 0 ) tarSize += tarGetSize( tmpPath );
					else curSize += tarAppend(tar + curSize, tmpPath, p->mtlBase);
		}
		/* Add textures */
		for(j=0; j< p->materialCount; j++){
			ZeroFill(tmpPath, DX_MAX_PATH);
			dxSprintf(tmpPath, "%s\\%s", p->outBase, p->fm[j].fileName);
			alreadyDone = 0;
			for(k=0; k<j; k++)
				if( dxStrCmp(p->fm[j].fileName, p->fm[k].fileName) == 0 )
					alreadyDone = 1;

			if( !alreadyDone && dxFileExists(tmpPath) && (dxStrLen(p->fm[j].fileName)>0) ){
				#if GZIP_DEBUG
				if( ti == 1 ) dxPrintf("%d. %s (%d / %d)\n", curCount+1, p->fm[j].fileName,
							dxGetFileSize(tmpPath), tarGetSize(tmpPath) );
				curCount++;
				#endif
				if( ti == 0 ) tarSize += tarGetSize( tmpPath );
					else curSize += tarAppend(tar + curSize, tmpPath, p->fm[j].fileName);
			}
		}
	}
	res = gzipToFile(tar, curSize, p->gzipFile);
	dxBigFree(tar);
	return res;
	#undef GZIP_DEBUG
}


/*int Pmd2ObjBin(TPMDObj* p, const char* fileName){
	int res;
	res = Read3dFile(p, fileName);
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


int Pmd2Obj(const char* fileName, int format, int precision){
	TPMDObj p;
	PmdObjInit(&p, format);
	if( (precision > 0) && (precision < 8) ) p.precision = precision;
	return Pmd2ObjBin(&p, fileName);
}*/

