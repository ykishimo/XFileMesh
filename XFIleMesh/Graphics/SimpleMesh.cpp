#include "stdafx.h"
#include <crtdbg.h>
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D2D1.h>
#include <list>
#include "XFileParser.h"
#include "Mesh.h"
#include "Animation.h"
#include "SimpleMesh.h"
#include "D3D11TextureDB.h"

#include "D3DContext.h"

//#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
//#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }
//#define SAFE_RELEASE(o) if(o){  (o)->Release(); o = NULL; }

ID3D11VertexShader	*CSimpleMeshRenderer::m_pVertexShader = NULL;
ID3D11PixelShader	*CSimpleMeshRenderer::m_pPixelShader = NULL;
ID3D11PixelShader	*CSimpleMeshRenderer::m_pNoTexPixelShader = NULL;
ID3D11InputLayout	*CSimpleMeshRenderer::m_pInputLayout = NULL;
ID3D11Buffer		*CSimpleMeshRenderer::m_pConstantBuffer = NULL;
INT					CSimpleMeshRenderer::m_iShaderReferenceCount = 0;

BYTE  *CSimpleMeshRenderer::m_pVertexShaderCode = NULL;
DWORD CSimpleMeshRenderer::m_dwVertexShaderCodeSize = 0L;
BYTE  *CSimpleMeshRenderer::m_pPixelShaderCode = NULL;
DWORD CSimpleMeshRenderer::m_dwPixelShaderCodeSize = 0L;
BYTE  *CSimpleMeshRenderer::m_pNoTexPixelShaderCode = NULL;
DWORD CSimpleMeshRenderer::m_dwNoTexPixelShaderCodeSize = 0L;


class SimpleMeshShaderBuilder{
public:
	SimpleMeshShaderBuilder(){
		CSimpleMeshRenderer::CompileShaderCodes();
	}
	~SimpleMeshShaderBuilder(){
		CSimpleMeshRenderer::RemoveShaderCodes();
	}
}	simpleMeshInitializer;


struct SimpleMeshVertex
{
	DirectX::XMFLOAT3 position;   //  position
	DirectX::XMFLOAT3 normal;     //  normal
	DirectX::XMFLOAT2 texture;    //  texture coord
};

typedef struct{
	DirectX::XMMATRIX	matWorld;
	DirectX::XMMATRIX	matView;
	DirectX::XMMATRIX	matProj;
	DirectX::XMFLOAT4	lightDir;
	DirectX::XMFLOAT4	lightDiffuse;
	DirectX::XMFLOAT4	lightAmbient;
	DirectX::XMFLOAT4	matDiffuse;
	DirectX::XMFLOAT4	matSpecular;
	DirectX::XMFLOAT4	matEmmisive;
	DirectX::XMFLOAT4	matPower;
}	SimpleMeshConstantBuffer;

void CollectMeshContainerInFrameHierarchy(MeshFrame *pFrame, std::list<FrameRendererData*> *pFrames);

//
//  ctor for simple mesh
//
CSimpleMesh::CSimpleMesh(TCHAR *pFilename){
	AnimationSet **ppAnimationSets = NULL;
	DWORD        dwNumAnimations;

	if (SUCCEEDED( CXFileParser::CreateMesh(pFilename,&m_pFrameRoot,&ppAnimationSets,&dwNumAnimations))){
		//  Ignore animations
		if (ppAnimationSets != NULL){
			for (DWORD i = 0; i < dwNumAnimations ; ++i){
				delete ppAnimationSets[i];
			}
			delete ppAnimationSets;
			ppAnimationSets = NULL;
		}
	}
	if (FAILED(AdjustVertexFormat())){
		SAFE_DELETE(m_pFrameRoot);
	}
}

CSimpleMesh::~CSimpleMesh(){
	MeshFrame *pFrame = ReplaceRootFrame(NULL);
	SAFE_DELETE(pFrame);
}

