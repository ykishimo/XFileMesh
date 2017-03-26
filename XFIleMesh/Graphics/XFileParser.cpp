//
//	CXFileParser
//
//	Desc: Read XFile and build data structure on memory
//
//	First edition: September 11, 2016
//	Author: Y.Kishimoto
//
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <list>
#include <vector>
#include <D3D11.h>
#include <DirectXMath.h>
#include "XFileParser.h"
#include "Mesh.h"
#include "Animation.h"
#include "binaryx.h"
#include "FilenameDecoder.h"

//#define _COMPRESSION_API_	//	use compression api

#ifdef _COMPRESSION_API_
#include <Compressapi.h>
#pragma comment(lib,"Cabinet.lib")
#include "rfc1951.h"
#else
#include "rfc1951.h"
#endif

using namespace DirectX;

#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }

//
//	Temporal Mesh
//
struct TempMesh{
	DirectX::XMFLOAT3	*pPositions;
	DirectX::XMFLOAT3	*pNormals;
	DirectX::XMFLOAT2   *pTexCoords;
	INT			*pTempIndices;
	INT			numVertices;
	INT			numOriginalFaces;	//	number of original faces
	INT			numIndices;			//  number of triangulated face indices
	INT			*pFaceReference;	//  reference from original face id to triangulated face index id
	INT			*pMaterialList;
	INT			numMaterialList;
	SkinWeights	**ppSkinWeights;
	INT			numSkinWeights;
	INT			numMaxSkinWeightsPerVertex;
	INT			numMaxSkinWeightsPerFace;
	INT			numBones;
	MeshContainer *pParent;
	TempMesh();
	~TempMesh();
};

//  skin weights
struct SkinWeights{
	CHAR *pBoneName;
	INT iFrameId;	//  Control source frame id
	INT	numWeights;
	INT *pIndices;
	FLOAT *pWeights;
	DirectX::XMMATRIX matOffset;
	SkinWeights();
	~SkinWeights();

	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	inline void *operator new[](size_t size){
		return _mm_malloc(size,16);
	}
	inline void operator delete(void *p){
		return _mm_free(p);
	}
};



struct TempAnimationSet{
	CHAR *pName;
	std::list<Animation*>	*pAnimations;
	TempAnimationSet(){
		this->pName = NULL;
		pAnimations = new std::list<Animation*>();
	}
	~TempAnimationSet(){
		SAFE_DELETE(pName);
		if (pAnimations != NULL){
			std::list<Animation*>::iterator it = pAnimations->begin();
			while(it != pAnimations->end()){
				SAFE_DELETE(*it);
				it = pAnimations->erase(it);
			}
			delete pAnimations;
			pAnimations = NULL;
		}
	}
};

//
//	Local data to read XFile
//
struct TempAnimationSet;
struct FileReadingContext{
	std::list<MeshFrame*>			*pListFramesLoaded;
	std::list<TempAnimationSet*>	*pTempAnimationSetsLoaded;
	std::list<TempMesh*>			*pListTempMeshLoaded;

	FileReadingContext(){
		pListFramesLoaded = new std::list<MeshFrame*>();
		pTempAnimationSetsLoaded = new std::list<TempAnimationSet*>();
		pListTempMeshLoaded = new std::list<TempMesh*>();
	}
	~FileReadingContext(){
		SAFE_DELETE(pListFramesLoaded);	//	中身は消さない
		if (pTempAnimationSetsLoaded != NULL){
			std::list<TempAnimationSet*>::iterator it = pTempAnimationSetsLoaded->begin();
			while (it != pTempAnimationSetsLoaded->end()){
				SAFE_DELETE(*it);
				it = pTempAnimationSetsLoaded->erase(it);
			}
			delete pTempAnimationSetsLoaded;
			pTempAnimationSetsLoaded = NULL;
		}
		if (pListTempMeshLoaded != NULL){
			std::list<TempMesh*>::iterator it = pListTempMeshLoaded->begin();
			while(it != pListTempMeshLoaded->end()){
				SAFE_DELETE(*it);
				it = pListTempMeshLoaded->erase(it);
			}
			delete pListTempMeshLoaded;
			pListTempMeshLoaded = NULL;
		}
	}
};

SkinWeights::SkinWeights(){
	pBoneName = NULL;
	iFrameId = 0;
	numWeights = 0;
	pIndices = NULL;
	pWeights = NULL;
}
SkinWeights::~SkinWeights(){
	SAFE_DELETE(pBoneName);
	SAFE_DELETE(pIndices);
	SAFE_DELETE(pWeights);
}

//
//	Local data to read SkinInfo
//
struct MeshLoaderContext{
	std::list<Material*>    *pMaterials;
	std::list<SkinWeights*>	*pWeights;
	MeshContainer *pParent;

	MeshLoaderContext(){
		pWeights = new std::list<SkinWeights*>();
		pMaterials = new std::list<Material*>();
		pParent = NULL;
	}
	~MeshLoaderContext(){
		if (pWeights != NULL){
			std::list<SkinWeights*>::iterator it = pWeights->begin();
			while(it != pWeights->end()){
				SAFE_DELETE(*it);
				it = pWeights->erase(it);
			}
			delete pWeights;
			pWeights = NULL;
		}
		SAFE_DELETE(pMaterials);
	}
};


typedef enum _arraytype{
	None = 0,
	Integer = 1,
	Float = 2
} ArrayType;

//
//	ctor
//
CXFileParser::CXFileParser(void)
{
	m_iTemporarySize = NULL;
	m_pFp = NULL;
	m_bBinaryMode = false;
	m_pBuffer = NULL;
	m_iDepth  = 0;
	m_dwSize = 0;
	m_pCurrent = NULL;
	m_pNextPosition = NULL;
	m_iTemporarySize = 32;
	m_pTemporary = new CHAR[m_iTemporarySize];
	m_pFileReadingContext = new FileReadingContext;
	m_pFrameRoot = NULL;
	m_ppFlattenFramePointers = NULL;
	m_ppAnimationSets = NULL;
	m_pFilePath = NULL;
	m_bDouble = false;
	m_bArray = false;
	m_iArrayCount = 0;
	m_iArrayType = ArrayType::None;
	m_iNumFrames = 0;
}


CXFileParser::~CXFileParser(void)
{
	SAFE_DELETE(m_ppFlattenFramePointers);
	SAFE_DELETE(m_pFileReadingContext);
	SAFE_DELETE(m_pFilePath);
	SAFE_DELETE(m_pBuffer);
	if (m_ppAnimationSets){
		for (int i = 0; i < m_iNumAnimationSets ; ++i){
			SAFE_DELETE(m_ppAnimationSets[i]);
		}
		delete[] m_ppAnimationSets;
		m_ppAnimationSets = NULL;
	}
	SAFE_DELETE(m_pTemporary);
}	

HRESULT CXFileParser::OpenRead(TCHAR *pFilename){
	struct _stat tmpStat;
	if (-1 == _tstat(pFilename,&tmpStat))
		return E_FAIL;	//  file not found;
	
	SAFE_DELETE(m_pBuffer);
	int size = (tmpStat.st_size+1+3)&0xfffffffc;
	m_bBinaryMode = false;
	m_pBuffer = (BYTE*)new DWORD[size >> 2];	//  
	FILE *fp;
	HRESULT hr = ERROR_FILE_INVALID;
	if (0 == _tfopen_s(&fp,pFilename,_T("rb"))){
		hr = E_OUTOFMEMORY;
		if (m_pBuffer != NULL){
			hr = ERROR_READ_FAULT;
			int sizRead = (INT)fread((void*)m_pBuffer,1,tmpStat.st_size,fp);
			if (sizRead == tmpStat.st_size){
				m_dwSize = sizRead;
				for (int i = tmpStat.st_size ; i < size ; ++i){
					m_pBuffer[i] = 0;
				}
				m_bOpenRead = true;
				SetPos(0);
				hr = S_OK;
			}
		}
	}
	fclose(fp);
	return	hr;
}

void	CXFileParser::Close(){
	if (m_pFp != NULL){
		fclose(m_pFp);
		m_pFp = NULL;
	}
	if (m_bOpenRead){
		SAFE_DELETE(m_pBuffer);
		m_pCurrent = NULL;
		SAFE_DELETE(m_pTemporary);
		m_iTemporarySize = 32;
		m_pTemporary = new CHAR[32];
		m_iDepth = 0;
		m_bOpenRead = false;
	}
}

//
//
//
static CHAR *pOBrace   = "{";	//	braces
static CHAR *pCBrace   = "}";	//
static CHAR *pOParen   = "(";	//  parentheses
static CHAR *pCParen   = ")";	//
static CHAR *pOBracket = "[";	//  brackets
static CHAR *pCBracket = "]";	//
static CHAR *pOAngle   = "<";		//	angle brackets
static CHAR *pCAngle   = ">";		//
static CHAR *pDot      = ".";
static CHAR *pComma    = ".";
static CHAR *pSemicolon = ";";
static CHAR *pTemplate = "Template";
static CHAR *pVoid    = "";
static CHAR *pWord    = "WORD";
static CHAR *pDWord   = "DWORD";
static CHAR *pFloat   = "FLOAT";
static CHAR *pDouble  = "DOUBLE";
static CHAR *pChar    = "CHAR";
static CHAR *pUChar   = "UCHAR";
static CHAR *pSWord   = "SWORD";
static CHAR *pSDWord  = "SDWORD";
static CHAR *pLPStr   = "LPSTR";
static CHAR *pUnicode = "UNICODE";
static CHAR *pCString = "CSTRING";
static CHAR *pArray   = "ARRAY";

CHAR *CXFileParser::ReturnFloat(FLOAT a){
	if (this->m_iTemporarySize < 33){
		this->m_iTemporarySize = 33;
		SAFE_DELETE(m_pTemporary);
		m_pTemporary = new CHAR[m_iTemporarySize];
	}
	sprintf_s(m_pTemporary,m_iTemporarySize,"%f",a);
	return m_pTemporary;
}

CHAR *CXFileParser::ReturnInt(INT a){
	if (this->m_iTemporarySize < 33){
		this->m_iTemporarySize = 33;
		SAFE_DELETE(m_pTemporary);
		m_pTemporary = new CHAR[m_iTemporarySize];
	}
	sprintf_s(m_pTemporary,m_iTemporarySize,"%i",a);
	return m_pTemporary;
}

CHAR *CXFileParser::DecodeArrayToText(){
	CHAR *pReturn = NULL;
	switch(m_iArrayType){
	case	ArrayType::Integer:
			pReturn = ReturnInt((INT)*(LONG*)m_pCurrent);
			m_pCurrent += sizeof(LONG);
		break;
	case	ArrayType::Float:
		if (m_bDouble){
			pReturn = ReturnFloat((FLOAT)*(DOUBLE*)m_pCurrent);
			m_pCurrent += sizeof(DOUBLE);
		}else{
			pReturn = ReturnFloat(*(FLOAT*)m_pCurrent);
			m_pCurrent += sizeof(FLOAT);
		}
		break;
	}
	if (0 >= --m_iArrayCount){
		m_bArray = false;
	}
	return pReturn;
}

CHAR *CXFileParser::CheckNextToken(){
	BOOL bArray      = m_bArray;
	INT  iArrayType  = m_iArrayType;
	INT  iArrayCount = m_iArrayCount;
	CHAR *pBackup    = m_pCurrent;
	CHAR *pReturn = GetToken();
	m_pNextPosition = m_pCurrent;
	m_pCurrent = pBackup;
	m_bArray = bArray;
	m_iArrayType = iArrayType;
	m_iArrayCount = iArrayCount;
	return pReturn;
}

void CXFileParser::SkipToken(){
	if (m_pNextPosition == NULL || m_bArray)
		GetToken();
	else{
		m_pCurrent = m_pNextPosition;
	}
}

