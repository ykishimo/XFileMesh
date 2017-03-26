#include "stdafx.h"
#include <D3D11.h>
#include <tchar.h>
#include "TextureLoader.h"
#include "D3D11TextureDB.h"

#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }
#define SAFE_RELEASE(o) if(o){  (o)->Release(); o = NULL; }

//  Create TextureDB instance
static class _TextureDBInitializer{
public:
	_TextureDBInitializer(){
		CD3D11TextureDB	*p = CD3D11TextureDB::GetInstance();
	}
	~_TextureDBInitializer(){
		CD3D11TextureDB::Destroy();
	}
} g;

CD3D11TextureDB *CD3D11TextureDB::m_pInstance = NULL;

//	Singleton
CD3D11TextureDB *CD3D11TextureDB::GetInstance(){
	if (m_pInstance == NULL){
		m_pInstance = new CD3D11TextureDB();
	}
	return m_pInstance;
}

void CD3D11TextureDB::Destroy(){
	SAFE_DELETE(m_pInstance);
}

CD3D11TextureDB::CD3D11TextureDB(){
	m_pSentinel = NULL;
	m_pNode = NULL;
	if (NULL != (m_pSentinel = new CTextureNode(_T("")))){
		m_pSentinel->m_pLeft = m_pSentinel;
		m_pSentinel->m_pRight = m_pSentinel;
		m_pSentinel->m_ppParent = NULL;
		m_pSentinel->m_pTextureDB = this;
		m_pNode = m_pSentinel;
	}
	dwNumNodes = 0L;
	dwUnique = 0L;
}

CD3D11TextureDB::~CD3D11TextureDB(){
	if (m_pNode != NULL){
		InvalidateObjectsOfDecendants(m_pNode);
		DeleteDecendants(m_pNode);
		if(m_pSentinel->m_strFilename != NULL){
			delete[]	m_pSentinel->m_strFilename;
			m_pSentinel->m_strFilename = NULL;
		}
		if (m_pSentinel != NULL){
			delete	m_pSentinel;
			m_pSentinel = NULL;
		}
	}
}

void CD3D11TextureDB::DeleteDecendants(CTextureNode *pNode){
	if (pNode == m_pSentinel)
		return;
	DeleteDecendants(pNode->m_pLeft);
	DeleteDecendants(pNode->m_pRight);
	delete	pNode;
}

void	CD3D11TextureDB::DeleteTheNode(CTextureNode *pNode){

	if (pNode == this->m_pSentinel)
		return;		//	番兵はこの関数では削除不能
	//	二分木からの切り離し（要バグチェック）
	if (pNode->m_pLeft == m_pSentinel){
		pNode->m_pRight->m_ppParent = pNode->m_ppParent;
		*(pNode->m_ppParent) = pNode->m_pRight;
	}else if (pNode->m_pRight == m_pSentinel){
		pNode->m_pLeft->m_ppParent = pNode->m_ppParent;
		*(pNode->m_ppParent) = pNode->m_pLeft;
	}else{
		CTextureNode	*q,*r;	//	q:移動するノード
		q = pNode->m_pRight;	//	pNode:削除するノード
		r = NULL;
		while(q->m_pLeft != m_pSentinel){
			r = q;
			q = q->m_pLeft;
		}
		*(pNode->m_ppParent) = q;
		q->m_ppParent = pNode->m_ppParent;

		if (r != NULL){
			r->m_pLeft = q->m_pRight;
			r->m_pLeft->m_ppParent = &(r->m_pLeft);
			q->m_pRight = pNode->m_pRight;
			q->m_pRight->m_ppParent = &(q->m_pRight);
		}
		q->m_pLeft = pNode->m_pLeft;
		q->m_pLeft->m_ppParent = &(q->m_pLeft);
	}
	//	テクスチャの消去
	if (NULL != pNode->m_pTexture){
		pNode->m_pTexture->Release();
		pNode->m_pTexture = NULL;
	}
	if (NULL != pNode->m_pTextureShaderResourceView){
		pNode->m_pTextureShaderResourceView->Release();
		pNode->m_pTextureShaderResourceView = NULL;
	}
	--dwNumNodes;
	delete	pNode->m_strFilename;
	delete	pNode;
}

HRESULT	CD3D11TextureDB::RestoreObjectsOfDecendants(ID3D11DeviceContext *pContext,CTextureNode *pNode)
{
	HRESULT	hr1, hr2, hr3;

	if (pNode == this->m_pSentinel){
		return S_OK;
	}
	hr1 = RestoreObjectsOfDecendants(pContext,pNode->m_pLeft);
	hr2 = RestoreObjectsOfDecendants(pContext,pNode->m_pRight);
	hr3 = S_OK;
	hr3 = RestoreNode(pContext,pNode);
	if (FAILED(hr3)){
		SAFE_RELEASE(pNode->m_pTexture);
		SAFE_RELEASE(pNode->m_pTextureShaderResourceView);
		return	hr3;
	}
	if (FAILED(hr1)){
		return	hr1;
	}
	if (FAILED(hr2)){
		return	hr2;
	}
	return	S_OK;
}