HRESULT CSimpleMeshRenderer::CreateFrameRendererDatas(){
	HRESULT hr = E_FAIL;
	if (m_pFrameRoot){
		std::list<FrameRendererData*> *pFrames = new std::list<FrameRendererData*>();
		CollectMeshContainerInFrameHierarchy(m_pFrameRoot,pFrames);
		hr = E_FAIL;
		if (pFrames->size() != 0){

			m_ppFrameRendererDatas = new FrameRendererData*[pFrames->size()];
			m_dwNumFrameRendererDatas = (DWORD)pFrames->size();
			ZeroMemory(m_ppFrameRendererDatas,sizeof(FrameRendererData*)*m_dwNumFrameRendererDatas);

			int k = 0;
			std::list<FrameRendererData*>::iterator it = pFrames->begin();
			while(it != pFrames->end()){
				m_ppFrameRendererDatas[k++] = *it;
				*it = NULL;
				it = pFrames->erase(it);
			}

			hr = S_OK;
		}
		delete pFrames;
		return hr;
	}
	return S_OK;
}

void ReleaseFrameRendererClients(FrameRendererData *pFrame){
	if (pFrame != NULL){
		SAFE_RELEASE(pFrame->m_pIndexBuffer);
		SAFE_RELEASE(pFrame->m_pVertexBuffer);
		DWORD numMaterials =  pFrame->pMeshContainer->dwNumMaterials;
		CTextureNode *pTex;
		for (DWORD mat = 0; mat < numMaterials; ++mat){
			pTex = (CTextureNode*)(pFrame->pMeshContainer->pMaterials[mat].pTextureData);
			pFrame->pMeshContainer->pMaterials[mat].pTextureData = NULL;
			SAFE_RELEASE(pTex);
		}
	}
}

HRESULT CSimpleMeshRenderer::DeleteFrameRendererDatas(){
	if (m_ppFrameRendererDatas != NULL){
		for (DWORD i = 0; i < m_dwNumFrameRendererDatas ; ++i){
			ReleaseFrameRendererClients(m_ppFrameRendererDatas[i]);
			SAFE_DELETE(m_ppFrameRendererDatas[i]);
		}
		delete[] m_ppFrameRendererDatas;
		m_ppFrameRendererDatas = NULL;
	}
	return S_OK;
}

HRESULT CSimpleMeshRenderer::AdjustVertexFormat(){
	HRESULT hr = E_FAIL;
	if (m_pFrameRoot){
		DWORD dwMask = 0xffffffffL;
		DWORD dwFVF = Mesh::FVF_XYZ;
		dwFVF |= Mesh::FVF_NORMAL|Mesh::FVF_TEX1;
		hr = AdjustMeshVertexFormatFVF(m_pFrameRoot,dwFVF,dwMask);
		if (FAILED(hr)){
			SAFE_DELETE(m_pFrameRoot);
		}
		DeleteFrameRendererDatas();
		CreateFrameRendererDatas();		
	}
	return hr;
}

CSimpleMeshRenderer::CSimpleMeshRenderer()
{
	InitFields();
	++m_iShaderReferenceCount;
}

void CSimpleMeshRenderer::InitFields(){
	m_pFrameRoot = NULL;

	m_pDeviceContext = NULL;
	m_pDevice = NULL;
	
	m_pVertexBuffer = NULL;
	m_pTextureSamplerState = NULL;

	m_ppFrameRendererDatas = NULL;
	m_dwNumFrameRendererDatas = 0;
	{
		DirectX::XMVECTOR camPos    = DirectX::XMVectorSet( 0.0f, 0.0f, -3.00f, 0.0f );
		DirectX::XMVECTOR camTarget = DirectX::XMVectorSet( 0.0f, 0.0f,  0.0f, 0.0f );
		DirectX::XMVECTOR camUpward = DirectX::XMVectorSet( 0.0f, 1.0f,  0.0f, 0.0f );
		m_matView  = DirectX::XMMatrixLookAtLH(camPos,camTarget,camUpward);
	}
	m_matWorld = DirectX::XMMatrixIdentity();
	m_matProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2,16.0f/9.0f,0.1f,1000.0f);

	m_vecLightDir = DirectX::XMFLOAT3(0.235f,0.235f,-0.943f);
	m_vecLightDiffuse = DirectX::XMFLOAT3(1.0f,1.0f,1.0f);
	m_vecLightAmbient = DirectX::XMFLOAT3(0.1f,0.1f,0.1f);

	m_bPrepared = false;
}

