//
// @file FilenameDecoder.cpp
// @brief implementation of CFilenameDecoder
//
#include "StdAfx.h"
#include <TCHAR.h>
#include "FilenameDecoder.h"

#define	SAFE_DELETE(o)	{if (o){	delete (o); (o) = NULL;	}}

typedef struct	_node{
	struct	_node *prev;
	struct	_node *next;
	INT		len;
	TCHAR	*pToken;
}	TokenNode;

//
//	ctor
//	@param pFilname : pointer to the pathname
//         bRemoveTheDotDirectory : if true remove "." and ".;" from the result,
//                               otherwise keep them.
//
CFilenameDecoder::CFilenameDecoder(TCHAR *pFilename, BOOL bRemoveTheDotDirectory)
{
	TokenNode dummy = {&dummy,&dummy,0,NULL};
	TokenNode *pRoot = &dummy;
	TokenNode *pNode;
	INT	numNodes;

	m_strPath = NULL;
	m_strFilename = NULL;

	numNodes = Decode(pFilename,pRoot);
	if (numNodes == 0)
		return;

	if (bRemoveTheDotDirectory){
		numNodes = InterpretTheDotDirectory(pRoot,numNodes);
	}
	CreatePathString(pRoot,numNodes);

	 
	pNode = pRoot->next;
	while(pNode != pRoot){
		SAFE_DELETE(pNode->pToken);
		pNode = pNode->next;
		delete pNode->prev;
	}
	pRoot->prev = pRoot->next = pRoot;
}

//
//	destructor
//
CFilenameDecoder::~CFilenameDecoder(void)
{
	SAFE_DELETE(m_strPath);
	SAFE_DELETE(m_strFilename);
}

//	split pathname with directory-separator and store them to linked list
INT		CFilenameDecoder::Decode(TCHAR *pFilename, VOID *pRootNode){
	TokenNode *pRoot, *pNode;
	pRoot = (TokenNode*)pRootNode;

	INT	top,tale,ix;
	INT	numNodes = 0;
	TCHAR	c;
	
	top = tale = 0;

	while(true){
		c = pFilename[tale];
		if (c == _T('\\') || c == _T('/') || c == _T('\0')){
			if (tale > top){
				pNode = new TokenNode;
				pNode->pToken = new TCHAR[tale - top + 1];
				pNode->len = tale - top;
				ix = 0;
				while(top < tale){
					pNode->pToken[ix++] = pFilename[top++];
				}
				pNode->pToken[ix] = _T('\0');
				pNode->next = pRoot;
				pNode->prev = pRoot->prev;
				pNode->next->prev = pNode;
				pNode->prev->next = pNode;
				numNodes++;
			}
			++top;
			if (c == _T('\0'))
				break;
		}
		++tale;
	}
	pRoot->len = numNodes;
	return	numNodes;
}
//
//  @method CreatePathString
//	@brief  assembles pathname string from linked list.
//
void CFilenameDecoder::CreatePathString(VOID *pRootNode, INT numNodes){
	TokenNode *pNode, *pRoot;
	INT	sizString = 0;
	pRoot = (TokenNode*)pRootNode;

	//	ƒpƒX–¼‚Ì¶¬
	sizString = 0;
	pNode = pRoot->next;
	for (int i = 0; i < numNodes - 1 ; ++i){
		sizString += pNode->len+1;
		pNode = pNode->next;
	}
	if (sizString > 0){
		TCHAR	*p;
		m_strPath = new TCHAR[sizString + 1];
		pNode = pRoot->next;
		p = m_strPath;
		for (int i = 0; i < numNodes - 2 ; ++i){
			_tcscpy_s(p,pNode->len+1,pNode->pToken);
			p += pNode->len;
			*p++ = _T('\\');
			pNode = pNode->next;
		}
		_tcscpy_s(p,pNode->len+1,pNode->pToken);
		p += pNode->len;
	}

	//	store the body filename
	pNode = pRoot->prev;
	if (pNode != pRoot){
		m_strFilename = new TCHAR[pNode->len + 1];
		_tcscpy_s(m_strFilename,pNode->len + 1, pNode->pToken);
	}
}

