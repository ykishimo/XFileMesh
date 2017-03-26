#include "stdafx.h"
#include <stdio.h>
#include <crtdbg.h>
#include "rfc1951.h"

#undef SAFE_DELETE_ARRAY
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }

typedef struct _customhuffmantable{
	WORD	binCode;
	WORD	length;
	WORD	huffmanCode;
}	HuffmanCode;


typedef struct{
	//BYTE *pTree;
	//WORD numElements;
	WORD numMaxBits;
	HuffmanCode	*pCode;
	WORD	numCodes;
}	HuffmanTree;

typedef struct {
	WORD	numLiteralLengthCodes;
	WORD	numDistanceCodes;
	WORD	*pLiteralLengthTree;
	WORD	numElementsOfLiteralLengthTree;
	WORD	*pDistanceTree;
	WORD	numElementsOfDistanceTree;
}	CustomHuffmanTree;

typedef struct _context{
	BYTE	*pSrc;
	__int64	i64BitsRead;
	__int64 i64SizeInBits;
	DWORD	dwSizeInBytes;
	BYTE	*pDest;
	DWORD	dwDestSize;
	DWORD	dwBytesDecompressed;
	HuffmanTree	customLiteralLength;
	HuffmanTree	customDistance;
	HuffmanTree	fixed;
}	DecompressionContext;

//  Prototypes
void	ClearContext(DecompressionContext *pContext);
WORD	GetBits(DecompressionContext *pContext,INT size);
BOOL	ProcessUncompressed(DecompressionContext *pContext);
BOOL	ProcessHuffmanFixed(DecompressionContext *pContext);
BOOL	ProcessHuffmanCustom(DecompressionContext *pContext);
VOID	SetupFixedHuffmanTree(DecompressionContext *pContext);
WORD	GetACodeWithHuffmanTree(DecompressionContext *pContext,HuffmanTree *pHuffmanTree);
BOOL	DecodeWithHuffmanTree(DecompressionContext *pContext,
	HuffmanTree *pLiteralLengthTree,
	HuffmanTree *pDistanceTree
);
VOID DisposeCustomHuffmanTree(DecompressionContext *pContext);

unsigned short binreverse(int code, int length);

//extern HuffmanCode	fixedCode[];
extern HuffmanCode	fixedCodeSorted[];


//
//! Decompress routine.
//
HRESULT DecompressRFC1951Block(BYTE *pCompressed, DWORD dwCompressedSize, BYTE *pDecompressed, DWORD dwDecompressedSize, DWORD *pBytesDecompressed){
	DecompressionContext	context;
	BOOL	bFinal = false;
	WORD	wTmp;
	ClearContext(&context);
	context.pSrc = pCompressed;
	context.pDest = pDecompressed;
	context.i64SizeInBits = (__int64)dwCompressedSize << 8LL;
	context.dwSizeInBytes = dwCompressedSize;
	context.dwDestSize = dwDecompressedSize;
	SetupFixedHuffmanTree(&context);

	while( !bFinal && context.i64BitsRead < context.i64SizeInBits){
		wTmp = GetBits(&context,1);
		if (wTmp == 1){
			bFinal = true;
		}
		wTmp = GetBits(&context,2);	//  
		switch(wTmp){
		case	0:
			ProcessUncompressed(&context);
			break;
		case	1:
			ProcessHuffmanFixed(&context);
			break;
		case	2:
			ProcessHuffmanCustom(&context);
			break;
		default:
			//  Error
			return E_FAIL;
		}
	}
	if (pBytesDecompressed != NULL){
		*pBytesDecompressed = context.dwBytesDecompressed;
	}

	//SAFE_DELETE_ARRAY(context.fixed.pCode);

	return S_OK;
}

#if 1
BOOL SetupHuffmanTree(HuffmanTree *pTree, HuffmanCode *pNode, int num){
	HuffmanCode *pSrc = pNode;
	pTree->pCode = new HuffmanCode[num];
	HuffmanCode *pDest = pTree->pCode;
	WORD length, code;
	int numCodes = 0;
	int maxBits = 0;
	for (int i = 0; i < num ; ++i){
		length = pSrc->length;
		if (length){
			int j = numCodes;
			//code = binreverse((INT)pSrc->huffmanCode,(INT)length);
			code = pSrc->huffmanCode;
			while(j > 0){
				int k = j - 1;
				if (pDest[k].length < length)
					break;
				if (pDest[k].length == length && pDest[k].huffmanCode <= code)
					break;
				pDest[j--] = pDest[k];
			}
			pDest[j].length = length;
			pDest[j].huffmanCode = code;
			pDest[j].binCode = pSrc->binCode;
			++numCodes;
			maxBits = max(maxBits,length);
		}
		++pSrc;
	}
	pTree->numMaxBits = maxBits;
	pTree->numCodes = numCodes;
	return true;
}
#else
INT	HuffmanComp(const void *pA, const void *pB){
	HuffmanCode *p1 = (HuffmanCode*)pA;
	HuffmanCode *p2 = (HuffmanCode*)pB;
	if (p1->length < p2->length)
		return -1;
	if (p1->length > p2->length)
		return 1;
	if (p1->huffmanCode < p2->huffmanCode)
		return -1;
	if (p1->huffmanCode != p2->huffmanCode)
		return 1;
	return 0;
}
BOOL SetupHuffmanTree(HuffmanTree *pTree, HuffmanCode *pNode, int num){
	HuffmanCode *pSrc = pNode;
	pTree->pCode = new HuffmanCode[num];
	HuffmanCode *pDest = pTree->pCode;
	WORD length, code;
	int numCodes = 0;
	int maxBits = 0;
	for (int i = 0; i < num ; ++i){
		length = pSrc->length;
		if (length){
			pDest[numCodes++] = *pSrc;
			maxBits = max(maxBits,length);
		}
		++pSrc;
	}
	qsort(pTree->pCode,numCodes,sizeof(HuffmanCode),HuffmanComp);
	pTree->numMaxBits = maxBits;
	pTree->numCodes = numCodes;
	return true;
}
#endif // 0

VOID SetupFixedHuffmanTree(DecompressionContext *pContext){
	//SetupHuffmanTree(&pContext->fixed,fixedCode,288);
	pContext->fixed.numCodes = 288;
	pContext->fixed.numMaxBits = 9;
	pContext->fixed.pCode = fixedCodeSorted;
}