CSimpleMeshRenderer::~CSimpleMeshRenderer(void)
{
	--m_iShaderReferenceCount;
	if (m_iShaderReferenceCount <= 0){
		ReleaseClassObjects();
	}
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);

	DeleteFrameRendererDatas();

	SAFE_DELETE(m_pFrameRoot);

}

BOOL	CSimpleMeshRenderer::IsPrepared(){
	return m_bPrepared;
}

MeshFrame *CSimpleMeshRenderer::ReplaceRootFrame(MeshFrame *pFrameRoot){
	MeshFrame *pCurrent = m_pFrameRoot;
	if (pCurrent != NULL){
		DeleteFrameRendererDatas();
	}
	m_pFrameRoot = pFrameRoot;
	CreateFrameRendererDatas();

	return pCurrent;
}


void CollectMeshContainerInFrameHierarchy(MeshFrame *pFrame, std::list<FrameRendererData*> *pFrames){
	if (pFrame == NULL)
		return;
	if (pFrame->pFrameSibling != NULL){
		CollectMeshContainerInFrameHierarchy(pFrame->pFrameSibling,pFrames);
	}
	if (pFrame->pFrameFirstChild != NULL){
		CollectMeshContainerInFrameHierarchy(pFrame->pFrameFirstChild,pFrames);
	}

	MeshContainer *pContainer = pFrame->pMeshContainer;
	while(pContainer != NULL){
		FrameRendererData *pData = new FrameRendererData;
		pData->pFrame = pFrame;
		pData->pMeshContainer = pContainer;
		BYTE *pVertices = (BYTE*)pContainer->pMeshData->pVertices;
		INT	iStride = GetFVFSize(pContainer->pMeshData->dwFVF);
		DirectX::XMFLOAT3 vec;
		DirectX::XMFLOAT3 vecMax(-FLT_MAX,-FLT_MAX,-FLT_MAX);
		DirectX::XMFLOAT3 vecMin( FLT_MAX, FLT_MAX, FLT_MAX);
		for (int i = 0; i < pContainer->pMeshData->numVertices ; ++i){
			vec = *((DirectX::XMFLOAT3*)pVertices);
			vecMax.x = max(vecMax.x,vec.x);
			vecMax.y = max(vecMax.y,vec.y);
			vecMax.z = max(vecMax.z,vec.z);
			vecMin.x = min(vecMin.x,vec.x);
			vecMin.y = min(vecMin.y,vec.y);
			vecMin.z = min(vecMin.z,vec.z);
			pVertices += iStride;
		}
		pData->m_vecObjCenter.x = (vecMax.x + vecMin.x)*0.5f;
		pData->m_vecObjCenter.y = (vecMax.y + vecMin.y)*0.5f;
		pData->m_vecObjCenter.z = (vecMax.z + vecMin.z)*0.5f;

		pData->m_pIndexBuffer = NULL;
		pData->m_pVertexBuffer = NULL;

		pFrames->push_back(pData);
		pContainer = pContainer->pNextMeshContainer;
	}
}

HRESULT CSimpleMeshRenderer::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
	HRESULT hr;
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	m_pDeviceContext = pContext;
	m_pDeviceContext->AddRef();
	m_pDeviceContext->GetDevice(&m_pDevice);

	RestoreClassObjects();
	hr = RestoreInstanceObjects();
	if (SUCCEEDED(hr)){
		m_bPrepared = true;
	}
	return hr;
}