CHAR *CXFileParser::GetToken(){
	if (this->m_bBinaryMode){
		return GetTokenBin();
	}
	return GetTokenText();
}
CHAR *CXFileParser::GetTokenText(){
	CHAR *p = m_pCurrent;
	CHAR *top, *tale;
	CHAR *end = (CHAR*)(m_pBuffer + m_dwSize);
	UINT sizToken, sizBuffer;
	CHAR c;
	m_pNextPosition = NULL;
	while(*p <= ' ' && p<end){
		p++;
	}
	if (p >= end)
		return NULL;

	top = p;

	//
	if (*p == '}'){
		tale = top+1;
		m_iDepth--;
		goto GETTOKEN_DONE;
	}
	if (*p == '{'){
		tale = top+1;
		m_iDepth++;
		goto GETTOKEN_DONE;
	}
	c = *p++;
	if (c == '\"'){  // Quoted string
		while(p < end){
			if (*p == '\\'){
				++p;
			}else if (*p == '\"'){
				tale = ++p;
				goto GETTOKEN_DONE;
			}
			++p;
		}
		return NULL;
	}

	if (c == ';'||c==','||c=='['||c==']'||c=='<'||c=='>'){
		tale = top+1;
		goto GETTOKEN_DONE;
	}
	do{
		c = *p;
		if (c == ';'||c==','||c == '{'||c=='}'||c=='['||c==']'||c=='}'||c=='<'||c=='>'|| c<=' '){
			tale = p;
			goto GETTOKEN_DONE;
		}
		p++;
	}while(p<end);

	tale = end;

GETTOKEN_DONE:
	sizToken  = (UINT)((BYTE*)tale- (BYTE*)top);
	if (*top == '\"' && *(tale-1) == '\"'){
		if (sizToken < 2)
			return NULL;
		++top;
		sizToken -= 2;
	}else{
		sizToken  = (UINT)((BYTE*)tale-(BYTE*)top);
	}
	sizBuffer = (sizToken+4)&0xfffffffcL;
	if ((INT)sizBuffer > m_iTemporarySize){
		m_iTemporarySize = (sizBuffer + 31)&0xffffffe0L;
		SAFE_DELETE(m_pTemporary);
		m_pTemporary = new CHAR[m_iTemporarySize];
	}
	for (unsigned int i = 0; i < sizToken ; ++i){
		m_pTemporary[i] = *top++;
	}
	m_pTemporary[sizToken] = '\0';
	m_pCurrent = tale;
	return m_pTemporary;
}

CHAR *CXFileParser::GetTokenBin(){
	CHAR *pReturn = NULL;
	m_pNextPosition = NULL;
	if (m_bArray){
		return DecodeArrayToText();
	}
	WORD type = *(WORD*)m_pCurrent;
	INT  len;
	m_pCurrent += sizeof(WORD);
	switch(type){
	//  record start
	case	TOKEN_NAME:
	case	TOKEN_STRING:
		len = (INT)*(DWORD*)m_pCurrent;
		m_pCurrent += sizeof(DWORD);
		if (m_iTemporarySize < (len+1)){
			m_iTemporarySize = len+1;
			SAFE_DELETE(m_pTemporary);
			m_pTemporary = new CHAR[m_iTemporarySize];
		}
		for (int i = 0; i < len ; ++i){
			m_pTemporary[i] = *m_pCurrent++;
		}
		m_pTemporary[len] = '\0';
		pReturn = m_pTemporary;
		break;
	case	TOKEN_INTEGER:
		{
			INT	iTmp = (INT)*(LONG*)m_pCurrent;
			m_pCurrent += sizeof(LONG);
			pReturn = ReturnInt((INT)iTmp);
		}
		break;
	case	TOKEN_GUID:
		if (m_iTemporarySize < 42){
			SAFE_DELETE(m_pTemporary);
			m_iTemporarySize = 42;
			m_pTemporary = new CHAR[m_iTemporarySize];
		}
		{
			DWORD	data1;
			WORD	data2, data3;
			BYTE	data4[8];
			data1 = *(DWORD*)m_pCurrent;
			m_pCurrent += sizeof(DWORD);
			data2 = *(WORD*)m_pCurrent;
			m_pCurrent += sizeof(WORD);
			data3 = *(WORD*)m_pCurrent;
			m_pCurrent += sizeof(WORD);
			for (int i = 0; i < 8 ; ++i){
				data4[i] = *(BYTE*)m_pCurrent++;
			}
			sprintf_s(m_pTemporary,m_iTemporarySize,"<%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x>",
				data1,data2,data3,data4[0],data4[1],data4[2],data4[3],data4[4],data4[5],data4[6],data4[7]);
			pReturn = m_pTemporary;
		}
		break;
	case	TOKEN_INTEGER_LIST:
		{
			m_iArrayCount = *(LONG*)m_pCurrent;
			m_pCurrent += sizeof(LONG);
			m_bArray = true;
			m_iArrayType = ArrayType::Integer;
			pReturn = DecodeArrayToText();
		}
		break;
	case	TOKEN_FLOAT_LIST:
		{
			m_iArrayCount = *(LONG*)m_pCurrent;
			m_pCurrent += sizeof(LONG);
			m_bArray = true;
			m_iArrayType = ArrayType::Float;
			pReturn = DecodeArrayToText();
		}
		break;
	//  stand alones
	case	TOKEN_OBRACE:
		m_iDepth++;
		return (CHAR*)pOBrace;
	case	TOKEN_CBRACE:
		m_iDepth--;
		return (CHAR*)pCBrace;
	case	TOKEN_OPAREN:
		return	pOParen;
	case	TOKEN_CPAREN:
		return	pCParen;
	case	TOKEN_OBRACKET:
		return	pOBracket;
	case	TOKEN_CBRACKET:
		return	pCBracket;
	case	TOKEN_OANGLE:
		return	pOAngle;
	case	TOKEN_CANGLE:
		return	pCAngle;
	case	TOKEN_DOT:
		return	pDot;
	case	TOKEN_COMMA:
		return	pComma;
	case	TOKEN_SEMICOLON:
		return	pSemicolon;
	case	TOKEN_TEMPLATE:
		return	pTemplate;
	case	TOKEN_WORD:
		return  pWord;
	case	TOKEN_DWORD:
		return  pDWord;
	case	TOKEN_FLOAT:
		return  pFloat;
	case	TOKEN_DOUBLE:
		return  pDouble;
	case	TOKEN_CHAR:
		return  pChar;
	case	TOKEN_UCHAR:
		return  pUChar;
	case	TOKEN_SWORD:
		return  pSWord;
	case	TOKEN_SDWORD:
		return  pSDWord;
	case	TOKEN_VOID:
		return pVoid;
	case	TOKEN_LPSTR:
		return  pLPStr;
	case	TOKEN_UNICODE:
		return  pUnicode;
	case	TOKEN_CSTRING:
		return  pCString;
	case	TOKEN_ARRAY:
		return  pArray;
	}
	return pReturn;
}

BOOL CXFileParser::GetNumericToken(CHAR **ppTextResult, FLOAT *pFloatResult, INT *pIntResult){
	if (m_bBinaryMode){
		return GetNumericTokenBin(ppTextResult,pFloatResult,pIntResult);
	}else{
		return GetNumericTokenText(ppTextResult,pFloatResult,pIntResult);
	}
}

BOOL CXFileParser::GetNumericTokenText(CHAR **ppTextResult, FLOAT *pFloatResult, INT *pIntResult){
	CHAR *pTextResult = GetToken();
	CHAR top = pTextResult[0];
	if (ppTextResult != NULL){
		*ppTextResult = pTextResult;
	}
	if (pTextResult == NULL)
		goto FAILED_EXIT;
	if (top == '-' || top == '+'){
		top = pTextResult[1];
	}
	if ((top >= '0' && top <= '9')|| top == '.'){
		if (pFloatResult != NULL){
			*pFloatResult = (FLOAT)atof(pTextResult);
		}
		if (pIntResult != NULL){
			*pIntResult = (INT)atoi(pTextResult);
		}
		return true;
	}
FAILED_EXIT:
	if (pFloatResult != NULL){
		*pFloatResult = 0.0f;
	}
	if (pIntResult != NULL){
		*pIntResult = 0;
	}
	return false;
}

BOOL CXFileParser::GetNumericTokenBin(CHAR **ppTextResult, FLOAT *pFloatResult, INT *pIntResult){

	FLOAT	fTmp = 0;
	INT		iTmp = 0;
	if (m_bArray){
		BOOL	bFloat = false;
		switch(m_iArrayType){
		case	ArrayType::Integer:
			iTmp = (INT)*(LONG*)m_pCurrent;
			m_pCurrent += sizeof(LONG);
			fTmp = (FLOAT)iTmp;
			break;
		case	ArrayType::Float:
			if (m_bDouble){
				fTmp = (FLOAT)*(DOUBLE*)m_pCurrent;
				m_pCurrent += sizeof(DOUBLE);
			}else{
				fTmp = *(FLOAT*)m_pCurrent;
				m_pCurrent += sizeof(FLOAT);
			}
			iTmp = (INT)fTmp;
			bFloat = true;
			break;
		}
		if (0 >= --m_iArrayCount){
			m_bArray = false;
		}
		if (pFloatResult != NULL)
			*pFloatResult = fTmp;
		if (pIntResult != NULL)
			*pIntResult = iTmp;
		if (ppTextResult != NULL){
			if (bFloat){
				*ppTextResult = ReturnFloat(fTmp);
			}else{
				*ppTextResult = ReturnInt(iTmp);
			}
		}
		return true;
	}else{
		WORD type = *(WORD*)m_pCurrent;
		switch(type){
		case	TOKEN_INTEGER:
			m_pCurrent += sizeof(WORD);
			iTmp = (LONG)*(LONG*)m_pCurrent;
			m_pCurrent += sizeof(LONG);
			fTmp = (FLOAT)iTmp;
			if (ppTextResult != NULL)
				*ppTextResult = ReturnInt(iTmp);
			if (pFloatResult != NULL)
				*pFloatResult = fTmp;
			if (pIntResult != NULL)
				*pIntResult = iTmp;
			return true;
		case	TOKEN_INTEGER_LIST:
			m_pCurrent   += sizeof(WORD);
			m_bArray      = true;
			m_iArrayType  = ArrayType::Integer;
			m_iArrayCount = (INT)*(LONG*)m_pCurrent;
			m_pCurrent += sizeof(LONG);
			return GetNumericTokenBin(ppTextResult,pFloatResult,pIntResult);
		case	TOKEN_FLOAT_LIST:
			m_pCurrent   += sizeof(WORD);
			m_bArray      = true;
			m_iArrayType  = ArrayType::Float;
			m_iArrayCount = (INT)*(LONG*)m_pCurrent;
			m_pCurrent += sizeof(LONG);
			return GetNumericTokenBin(ppTextResult,pFloatResult,pIntResult);
		default:
			if (ppTextResult != NULL)
				*ppTextResult = GetTokenBin();
			if (pFloatResult != NULL)
				*pFloatResult = fTmp;
			if (pIntResult != NULL)
				*pIntResult = iTmp;
			return false;
		}
	}
}

UINT CXFileParser::GetPos(){
	return(INT)((BYTE*)m_pCurrent - (BYTE*)m_pBuffer);
}

void CXFileParser::SetPos(UINT pos){
	m_pCurrent = (CHAR*)(m_pBuffer + pos);
}

UINT CXFileParser::GetSize(){

	return	m_dwSize;
}

HRESULT CXFileParser::SkipToNextBrace(){
	HRESULT hr = E_FAIL;
	INT	iDepth = this->m_iDepth;
	CHAR *pTmp;
	while(NULL != (pTmp = GetToken())){
		if ('}' == *pTmp && m_iDepth < iDepth){
			hr = S_OK;
			break;
		}
	}
	return hr;
}