void	ClearContext(DecompressionContext *pContext){
	pContext->pSrc = NULL;
	pContext->i64BitsRead = 0LL;
	pContext->i64SizeInBits = 0LL;
	pContext->dwSizeInBytes = 0L;
	pContext->pDest = NULL;
	pContext->dwDestSize = 0L;
	pContext->dwBytesDecompressed = 0L;
	pContext->customDistance.numMaxBits = 0;
	pContext->customDistance.pCode = NULL;
	pContext->customDistance.numCodes = 0;
	pContext->customLiteralLength.numMaxBits = 0;
	pContext->customLiteralLength.pCode = NULL;
	pContext->customLiteralLength.numCodes = 0;
	pContext->fixed.numMaxBits = 0;
	pContext->fixed.pCode = NULL;
	pContext->fixed.numCodes = 0;
}

static inline WORD GetABit(DecompressionContext *pContext){
	WORD	w;
	w = pContext->pSrc[(INT)(pContext->i64BitsRead >> 3)];
	return (w >> ((pContext->i64BitsRead++) & 7)) & 1;
}

//
//!  GetBits 
//!  @Note : larger bits means later bits.
//!	Read bits LSB->MSB order
//	Store bits LSB->MSB order
//
WORD	GetBits(DecompressionContext *pContext,INT size){
	WORD	wTmp, w;
	pContext->i64BitsRead += size;
	__int64 i64Current = pContext->i64BitsRead;
	wTmp = 0;
	for (int i = 0; i < size ; ++i){
		wTmp = wTmp << 1;
		--i64Current;
		w = pContext->pSrc[(INT)(i64Current >> 3)];
		wTmp |= (w >> (i64Current & 7)) & 1;
	}
	return wTmp;
}

//! Process uncompressed block
BOOL ProcessUncompressed(DecompressionContext *pContext){
	WORD	size, wTmp;
	
	size = GetBits(pContext,8) & 0xff;
	size |= (GetBits(pContext,8) & 0xff) << 8;

	wTmp = GetBits(pContext,8) & 0xff;
	wTmp |= (GetBits(pContext,8) & 0xff) << 8;

	wTmp = (~wTmp);
	if (wTmp != size)
		return false;	//  error
	for ( int i = 0; i < (INT)size ; ++i){
		wTmp = GetBits(pContext,8);
		*(pContext->pDest + pContext->dwBytesDecompressed)
			= (BYTE)(wTmp & 0xff);
		pContext->dwBytesDecompressed++;
	}
	return true;
}


LONG DecodeLength(DecompressionContext *pContext, WORD baseCode){
	WORD extra;
	WORD w,x,y;
	WORD numExtraBits;
	if (baseCode <= 264){
		return baseCode - 257+3;
	}
	if (baseCode == 285)
		return 258;
	if (baseCode > 285)
		return 0xffff;	//!< error

	w = baseCode - 265;
	x = w >> 2;
	numExtraBits = x + 1;
	y  = (4 << numExtraBits) + 3;
	y += (w & 3) << numExtraBits;
	extra = GetBits(pContext,numExtraBits);
	return y + extra;
}

LONG DecodeDistance(DecompressionContext *pContext, WORD baseCode){
	WORD extra;
	WORD w,x,y;
	WORD numExtraBits;
	if (baseCode <= 3){
		return baseCode +1;
	}
	if (baseCode > 29)
		return 0; //!< error

	w = baseCode - 4;
	x = w >> 1;
	numExtraBits = x + 1;
	y = (2 << numExtraBits) + 1;
	y += (w & 1) << numExtraBits;
	extra = GetBits(pContext,numExtraBits);
	return y + extra;
}

//! Process the block compressed with fixed huffman.
BOOL ProcessHuffmanFixed(DecompressionContext *pContext){
	return DecodeWithHuffmanTree(pContext,&pContext->fixed,NULL);
}

//! reverse the bit order
//! bits are restricted with length from LSB.
unsigned short binreverse(int code, int length){
	unsigned short w = 0;
	int	count = length - 1;
	for (int i = 0; i < length ; ++i){
		w <<= 1;
		w |= code & 1;
		code >>= 1;
	}
	return w;
}

//!  Compare Huffman Code
static inline INT compareCode(HuffmanCode *a, WORD length, WORD code){
	if (a->length < length)
		return -1;
	if (a->length > length)
		return 1;
	if (a->huffmanCode < code)
		return -1;
	if (a->huffmanCode == code)
		return 0;
	return 1;
}

//! Read bits and decode it
WORD GetACodeWithHuffmanTree(DecompressionContext *pContext,HuffmanTree *pHuffmanTree)
{
	WORD maxBits;
	WORD w, result = 0xffff;
	int minBits = 0;
	maxBits = pHuffmanTree->numMaxBits;
	if (pHuffmanTree->pCode != NULL){
		WORD	bits;
		HuffmanCode		*pCode = pHuffmanTree->pCode;
		w = 0;
		minBits = pHuffmanTree->pCode[0].length;
		for (bits = 0 ; bits < minBits ; ++bits){
			w <<= 1;
			w |= GetABit(pContext);
		}
		while(bits <= maxBits){
			//  search codes.
			//	search tree by binary search.
			int left = 0;
			int right = pHuffmanTree->numCodes;
			int x;
			INT comp;
			do{
				x = (left + right) >> 1;
				comp = compareCode(&pCode[x],bits,w);
				if (comp == 0){
					return pCode[x].binCode;
				}else if (comp < 0){
					left = x+1;
				}else{
					right = x;
				}
			}while(left < right);
			w <<= 1;
			w |= GetABit(pContext);
			++bits;
		}
		return 0xffff;
	}
	return 0xffff;
}