HRESULT CSimpleMeshRenderer::RestoreInstanceObjects(){
	INT numVertices;
	FrameRendererData *pFrameData = NULL;
	MeshContainer *pContainer = NULL;
	Mesh		*pMesh = NULL;
	HRESULT hr;

	//  Prepare device
	ReleaseInstanceObjects();

	if (m_pFrameRoot == NULL){
		return E_FAIL;	//!<  if there is no renderable datas.
	}

	//
	for (DWORD i = 0 ; i < m_dwNumFrameRendererDatas ; ++i){
		pFrameData = m_ppFrameRendererDatas[i];
		pContainer = pFrameData->pMeshContainer;
		pMesh = pContainer->pMeshData;
		numVertices = pMesh->numVertices;

		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.Usage          = D3D11_USAGE_DEFAULT;
		bd.ByteWidth      = sizeof( SimpleMeshVertex ) * numVertices;
		bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// サブリソースの設定.
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory( &initData, sizeof( D3D11_SUBRESOURCE_DATA ) );
		initData.pSysMem = pMesh->pVertices;
		SimpleMeshVertex *pVertex = (SimpleMeshVertex*)pMesh->pVertices;
		// 頂点バッファの生成.
		SAFE_RELEASE(pFrameData->m_pVertexBuffer);
		SAFE_RELEASE(pFrameData->m_pIndexBuffer);

		hr = m_pDevice->CreateBuffer( &bd, &initData, &(pFrameData->m_pVertexBuffer) );
		if ( FAILED( hr ) )
		{
			return	hr;
		}

		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.Usage          = D3D11_USAGE_DEFAULT;
		bd.ByteWidth      = sizeof( USHORT ) * pMesh->numIndices * 3;
		bd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		ZeroMemory( &initData, sizeof( D3D11_SUBRESOURCE_DATA ) );
		initData.pSysMem = pMesh->pIndices;

		hr = m_pDevice->CreateBuffer( &bd, &initData, &(pFrameData->m_pIndexBuffer) );
		if ( FAILED( hr ) )
		{
			return	hr;
		}	

		DWORD numMaterials =  m_ppFrameRendererDatas[i]->pMeshContainer->dwNumMaterials;
		CTextureNode *pTex;
		Material	*pMat;
		for (DWORD mat = 0; mat < numMaterials; ++mat){
			pMat = &m_ppFrameRendererDatas[i]->pMeshContainer->pMaterials[mat];
			if (pMat->pTextureFilename != NULL && pMat->pTextureFilename[0] != _T('\0')){
				//
				CD3D11TextureDB *pDB = CD3D11TextureDB::GetInstance();
				pDB->RegisterNewKey(pMat->pTextureFilename,&pTex);
				pMat->pTextureData = (VOID*)pTex;
				pDB->RestoreNode(m_pDeviceContext,pTex);
			}
		}
	}
	{
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;         // サンプリング時に使用するフィルタ。ここでは異方性フィルターを使用する。
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 ～ 1 の範囲外にある u テクスチャー座標の描画方法
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 ～ 1 の範囲外にある v テクスチャー座標
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 ～ 1 の範囲外にある w テクスチャー座標
		samplerDesc.MipLODBias = 0;                            // 計算されたミップマップ レベルからのバイアス
		samplerDesc.MaxAnisotropy = 16;                        // サンプリングに異方性補間を使用している場合の限界値。有効な値は 1 ～ 16 。
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;  // 比較オプション。
		memcpy((void*)&samplerDesc.BorderColor,(void*)&DirectX::XMFLOAT4(0,0,0,0),4*sizeof(FLOAT));
		samplerDesc.MinLOD = 0;                                // アクセス可能なミップマップの下限値
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;                // アクセス可能なミップマップの上限値
		// ID3D11Device::CreateSamplerState
		hr = m_pDevice->CreateSamplerState( &samplerDesc, &m_pTextureSamplerState );
		if( FAILED( hr ) )
			return	hr;
	}

	return S_OK;
}

HRESULT CSimpleMeshRenderer::ReleaseDeviceObjects(){
	ReleaseInstanceObjects();
	ReleaseClassObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	m_bPrepared = false;
	return S_OK;
}