//
//	Decompress the compressed x file.
//
//	  @param
//      pBufferCompressed : (in)compressed x file data
//      dwSize            : (in)size of compressed data
//      ppBufferUncompressed : (out)buffer to store uncompressed data
//      pdwUncompressedSize  : (out)size of uncompressed data
//
//    returns
//      S_OK   : succeeded
//      E_FAIL : failed
//
HRESULT DecompressMSZipXFile(BYTE *pBufferCompressed, DWORD dwSize, BYTE **ppBufferUncompressed, DWORD *pdwUncompressedSize){
	HRESULT hr = E_FAIL;
	BYTE *pNewBuffer = NULL;
	BYTE *pSrc, *pDest;
	DWORD finalSize;
	DWORD compressedSize, bytesRead;	//
	DWORD uncompressedSize, uncompressedSum;

	//	RFC 1951 compression
#ifdef _COMPRESSION_API_
	DECOMPRESSOR_HANDLE	hCompress = NULL;
	if (0 == CreateDecompressor(COMPRESS_ALGORITHM_MSZIP|COMPRESS_RAW,NULL,&hCompress)){
		goto EXIT;
	}
#endif
	pSrc = pBufferCompressed + 16;
	bytesRead = 16;
	uncompressedSize = 0;
	uncompressedSum = 0;
	finalSize = *(DWORD*)pSrc;	//  Read the uncompressed size
	pNewBuffer = new BYTE[finalSize];
	pDest = pNewBuffer;
	pSrc += sizeof(DWORD);
	bytesRead += sizeof(DWORD);
	uncompressedSum += 16;
	pDest += 16;
	while(bytesRead < dwSize && uncompressedSum < finalSize){
		uncompressedSize = *(WORD*)pSrc;
		pSrc += sizeof(WORD);
		compressedSize = *(WORD*)pSrc;
		pSrc += sizeof(WORD);
#ifdef _COMPRESSION_API_
		if (0 == Decompress(hCompress,(VOID*)pSrc,compressedSize,pDest,uncompressedSize,0)){
			//DWORD err = GetLastError();
			SAFE_DELETE(pNewBuffer);
			goto EXIT;
		}

#else
		DecompressRFC1951Block(pSrc+2,compressedSize-2,pDest,uncompressedSize,NULL);   //!< remove "CK"
#endif
		bytesRead += compressedSize+4;
		pSrc  += compressedSize;
		pDest += uncompressedSize;
		uncompressedSum += uncompressedSize;
	}
	{
		//  Clone the x file header
		DWORD *pdwSrc = (DWORD*)pBufferCompressed;
		DWORD *pdwDest = (DWORD*)pNewBuffer;
		*pdwDest++ = *pdwSrc++;	//  xof
		*pdwDest++ = *pdwSrc++;	//  vers
		//  bin
		if (0==_strnicmp((CHAR*)pdwSrc,"bzip",4)){
			*pdwDest = *(DWORD*)"bin ";
		}else if (0==_strnicmp((CHAR*)pdwSrc,"tzip",4)){
			*pdwDest = *(DWORD*)"txt ";
		}else{
			SAFE_DELETE(pNewBuffer);
			goto EXIT;
		}
		pdwDest++;
		pdwSrc++;

		*pdwDest++ = *pdwSrc++;	//  float length
	}
	hr = S_OK;
EXIT:
#ifdef _COMPRESSION_API_
	if (hCompress != NULL)
		CloseDecompressor(hCompress);
#endif
	*ppBufferUncompressed = pNewBuffer;
	*pdwUncompressedSize = uncompressedSum;
	return hr;
}

HRESULT CXFileParser::DecompressMSZip(){
	DWORD dwTmp, dwNewSize;
	BYTE	*pNewBuffer = NULL;
	if (FAILED(DecompressMSZipXFile(m_pBuffer,m_dwSize,&pNewBuffer,&dwNewSize)))
		return E_FAIL;
	dwTmp = (INT)((BYTE*)m_pCurrent - m_pBuffer);
	SAFE_DELETE(m_pBuffer);
	m_pBuffer = pNewBuffer;
	m_pCurrent = (CHAR*)(m_pBuffer + dwTmp);
	m_pNextPosition = NULL;
	m_dwSize = dwNewSize;
	return S_OK;
}

//
//!  Check X File Header
//!  @brief  check format and decompress if compressed.
//
HRESULT CXFileParser::CheckHeader(){

	if (0 != _strnicmp(m_pCurrent,"xof ",4))
		return E_FAIL;
	this->m_pCurrent += 4;
	
	if (0 != strncmp(m_pCurrent,"0303",4)){
		if (0 != strncmp(m_pCurrent,"0302",4)){
			return E_FAIL;
		}
	}
	m_pCurrent += 4;
	if (0 == _strnicmp(m_pCurrent,"txt ",4)){
		m_bBinaryMode = false;
	}else if (0 == _strnicmp(m_pCurrent,"bin ",4)){
		m_bBinaryMode = true;
	}else if (0 == _strnicmp(m_pCurrent,"bzip",4)){
		DecompressMSZip();
		m_bBinaryMode = true;
	}else if (0 == _strnicmp(m_pCurrent,"tzip",4)){
		DWORD dwTmp, dwNewSize;
		BYTE	*pNewBuffer = NULL;
		if (FAILED(DecompressMSZipXFile(m_pBuffer,m_dwSize,&pNewBuffer,&dwNewSize)))
			return E_FAIL;
		dwTmp = (DWORD)((BYTE*)m_pCurrent - m_pBuffer);
		SAFE_DELETE(m_pBuffer);
		m_pBuffer = pNewBuffer;
		m_pCurrent = (CHAR*)(m_pBuffer + dwTmp);
		m_pNextPosition = NULL;
		m_dwSize = dwNewSize;
		m_bBinaryMode = false;
	}else{
		return E_FAIL;
	}
	m_pCurrent += 4;
	if (0 == strncmp(m_pCurrent,"0032",4)){
		m_bDouble = false;
	}else if (0 == strncmp(m_pCurrent,"0064",4)){
		m_bDouble = true;
	}else{
		return E_FAIL;
	}
	m_pCurrent += 4;
	return S_OK;
}

//
//!  Just skip the template
//
HRESULT CXFileParser::SkipTemplate(){
	HRESULT hr = S_OK;
	CHAR *pTmp;
	INT	 iDepth = m_iDepth;
	while(NULL != (pTmp = CheckNextToken())){
		//  support unclosed template
		if (0 == _stricmp(pTmp,"template")){
			m_iDepth = iDepth;
			break;
		}
		SkipToken();
		if (*pTmp == '}'){
			if (m_iDepth <= iDepth)
				break;
		}
	}
	if (pTmp == NULL)
		hr = E_FAIL;
	return hr;
}
//
//	Create Mesh from file.
//	@param: pFilename (in) ･･･ Filename to read.
//			ppResult  (out) ･･･ mesh created (Hierarchy's root)
//			pppAnimationSets (out) ･･･ AnimationSets loaded
//			pdwNumAnimationSets (out) ･･･ the number or loaded animationsets
//
HRESULT CXFileParser::CreateMesh(TCHAR *pFilename, MeshFrame **ppResult, AnimationSet ***pppAnimationSets, DWORD *pdwNumAnimationSets){
	CXFileParser *pInstance = new CXFileParser;
	CHAR *pToken = NULL;
	HRESULT hr = S_OK;
	MeshFrame	*pTale = NULL;
	INT numAnimations = 0;

	//_CrtSetBreakAlloc(286);
	if (FAILED(pInstance->OpenRead(pFilename))){
		hr = E_FAIL;
		goto EXIT;
	}

	SAFE_DELETE(pInstance->m_pFilePath);

	CFilenameDecoder	*dec = new CFilenameDecoder(pFilename,true);
	DWORD dwTmp = 0L;
	dec->GetPath(&dwTmp,NULL);
	if (dwTmp > 0){
		pInstance->m_pFilePath = new TCHAR[dwTmp];
		dec->GetPath(&dwTmp,pInstance->m_pFilePath);
	}
	SAFE_DELETE(dec);

	hr = pInstance->CheckHeader();
	if (FAILED(hr)){
		delete pInstance;
		return hr;
	}

	while(NULL != (pToken = pInstance->GetToken())){
		if (0 == _stricmp(pToken,"template")){
			CHAR *ptemplatename = pInstance->GetToken();
			if (ptemplatename != NULL){
				CHAR *pBrace = pInstance->GetToken();
				if (0 == strcmp(pBrace,"{")){
					hr = pInstance->SkipTemplate();
					if (FAILED(hr))
						goto EXIT;
				}
			}
		}else if (0 == _stricmp(pToken,"Frame")){
			MeshFrame *pFrame = NULL;
			hr = pInstance->GetMeshFrame(&pFrame);
			if (SUCCEEDED(hr)&&pFrame != NULL){
				pInstance->m_pFileReadingContext->pListFramesLoaded->push_back(pFrame);
			}
			if (pTale == NULL){
				pInstance->m_pFrameRoot = pFrame;
				pTale = pFrame;
			}else{
				pTale->pFrameSibling = pFrame;
				pTale = pFrame;
			}
		}else if (0 == _stricmp(pToken,"AnimationSet")){
			TempAnimationSet *pTempAnimationSet = NULL;
			hr = pInstance->GetAnimationSet(&pTempAnimationSet);
			if (FAILED(hr)){
				SAFE_DELETE(pTempAnimationSet);
				goto EXIT;
			}else{
				pInstance->m_pFileReadingContext->pTempAnimationSetsLoaded->push_back(pTempAnimationSet);
			}
			hr = E_FAIL;
		}else if (0 == _stricmp(pToken,"Mesh")){
			//  Get root mesh
			MeshFrame *pFrame = NULL;
			hr = pInstance->GetRootMesh(&pFrame);
			if (SUCCEEDED(hr)&&pFrame != NULL){
				pInstance->m_pFileReadingContext->pListFramesLoaded->push_back(pFrame);
			}
			if (pTale == NULL){
				pInstance->m_pFrameRoot = pFrame;
				pTale = pFrame;
			}else{
				pTale->pFrameSibling = pFrame;
				pTale = pFrame;
			}
		}else if (0 == strcmp(pToken,"{")){
			//  Unknown section
			INT iDepth2 = pInstance->m_iDepth;
			while(pInstance->m_iDepth >= iDepth2){
				if (NULL == (pToken = pInstance->GetToken()))
					goto EXIT;
			}
		}else{
			_RPT0(_CRT_WARN,pToken);
		}
	}
	hr = S_OK;  //  End of file
EXIT:
	if (SUCCEEDED(hr)){
		//  Reorganize mesh datas
		if (pInstance->m_pFrameRoot){
			pInstance->EnflatLoadedFrames();
			AdjustFrameMatrices(pInstance->m_pFrameRoot);
			//pInstance->OrganizeFrames(pInstance->m_pFrameRoot);		
			pInstance->OrganizeLoadedMeshes();
		}
		//  Reorganize animations
		hr = pInstance->EnflatAnimations();
		if (SUCCEEDED(hr)){
			if (pInstance->m_ppAnimationSets){
				*pppAnimationSets = pInstance->m_ppAnimationSets;
				*pdwNumAnimationSets = (DWORD)pInstance->m_iNumAnimationSets;
				pInstance->m_ppAnimationSets = NULL;
			}else{
				*pppAnimationSets = NULL;
				*pdwNumAnimationSets = 0;
			}
		}

		*ppResult = pInstance->m_pFrameRoot;
	}
	//  File Read completed
	SAFE_DELETE(pInstance->m_pFileReadingContext);

	//  delete frames (temporal) 
	SAFE_DELETE_ARRAY(pInstance->m_ppFlattenFramePointers);
	
	//SAFE_DELETE(pInstance->m_pFrameRoot);
	delete pInstance;
	
	return hr;
}

#define IS_SEPARATOR1(c)	(c == ';'||c == ',')
#define IS_SEPARATOR(c)	(c == ';'||c == ','||c=='{'||c=='}'||c=='['||c==']')

//
//	method GetMeshFrame
//	@param : ppMeshFrame 
//  @return : S_OK (succeeded) / error value
//
HRESULT CXFileParser::GetMeshFrame(MeshFrame **ppMeshFrame){
	MeshFrame *pResult = NULL;
	MeshFrame *pFrameLastChild = NULL;
	HRESULT hr;
	CHAR *pToken,c;
	int len;
	INT iDepth;

	pResult = new MeshFrame;

	//  Get name.
	pToken = GetToken();
	hr = E_FAIL;
	if (pToken== NULL)
		goto ERROR_EXIT;
	c = *pToken;
	if (IS_SEPARATOR(c)){
		goto ERROR_EXIT;
	}
	len = (int)strlen(pToken);
	pResult->pName = new CHAR[(len+4)&0xfffffffc];
	strcpy_s(pResult->pName,len+1,pToken);

	//  Enter the brace;
	pToken = GetToken();
	if (*pToken != '{')
		goto ERROR_EXIT;

	iDepth = m_iDepth;
	while(true){

		pToken = GetToken();
		if (pToken == NULL){
			hr = E_FAIL;
			goto ERROR_EXIT;
		}

		if (*pToken == '}'){
			if (m_iDepth < iDepth)
				break;
		}else if (0 == _stricmp(pToken,"FrameTransformMatrix")){
			XMMATRIX	matFrame;
			hr = GetMatrix(&pResult->FrameMatrix);
			if (FAILED(hr))
				goto ERROR_EXIT;
		}else if (0 == _stricmp(pToken,"ObjectMatrixComment")){
			XMMATRIX	matFrame;
			hr = GetMatrix(&matFrame);
			if (FAILED(hr))
				goto ERROR_EXIT;
		}else if (0 == _stricmp(pToken,"Mesh")){
			MeshContainer *pMeshContainer = NULL;
			MeshContainer *pTale = pResult->pMeshContainer;
			hr = GetMeshContainer(&pMeshContainer);
			if (FAILED(hr)){
				SAFE_DELETE(pMeshContainer);
				goto ERROR_EXIT;
			}
			if (pResult->pMeshContainer == NULL){
				pResult->pMeshContainer = pMeshContainer;
			}else{
				while(pTale->pNextMeshContainer != NULL){
					pTale = pTale->pNextMeshContainer;
				}
				pTale->pNextMeshContainer = pMeshContainer;
			}
		}else if (0 == _stricmp(pToken , "Frame")){
			MeshFrame *pChild = NULL;
			hr = GetMeshFrame(&pChild);
			if (FAILED(hr)){
				SAFE_DELETE(pChild);
				goto ERROR_EXIT;
			}else if (pChild != NULL){
				if (pFrameLastChild == NULL){
				pResult->pFrameFirstChild = pChild;
				pFrameLastChild = pChild;
				}else{
					pFrameLastChild->pFrameSibling = pChild;
					pFrameLastChild = pChild;
				}
				m_pFileReadingContext->pListFramesLoaded->push_back(pChild);
			}
		}else if (0 == strcmp(pToken,"{")){  //  Unknown data
			INT iDepth2 = m_iDepth;
			while(m_iDepth >= iDepth2){
				if (NULL == (pToken = GetToken()))
					goto ERROR_EXIT;
			}
		}
	}

ERROR_EXIT:
	if (FAILED(hr)){
		SAFE_DELETE(pResult->pName);
		SAFE_DELETE(pResult);
	}else{
		*ppMeshFrame = pResult;
	}
	
	return hr;
}