//
//! CreateHuffmanTreeTable
//! @breif: Remove 0bit code & sort codes by lengths and code to make it searchable with binary search
//
BOOL CreateHuffmanTreeTable(DecompressionContext *pContext){
	WORD	numCodeLengthCode;
	BYTE	codeLengthsOfTheCodeLength[20];
	static const int itsOrder[]={
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};
	HuffmanTree	CodeLengthsTree={NULL,0,0};
	WORD	numLiteralLengthCodes;
	WORD	numDistanceCodes;
	int *bitLengths = new int[20];
	int *nextCodes  = new int[20];
	BOOL	bSuccess = true;
	numLiteralLengthCodes = GetBits(pContext,5) + 257;
	numDistanceCodes = GetBits(pContext,5) + 1;
	numCodeLengthCode = GetBits(pContext,4) + 4;

	memset((VOID*)codeLengthsOfTheCodeLength,0,sizeof(codeLengthsOfTheCodeLength));

	for (int i = 0; i < numCodeLengthCode ; ++i){
		codeLengthsOfTheCodeLength[ itsOrder[i] ] = (BYTE)GetBits(pContext,3);
	}

	//  Create CodeLengthTree
	HuffmanCode	node[19];
	int maxBits = -1;
	memset((VOID*)bitLengths,0,sizeof(int)*19);
	memset((VOID*)nextCodes,0,sizeof(int)*19);
	memset((VOID*)node,0,sizeof(node));
	for (int i = 0; i <= 18 ; ++i){
		int len = codeLengthsOfTheCodeLength[i];
		node[i].length = len;
		bitLengths[len]++;
		if (maxBits < len)
			maxBits = len;
	}
	int code = 0;
	bitLengths[0] = 0;
	for (int bits = 1; bits <= 18; bits++) {
        code = (code + bitLengths[bits-1]) << 1;
        nextCodes[bits] = code;
    }

	for (int n = 0; n <= 18 ; n++) {
		int len = node[n].length;
		node[n].binCode = (WORD)n;
		if (len != 0) {
			node[n].huffmanCode = nextCodes[len];
			nextCodes[len]++;
        }
    }
	SetupHuffmanTree(&CodeLengthsTree,node,19);

	//	Create LiteralLengthTree
	{
		HuffmanCode	LiteralLengthTree[288];
		WORD	prev, repeat;
		INT		len;
		INT		n;
		int		indvCount = 0;
		int		repeatCount = 0;
		delete []bitLengths;
		delete []nextCodes;
		bitLengths = nextCodes = NULL;
		maxBits = -1;
		prev = 0xffff;
		repeat = 0;
		n = 0;
		while(n < numLiteralLengthCodes){
			LiteralLengthTree[n].binCode = n;
			if (repeat > 0){
				LiteralLengthTree[n++].length = prev;
				repeatCount++;
				--repeat;
			}else{
				len = GetACodeWithHuffmanTree(pContext,&CodeLengthsTree);
				switch(len){
				case	16:	//  Copy the previous code lengths for 3-6 stimes
					repeat = GetBits(pContext,2) + 3;
					break;
				case	17: //  Repeat 0 for 3-10 times.
					prev = 0;
					repeat = GetBits(pContext,3) + 3;
					break;
				case	18:
					prev = 0;
					repeat = GetBits(pContext,7) + 11;
					break;
				default:
					if (repeat > 0)
						return false;	//  somethings wrong
					//
					prev = len;
					repeat = 0;
					LiteralLengthTree[n++].length = len;
					++indvCount;
					if (len > (int)maxBits)
						maxBits = len;
				}
			}

		}
		if (repeat > 0)
			return false;	//  somethings wrong

		bitLengths = new int[maxBits+1];
		nextCodes = new int[maxBits+1];
		memset((void*)bitLengths,0,sizeof(int)*(maxBits+1));
		memset((void*)nextCodes,0,sizeof(int)*(maxBits+1));
		for (int i = 0; i < numLiteralLengthCodes ; ++i){
			bitLengths[LiteralLengthTree[i].length]++;		
		}
		bitLengths[0] = 0;
		code = 0;
		for (int bits = 1; bits <= maxBits; bits++) {
			code = (code + bitLengths[bits-1]) << 1;
			nextCodes[bits] = code;
		}
		
		for (int n = 0; n < numLiteralLengthCodes ; n++) {
			int len = LiteralLengthTree[n].length;
			if (len != 0) {
				LiteralLengthTree[n].huffmanCode = nextCodes[len];
				nextCodes[len]++;
			}
		}

		bSuccess = SetupHuffmanTree(
			&pContext->customLiteralLength,
			LiteralLengthTree,
			numLiteralLengthCodes
		);
		delete []bitLengths;
		delete []nextCodes;
	}
	if (bSuccess){
		//  Create DistanceTree
		HuffmanCode	DistanceTree[36];
		WORD	prev, repeat;
		INT		len;
		INT		n;
		bitLengths = nextCodes = NULL;
		maxBits = -1;
		prev = 0xffff;
		repeat = 0;
		n = 0;
		while(n < numDistanceCodes){
			DistanceTree[n].binCode = n;
			if (repeat > 0){
				DistanceTree[n++].length = prev;
				--repeat;
			}else{
				len = GetACodeWithHuffmanTree(pContext,&CodeLengthsTree);
				switch(len){
				case	16:	//  Copy the previous code lengths for 3-6 stimes
					repeat = GetBits(pContext,2) + 3;
					break;
				case	17: //  Repeat 0 for 3-10 times.
					prev = 0;
					repeat = GetBits(pContext,3) + 3;
					break;
				case	18:
					prev = 0;
					repeat = GetBits(pContext,7) + 11;
					break;
				default:
					if (repeat > 0)
						return false;	//  somethings wrong
					//
					prev = len;
					repeat = 0;
					DistanceTree[n++].length = len;
					if (len > (int)maxBits)
						maxBits = len;
				}

			}
		}
		if (repeat > 0)
			return false;	//  somethings wrong
		bitLengths = new int[maxBits+1];
		nextCodes = new int[maxBits+1];
		memset((void*)bitLengths,0,sizeof(int)*(maxBits+1));
		memset((void*)nextCodes,0,sizeof(int)*(maxBits+1));
		for (int i = 0; i < numDistanceCodes ; ++i){
			bitLengths[DistanceTree[i].length]++;		
		}

		bitLengths[0] = 0;
		code = 0;
		for (int bits = 1; bits <= maxBits; bits++) {
			code = (code + bitLengths[bits-1]) << 1;
			nextCodes[bits] = code;
		}

		for (int n = 0; n < numDistanceCodes ; n++) {
			int len = DistanceTree[n].length;
			if (len != 0) {
				DistanceTree[n].huffmanCode = nextCodes[len];
				nextCodes[len]++;
			}
		}
		bSuccess = SetupHuffmanTree(
			&pContext->customDistance,
			DistanceTree,
			numDistanceCodes
		);

		delete []bitLengths;
		delete []nextCodes;
	}
	SAFE_DELETE_ARRAY(CodeLengthsTree.pCode);
	if (!bSuccess){
		DisposeCustomHuffmanTree(pContext);
	}
	return bSuccess;
}