HRESULT CD3D11TextureDB::RestoreNode(ID3D11DeviceContext *pContext, CTextureNode *pNode){
	HRESULT hr = S_OK;
	if (pNode->m_pTexture == NULL){
		DWORD w, h;
		hr = CTextureLoader::CreateTextureFromFile(pContext,
			pNode->m_strFilename,&pNode->m_pTexture,&w,&h,FillMode::Linear);
		if (SUCCEEDED(hr)){
			SAFE_RELEASE(pNode->m_pTextureShaderResourceView);
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
			D3D11_TEXTURE2D_DESC	texDesc;
			ID3D11Device *pDevice;
			pContext->GetDevice(&pDevice);
			ZeroMemory( &srvd, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
			pNode->m_pTexture->GetDesc(&texDesc);
			srvd.Format = texDesc.Format;
  			srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvd.Texture2D.MipLevels = texDesc.MipLevels;
			srvd.Texture2D.MostDetailedMip = 0;
			hr = pDevice->CreateShaderResourceView( pNode->m_pTexture, &srvd, &pNode->m_pTextureShaderResourceView);
			pDevice->Release();
		}
	}
	return hr;
}


HRESULT	CD3D11TextureDB::RestoreDeviceObjects(ID3D11DeviceContext *pContext)
{
	return RestoreObjectsOfDecendants(pContext,this->m_pNode);
}

HRESULT CD3D11TextureDB::InvalidateObjectsOfDecendants(CTextureNode *pNode)
{
	if (pNode == m_pSentinel)
		return S_OK;
	
	InvalidateObjectsOfDecendants(pNode->m_pLeft);
	InvalidateObjectsOfDecendants(pNode->m_pRight);

	SAFE_RELEASE(pNode->m_pTexture)
	SAFE_RELEASE(pNode->m_pTextureShaderResourceView)
	return	S_OK;
}

HRESULT	CD3D11TextureDB::InvalidateDeviceObjects()
{
	return InvalidateObjectsOfDecendants(this->m_pNode);
}


//=====================================================================
// Name: RegisterNewKey
// Desc: キー値の登録、返値：true 新しいノード false:キー値登録済み
// Note: キー値がNULL もしくは空文字列の場合は、pRet に NULL を返す
//=====================================================================
BOOL CD3D11TextureDB::RegisterNewKey(TCHAR *key,CTextureNode **pRet){
	CTextureNode **pNode;
	int		f;
	int		len;
	if ((key == NULL)||(key[0] == _T('\0'))){
		*pRet = NULL;
		return	false;
	}
	if (m_pSentinel->m_strFilename != NULL){
		delete[]	m_pSentinel->m_strFilename;
		m_pSentinel->m_strFilename = NULL;
	}
	len = (INT)_tcslen(key)+1;
	if (NULL == (m_pSentinel->m_strFilename = new TCHAR[len])){
		*pRet = NULL;	//	メモリ不足でエラー
		return	false;	//
	}
	_tcscpy_s(m_pSentinel->m_strFilename,len,key);
	pNode = &m_pNode;
	while((f = _tcscmp(key,(*pNode)->m_strFilename))!=0){
		if (f < 0)
			pNode = &((*pNode)->m_pLeft);
		else
			pNode = &((*pNode)->m_pLeft);
	}
	if ((*pNode) != m_pSentinel){
		(*pNode)->m_iRefCount++;
		*pRet = *pNode;
		return	false;
	}
	CTextureNode *newNode;
	newNode = new CTextureNode(key);
	if (newNode == NULL){
		*pRet = NULL;
		return	false;
	}
	newNode->m_ppParent = pNode;
	newNode->m_pLeft = m_pSentinel;
	newNode->m_pRight = m_pSentinel;
	newNode->m_pTextureDB = this;
	*pNode = newNode;
	++dwUnique;
	++dwNumNodes;
	*pRet = newNode;
	return	true;
}

//=====================================================================
// Name: Release
// Desc: テクスチャの使用終了
//=====================================================================
void	CTextureNode::Release(){
	if (0 >= --m_iRefCount){
		if (m_pTexture != NULL){
			m_pTexture->Release();
			m_pTexture = NULL;
		}
		this->m_pTextureDB->DeleteTheNode(this);
	}
}

//  ctor for CTextureNode
CTextureNode::CTextureNode(TCHAR *pStr,ID3D11Texture2D *pTex){
	m_iRefCount = 1;
	int	len;
	len = (int)_tcslen((TCHAR*)pStr);
	m_strFilename = new TCHAR[++len];
	_tcscpy_s(m_strFilename,len,(TCHAR*)pStr);
	m_pTexture = pTex;
	m_pTextureShaderResourceView = NULL;
}