HRESULT CSimpleMeshRenderer::ReleaseInstanceObjects(){

	SAFE_RELEASE(m_pTextureSamplerState);

	if (m_ppFrameRendererDatas != NULL){
		for (DWORD i = 0; i < m_dwNumFrameRendererDatas ; ++i){
			if (m_ppFrameRendererDatas[i] != NULL){
				ReleaseFrameRendererClients(m_ppFrameRendererDatas[i]);
				/*
				SAFE_RELEASE(m_ppFrameRendererDatas[i]->m_pIndexBuffer);
				SAFE_RELEASE(m_ppFrameRendererDatas[i]->m_pVertexBuffer);
				DWORD numMaterials =  m_ppFrameRendererDatas[i]->pMeshContainer->dwNumMaterials;
				CTextureNode *pTex;
				for (DWORD mat = 0; mat < numMaterials; ++mat){
					pTex = (CTextureNode*)(m_ppFrameRendererDatas[i]->pMeshContainer->pMaterials[mat].pTextureData);
					m_ppFrameRendererDatas[i]->pMeshContainer->pMaterials[mat].pTextureData = NULL;
					SAFE_RELEASE(pTex);
				}
				*/
			}
		}
	}
	return S_OK;
}

HRESULT CSimpleMeshRenderer::ReleaseClassObjects(){

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pNoTexPixelShader);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pConstantBuffer);

	return S_OK;
}

HRESULT CSimpleMeshRenderer::RestoreClassObjects(){
	HRESULT hr = E_FAIL;

	if (m_pVertexShader != NULL && m_pPixelShader != NULL && m_pNoTexPixelShader != NULL && m_pInputLayout != NULL && m_pConstantBuffer != NULL){
		return S_OK;
	}

	if (m_pVertexShaderCode == NULL || m_pPixelShaderCode == NULL || m_pNoTexPixelShaderCode == NULL)
		return E_FAIL;

	SAFE_RELEASE(this->m_pVertexShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pNoTexPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);
	SAFE_RELEASE(this->m_pConstantBuffer);

	//  Compile shaders
	hr = m_pDevice->CreateVertexShader(m_pVertexShaderCode,m_dwVertexShaderCodeSize,NULL,&m_pVertexShader);
	if (FAILED(hr))
		goto EXIT;

	// 入力レイアウトの定義.
	static D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {
			"POSITION", 
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 
		},
	    {
			"NORMAL", 
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 
		},
	    {
			"TEXCOORD", 
			0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 
		}
	};
	UINT numElements = ARRAYSIZE( layout );
	// 入力レイアウトを生成.
	hr = m_pDevice->CreateInputLayout( 
	    layout,
	    numElements,
	    m_pVertexShaderCode,
	    m_dwVertexShaderCodeSize,
	    &m_pInputLayout
	);

	if (FAILED(hr))
		goto EXIT;


	//  ピクセルシェーダの生成
	hr = m_pDevice->CreatePixelShader(m_pPixelShaderCode,m_dwPixelShaderCodeSize,NULL,&m_pPixelShader);
	if (FAILED(hr))
		goto EXIT;

	hr = m_pDevice->CreatePixelShader(m_pNoTexPixelShaderCode,m_dwNoTexPixelShaderCodeSize,NULL,&m_pNoTexPixelShader);
	if (FAILED(hr))
		goto EXIT;

	// 定数バッファの生成.
	{
		// 定数バッファの設定.
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.ByteWidth        = sizeof( SimpleMeshConstantBuffer );
		bd.Usage            = D3D11_USAGE_DEFAULT;
		bd.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags   = 0;

		// 定数バッファを生成.
		hr = m_pDevice->CreateBuffer( &bd, NULL, &m_pConstantBuffer );
		if ( FAILED( hr ) )
		{
			goto EXIT;
		}
	}

	hr = S_OK;
EXIT:
	return hr;
}