//
//	method GetRootMesh
//	@param : ppMeshFrame 
//  @return : S_OK (succeeded) / error value
//
HRESULT CXFileParser::GetRootMesh(MeshFrame **ppMeshFrame){
	MeshFrame *pResult = NULL;
	MeshFrame *pFrameLastChild = NULL;
	HRESULT hr;
	//CHAR *pToken,c;

	pResult = new MeshFrame;
	pResult->pName = new CHAR[1];
	pResult->pName[0] = '\0';

	pResult->FrameMatrix = XMMatrixIdentity();
	
	MeshContainer *pMeshContainer = NULL;
	MeshContainer *pTale = pResult->pMeshContainer;

	hr = GetMeshContainer(&pMeshContainer);
	if (FAILED(hr)){
		SAFE_DELETE(pMeshContainer);
		goto ERROR_EXIT;
	}
	if (pResult->pMeshContainer == NULL){
		pResult->pMeshContainer = pMeshContainer;
	}else{
		while(pTale->pNextMeshContainer != NULL){
			pTale = pTale->pNextMeshContainer;
		}
		pTale->pNextMeshContainer = pMeshContainer;
	}

ERROR_EXIT:
	if (FAILED(hr)){
		SAFE_DELETE(pResult->pName);
		SAFE_DELETE(pResult);
	}else{
		*ppMeshFrame = pResult;
	}
	
	return hr;
}

//
//	Get Matrix
//
HRESULT CXFileParser::GetMatrix(XMMATRIX *pMatrix){
	CHAR *pToken = GetToken();
	HRESULT hr = E_FAIL;
	XMMATRIX matResult;
	INT iDepth;
	INT index;
	CHAR c;
	if (*pToken != '{'){
		pToken = GetToken();
	}
	if (*pToken != '{'){
		goto ERROR_EXIT;
	}
	iDepth = m_iDepth;
	index = 0;

	while(true){
		FLOAT fTmp;
		if (!GetNumericToken(&pToken,&fTmp,NULL)){
			c = *pToken;
			if (c == '}'){
				if (m_iDepth < iDepth){
					hr = S_OK;
					goto ERROR_EXIT;
				}
			}else if (!IS_SEPARATOR(c)){
				goto ERROR_EXIT;
			}
		}else{
			matResult.r[index >> 2].m128_f32[index & 3] = fTmp;
			++index;
		}
	}
ERROR_EXIT:
	if (SUCCEEDED(hr)){
		*pMatrix = matResult;
	}
	return hr;
}

//
//  Get 3D vectors
//
HRESULT CXFileParser::Get3DVectors(XMFLOAT3 **ppVectors, INT *pNumVectors){
	HRESULT hr = E_FAIL;
	CHAR *pToken;
	CHAR c;
	XMFLOAT3 *pVectors = NULL;
	INT	numVectors = 0;
	INT iDepth = m_iDepth;
	INT   iTmp;
	FLOAT fTmp;

	//  Skip to next section
	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		c = *pToken;
		if (c == '}' && m_iDepth < iDepth){
			hr = S_OK;
			goto EXIT;
		}
		if (!IS_SEPARATOR(c))
			goto EXIT;
	}
	numVectors = iTmp;

	pVectors = new XMFLOAT3[numVectors];
	for (int i = 0; i < numVectors ; ++i){
		FLOAT vecBuffer[3];
		for (int j = 0; j < 3 ; ++j){

			while (!GetNumericToken(&pToken,&fTmp,NULL)){
				c = *pToken;
				if (c == '}' && m_iDepth < iDepth){
					hr = S_OK;
					goto EXIT;
				}else if (!IS_SEPARATOR(c))
					goto EXIT;
			}
			vecBuffer[j] = fTmp;
		}
		pVectors[i].x = vecBuffer[0];
		pVectors[i].y = vecBuffer[1];
		pVectors[i].z = vecBuffer[2];
	}
	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		*ppVectors = pVectors;
		*pNumVectors = numVectors;
	}else{
		SAFE_DELETE(pVectors);
	}
	return hr;
}

//
//  Get 2D vectors
//
HRESULT CXFileParser::Get2DVectors(XMFLOAT2 **ppVectors, INT *pNumVectors){
	HRESULT hr = E_FAIL;
	CHAR *pToken;
	CHAR c;
	XMFLOAT2 *pVectors = NULL;
	INT	numVectors = 0;
	INT iDepth = m_iDepth;
	INT   iTmp;
	FLOAT fTmp;

	//  Skip to next section
	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		c = *pToken;
		if (c == '}' && m_iDepth < iDepth){
			hr = S_OK;
			goto EXIT;
		}
		if (!IS_SEPARATOR(c))
			goto EXIT;
	}
	numVectors = iTmp;

	pVectors = new XMFLOAT2[numVectors];
	for (int i = 0; i < numVectors ; ++i){
		FLOAT vecBuffer[2];
		for (int j = 0; j < 2 ; ++j){
			while (!GetNumericToken(&pToken,&fTmp,NULL)){
				c = *pToken;
				if (c == '}' && m_iDepth < iDepth){
					hr = S_OK;
					goto EXIT;
				}else if (!IS_SEPARATOR(c))
					goto EXIT;
			}
			vecBuffer[j] = fTmp;
		}
		pVectors[i].x = vecBuffer[0];
		pVectors[i].y = vecBuffer[1];
	}
	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		*ppVectors = pVectors;
		*pNumVectors = numVectors;
	}else{
		SAFE_DELETE(pVectors);
	}
	return hr;
}

//
//   GetIndices
//
HRESULT CXFileParser::GetTriangleIndices(INT **ppIndices, INT *pNumIndices, INT **ppOriginalFaceReference, INT *pNumReference){
	HRESULT hr = E_FAIL;
	CHAR *pToken;
	CHAR c;
	INT *pIndices = NULL;
	INT	numIndices = 0;
	INT iDepth = m_iDepth;
	int	iResultSize = 0;
	INT numOriginalFaces = 0;
	INT *pOriginalReference = NULL;
	INT   iTmp;
	std::vector<int> indices(0);

	//  Skip to next section
	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		c = *pToken;
		if (c == '}' && m_iDepth < iDepth){
			hr = S_OK;
			goto EXIT;
		}
		if (!IS_SEPARATOR(c))
			goto EXIT;
	}
	numOriginalFaces = iTmp;
	//indices.resize(numOriginalFaces * 3);
	int	iElemCount = 0;	//  
	int iFaceCount = 0;
	int iTriangluatedFaceCount = 0;
	pOriginalReference = new INT[numOriginalFaces * 2];	//  face id / count
	hr = E_FAIL;
	while(iFaceCount < numOriginalFaces){
		while(!GetNumericToken(&pToken,NULL,&iTmp)){
			c = *pToken;
			if (c == '}' && m_iDepth < iDepth){
				hr = S_OK;
				goto EXIT;
			}
			if (!IS_SEPARATOR(c))
				goto EXIT;
		}
		indices.push_back(iTmp);
		if (iElemCount==0){
			iElemCount = iTmp;
			INT iTriangleCount = (iElemCount > 2)?(iElemCount - 2):0;
			//  Triangulate the polygon number of triangles to each polygon is vertex count - 2.
			pOriginalReference[iFaceCount * 2]   = iTriangluatedFaceCount;
			pOriginalReference[iFaceCount * 2+1] = iTriangleCount;
			iTriangluatedFaceCount += iTriangleCount;
		}else{
			--iElemCount;
		}
		if (iElemCount == 0){
			++iFaceCount;
		}
	}
	pIndices = new INT[iTriangluatedFaceCount * 3];
	int index = 0;
	int faceIndex = 0;
	int faceWidth;
	int start, middle, last;
	while(iElemCount < (INT)indices.size()){
		faceWidth = 0;
		iTmp = indices[iElemCount++];
		if (iTmp < 3){
			iElemCount += iTmp;
		}else{
			start = indices[iElemCount++];
			middle = indices[iElemCount++];
			last = indices[iElemCount++];
			pIndices[index++] = start;
			pIndices[index++] = middle;
			pIndices[index++] = last;
			iTmp -= 3;
			while(0 < iTmp--){
				middle = last;
				last = indices[iElemCount++];
				pIndices[index++] = start;
				pIndices[index++] = middle;
				pIndices[index++] = last;
			}
		}
	}

	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		*ppIndices = pIndices;
		*pNumIndices = iTriangluatedFaceCount;
		if (ppOriginalFaceReference){
			*ppOriginalFaceReference = pOriginalReference;
			if (pNumReference != NULL){
				*pNumReference = numOriginalFaces;
			}
		}else
			delete pOriginalReference;
	}else{
		*ppIndices = NULL;
		SAFE_DELETE(pIndices);
		SAFE_DELETE(pOriginalReference);
	}
	return hr;
}