//! Terminate the trees
VOID DisposeCustomHuffmanTree(DecompressionContext *pContext){
	SAFE_DELETE_ARRAY(pContext->customDistance.pCode);
	pContext->customDistance.numMaxBits = 0;
	pContext->customDistance.numCodes = 0;

	SAFE_DELETE_ARRAY(pContext->customLiteralLength.pCode);
	pContext->customLiteralLength.numMaxBits = 0;
	pContext->customLiteralLength.numCodes = 0;
}

//! Process the block compressed with dynamic huffman.
BOOL ProcessHuffmanCustom(DecompressionContext *pContext){
	BOOL bRet;
	bRet = CreateHuffmanTreeTable(pContext);
	DecodeWithHuffmanTree(pContext,&pContext->customLiteralLength,&pContext->customDistance);
	DisposeCustomHuffmanTree(pContext);
	return true;
}
//! Decode with huffman tree
BOOL DecodeWithHuffmanTree(DecompressionContext *pContext,
	HuffmanTree *pLiteralLengthTree,
	HuffmanTree *pDistanceTree
){
	WORD	w;
	while(true){
		w = GetACodeWithHuffmanTree(pContext,pLiteralLengthTree);

		if (w == 0xffff)	//  Error;
			return false;
		if (w == 256)
			break;	//  Completion
		if (w < 256){
			pContext->pDest[pContext->dwBytesDecompressed] = (BYTE)w;
			pContext->dwBytesDecompressed++;
		}else{
			LONG length = DecodeLength(pContext,w);
			if (pDistanceTree!=NULL)
				w = GetACodeWithHuffmanTree(pContext,pDistanceTree);
			else{
				w = GetBits(pContext,5);
				w = binreverse(w,5);
			}
			LONG distance = DecodeDistance(pContext,w);
			BYTE *pDict = pContext->pDest+pContext->dwBytesDecompressed;
			pDict -= distance;
			for (int i = 0; i < length ; ++i){
				pContext->pDest[pContext->dwBytesDecompressed++] = *pDict++;
			}
		}
	}
	return true;
}