//
// @method InterpretTheDotDirectory
// @brief removes "." and ".." without changing what they totally mean.
//
INT	CFilenameDecoder::InterpretTheDotDirectory(VOID *pRootNode,INT numNodes){
	TokenNode *pNode, *pRoot, *pNext;
	pRoot = (TokenNode*)pRootNode;
	pNode = pRoot->next;
	while(pNode != pRoot){
		pNext = pNode->next;
		if (0 == _tcscmp(pNode->pToken,_T("."))){
			pNode->prev->next = pNode->next;
			pNode->next->prev = pNode->prev;
			SAFE_DELETE(pNode->pToken);
			delete	pNode;
			--numNodes;
		}else if (0 == _tcscmp(pNode->pToken,_T(".."))){
			TokenNode *pNode2 = pNode->prev;
			TokenNode *pNode3 = pNode2->prev;
			if (pNode2 != pRoot && pNode3 != pRoot){
				pNode2->prev->next = pNode2->next;
				pNode2->next->prev = pNode2->prev;
				SAFE_DELETE(pNode2->pToken);
				delete	pNode2;
				--numNodes;
			}
			pNode->prev->next = pNode->next;
			pNode->next->prev = pNode->prev;
			SAFE_DELETE(pNode->pToken);
			delete	pNode;
			--numNodes;
		}
		pNode = pNext;
	}
	return numNodes;
}


//
//  @method GetPath
//	@brief Get the pathname from this object
//  @param *length : (in/out)length of result buffer if pBuffer is not null
//                  or the buffer length needed if pBuffer is null.
//         *pBuffer : (in)pointer to the result buffer.
//  @return : none
//
void CFilenameDecoder::GetPath(DWORD *length, TCHAR *pBuffer){
	if (m_strPath == NULL){
		*length = 0;
		return;
	}
	if (*length == 0){
		*length = (DWORD)_tcslen(m_strPath) + 1;
	}else{
		_tcscpy_s(pBuffer,*length,m_strPath);
	}
}
//
//  @method GetFilename
//	@brief Get the body filename from this object.
//  @param *length : (in/out)length of result buffer if pBuffer is not null
//                  or the buffer length needed if pBuffer is null.
//         *pBuffer : (in)pointer to the result buffer.
//  @return : none
//
void CFilenameDecoder::GetFilename(DWORD *length, TCHAR *pBuffer){
	if (m_strFilename == NULL){
		*length = 0;
		return;
	}
	if (*length == 0){
		*length = (DWORD)_tcslen(m_strFilename) + 1;
	}else{
		_tcscpy_s(pBuffer,*length,m_strFilename);
	}
}
//
//  @name GetFullname
//  @brief:Get full-path filename of what this object indicates.
//  @param *length : (in/out)length of result buffer if pBuffer is not null
//                  or the buffer length needed if pBuffer is null.
//         *pBuffer : (in)pointer to the result buffer.
//  @return : none
//
void CFilenameDecoder::GetFullname(DWORD *length, TCHAR *pBuffer){
	if (m_strFilename == NULL && m_strPath== NULL){
		*length = 0;
		return;
	}
	if (*length == 0){
		DWORD	len = 0;
		if (m_strPath)
			len = (DWORD)_tcslen(m_strPath);
		if (m_strFilename){
			len += (DWORD)_tcslen(m_strFilename)+1;
		}
		*length = len + 1;
	}else{
		TCHAR	*p = pBuffer;
		DWORD	len = *length;
		INT		len2;
		*pBuffer = _T('\0');
		if (m_strPath != NULL && m_strPath[0] != _T('\0')){
			_tcscpy_s(p,len,m_strPath);
			len2 = (INT)_tcslen(m_strPath);
			p+= len2;
		}
		if (m_strFilename != NULL && m_strFilename[0] != _T('\0')){
			*p++ = _T('\\');
			len -= len2+1;
			_tcscpy_s(p,len,m_strFilename);
		}
	}
}