//
//	Get Skin Weights
//
HRESULT CXFileParser::GetSkinWeights(SkinWeights **ppSkinWeights){
	SkinWeights *pSkinWeights = NULL;
	INT iDepth;
	CHAR	*pToken, c;
	INT	len = 0;
	INT sizName = 0;
	HRESULT hr = E_FAIL;
	INT   iTmp;
	FLOAT fTmp;
	//  Get SkinWeights
			
	if (NULL == (pToken = GetToken()))
		goto EXIT;
	c = *pToken;

	//  名前があれば無視
	if (c != '{'){
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
		if (c != '{')
			goto EXIT;	//  ERROR
	}


	iDepth = m_iDepth;
	pSkinWeights = new SkinWeights;

	//Get Bone Name
	if (NULL == (pToken = GetToken()))
		goto EXIT;

	len = (INT)strlen(pToken);
	sizName = (len + 4)&0xfffffffc;
	pSkinWeights->pBoneName = new CHAR[sizName];
	strcpy_s(pSkinWeights->pBoneName,sizName,pToken);

	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		if (pToken == NULL)
			goto EXIT;
		if (!IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	pSkinWeights->numWeights = iTmp;
	pSkinWeights->pIndices = new INT[pSkinWeights->numWeights];
	for (int i = 0; i < pSkinWeights->numWeights; ++i){
		while(!GetNumericToken(&pToken,NULL,&iTmp)){
			if (pToken == NULL)
				goto EXIT;
			if (!IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		pSkinWeights->pIndices[i] = iTmp;
	}

	pSkinWeights->pWeights = new FLOAT[pSkinWeights->numWeights];
	for (int i = 0; i < pSkinWeights->numWeights; ++i){
		while(!GetNumericToken(&pToken,&fTmp,NULL)){
			if (pToken == NULL)
				goto EXIT;
			if (!IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		pSkinWeights->pWeights[i] = fTmp;
	}

	//  Offset Matrix
	for (int i = 0; i < 16; ++i){
		while(!GetNumericToken(&pToken,&fTmp,NULL)){
			if (pToken == NULL)
				goto EXIT;
			if (!IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		pSkinWeights->matOffset.r[i>>2].m128_f32[i&3] = fTmp;
	}

	//  skip to next section
	do{
		if (NULL == (pToken = GetToken())){
			goto EXIT;
		}
	}while(m_iDepth >= iDepth);

	hr = S_OK;

EXIT:
	if (FAILED(hr)){
		SAFE_DELETE(pSkinWeights);
	}else{
		*ppSkinWeights = pSkinWeights;
	}
	return hr;
}

HRESULT CXFileParser::GetMeshMaterialList(TempMesh *pMeshData){
	HRESULT hr = E_FAIL;
	CHAR	*pToken, c;
	INT		iDepth;
	INT		numMaterials;
	INT		numFaceIndex;
	INT		*pMaterialList = NULL;
	INT		iTmp;
	if (NULL == (pToken = GetToken()))
		goto EXIT;
	c = *pToken;

	if (c != '{'){
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
		if (c != '{')
			goto EXIT;	//  ERROR
	}
	iDepth = m_iDepth;

	if (!GetNumericToken(NULL,NULL,&iTmp))
		goto EXIT;

	numMaterials = iTmp;

	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		if (pToken == NULL)
			goto EXIT;
		if (!IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	numFaceIndex = iTmp;

	pMaterialList = new INT[numFaceIndex];
	
	for ( int i = 0; i < numFaceIndex ; ++i){

		while(!GetNumericToken(&pToken,NULL,&iTmp)){
			if (pToken == NULL)
				goto EXIT;
			if (!IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		pMaterialList[i] = iTmp;
	}
	if (NULL == (pToken = CheckNextToken()))
		goto EXIT;
	if (*pToken == ';'){	//  last face material index
		SkipToken();
	}
	hr = S_OK;

EXIT:
	if (FAILED(hr)){
		SAFE_DELETE(pMaterialList);
	}else{
		pMeshData->pMaterialList = pMaterialList;
		pMeshData->numMaterialList = numFaceIndex;
	}
	return hr;
}


//
//	Get Mesh Material
//
HRESULT CXFileParser::GetMaterial(Material **ppMaterial){
	HRESULT hr = E_FAIL;
	CHAR *pToken,c;
	INT iDepth;
	FLOAT v[4];
	Material *pMaterial = NULL;
	CHAR *pName = NULL;
	FLOAT fTmp;
	//  Try to get Material name
	if (NULL == (pToken = GetToken()))
		goto EXIT;
	c = *pToken;

	if (c != '{'){
		INT len = (INT)strlen(pToken);
		INT sizName = (len+4)&0xfffffffc;
		pName = new CHAR[sizName];
		strcpy_s(pName,sizName,pToken);
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
		if (c != '{')
			goto EXIT;	//  ERROR
	}
	iDepth = m_iDepth;

	pMaterial = new Material;
	pMaterial->pName = pName;
	pName = NULL;

	//  Get face color.
	for (int i = 0 ; i < 4 ; ++i){
		while(!GetNumericToken(&pToken,&fTmp,NULL)){
			if (pToken == NULL)
				goto EXIT;
			if (!IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		v[i] = fTmp;
	}
	pMaterial->vecDiffuse.x = v[0];	//	r
	pMaterial->vecDiffuse.y = v[1];	//	g
	pMaterial->vecDiffuse.z = v[2];	//	b
	pMaterial->vecDiffuse.w = v[3];	//	a

	//  get power
	while(!GetNumericToken(&pToken,&fTmp,NULL)){
		if (pToken == NULL)
			goto EXIT;
		if (!IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	pMaterial->fPower = fTmp;

		//  specular
	for (int i = 0 ; i < 3 ; ++i){
		while(!GetNumericToken(&pToken,&fTmp,NULL)){
			if (pToken == NULL)
				goto EXIT;
			if (!IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		v[i] = fTmp;
	}
	pMaterial->vecSpecular.x = v[0];	//	r
	pMaterial->vecSpecular.y = v[1];	//	g
	pMaterial->vecSpecular.z = v[2];	//	b
	pMaterial->vecSpecular.w = 0;

	//  emmissive
	for (int i = 0 ; i < 3 ; ++i){
		while(!GetNumericToken(&pToken,&fTmp,NULL)){
			if (pToken == NULL)
				goto EXIT;
			if (!IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		v[i] = fTmp;

	}
	pMaterial->vecEmmisive.x = v[0];	//	r
	pMaterial->vecEmmisive.y = v[1];	//	g
	pMaterial->vecEmmisive.z = v[2];	//	b
	pMaterial->vecEmmisive.w = 0;

	//  Get optional datas.
	do{
		if (NULL == (pToken = GetToken())){
			goto EXIT;
		}
		if (0 == _stricmp(pToken,"TextureFilename")){
			//  Try to get the section name
			INT iDepth2;
			if (NULL == (pToken = GetToken()))
				goto EXIT;
			c = *pToken;

			if (c != '{'){
				if (NULL == (pToken = GetToken()))
					goto EXIT;
				c = *pToken;
				if (c != '{')
					goto EXIT;	//  ERROR
			}
			iDepth2 = m_iDepth;

			if (NULL != (pToken = GetToken())){
				SAFE_DELETE(pMaterial->pTextureFilename);
				TCHAR *p;
#ifndef _UNICODE
				INT len = strlen(pToken);
				INT sizFilename = (len + 4)&0xfffffffc;
				TCHAR	*pFilename = new TCHAR[sizFilename];
				strcpy_s(pFilename,sizFilename,pToken);
#else
				INT len = (INT)_mbstrlen(pToken);
				INT sizFilename = (len+4)&0xfffffffc;
				size_t numConverted;
				TCHAR	*pFilename = new TCHAR[sizFilename];
				mbstowcs_s(&numConverted,pFilename,sizFilename,pToken,_TRUNCATE);
#endif
				p = pFilename;
				//  remove the quote.
				if (p[0] == _T('\"')){
					++p;
					TCHAR *p2 = _tcsrchr(p,_T('\"'));
					if (p2 != NULL){
						*p2 = _T('\0');
					}
				}
				if ((p[0] == _T('\\') && p[1] == _T('\\')) ||(p[0] == _T('/') && p[1] == _T('/'))|| (p[1] == _T(':'))){
					//  fullpath -> body name
					CFilenameDecoder	*dec = new CFilenameDecoder(p,true);
					DWORD dwTemp = 0;
					TCHAR *pBodyname = NULL;
					dec->GetFilename(&dwTemp,NULL);
					pBodyname = new TCHAR[dwTemp];
					dec->GetFilename(&dwTemp,pBodyname);
					SAFE_DELETE(dec);
					p = pBodyname;
					SAFE_DELETE(pFilename);
				}
				len = (INT)_tcslen(p);
				len += (INT)_tcslen(this->m_pFilePath)+2;
				len = (len+3)&0xfffffffcL;
				TCHAR	*pTmp = new TCHAR[len];
				_tcscpy_s(pTmp,len,m_pFilePath);
				_tcscat_s(pTmp,len,_T("\\"));
				_tcscat_s(pTmp,len,p);
				SAFE_DELETE(pFilename);

				CFilenameDecoder	*dec = new CFilenameDecoder(pTmp,true);
				DWORD	dwTemp = 0;
				dec->GetFullname(&dwTemp,NULL);
				pFilename = new TCHAR[dwTemp];
				dec->GetFullname(&dwTemp,pFilename);
				SAFE_DELETE(dec);
				SAFE_DELETE(pTmp);
				pMaterial->pTextureFilename = pFilename;
			}
			
			//  Skip to next sction
			do{
				if (NULL == (pToken = GetToken())){
					goto EXIT;
				}
			}while(m_iDepth >= iDepth2);
		}
	}while(m_iDepth >= iDepth);

	hr = S_OK;

EXIT:
	SAFE_DELETE(pName);
	if (FAILED(hr)){
		SAFE_DELETE(pMaterial);
	}else{
		*ppMaterial = pMaterial;
	}
	return hr;
}

//
//	GetMeshGeometry
//  Get Vertices and Indices from data
//
HRESULT CXFileParser::GetMeshGeometry(TempMesh *pMeshData){
	HRESULT hr = E_FAIL;
	INT iDepth = m_iDepth;
	//  Get Vetices
	hr = Get3DVectors(&pMeshData->pPositions,
		&pMeshData->numVertices);

	hr = E_FAIL;	

	//  Get Indices
	hr = GetTriangleIndices(&pMeshData->pTempIndices,
		&pMeshData->numIndices,&pMeshData->pFaceReference,&pMeshData->numOriginalFaces);

	if (FAILED(hr)|| m_iDepth < iDepth)
		goto EXIT;

	hr = S_OK;
EXIT:
	return hr;
}

//
//	Get mesh normal vectors
//
HRESULT CXFileParser::GetMeshNormals(TempMesh *pMeshData){
	HRESULT hr = E_FAIL;
	CHAR *pToken, c;
	DirectX::XMFLOAT3	*pNormals = NULL;

	//  Get Normals
	if (NULL == (pToken = GetToken()))
		goto EXIT;
	c = *pToken;
	//  法線の名前は無視（今のところ、おそらくtangent 対応がいるのか・・・・）
	if (c != '{'){
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
	}

	if (c != '{')
		goto EXIT;	//  ERROR

	int iDepth2 = m_iDepth;

	int iNumNormals;
	hr = Get3DVectors(&pNormals, &iNumNormals);

	if (FAILED(hr))
		goto EXIT;

	//if (iNumNormals != pMeshData->numVertices){
	//	hr = E_FAIL;
	//	goto EXIT;
	//}
	if (m_iDepth < iDepth2){
		goto EXIT;
	}
	hr = E_FAIL;

	//  Get Indices
	INT *pNormalIndices;
	INT numNormalIndices;
	hr = GetTriangleIndices(&pNormalIndices,&numNormalIndices,NULL,NULL);
	
	if (FAILED(hr))
		goto EXIT;

	if (iNumNormals != pMeshData->numVertices){
		if (numNormalIndices != pMeshData->numIndices){
			SAFE_DELETE(pNormalIndices);
			goto EXIT;
		}
		int iSrc, iDest;
		pMeshData->pNormals = new DirectX::XMFLOAT3[pMeshData->numVertices];
		for (int i = 0; i < numNormalIndices * 3; ++i){
			iSrc = pNormalIndices[i];
			iDest = pMeshData->pTempIndices[i];
			pMeshData->pNormals[iDest] = pNormals[iSrc];
		}
		SAFE_DELETE(pNormals);
	}else{
		pMeshData->pNormals = pNormals;
		pNormals = NULL;
	}
	SAFE_DELETE(pNormalIndices);
	hr = E_FAIL;
	//  skip to next data
	while (m_iDepth >= iDepth2){
		if (NULL == (pToken = GetToken())){
			goto EXIT;
		}
	}

	hr = S_OK;
EXIT:
	SAFE_DELETE(pNormals);
	return hr;
}

//
//	Get mesh texture coords
//
HRESULT CXFileParser::GetMeshTextureCoords(TempMesh *pMeshData){
	HRESULT hr = E_FAIL;
	CHAR *pToken, c;

	//  Get Normals
	if (NULL == (pToken = GetToken()))
		goto EXIT;
	c = *pToken;
	//  テクスチャ座標の名前は無視
	if (c != '{'){
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
	}

	if (c != '{')
		goto EXIT;	//  ERROR

	int iDepth2 = m_iDepth;

	int iNumTexcoords;
	hr = Get2DVectors(&pMeshData->pTexCoords,
		&iNumTexcoords);
			
	if (FAILED(hr))	//  Get tex coords error
		goto EXIT;

	if (iNumTexcoords != pMeshData->numVertices){
		hr = E_FAIL;
		goto EXIT;
	}

	hr = E_FAIL;
	//  skip to next data
	while (m_iDepth >= iDepth2){
		if (NULL == (pToken = GetToken())){
			goto EXIT;
		}
	}

	hr = S_OK;
EXIT:
	return hr;
}

HRESULT CXFileParser::GetSkinMeshHeader(INT *pNumBones, INT *pNumMaxWeightsPerVertex, INT *pNumMaxWeightsPerFace){
	HRESULT hr = E_FAIL;
	CHAR *pToken , c;
	INT iNumBones, iNumMaxWeightsPerVertex, iNumMaxWeightsPerFace;
	INT iDepth2;
	INT	iTmp;
	//  Get SkinMesh Header
	if (NULL == (pToken = GetToken()))
		goto EXIT;
	c = *pToken;

	//  名前があれば無視
	if (c != '{'){
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
	}

	if (c != '{')
		goto EXIT;	//  ERROR
			
	iDepth2 = m_iDepth;

	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		if (pToken == NULL || !IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	iNumMaxWeightsPerVertex = iTmp;

	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		if (pToken == NULL || !IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	iNumMaxWeightsPerFace = iTmp;

	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		if (pToken == NULL || !IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	iNumBones = iTmp;

	do{
		if (NULL == (pToken = GetToken())){
			goto EXIT;
		}
	}while(m_iDepth >= iDepth2);
	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		*pNumBones = iNumBones;
		*pNumMaxWeightsPerVertex = iNumMaxWeightsPerVertex;
		*pNumMaxWeightsPerFace = iNumMaxWeightsPerFace;
	}
	return hr;
}

//
//  GetMeshContainer
//
HRESULT CXFileParser::GetMeshContainer(MeshContainer **ppMeshContainer){
	HRESULT hr = E_FAIL;
	INT len;
	CHAR *pToken,c;

	FLOAT	*fbuffer = NULL;
	XMFLOAT3 *pVertices = NULL;
	XMFLOAT3 *pNormals = NULL;
	INT		*pIndices = NULL;
	INT		iDepth;
	MeshContainer *pMeshContainer = new MeshContainer;
	MeshLoaderContext	*pMeshLoaderContext = new MeshLoaderContext;
	TempMesh	*pMeshData = NULL;

	//  Get Mesh Name
	if (NULL == (pToken = GetToken()))
		goto EXIT;

	c = *pToken;
	if (!IS_SEPARATOR(c)){
		INT count;
		len = (INT)strlen(pToken);
		pMeshContainer->pName = new CHAR[count = (len+4)&0xfffffffc];
		strcpy_s(pMeshContainer->pName,count,pToken);
	}

	if (c != '{'){
		if (NULL == (pToken = GetToken()))
			goto EXIT;

		c = *pToken;
	}
	if (c != '{')
		goto EXIT;	//  ERROR

	iDepth = m_iDepth;		//  nest level
	pMeshData = new TempMesh;

	//  Get Vertices and Indexes
	hr = GetMeshGeometry(pMeshData);
	if (FAILED(hr))
		goto EXIT;

	hr = E_FAIL;
	while(true){
		hr = E_FAIL;	
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
		if (c == '}'){
			if (m_iDepth < iDepth){
				hr = S_OK;	
				goto EXIT;	//  Complete reading mesh.
			}
			continue;
		}
		if (0 == _stricmp(pToken,"MeshNormals")){

			hr = GetMeshNormals(pMeshData);
			if (FAILED(hr))
				goto EXIT;

		}else if (0 == _stricmp(pToken,"MeshTextureCoords")){
			
			hr = GetMeshTextureCoords(pMeshData);
			if (FAILED(hr))
				goto EXIT;

		}else if (0 == _stricmp(pToken,"XSkinMeshHeader")){
			
			INT iNumBones, iNumMaxWeightsPerVertex, iNumMaxWeightsPerFace;
			hr = GetSkinMeshHeader(&iNumBones, &iNumMaxWeightsPerVertex, &iNumMaxWeightsPerFace);
			if (FAILED(hr))
				goto EXIT;
			pMeshData->numBones = iNumBones;			
			pMeshData->numMaxSkinWeightsPerVertex = iNumMaxWeightsPerVertex;
			pMeshData->numMaxSkinWeightsPerFace = iNumMaxWeightsPerFace;

		}else if (0 == _stricmp(pToken,"SkinWeights")){
			SkinWeights *pSkinWeights = NULL;

			hr = GetSkinWeights(&pSkinWeights);

			if (FAILED(hr))
				goto EXIT;

			if (pSkinWeights != NULL){
				pMeshLoaderContext->pWeights->push_back(pSkinWeights);
			}
		}else if (0 == _stricmp(pToken,"MeshMaterialList")){
			if (pMeshData != NULL){
				hr = GetMeshMaterialList(pMeshData);
			}
			if (FAILED(hr))
				goto EXIT;
		}else if (0 == _stricmp(pToken,"Material")){
			if (pMeshData != NULL){
				Material *pMaterial = NULL;

				hr = GetMaterial(&pMaterial);
				if (FAILED(hr))
					goto EXIT;
				if (pMaterial){
					pMeshLoaderContext->pMaterials->push_back(pMaterial);
				}
			}
		}else if (0 == strcmp(pToken,"{")){  //  Unknown data
			//  VertexDuplicationIndices is ignored in this version.
			SkipToNextBrace();
		}else{

		}
	}
EXIT:
	if (!pMeshLoaderContext->pMaterials->empty()){
		//  Enflat materials
		INT numMaterials = (INT)pMeshLoaderContext->pMaterials->size();
		INT i;
		std::list<Material*>::iterator it;
		pMeshContainer->pMaterials = new Material[numMaterials];
		pMeshContainer->dwNumMaterials = numMaterials;

		it = pMeshLoaderContext->pMaterials->begin();
		i = 0;
		while(it != pMeshLoaderContext->pMaterials->end()){
			memcpy(&pMeshContainer->pMaterials[i++],*it,sizeof(Material));
			(*it)->pName = NULL;			//	Cheat!
			(*it)->pTextureFilename = NULL;	//	Cheat!
			delete *it;
			it = pMeshLoaderContext->pMaterials->erase(it);
		}
		delete pMeshLoaderContext->pMaterials;
		pMeshLoaderContext->pMaterials = NULL;
	}
	//	


	if (!pMeshLoaderContext->pWeights->empty()){
		//  Enflat weights
		INT index;
		std::list<SkinWeights*> *pList = pMeshLoaderContext->pWeights;
		pMeshData->numSkinWeights = (INT)pList->size();
		pMeshData->ppSkinWeights = new SkinWeights*[pMeshData->numSkinWeights];
		std::list<SkinWeights*>::iterator itWeight = pList->begin();
		index = 0;
		while(itWeight != pList->end()){
			pMeshData->ppSkinWeights[index++] = *itWeight;
			*itWeight = NULL;
			itWeight = pList->erase(itWeight);
		}
	}


	if (FAILED(hr)){
		SAFE_DELETE(pMeshData);
		SAFE_DELETE(pMeshContainer);
	}else{
		//  make reference to the container
		pMeshData->pParent = pMeshContainer;

		//  register to context;
		this->m_pFileReadingContext->pListTempMeshLoaded->push_back(pMeshData);
		*ppMeshContainer = pMeshContainer;
	}
	SAFE_DELETE(pMeshLoaderContext);
	pMeshData = NULL;
	return hr;
}

//
//	GetTempAnimationSets
//
HRESULT CXFileParser::GetAnimationSet(TempAnimationSet **ppTempAnimationSet){
	HRESULT hr = E_FAIL;
	CHAR *pToken, c;
	TempAnimationSet *pTempAnimationSet=NULL;
	INT	len;
	INT	iDepth;

	pTempAnimationSet = new TempAnimationSet;

	//  Get Animation Name
	if (NULL == (pToken = GetToken()))
		goto EXIT;

	c = *pToken;
	if (!IS_SEPARATOR(c)){
		int count;
		len = (INT)strlen(pToken);
		pTempAnimationSet->pName = new CHAR[count = (len+4)&0xfffffffc];
		strcpy_s(pTempAnimationSet->pName,count,pToken);
		if (NULL == (pToken = GetToken()))
			goto EXIT;

		c = *pToken;
	}
	if (c != '{')
		goto EXIT;
	iDepth = m_iDepth;

	while(m_iDepth >= iDepth){
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
		if (c == ';'||c == ','||c=='}'||c=='['||c==']')
			continue;
		if (0 == _stricmp(pToken,"Animation")){
			Animation *pAnimation = NULL;
			hr = GetAnimation(&pAnimation);
			if (FAILED(hr)){
				SAFE_DELETE(pAnimation);
				goto EXIT;
			}
			hr = E_FAIL;
			pTempAnimationSet->pAnimations->push_back(pAnimation);
		}else if (*pToken == '{'){
			INT iDepth2 = m_iDepth;
			do{
				if (NULL == (pToken = GetToken()))
					goto EXIT;
			}while(m_iDepth >= iDepth2);
		}
	}
	hr = S_OK;
EXIT:
	if (FAILED(hr)){
		SAFE_DELETE(pTempAnimationSet);
	}else{
		*ppTempAnimationSet = pTempAnimationSet;
	}
	return hr;
}

//
//	Get animation
//
HRESULT CXFileParser::GetAnimation(Animation **ppAnimation){
	HRESULT hr = E_FAIL;
	Animation *pAnimation = NULL;
	CHAR *pToken , c;
	INT  iDepth;
	INT	 len;
	pToken = GetToken();
	c = *pToken;
	if (c != '{'){
		pToken = GetToken();
		c = *pToken;
		if (c != '{')
			goto EXIT;		//  ERROR
	}
	iDepth = m_iDepth;
	pAnimation = new Animation;
	
	while(m_iDepth >= iDepth){
		if (NULL == (pToken = GetToken()))
			goto EXIT;
		c = *pToken;
		
		if (c == '{'){
			//  Get slave frame name
			INT iDepth2 = m_iDepth;
			INT sizName;

			if (NULL == (pToken = GetToken()))
				goto EXIT;
			len = (INT)strlen(pToken);
			sizName = (len + 4) & 0xfffffffc;
			pAnimation->pSlaveFrameName = new CHAR[sizName];
			strcpy_s(pAnimation->pSlaveFrameName, sizName,pToken);
			do {
				if (NULL == (pToken = GetToken()))
					goto EXIT;
			}while(m_iDepth >= iDepth2);
		}
		
		if (c == ';'||c == ','||c=='}'||c=='['||c==']')
			continue;

		if (0 == _stricmp(pToken, "AnimationKey")){
			INT keyType;
			INT iDepth2;
			pToken = GetToken();
			if (*pToken != '{'){
				if (NULL == (pToken = GetToken()))
					goto EXIT;
				if (*pToken != '{')
					goto EXIT;
			}
			iDepth2 = m_iDepth;

			if (!GetNumericToken(&pToken,NULL,&keyType)){
				goto EXIT;
			}

			switch(keyType){
				case	0:	//  Rotation
					hr = Get4DAnimationKeys(&pAnimation->pRotations,&pAnimation->numRotationKeys);
					break;
				case	1:	//  Scaling
					hr = Get3DAnimationKeys(&pAnimation->pScalings,&pAnimation->numScalingKeys);
					break;
				case	2:	//  Position
					hr = Get3DAnimationKeys(&pAnimation->pPositions,&pAnimation->numPositionKeys);
					break;
			}
			if (FAILED(hr))
				goto EXIT;
			hr = E_FAIL;
			do {
				if (NULL == (pToken = GetToken()))
					goto EXIT;
			}while(m_iDepth >= iDepth2);
		}else if (*pToken == '{'){
			//  unknown section
			INT iDepth2 = m_iDepth;
			do {
				if (NULL == (pToken = GetToken()))
					goto EXIT;
			}while(m_iDepth >= iDepth2);
		}
	}
	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		*ppAnimation = pAnimation;
	}else{
		SAFE_DELETE(pAnimation);
	}
	return hr;
}


//
//	Get quaternion keys
//
HRESULT CXFileParser::Get4DAnimationKeys(QuaternionKeyFrame **ppAnimationKeys, INT *pCount){
	HRESULT hr = E_FAIL;
	CHAR	*pToken;
	QuaternionKeyFrame *pAnimationKeys = NULL;
	QuaternionKeyFrame key;
	INT numKeyFrames=0,iTmp;
	FLOAT	fTmp;
	INT	i;
	FLOAT	v[4];

	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		if (pToken == NULL || !IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	numKeyFrames = iTmp;

	pAnimationKeys = new QuaternionKeyFrame[numKeyFrames];

	for (i = 0; i < numKeyFrames; ++i){
		int j;
		while(!GetNumericToken(&pToken,NULL,&iTmp)){
			if (pToken == NULL || !IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		key.iFrame = iTmp;

		while(!GetNumericToken(&pToken,NULL,&iTmp)){
			if (pToken == NULL || !IS_SEPARATOR(*pToken))
				goto EXIT;
		}

		if (iTmp != 4)
			goto EXIT;

		for (j = 0; j < iTmp ; ++j){

			while(!GetNumericToken(&pToken,&fTmp,NULL)){
				if (pToken == NULL || !IS_SEPARATOR(*pToken))
					goto EXIT;
			}
			v[j] = fTmp;

		}

		key.keyQuaternion = DirectX::XMVectorSet(v[1],v[2],v[3],v[0]);
		j = i;
		//  insert new key
		while (j > 0){
			if (pAnimationKeys[j-1].iFrame < key.iFrame)
				break;
			pAnimationKeys[j] = pAnimationKeys[j-1];
			--j;
		}
		pAnimationKeys[j] = key;
	}
	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		*pCount = numKeyFrames;
		*ppAnimationKeys = pAnimationKeys;
	}else{
		SAFE_DELETE(pAnimationKeys);
	}
	return hr;
}

//
//	Get quaternion keys
//
HRESULT CXFileParser::Get3DAnimationKeys(Vector3KeyFrame **ppAnimationKeys, INT *pCount){
	HRESULT hr = E_FAIL;
	CHAR *pToken;
	Vector3KeyFrame *pAnimationKeys = NULL;
	Vector3KeyFrame key;
	INT numKeyFrames=0,iTmp;
	FLOAT fTmp;
	INT	i;
	FLOAT	v[3];

	while(!GetNumericToken(&pToken,NULL,&iTmp)){
		if (pToken == NULL || !IS_SEPARATOR(*pToken))
			goto EXIT;
	}
	numKeyFrames = iTmp;

	pAnimationKeys = new Vector3KeyFrame[numKeyFrames];

	for (i = 0; i < numKeyFrames; ++i){
		int j;

		while(!GetNumericToken(&pToken,NULL,&iTmp)){
			if (pToken == NULL || !IS_SEPARATOR(*pToken))
				goto EXIT;
		}
		key.iFrame = iTmp;

		while(!GetNumericToken(&pToken,NULL,&iTmp)){
			if (pToken == NULL || !IS_SEPARATOR(*pToken))
				goto EXIT;
		}

		if (iTmp != 3)
			goto EXIT;

		for (j = 0; j < iTmp ; ++j){

			while(!GetNumericToken(&pToken,&fTmp,NULL)){
				if (pToken == NULL || !IS_SEPARATOR(*pToken))
					goto EXIT;
			}
			v[j] = fTmp;

		}
		key.keyVector3.x = v[0];
		key.keyVector3.y = v[1];
		key.keyVector3.z = v[2];
		j = i;
		//  insert new key
		while (j > 0){
			if (pAnimationKeys[j-1].iFrame < key.iFrame)
				break;
			pAnimationKeys[j] = pAnimationKeys[j-1];
			--j;
		}
		pAnimationKeys[j] = key;
	}
	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		*pCount = numKeyFrames;
		*ppAnimationKeys = pAnimationKeys;
	}else{
		SAFE_DELETE(pAnimationKeys);
	}
	return hr;
}

//
//	Sort Face By Material Id
//
HRESULT CXFileParser::SortFacesByMaterial(MeshContainer *pMeshContainer,TempMesh *pTMesh, Mesh *pMesh){
	HRESULT		hr = E_FAIL;
	USHORT		*pNewIndices = NULL;
	INT			numFaces;
	MaterialAttributeRange *pAttribute = NULL;

	if (pTMesh == NULL){
		hr = S_OK;
		goto EXIT;
	}

	if (pTMesh->numOriginalFaces != pTMesh->numMaterialList){
		if (pTMesh->numOriginalFaces < pTMesh->numMaterialList){
			goto EXIT;
		}
		//    We do this because there are some XFile supplies 
		//  smaller MaterialList like Tiger.x 
		INT	*pTempMaterialList = new INT[pTMesh->numOriginalFaces];
		INT i;
		for (i = 0; i < pTMesh->numMaterialList ; ++i){
			pTempMaterialList[i] = pTMesh->pMaterialList[i];
		}
		while(i < pTMesh->numOriginalFaces){
			pTempMaterialList[i++] = 0;	//  assume rest of the face's material is zero
		}
		SAFE_DELETE(pTMesh->pMaterialList);
		pTMesh->pMaterialList = pTempMaterialList;
	}

	numFaces = pTMesh->numIndices;

	pAttribute = new MaterialAttributeRange[pMeshContainer->dwNumMaterials];

	if (pMeshContainer->dwNumMaterials == 1){
		pAttribute[0].FaceStart = 0;
		pAttribute[0].FaceCount = numFaces;
		pAttribute[0].MaterialId = 0;
		pNewIndices = new USHORT[numFaces * 3];
		for (int i = 0; i < numFaces*3 ; ++i){
			pNewIndices[i] = (USHORT)pTMesh->pTempIndices[i];
		}
	}else if (pTMesh->pMaterialList == NULL){
		goto EXIT;
	}else if (pTMesh->numIndices == pTMesh->numMaterialList){
		INT k = 0;
		int	startFace, faceCount;
		if (numFaces <= 0)
			goto EXIT;
		pNewIndices = new USHORT[numFaces * 3];

		for (DWORD mat = 0; mat < pMeshContainer->dwNumMaterials ; ++mat){
			faceCount = 0;
			startFace = pTMesh->numIndices;

			for (int j = 0 ; j < numFaces ; ++j){
				if (pTMesh->pMaterialList[j]== mat){
					pNewIndices[k*3  ] = (USHORT)pTMesh->pTempIndices[j*3  ];
					pNewIndices[k*3+1] = (USHORT)pTMesh->pTempIndices[j*3+1];
					pNewIndices[k*3+2] = (USHORT)pTMesh->pTempIndices[j*3+2];
					if (k < startFace)
						startFace = k;
					++faceCount;
					++k;
				}
			}
			pAttribute[mat].FaceStart = startFace;
			pAttribute[mat].FaceCount = faceCount;
			pAttribute[mat].MaterialId = mat;
		}
	}else{
		//  if the original mesh contains non triangle polygons.
		INT k = 0;
		INT	startFace, faceCount;
		INT iTmp,iNum;
		if (numFaces <= 0)
			goto EXIT;
		pNewIndices = new USHORT[numFaces * 3];

		for (DWORD mat = 0; mat < pMeshContainer->dwNumMaterials ; ++mat){
			faceCount = 0;
			startFace = pTMesh->numIndices;
			
			for (int j = 0 ; j < pTMesh->numMaterialList ; ++j){
				if (pTMesh->pMaterialList[j]== mat){
					iTmp = pTMesh->pFaceReference[j*2];
					iNum = pTMesh->pFaceReference[j*2 + 1];
					for (int i = 0; i < iNum ; ++i){
						pNewIndices[k*3  ] = (USHORT)pTMesh->pTempIndices[iTmp*3  ];
						pNewIndices[k*3+1] = (USHORT)pTMesh->pTempIndices[iTmp*3+1];
						pNewIndices[k*3+2] = (USHORT)pTMesh->pTempIndices[iTmp*3+2];
						if (k < startFace)
							startFace = k;
						++faceCount;
						++iTmp;
						++k;
					}
				}
			}
			pAttribute[mat].FaceStart = startFace;
			pAttribute[mat].FaceCount = faceCount;
			pAttribute[mat].MaterialId = mat;
		}

	}
	SAFE_DELETE(pTMesh->pMaterialList);
	pTMesh->pMaterialList = new INT[numFaces];
	for (DWORD i = 0; i < pMeshContainer->dwNumMaterials ; ++i){
		int start, end, id;
		start = pAttribute[i].FaceStart;
		end = pAttribute[i].FaceCount + start;
		id = pAttribute[i].MaterialId;
		for (int j = start; j < end ; ++j){
			pTMesh->pMaterialList[j] = id;
		}
	}

	hr = S_OK;
EXIT:
	if (SUCCEEDED(hr)){
		pMesh->pIndices = pNewIndices;
		pMesh->pMaterialAttributeRange = pAttribute;
		pMesh->numIndices = numFaces;
	}else{
		SAFE_DELETE(pAttribute);
		SAFE_DELETE(pNewIndices);
	}
	return hr;
}


//
//	Organize Vertex
//
HRESULT CXFileParser::PrepareVertices(MeshContainer *pMeshContainer,TempMesh *pTMesh, Mesh *pMesh){
	HRESULT hr=E_FAIL;
	INT	numVertices = pTMesh->numVertices;
	DWORD *pBuffer = NULL;

	//  Prepare vertex format
	int offset = 0;
	int pos = -1, norm = -1, weight = -1, boneindex = -1, texcoord = -1;
	pMesh->dwFVF = Mesh::FVF_XYZ;
	
	//  locate the postion
	pos = offset;
	offset += sizeof(FLOAT)*3;

	//  locate the weights
	switch(pTMesh->numMaxSkinWeightsPerVertex){
	case	1:
		weight = offset;
		offset += sizeof(FLOAT);
		pMesh->dwFVF |= Mesh::FVF_WEIGHT1;	break;
	case	2:
		weight = offset;
		offset += sizeof(FLOAT)*2;
		pMesh->dwFVF |= Mesh::FVF_WEIGHT2;	break;
	case	3:
		weight = offset;
		offset += sizeof(FLOAT)*3;
		pMesh->dwFVF |= Mesh::FVF_WEIGHT3;	break;
	case	4:
		weight = offset;
		offset += sizeof(FLOAT)*4;
		pMesh->dwFVF |= Mesh::FVF_WEIGHT4;	break;
	}
	//  locate the index
	if (weight > 0){
		boneindex = offset;
		offset += sizeof(BYTE)*4;
		pMesh->dwFVF |= Mesh::FVF_INDEXB4;
	}

	//  locate the normal
	if (pTMesh->pNormals != NULL){
		pMesh->dwFVF |= Mesh::FVF_NORMAL;
		norm = offset;
		offset += sizeof(FLOAT)*3;
	}

	//  locate the texture coords
	if (pTMesh->pTexCoords != NULL){
		texcoord = offset;
		offset += sizeof(FLOAT)*2;
		pMesh->dwFVF |= Mesh::FVF_TEX1;
	}

	INT	sizBuffer = ( offset+3 ) & 0xfffffffcL;
	pMesh->iVertexStride = sizBuffer; 
	sizBuffer = sizBuffer * pTMesh->numVertices;
	pBuffer = new DWORD[sizBuffer>>2];

	//  copy position
	BYTE *pVoid = (BYTE*)pBuffer;
	INT stride = pMesh->iVertexStride;
	for (int i = 0; i < numVertices ; ++i){
		FLOAT *pTmp = (FLOAT*)(pVoid+pos);
		*pTmp++ = pTMesh->pPositions[i].x;
		*pTmp++ = pTMesh->pPositions[i].y;
		*pTmp++ = pTMesh->pPositions[i].z;
		pVoid += stride;
	}
	//	copy normals
	if (pTMesh->pNormals != NULL && norm >= 0){
		pVoid = (BYTE*)pBuffer;
		for (int i = 0; i < numVertices ; ++i){
			FLOAT *pTmp = (FLOAT*)(pVoid+norm);
			*pTmp++ = pTMesh->pNormals[i].x;
			*pTmp++ = pTMesh->pNormals[i].y;
			*pTmp++ = pTMesh->pNormals[i].z;
			pVoid += stride;
		}
	}
	
	//	copy tex coords
	if (pTMesh->pTexCoords != NULL && norm >= 0){
		pVoid = (BYTE*)pBuffer;
		for (int i = 0; i < numVertices ; ++i){
			FLOAT *pTmp = (FLOAT*)(pVoid+texcoord);
			*pTmp++ = pTMesh->pTexCoords[i].x;
			*pTmp++ = pTMesh->pTexCoords[i].y;
			pVoid += stride;
		}
	}

	if (weight> 0){
		hr = PrepareWeights(pMeshContainer,pTMesh,(BYTE*)pBuffer,weight,stride);
		if (FAILED(hr))
			goto EXIT;
	}
	hr = S_OK;
EXIT:
	if (FAILED(hr)){
		SAFE_DELETE(pBuffer);
		pMesh->numVertices = NULL;
	}else{
		pMesh->pVertices = (VOID*)pBuffer;
		pMesh->numVertices = pTMesh->numVertices;
	}
	return hr;
}

//
//	Insert weights into vertices
//
HRESULT CXFileParser::PrepareWeights(MeshContainer *pMeshContainer, TempMesh *pTMesh, BYTE* pBuffer, INT weightOffset, INT stride){
	HRESULT hr = E_FAIL;
	BYTE	*pTmp = NULL;
	INT numWeight=pTMesh->numMaxSkinWeightsPerVertex;
	INT boneindex = numWeight * sizeof(FLOAT) + weightOffset;
	hr = CheckBoneId(pTMesh);
	if (FAILED(hr))
		goto EXIT;
	hr = E_FAIL;

	//  clear index;
	pTmp = pBuffer;
	for (int i = 0; i < pTMesh->numVertices ; ++i){
		*(ULONG*)(pTmp + boneindex) = 0x00000000L;
		pTmp += stride;
	}
	//  clear weights
	pTmp = pBuffer;
	for (int i = 0; i < pTMesh->numVertices ; ++i){
		FLOAT *fpTmp = (FLOAT*)(pTmp+weightOffset);
		for (int j = 0; j < numWeight ; ++j)
			*fpTmp++ = -FLT_MAX;
		pTmp += stride;
	}

	hr = E_FAIL;

	//  Organize weights;
	{
		SkinWeights *pWeights;
		INT	index;
		INT boneId=0;
		FLOAT weight;
		INT maxWeights = pTMesh->numMaxSkinWeightsPerVertex;
		XMMATRIX **ppMatrices = NULL;
		for (int k = 0; k < pTMesh->numSkinWeights ; ++k){
			pWeights = pTMesh->ppSkinWeights[k];
			for (int i = 0; i < pWeights->numWeights ; ++i){
				int j;
				index = pWeights->pIndices[i];
				weight = pWeights->pWeights[i];
				pTmp = pBuffer + (stride*index);
				FLOAT *fpTmp = (FLOAT *)(pTmp + weightOffset);
				BYTE  *bpTmp = (BYTE*)(pTmp + boneindex);
				for (j = 0 ; j < maxWeights ; ++j){
					if (*fpTmp == -FLT_MAX){
						*fpTmp = weight;
						*bpTmp = (BYTE)(boneId&0xff);
						break;
					}
					bpTmp++;
					fpTmp++;
				}
				if (j >= maxWeights)
					goto EXIT;
			}
			boneId++;
		}

		//  prepare bone offset matrices.
		pMeshContainer->numBones = pTMesh->numSkinWeights;
		pMeshContainer->pBoneOffsetMatrices = (XMMATRIX*)_mm_malloc(sizeof(XMMATRIX)*pMeshContainer->numBones,16);//new XMMATRIX[pMeshContainer->numBones];
		for (index = 0; index < pTMesh->numSkinWeights ; ++index){
			pMeshContainer->pBoneOffsetMatrices[index] = pTMesh->ppSkinWeights[index]->matOffset;
		}
		//  Make the rest of matrixpointers to null
		for (;index < pMeshContainer->numBones ; ++index){
			pMeshContainer->pBoneOffsetMatrices[index] = DirectX::XMMatrixIdentity();
		}
		//  prepare bone matrix pointers
		ppMatrices = new XMMATRIX*[this->m_iNumFrames];
		for (int index = 0; index < m_iNumFrames ; ++index){
			ppMatrices[index] = &m_ppFlattenFramePointers[index]->CombinedMatrix;
		}

		pMeshContainer->ppBoneMatrixPointers = new XMMATRIX*[pMeshContainer->numBones];
		for (index = 0; index < pTMesh->numSkinWeights ; ++index){
			pWeights = pTMesh->ppSkinWeights[index];
			pMeshContainer->ppBoneMatrixPointers[index] = ppMatrices[pWeights->iFrameId];
		}
		//  Make the rest of matrixpointers to null
		for (;index < pMeshContainer->numBones ; ++index){
			pMeshContainer->ppBoneMatrixPointers[index] = NULL;
		}
		SAFE_DELETE_ARRAY(ppMatrices);
	}

	//  clear unused weights
	pTmp = pBuffer;
	for (int i = 0; i < pTMesh->numVertices ; ++i){
		FLOAT *fpTmp = (FLOAT*)(pTmp+weightOffset);
		BYTE	*bpTmp = (BYTE*)(pTmp + boneindex);
		for (int j = 0; j < numWeight ; ++j){
			if (*fpTmp == -FLT_MAX){
				*fpTmp = 0;
				*bpTmp = 0;
			}
			fpTmp++;
			bpTmp++;
		}
		pTmp += stride;
	}
	hr = S_OK;
EXIT:
	return hr;
}

//
//	Check frame reference from a bone,
//	and convert the name to an index of 
//  array of frames.
//
HRESULT CXFileParser::CheckBoneId(TempMesh *pTMesh){
	HRESULT hr = E_FAIL;
	SkinWeights *pWeights = NULL;
	INT	index;
	for (index = 0; index < pTMesh->numSkinWeights ; ++index){
		INT id = 0;
		pWeights = pTMesh->ppSkinWeights[index];
		pWeights->iFrameId = INT_MAX;
		for (id = 0; id < m_iNumFrames ; ++id){
			if (0 == strcmp(pWeights->pBoneName,m_ppFlattenFramePointers[id]->pName)){
				pWeights->iFrameId = id;
				break;
			}
		}
		if (id >= m_iNumFrames)
			goto EXIT;

	}
	hr = S_OK;
EXIT:
	
	return hr;
}

//
//	Organize Mesh Container
//
HRESULT CXFileParser::OrganizeMeshContainer(MeshContainer *pMeshContainer, TempMesh *pTMesh){
	HRESULT hr = E_FAIL;
	Mesh	*pMesh = new Mesh;
	SortFacesByMaterial(pMeshContainer,pTMesh, pMesh);
	PrepareVertices(pMeshContainer,pTMesh, pMesh);

	pMeshContainer->pMeshData = pMesh;
	return hr;
}

//
//	Organize loaded meshes
//
HRESULT CXFileParser::OrganizeLoadedMeshes(){
	HRESULT hr = E_FAIL;
	TempMesh *pMesh;
	MeshContainer *pMeshContainer = NULL;
	std::list<TempMesh*>::iterator itMesh = this->m_pFileReadingContext->pListTempMeshLoaded->begin();
	while(itMesh != m_pFileReadingContext->pListTempMeshLoaded->end()){
		pMesh = *itMesh;
		if (pMesh == NULL)
			goto EXIT;
		pMeshContainer = pMesh->pParent;
		hr = OrganizeMeshContainer(pMeshContainer,pMesh);
		delete *itMesh;
		*itMesh = NULL;
		itMesh = m_pFileReadingContext->pListTempMeshLoaded->erase(itMesh);
	}
	
	hr = S_OK;
EXIT:
	return hr;
}




//
//	Enflat loaded frames
//
HRESULT CXFileParser::EnflatLoadedFrames(){
	HRESULT hr = E_FAIL;
	std::list<MeshFrame*> *pList = m_pFileReadingContext->pListFramesLoaded;
	std::list<MeshFrame*>::iterator itFrames;
	std::list<MeshFrame*>::iterator endFrames;
	INT index = 0;
	if (pList != NULL){
		m_iNumFrames = (INT)pList->size();
		m_ppFlattenFramePointers = new MeshFrame*[m_iNumFrames];
		itFrames = pList->begin();
		endFrames = pList->end();
		while(itFrames != endFrames){
			(*itFrames)->iFrameId = index;		//  iFrameId is a serial number of frame. it's incremental from 0.
			m_ppFlattenFramePointers[index++] = *itFrames;
			*itFrames = NULL;
			itFrames = pList->erase(itFrames);
		}
		delete m_pFileReadingContext->pListFramesLoaded;
		m_pFileReadingContext->pListFramesLoaded = NULL;
	}
	return hr;
}

//
//	Enflat animations
//
HRESULT CXFileParser::EnflatAnimations(){
	HRESULT hr = E_FAIL;
	int numAnimationSets;
	int index = 0;
	AnimationSet	*pNewAnimationSet = NULL;
	std::list<TempAnimationSet*>::iterator itAnimationSets;
	if (this->m_pFileReadingContext->pTempAnimationSetsLoaded->empty()){
		hr = S_OK;
		goto EXIT;
	}
	numAnimationSets = (INT)m_pFileReadingContext->pTempAnimationSetsLoaded->size();
	this->m_iNumAnimationSets = numAnimationSets;
	m_ppAnimationSets = new AnimationSet*[numAnimationSets];
	itAnimationSets = m_pFileReadingContext->pTempAnimationSetsLoaded->begin();
	while(itAnimationSets != m_pFileReadingContext->pTempAnimationSetsLoaded->end()){
		TempAnimationSet *pAnimationSet = *itAnimationSets;
		if (NULL !=  pAnimationSet){
			std::list<Animation*>::iterator itAnim = pAnimationSet->pAnimations->begin();
			INT numAnimations = (INT)(*itAnimationSets)->pAnimations->size();
			INT index2;
			pNewAnimationSet = new AnimationSet;
			pNewAnimationSet->ppAnimations = new Animation*[numAnimations];
			pNewAnimationSet->numAnimations = numAnimations;
			pNewAnimationSet->pName = pAnimationSet->pName;
			pAnimationSet->pName = NULL;
			index2 = 0;
			while(itAnim != pAnimationSet->pAnimations->end()){
				pNewAnimationSet->ppAnimations[index2++] = (*itAnim);
				*itAnim = NULL;
				itAnim = pAnimationSet->pAnimations->erase(itAnim);
			}
			ConfirmAnimationSet(pNewAnimationSet);	//  it never returns error
			m_ppAnimationSets[index] = pNewAnimationSet;
			index++;
			delete *itAnimationSets;
			*itAnimationSets = NULL;
		}
		itAnimationSets = m_pFileReadingContext->pTempAnimationSetsLoaded->erase(itAnimationSets);
	}
	delete m_pFileReadingContext->pTempAnimationSetsLoaded;
	m_pFileReadingContext->pTempAnimationSetsLoaded = NULL;
	hr = S_OK;
EXIT:
	return hr;
}

//
//	Confirm an animatin set
//
HRESULT CXFileParser::ConfirmAnimationSet(AnimationSet *pAnimationSet){
	HRESULT hr = E_FAIL;
	INT	i;
	INT iFrame;
	INT	maxAnimationFrame = 0;
	INT maxKeyCount = 0;
	Animation *pAnimation;
	for (i = 0; i < pAnimationSet->numAnimations ; ++i){
		pAnimation = pAnimationSet->ppAnimations[i];
		if (pAnimation){
			INT keyCount = pAnimation->numPositionKeys;
			if (pAnimation->pPositions&& keyCount > 0){
				if (keyCount > maxKeyCount)
					maxKeyCount = keyCount;
				iFrame = pAnimation->pPositions[pAnimation->numPositionKeys-1].iFrame;
				if (iFrame > maxAnimationFrame)
					maxAnimationFrame = iFrame;
			}
			keyCount = pAnimation->numRotationKeys;
			if (pAnimation->pRotations && keyCount > 0){
				if (keyCount > maxKeyCount)
					maxKeyCount = keyCount;
				iFrame = pAnimation->pRotations[pAnimation->numRotationKeys-1].iFrame;
				if (iFrame > maxAnimationFrame)
					maxAnimationFrame = iFrame;
			}
			keyCount = pAnimation->numScalingKeys;
			if (pAnimation->pScalings && keyCount > 0){
				if (keyCount > maxKeyCount)
					maxKeyCount = keyCount;
				iFrame = pAnimation->pScalings[pAnimation->numScalingKeys-1].iFrame;
				if (iFrame > maxAnimationFrame)
					maxAnimationFrame = iFrame;
			}
		}
		pAnimation->pSlaveMeshFrame = NULL;
		for (int i = 0; i < m_iNumFrames ; ++i){
			if (m_ppFlattenFramePointers[i] != NULL){
				if (0 == strcmp(pAnimation->pSlaveFrameName,m_ppFlattenFramePointers[i]->pName)){
					pAnimation->pSlaveMeshFrame = m_ppFlattenFramePointers[i];
				}
			}
		}
	}
	pAnimationSet->totalFrames = 0;
	if (maxKeyCount > 0){
		pAnimationSet->totalFrames = maxAnimationFrame;
	}
	hr = S_OK;
//EXIT:
	return hr;
}


//
//  ctor for mesh
//
TempMesh::TempMesh(){
	this->pPositions = NULL;
	this->pTempIndices = NULL;
	this->pNormals = NULL;
	this->pTexCoords = NULL;
	this->pMaterialList = NULL;
	this->pParent = NULL;

	this->pFaceReference = NULL;

	ppSkinWeights = NULL;
	numMaxSkinWeightsPerVertex = 0;
	numMaxSkinWeightsPerFace = 0;
	numBones = 0;
}
//
//  destructor for mesh
//
TempMesh::~TempMesh(){
	this->pParent = NULL;
	SAFE_DELETE(this->pPositions);
	SAFE_DELETE(this->pTempIndices);
	SAFE_DELETE(this->pNormals);
	SAFE_DELETE(this->pTexCoords);
	SAFE_DELETE(this->pMaterialList);
	SAFE_DELETE(pFaceReference);
	if (ppSkinWeights != NULL){
		for (int i = 0; i < this->numSkinWeights ; ++i){
			SAFE_DELETE(ppSkinWeights[i]);
		}
		delete [] ppSkinWeights;
		ppSkinWeights = NULL;
	}
}