//
//	Method : Render
//
void CSimpleMeshRenderer::Render(ID3D11DeviceContext *pContext){
	
	FrameRendererData *pFrameData = NULL;
	MeshContainer *pContainer = NULL;
	Mesh		*pMesh = NULL;
	DWORD		dwNumVertices;

	//
	//　シェーダを設定して描画.
	pContext->VSSetShader( m_pVertexShader, NULL, 0 );
	pContext->GSSetShader( NULL,			NULL, 0 );

	SimpleMeshConstantBuffer	cb;
	cb.matView  = m_matView;
	cb.matProj  = m_matProjection;

	cb.lightDir.x = m_vecLightDir.x;
	cb.lightDir.y = m_vecLightDir.y;
	cb.lightDir.z = m_vecLightDir.z;
	cb.lightDir.w = 1.0f;

	cb.lightDiffuse.x = m_vecLightDiffuse.x;
	cb.lightDiffuse.y = m_vecLightDiffuse.y;
	cb.lightDiffuse.z = m_vecLightDiffuse.z;
	cb.lightDiffuse.w = 1.0f;

	cb.lightAmbient.x = m_vecLightAmbient.x;
	cb.lightAmbient.y = m_vecLightAmbient.y;
	cb.lightAmbient.z = m_vecLightAmbient.z;
	cb.lightAmbient.w = 1.0f;

	for (DWORD i = 0 ; i < m_dwNumFrameRendererDatas ; ++i){
		pFrameData = m_ppFrameRendererDatas[i];
		pContainer = pFrameData->pMeshContainer;
		pMesh = pContainer->pMeshData;
		dwNumVertices = pMesh->numVertices;
		cb.matWorld = pFrameData->pFrame->CombinedMatrix * m_matWorld;

		// 入力アセンブラに頂点バッファを設定.
		UINT stride = sizeof( SimpleMeshVertex );
		UINT offset = 0;
		pContext->IASetVertexBuffers( 0, 1, &pFrameData->m_pVertexBuffer, &stride, &offset );
		pContext->IASetIndexBuffer(pFrameData->m_pIndexBuffer,DXGI_FORMAT_R16_UINT, 0);
		
		// 入力アセンブラに入力レイアウトを設定.
		pContext->IASetInputLayout( m_pInputLayout );

		// プリミティブの種類を設定.
		pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		for (DWORD mat = 0; mat < pContainer->dwNumMaterials ; ++mat){
			int startIndex = pMesh->pMaterialAttributeRange[mat].FaceStart;
			int faceCount  = pMesh->pMaterialAttributeRange[mat].FaceCount;
			Material *pMaterial = &pContainer->pMaterials[mat];
			FLOAT	power = pMaterial->fPower;
			cb.matDiffuse  = pMaterial->vecDiffuse;
			cb.matDiffuse.w = 1.0f;
			cb.matEmmisive = pMaterial->vecEmmisive;
			cb.matSpecular = pMaterial->vecSpecular;
			cb.matPower   = DirectX::XMFLOAT4( power, power, power, power);
			// サブリソースを更新.
			pContext->UpdateSubresource( m_pConstantBuffer, 0, NULL, &cb, 0, 0 );

			// 頂点シェーダに定数バッファを設定.
			pContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );

			//  テクスチャをセット
			CTextureNode *pNode = (CTextureNode*)pMaterial->pTextureData;
			if (pNode != NULL){
				ID3D11ShaderResourceView* ppShaderResourceViews[] = { pNode->GetTextureShaderResourceView(), 0 };
				ID3D11SamplerState	*ppSamplerStates[] = { m_pTextureSamplerState, 0 };
				pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
				pContext->PSSetSamplers( 0, 1, ppSamplerStates );
				pContext->PSSetShader( m_pPixelShader,  NULL, 0 );
			}else{
				ID3D11ShaderResourceView* ppShaderResourceViews[] = { 0 };
				ID3D11SamplerState	*ppSamplerStates[] = { m_pTextureSamplerState, 0 };
				pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
				pContext->PSSetSamplers( 0, 1, ppSamplerStates );
				pContext->PSSetShader( m_pNoTexPixelShader,  NULL, 0 );
			}
			//  描画実行
			pContext->DrawIndexed(faceCount * 3 , startIndex * 3, 0);
		}
	}

	pContext->IASetIndexBuffer(NULL,DXGI_FORMAT_R16_UINT, 0);

}

void	CSimpleMeshRenderer::SetWorldMatrix(DirectX::XMMATRIX *pMatWorld){
	m_matWorld = *pMatWorld;
}

void	CSimpleMeshRenderer::SetViewMatrix(DirectX::XMMATRIX  *pMatView){
	m_matView = *pMatView;
}