HuffmanCode	fixedCodeSorted[]={
{  0x100 , 7, 0x0 },
{  0x101 , 7, 0x1 },
{  0x102 , 7, 0x2 },
{  0x103 , 7, 0x3 },
{  0x104 , 7, 0x4 },
{  0x105 , 7, 0x5 },
{  0x106 , 7, 0x6 },
{  0x107 , 7, 0x7 },
{  0x108 , 7, 0x8 },
{  0x109 , 7, 0x9 },
{  0x10a , 7, 0xa },
{  0x10b , 7, 0xb },
{  0x10c , 7, 0xc },
{  0x10d , 7, 0xd },
{  0x10e , 7, 0xe },
{  0x10f , 7, 0xf },
{  0x110 , 7, 0x10 },
{  0x111 , 7, 0x11 },
{  0x112 , 7, 0x12 },
{  0x113 , 7, 0x13 },
{  0x114 , 7, 0x14 },
{  0x115 , 7, 0x15 },
{  0x116 , 7, 0x16 },
{  0x117 , 7, 0x17 },
{  0x0 , 8, 0x30 },
{  0x1 , 8, 0x31 },
{  0x2 , 8, 0x32 },
{  0x3 , 8, 0x33 },
{  0x4 , 8, 0x34 },
{  0x5 , 8, 0x35 },
{  0x6 , 8, 0x36 },
{  0x7 , 8, 0x37 },
{  0x8 , 8, 0x38 },
{  0x9 , 8, 0x39 },
{  0xa , 8, 0x3a },
{  0xb , 8, 0x3b },
{  0xc , 8, 0x3c },
{  0xd , 8, 0x3d },
{  0xe , 8, 0x3e },
{  0xf , 8, 0x3f },
{  0x10 , 8, 0x40 },
{  0x11 , 8, 0x41 },
{  0x12 , 8, 0x42 },
{  0x13 , 8, 0x43 },
{  0x14 , 8, 0x44 },
{  0x15 , 8, 0x45 },
{  0x16 , 8, 0x46 },
{  0x17 , 8, 0x47 },
{  0x18 , 8, 0x48 },
{  0x19 , 8, 0x49 },
{  0x1a , 8, 0x4a },
{  0x1b , 8, 0x4b },
{  0x1c , 8, 0x4c },
{  0x1d , 8, 0x4d },
{  0x1e , 8, 0x4e },
{  0x1f , 8, 0x4f },
{  0x20 , 8, 0x50 },
{  0x21 , 8, 0x51 },
{  0x22 , 8, 0x52 },
{  0x23 , 8, 0x53 },
{  0x24 , 8, 0x54 },
{  0x25 , 8, 0x55 },
{  0x26 , 8, 0x56 },
{  0x27 , 8, 0x57 },
{  0x28 , 8, 0x58 },
{  0x29 , 8, 0x59 },
{  0x2a , 8, 0x5a },
{  0x2b , 8, 0x5b },
{  0x2c , 8, 0x5c },
{  0x2d , 8, 0x5d },
{  0x2e , 8, 0x5e },
{  0x2f , 8, 0x5f },
{  0x30 , 8, 0x60 },
{  0x31 , 8, 0x61 },
{  0x32 , 8, 0x62 },
{  0x33 , 8, 0x63 },
{  0x34 , 8, 0x64 },
{  0x35 , 8, 0x65 },
{  0x36 , 8, 0x66 },
{  0x37 , 8, 0x67 },
{  0x38 , 8, 0x68 },
{  0x39 , 8, 0x69 },
{  0x3a , 8, 0x6a },
{  0x3b , 8, 0x6b },
{  0x3c , 8, 0x6c },
{  0x3d , 8, 0x6d },
{  0x3e , 8, 0x6e },
{  0x3f , 8, 0x6f },
{  0x40 , 8, 0x70 },
{  0x41 , 8, 0x71 },
{  0x42 , 8, 0x72 },
{  0x43 , 8, 0x73 },
{  0x44 , 8, 0x74 },
{  0x45 , 8, 0x75 },
{  0x46 , 8, 0x76 },
{  0x47 , 8, 0x77 },
{  0x48 , 8, 0x78 },
{  0x49 , 8, 0x79 },
{  0x4a , 8, 0x7a },
{  0x4b , 8, 0x7b },
{  0x4c , 8, 0x7c },
{  0x4d , 8, 0x7d },
{  0x4e , 8, 0x7e },
{  0x4f , 8, 0x7f },
{  0x50 , 8, 0x80 },
{  0x51 , 8, 0x81 },
{  0x52 , 8, 0x82 },
{  0x53 , 8, 0x83 },
{  0x54 , 8, 0x84 },
{  0x55 , 8, 0x85 },
{  0x56 , 8, 0x86 },
{  0x57 , 8, 0x87 },
{  0x58 , 8, 0x88 },
{  0x59 , 8, 0x89 },
{  0x5a , 8, 0x8a },
{  0x5b , 8, 0x8b },
{  0x5c , 8, 0x8c },
{  0x5d , 8, 0x8d },
{  0x5e , 8, 0x8e },
{  0x5f , 8, 0x8f },
{  0x60 , 8, 0x90 },
{  0x61 , 8, 0x91 },
{  0x62 , 8, 0x92 },
{  0x63 , 8, 0x93 },
{  0x64 , 8, 0x94 },
{  0x65 , 8, 0x95 },
{  0x66 , 8, 0x96 },
{  0x67 , 8, 0x97 },
{  0x68 , 8, 0x98 },
{  0x69 , 8, 0x99 },
{  0x6a , 8, 0x9a },
{  0x6b , 8, 0x9b },
{  0x6c , 8, 0x9c },
{  0x6d , 8, 0x9d },
{  0x6e , 8, 0x9e },
{  0x6f , 8, 0x9f },
{  0x70 , 8, 0xa0 },
{  0x71 , 8, 0xa1 },
{  0x72 , 8, 0xa2 },
{  0x73 , 8, 0xa3 },
{  0x74 , 8, 0xa4 },
{  0x75 , 8, 0xa5 },
{  0x76 , 8, 0xa6 },
{  0x77 , 8, 0xa7 },
{  0x78 , 8, 0xa8 },
{  0x79 , 8, 0xa9 },
{  0x7a , 8, 0xaa },
{  0x7b , 8, 0xab },
{  0x7c , 8, 0xac },
{  0x7d , 8, 0xad },
{  0x7e , 8, 0xae },
{  0x7f , 8, 0xaf },
{  0x80 , 8, 0xb0 },
{  0x81 , 8, 0xb1 },
{  0x82 , 8, 0xb2 },
{  0x83 , 8, 0xb3 },
{  0x84 , 8, 0xb4 },
{  0x85 , 8, 0xb5 },
{  0x86 , 8, 0xb6 },
{  0x87 , 8, 0xb7 },
{  0x88 , 8, 0xb8 },
{  0x89 , 8, 0xb9 },
{  0x8a , 8, 0xba },
{  0x8b , 8, 0xbb },
{  0x8c , 8, 0xbc },
{  0x8d , 8, 0xbd },
{  0x8e , 8, 0xbe },
{  0x8f , 8, 0xbf },
{  0x118 , 8, 0xc0 },
{  0x119 , 8, 0xc1 },
{  0x11a , 8, 0xc2 },
{  0x11b , 8, 0xc3 },
{  0x11c , 8, 0xc4 },
{  0x11d , 8, 0xc5 },
{  0x11e , 8, 0xc6 },
{  0x11f , 8, 0xc7 },
{  0x90 , 9, 0x190 },
{  0x91 , 9, 0x191 },
{  0x92 , 9, 0x192 },
{  0x93 , 9, 0x193 },
{  0x94 , 9, 0x194 },
{  0x95 , 9, 0x195 },
{  0x96 , 9, 0x196 },
{  0x97 , 9, 0x197 },
{  0x98 , 9, 0x198 },
{  0x99 , 9, 0x199 },
{  0x9a , 9, 0x19a },
{  0x9b , 9, 0x19b },
{  0x9c , 9, 0x19c },
{  0x9d , 9, 0x19d },
{  0x9e , 9, 0x19e },
{  0x9f , 9, 0x19f },
{  0xa0 , 9, 0x1a0 },
{  0xa1 , 9, 0x1a1 },
{  0xa2 , 9, 0x1a2 },
{  0xa3 , 9, 0x1a3 },
{  0xa4 , 9, 0x1a4 },
{  0xa5 , 9, 0x1a5 },
{  0xa6 , 9, 0x1a6 },
{  0xa7 , 9, 0x1a7 },
{  0xa8 , 9, 0x1a8 },
{  0xa9 , 9, 0x1a9 },
{  0xaa , 9, 0x1aa },
{  0xab , 9, 0x1ab },
{  0xac , 9, 0x1ac },
{  0xad , 9, 0x1ad },
{  0xae , 9, 0x1ae },
{  0xaf , 9, 0x1af },
{  0xb0 , 9, 0x1b0 },
{  0xb1 , 9, 0x1b1 },
{  0xb2 , 9, 0x1b2 },
{  0xb3 , 9, 0x1b3 },
{  0xb4 , 9, 0x1b4 },
{  0xb5 , 9, 0x1b5 },
{  0xb6 , 9, 0x1b6 },
{  0xb7 , 9, 0x1b7 },
{  0xb8 , 9, 0x1b8 },
{  0xb9 , 9, 0x1b9 },
{  0xba , 9, 0x1ba },
{  0xbb , 9, 0x1bb },
{  0xbc , 9, 0x1bc },
{  0xbd , 9, 0x1bd },
{  0xbe , 9, 0x1be },
{  0xbf , 9, 0x1bf },
{  0xc0 , 9, 0x1c0 },
{  0xc1 , 9, 0x1c1 },
{  0xc2 , 9, 0x1c2 },
{  0xc3 , 9, 0x1c3 },
{  0xc4 , 9, 0x1c4 },
{  0xc5 , 9, 0x1c5 },
{  0xc6 , 9, 0x1c6 },
{  0xc7 , 9, 0x1c7 },
{  0xc8 , 9, 0x1c8 },
{  0xc9 , 9, 0x1c9 },
{  0xca , 9, 0x1ca },
{  0xcb , 9, 0x1cb },
{  0xcc , 9, 0x1cc },
{  0xcd , 9, 0x1cd },
{  0xce , 9, 0x1ce },
{  0xcf , 9, 0x1cf },
{  0xd0 , 9, 0x1d0 },
{  0xd1 , 9, 0x1d1 },
{  0xd2 , 9, 0x1d2 },
{  0xd3 , 9, 0x1d3 },
{  0xd4 , 9, 0x1d4 },
{  0xd5 , 9, 0x1d5 },
{  0xd6 , 9, 0x1d6 },
{  0xd7 , 9, 0x1d7 },
{  0xd8 , 9, 0x1d8 },
{  0xd9 , 9, 0x1d9 },
{  0xda , 9, 0x1da },
{  0xdb , 9, 0x1db },
{  0xdc , 9, 0x1dc },
{  0xdd , 9, 0x1dd },
{  0xde , 9, 0x1de },
{  0xdf , 9, 0x1df },
{  0xe0 , 9, 0x1e0 },
{  0xe1 , 9, 0x1e1 },
{  0xe2 , 9, 0x1e2 },
{  0xe3 , 9, 0x1e3 },
{  0xe4 , 9, 0x1e4 },
{  0xe5 , 9, 0x1e5 },
{  0xe6 , 9, 0x1e6 },
{  0xe7 , 9, 0x1e7 },
{  0xe8 , 9, 0x1e8 },
{  0xe9 , 9, 0x1e9 },
{  0xea , 9, 0x1ea },
{  0xeb , 9, 0x1eb },
{  0xec , 9, 0x1ec },
{  0xed , 9, 0x1ed },
{  0xee , 9, 0x1ee },
{  0xef , 9, 0x1ef },
{  0xf0 , 9, 0x1f0 },
{  0xf1 , 9, 0x1f1 },
{  0xf2 , 9, 0x1f2 },
{  0xf3 , 9, 0x1f3 },
{  0xf4 , 9, 0x1f4 },
{  0xf5 , 9, 0x1f5 },
{  0xf6 , 9, 0x1f6 },
{  0xf7 , 9, 0x1f7 },
{  0xf8 , 9, 0x1f8 },
{  0xf9 , 9, 0x1f9 },
{  0xfa , 9, 0x1fa },
{  0xfb , 9, 0x1fb },
{  0xfc , 9, 0x1fc },
{  0xfd , 9, 0x1fd },
{  0xfe , 9, 0x1fe },
{  0xff , 9, 0x1ff },
};
#if 0
HuffmanCode	fixedCode[]={
{ 0x000,  8, 0x30 }, //   00110000
{ 0x001,  8, 0x31 }, //   00110001
{ 0x002,  8, 0x32 }, //   00110010
{ 0x003,  8, 0x33 }, //   00110011
{ 0x004,  8, 0x34 }, //   00110100
{ 0x005,  8, 0x35 }, //   00110101
{ 0x006,  8, 0x36 }, //   00110110
{ 0x007,  8, 0x37 }, //   00110111
{ 0x008,  8, 0x38 }, //   00111000
{ 0x009,  8, 0x39 }, //   00111001
{ 0x00a,  8, 0x3a }, //   00111010
{ 0x00b,  8, 0x3b }, //   00111011
{ 0x00c,  8, 0x3c }, //   00111100
{ 0x00d,  8, 0x3d }, //   00111101
{ 0x00e,  8, 0x3e }, //   00111110
{ 0x00f,  8, 0x3f }, //   00111111
{ 0x010,  8, 0x40 }, //   01000000
{ 0x011,  8, 0x41 }, //   01000001
{ 0x012,  8, 0x42 }, //   01000010
{ 0x013,  8, 0x43 }, //   01000011
{ 0x014,  8, 0x44 }, //   01000100
{ 0x015,  8, 0x45 }, //   01000101
{ 0x016,  8, 0x46 }, //   01000110
{ 0x017,  8, 0x47 }, //   01000111
{ 0x018,  8, 0x48 }, //   01001000
{ 0x019,  8, 0x49 }, //   01001001
{ 0x01a,  8, 0x4a }, //   01001010
{ 0x01b,  8, 0x4b }, //   01001011
{ 0x01c,  8, 0x4c }, //   01001100
{ 0x01d,  8, 0x4d }, //   01001101
{ 0x01e,  8, 0x4e }, //   01001110
{ 0x01f,  8, 0x4f }, //   01001111
{ 0x020,  8, 0x50 }, //   01010000
{ 0x021,  8, 0x51 }, //   01010001
{ 0x022,  8, 0x52 }, //   01010010
{ 0x023,  8, 0x53 }, //   01010011
{ 0x024,  8, 0x54 }, //   01010100
{ 0x025,  8, 0x55 }, //   01010101
{ 0x026,  8, 0x56 }, //   01010110
{ 0x027,  8, 0x57 }, //   01010111
{ 0x028,  8, 0x58 }, //   01011000
{ 0x029,  8, 0x59 }, //   01011001
{ 0x02a,  8, 0x5a }, //   01011010
{ 0x02b,  8, 0x5b }, //   01011011
{ 0x02c,  8, 0x5c }, //   01011100
{ 0x02d,  8, 0x5d }, //   01011101
{ 0x02e,  8, 0x5e }, //   01011110
{ 0x02f,  8, 0x5f }, //   01011111
{ 0x030,  8, 0x60 }, //   01100000
{ 0x031,  8, 0x61 }, //   01100001
{ 0x032,  8, 0x62 }, //   01100010
{ 0x033,  8, 0x63 }, //   01100011
{ 0x034,  8, 0x64 }, //   01100100
{ 0x035,  8, 0x65 }, //   01100101
{ 0x036,  8, 0x66 }, //   01100110
{ 0x037,  8, 0x67 }, //   01100111
{ 0x038,  8, 0x68 }, //   01101000
{ 0x039,  8, 0x69 }, //   01101001
{ 0x03a,  8, 0x6a }, //   01101010
{ 0x03b,  8, 0x6b }, //   01101011
{ 0x03c,  8, 0x6c }, //   01101100
{ 0x03d,  8, 0x6d }, //   01101101
{ 0x03e,  8, 0x6e }, //   01101110
{ 0x03f,  8, 0x6f }, //   01101111
{ 0x040,  8, 0x70 }, //   01110000
{ 0x041,  8, 0x71 }, //   01110001
{ 0x042,  8, 0x72 }, //   01110010
{ 0x043,  8, 0x73 }, //   01110011
{ 0x044,  8, 0x74 }, //   01110100
{ 0x045,  8, 0x75 }, //   01110101
{ 0x046,  8, 0x76 }, //   01110110
{ 0x047,  8, 0x77 }, //   01110111
{ 0x048,  8, 0x78 }, //   01111000
{ 0x049,  8, 0x79 }, //   01111001
{ 0x04a,  8, 0x7a }, //   01111010
{ 0x04b,  8, 0x7b }, //   01111011
{ 0x04c,  8, 0x7c }, //   01111100
{ 0x04d,  8, 0x7d }, //   01111101
{ 0x04e,  8, 0x7e }, //   01111110
{ 0x04f,  8, 0x7f }, //   01111111
{ 0x050,  8, 0x80 }, //   10000000
{ 0x051,  8, 0x81 }, //   10000001
{ 0x052,  8, 0x82 }, //   10000010
{ 0x053,  8, 0x83 }, //   10000011
{ 0x054,  8, 0x84 }, //   10000100
{ 0x055,  8, 0x85 }, //   10000101
{ 0x056,  8, 0x86 }, //   10000110
{ 0x057,  8, 0x87 }, //   10000111
{ 0x058,  8, 0x88 }, //   10001000
{ 0x059,  8, 0x89 }, //   10001001
{ 0x05a,  8, 0x8a }, //   10001010
{ 0x05b,  8, 0x8b }, //   10001011
{ 0x05c,  8, 0x8c }, //   10001100
{ 0x05d,  8, 0x8d }, //   10001101
{ 0x05e,  8, 0x8e }, //   10001110
{ 0x05f,  8, 0x8f }, //   10001111
{ 0x060,  8, 0x90 }, //   10010000
{ 0x061,  8, 0x91 }, //   10010001
{ 0x062,  8, 0x92 }, //   10010010
{ 0x063,  8, 0x93 }, //   10010011
{ 0x064,  8, 0x94 }, //   10010100
{ 0x065,  8, 0x95 }, //   10010101
{ 0x066,  8, 0x96 }, //   10010110
{ 0x067,  8, 0x97 }, //   10010111
{ 0x068,  8, 0x98 }, //   10011000
{ 0x069,  8, 0x99 }, //   10011001
{ 0x06a,  8, 0x9a }, //   10011010
{ 0x06b,  8, 0x9b }, //   10011011
{ 0x06c,  8, 0x9c }, //   10011100
{ 0x06d,  8, 0x9d }, //   10011101
{ 0x06e,  8, 0x9e }, //   10011110
{ 0x06f,  8, 0x9f }, //   10011111
{ 0x070,  8, 0xa0 }, //   10100000
{ 0x071,  8, 0xa1 }, //   10100001
{ 0x072,  8, 0xa2 }, //   10100010
{ 0x073,  8, 0xa3 }, //   10100011
{ 0x074,  8, 0xa4 }, //   10100100
{ 0x075,  8, 0xa5 }, //   10100101
{ 0x076,  8, 0xa6 }, //   10100110
{ 0x077,  8, 0xa7 }, //   10100111
{ 0x078,  8, 0xa8 }, //   10101000
{ 0x079,  8, 0xa9 }, //   10101001
{ 0x07a,  8, 0xaa }, //   10101010
{ 0x07b,  8, 0xab }, //   10101011
{ 0x07c,  8, 0xac }, //   10101100
{ 0x07d,  8, 0xad }, //   10101101
{ 0x07e,  8, 0xae }, //   10101110
{ 0x07f,  8, 0xaf }, //   10101111
{ 0x080,  8, 0xb0 }, //   10110000
{ 0x081,  8, 0xb1 }, //   10110001
{ 0x082,  8, 0xb2 }, //   10110010
{ 0x083,  8, 0xb3 }, //   10110011
{ 0x084,  8, 0xb4 }, //   10110100
{ 0x085,  8, 0xb5 }, //   10110101
{ 0x086,  8, 0xb6 }, //   10110110
{ 0x087,  8, 0xb7 }, //   10110111
{ 0x088,  8, 0xb8 }, //   10111000
{ 0x089,  8, 0xb9 }, //   10111001
{ 0x08a,  8, 0xba }, //   10111010
{ 0x08b,  8, 0xbb }, //   10111011
{ 0x08c,  8, 0xbc }, //   10111100
{ 0x08d,  8, 0xbd }, //   10111101
{ 0x08e,  8, 0xbe }, //   10111110
{ 0x08f,  8, 0xbf }, //   10111111
{ 0x090,  9, 0x190 }, //  110010000
{ 0x091,  9, 0x191 }, //  110010001
{ 0x092,  9, 0x192 }, //  110010010
{ 0x093,  9, 0x193 }, //  110010011
{ 0x094,  9, 0x194 }, //  110010100
{ 0x095,  9, 0x195 }, //  110010101
{ 0x096,  9, 0x196 }, //  110010110
{ 0x097,  9, 0x197 }, //  110010111
{ 0x098,  9, 0x198 }, //  110011000
{ 0x099,  9, 0x199 }, //  110011001
{ 0x09a,  9, 0x19a }, //  110011010
{ 0x09b,  9, 0x19b }, //  110011011
{ 0x09c,  9, 0x19c }, //  110011100
{ 0x09d,  9, 0x19d }, //  110011101
{ 0x09e,  9, 0x19e }, //  110011110
{ 0x09f,  9, 0x19f }, //  110011111
{ 0x0a0,  9, 0x1a0 }, //  110100000
{ 0x0a1,  9, 0x1a1 }, //  110100001
{ 0x0a2,  9, 0x1a2 }, //  110100010
{ 0x0a3,  9, 0x1a3 }, //  110100011
{ 0x0a4,  9, 0x1a4 }, //  110100100
{ 0x0a5,  9, 0x1a5 }, //  110100101
{ 0x0a6,  9, 0x1a6 }, //  110100110
{ 0x0a7,  9, 0x1a7 }, //  110100111
{ 0x0a8,  9, 0x1a8 }, //  110101000
{ 0x0a9,  9, 0x1a9 }, //  110101001
{ 0x0aa,  9, 0x1aa }, //  110101010
{ 0x0ab,  9, 0x1ab }, //  110101011
{ 0x0ac,  9, 0x1ac }, //  110101100
{ 0x0ad,  9, 0x1ad }, //  110101101
{ 0x0ae,  9, 0x1ae }, //  110101110
{ 0x0af,  9, 0x1af }, //  110101111
{ 0x0b0,  9, 0x1b0 }, //  110110000
{ 0x0b1,  9, 0x1b1 }, //  110110001
{ 0x0b2,  9, 0x1b2 }, //  110110010
{ 0x0b3,  9, 0x1b3 }, //  110110011
{ 0x0b4,  9, 0x1b4 }, //  110110100
{ 0x0b5,  9, 0x1b5 }, //  110110101
{ 0x0b6,  9, 0x1b6 }, //  110110110
{ 0x0b7,  9, 0x1b7 }, //  110110111
{ 0x0b8,  9, 0x1b8 }, //  110111000
{ 0x0b9,  9, 0x1b9 }, //  110111001
{ 0x0ba,  9, 0x1ba }, //  110111010
{ 0x0bb,  9, 0x1bb }, //  110111011
{ 0x0bc,  9, 0x1bc }, //  110111100
{ 0x0bd,  9, 0x1bd }, //  110111101
{ 0x0be,  9, 0x1be }, //  110111110
{ 0x0bf,  9, 0x1bf }, //  110111111
{ 0x0c0,  9, 0x1c0 }, //  111000000
{ 0x0c1,  9, 0x1c1 }, //  111000001
{ 0x0c2,  9, 0x1c2 }, //  111000010
{ 0x0c3,  9, 0x1c3 }, //  111000011
{ 0x0c4,  9, 0x1c4 }, //  111000100
{ 0x0c5,  9, 0x1c5 }, //  111000101
{ 0x0c6,  9, 0x1c6 }, //  111000110
{ 0x0c7,  9, 0x1c7 }, //  111000111
{ 0x0c8,  9, 0x1c8 }, //  111001000
{ 0x0c9,  9, 0x1c9 }, //  111001001
{ 0x0ca,  9, 0x1ca }, //  111001010
{ 0x0cb,  9, 0x1cb }, //  111001011
{ 0x0cc,  9, 0x1cc }, //  111001100
{ 0x0cd,  9, 0x1cd }, //  111001101
{ 0x0ce,  9, 0x1ce }, //  111001110
{ 0x0cf,  9, 0x1cf }, //  111001111
{ 0x0d0,  9, 0x1d0 }, //  111010000
{ 0x0d1,  9, 0x1d1 }, //  111010001
{ 0x0d2,  9, 0x1d2 }, //  111010010
{ 0x0d3,  9, 0x1d3 }, //  111010011
{ 0x0d4,  9, 0x1d4 }, //  111010100
{ 0x0d5,  9, 0x1d5 }, //  111010101
{ 0x0d6,  9, 0x1d6 }, //  111010110
{ 0x0d7,  9, 0x1d7 }, //  111010111
{ 0x0d8,  9, 0x1d8 }, //  111011000
{ 0x0d9,  9, 0x1d9 }, //  111011001
{ 0x0da,  9, 0x1da }, //  111011010
{ 0x0db,  9, 0x1db }, //  111011011
{ 0x0dc,  9, 0x1dc }, //  111011100
{ 0x0dd,  9, 0x1dd }, //  111011101
{ 0x0de,  9, 0x1de }, //  111011110
{ 0x0df,  9, 0x1df }, //  111011111
{ 0x0e0,  9, 0x1e0 }, //  111100000
{ 0x0e1,  9, 0x1e1 }, //  111100001
{ 0x0e2,  9, 0x1e2 }, //  111100010
{ 0x0e3,  9, 0x1e3 }, //  111100011
{ 0x0e4,  9, 0x1e4 }, //  111100100
{ 0x0e5,  9, 0x1e5 }, //  111100101
{ 0x0e6,  9, 0x1e6 }, //  111100110
{ 0x0e7,  9, 0x1e7 }, //  111100111
{ 0x0e8,  9, 0x1e8 }, //  111101000
{ 0x0e9,  9, 0x1e9 }, //  111101001
{ 0x0ea,  9, 0x1ea }, //  111101010
{ 0x0eb,  9, 0x1eb }, //  111101011
{ 0x0ec,  9, 0x1ec }, //  111101100
{ 0x0ed,  9, 0x1ed }, //  111101101
{ 0x0ee,  9, 0x1ee }, //  111101110
{ 0x0ef,  9, 0x1ef }, //  111101111
{ 0x0f0,  9, 0x1f0 }, //  111110000
{ 0x0f1,  9, 0x1f1 }, //  111110001
{ 0x0f2,  9, 0x1f2 }, //  111110010
{ 0x0f3,  9, 0x1f3 }, //  111110011
{ 0x0f4,  9, 0x1f4 }, //  111110100
{ 0x0f5,  9, 0x1f5 }, //  111110101
{ 0x0f6,  9, 0x1f6 }, //  111110110
{ 0x0f7,  9, 0x1f7 }, //  111110111
{ 0x0f8,  9, 0x1f8 }, //  111111000
{ 0x0f9,  9, 0x1f9 }, //  111111001
{ 0x0fa,  9, 0x1fa }, //  111111010
{ 0x0fb,  9, 0x1fb }, //  111111011
{ 0x0fc,  9, 0x1fc }, //  111111100
{ 0x0fd,  9, 0x1fd }, //  111111101
{ 0x0fe,  9, 0x1fe }, //  111111110
{ 0x0ff,  9, 0x1ff }, //  111111111
{ 0x100,  7, 0x0 }, //    0000000
{ 0x101,  7, 0x1 }, //    0000001
{ 0x102,  7, 0x2 }, //    0000010
{ 0x103,  7, 0x3 }, //    0000011
{ 0x104,  7, 0x4 }, //    0000100
{ 0x105,  7, 0x5 }, //    0000101
{ 0x106,  7, 0x6 }, //    0000110
{ 0x107,  7, 0x7 }, //    0000111
{ 0x108,  7, 0x8 }, //    0001000
{ 0x109,  7, 0x9 }, //    0001001
{ 0x10a,  7, 0xa }, //    0001010
{ 0x10b,  7, 0xb }, //    0001011
{ 0x10c,  7, 0xc }, //    0001100
{ 0x10d,  7, 0xd }, //    0001101
{ 0x10e,  7, 0xe }, //    0001110
{ 0x10f,  7, 0xf }, //    0001111
{ 0x110,  7, 0x10 }, //    0010000
{ 0x111,  7, 0x11 }, //    0010001
{ 0x112,  7, 0x12 }, //    0010010
{ 0x113,  7, 0x13 }, //    0010011
{ 0x114,  7, 0x14 }, //    0010100
{ 0x115,  7, 0x15 }, //    0010101
{ 0x116,  7, 0x16 }, //    0010110
{ 0x117,  7, 0x17 }, //    0010111
{ 0x118,  8, 0xc0 }, //   11000000
{ 0x119,  8, 0xc1 }, //   11000001
{ 0x11a,  8, 0xc2 }, //   11000010
{ 0x11b,  8, 0xc3 }, //   11000011
{ 0x11c,  8, 0xc4 }, //   11000100
{ 0x11d,  8, 0xc5 }, //   11000101
{ 0x11e,  8, 0xc6 }, //   11000110
{ 0x11f,  8, 0xc7 }, //   11000111
};
#endif