void	CSimpleMeshRenderer::SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection){
	m_matProjection = *pMatProjection;
}

void	CSimpleMeshRenderer::SetLightDir(DirectX::XMFLOAT3 *pVecDir){
	m_vecLightDir = *pVecDir;
}
void	CSimpleMeshRenderer::SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse){
	m_vecLightDiffuse = *pVecDiffuse;
}
void	CSimpleMeshRenderer::SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient){
	m_vecLightAmbient = *pVecAmbient;
}

//
//	Prepare shaders
//
#ifndef COMPILE_AND_SAVE_SHADER
#include "simplemeshvertexixelshadecode.inc"
#include "simplemeshpixelshadecode.inc"
#include "simplemeshpixelshadernotexcode.inc"
#endif
#define FILENAME "res\\simplemesh.fx"

void CSimpleMeshRenderer::CompileShaderCodes(){
	RemoveShaderCodes();

#ifndef COMPILE_AND_SAVE_SHADER
	m_pVertexShaderCode = (BYTE*)SimpleMeshVertexShaderCode;
	m_pPixelShaderCode = (BYTE*)SimpleMeshPixelShaderCode;
	m_pNoTexPixelShaderCode = (BYTE*)SimpleMeshNoTexPixelShaderCode;
	m_dwVertexShaderCodeSize = _countof(SimpleMeshVertexShaderCode);
	m_dwPixelShaderCodeSize = _countof(SimpleMeshPixelShaderCode);
	m_dwNoTexPixelShaderCodeSize = _countof(SimpleMeshNoTexPixelShaderCode);
#else
	HRESULT	hr;
	BYTE *pSourceCode = NULL;
	struct _stat tmpStat;

	if (-1 !=_tstat(_T(FILENAME),&tmpStat)){
		pSourceCode = new BYTE[tmpStat.st_size+3&0xfffffffc];
		FILE *fp;
		_tfopen_s(&fp,_T(FILENAME),_T("rb"));
		fread((void*)pSourceCode,1,tmpStat.st_size,fp);
		fclose(fp);
	}else
		return;

	//  Compile shaders
	ID3DBlob *pCode = NULL;
	size_t len;
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"SMVSFunc","vs_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pVertexShaderCode = new BYTE[len];
	memcpy(m_pVertexShaderCode,pCode->GetBufferPointer(),len);
	m_dwVertexShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"SMPSFunc","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pPixelShaderCode = new BYTE[len];
	memcpy(m_pPixelShaderCode,pCode->GetBufferPointer(),len);
	m_dwPixelShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"SMPSFuncNoTex","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pNoTexPixelShaderCode = new BYTE[len];
	memcpy(m_pNoTexPixelShaderCode,pCode->GetBufferPointer(),len);
	m_dwNoTexPixelShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	SaveShaderCode(m_pVertexShaderCode,m_dwVertexShaderCodeSize,_T("simplemeshvertexixelshadecode.inc"),"SimpleMeshVertexShaderCode");
	SaveShaderCode(m_pPixelShaderCode,m_dwPixelShaderCodeSize,_T("simplemeshpixelshadecode.inc"),"SimpleMeshPixelShaderCode");
	SaveShaderCode(m_pNoTexPixelShaderCode,m_dwNoTexPixelShaderCodeSize,_T("simplemeshpixelshadernotexcode.inc"),"SimpleMeshNoTexPixelShaderCode");

	hr = S_OK;
EXIT:
	if (FAILED(hr)){
		RemoveShaderCodes();
	}
	SAFE_DELETE_ARRAY(pSourceCode);
#endif
}

void CSimpleMeshRenderer::RemoveShaderCodes(){
#ifdef COMPILE_AND_SAVE_SHADER
	SAFE_DELETE_ARRAY(m_pVertexShaderCode);
	SAFE_DELETE_ARRAY(m_pPixelShaderCode);
	SAFE_DELETE_ARRAY(m_pNoTexPixelShaderCode);
#endif
	m_dwVertexShaderCodeSize = 0L;
	m_dwPixelShaderCodeSize = 0L;
	m_dwNoTexPixelShaderCodeSize = 0L;
}
