#include "stdafx.h"
#include <d3d11.h>
#include <directxmath.h>
#include "TextureLoader.h"
#include <vector>
#include "D3D11PointSprite.h"
#include "D3DContext.h"

//  テクスチャ管理用のノード構造体
struct PointSpriteTexture{
	TCHAR				*pFilename;
	ID3D11Texture2D		*pTexture;
	ID3D11ShaderResourceView	*pTextureShaderResourceView;
	DWORD				dwSrcWidth;
	DWORD				dwSrcHeight;

	PointSpriteTexture(){
		pFilename = NULL;
		pTexture = NULL;
		pTextureShaderResourceView = NULL;
	}
	~PointSpriteTexture(){
		SAFE_DELETE(pFilename);
		SAFE_RELEASE(pTexture);
		SAFE_RELEASE(pTextureShaderResourceView);
	}
};

//  Vertex Structure
typedef struct {
	DirectX::XMFLOAT3 position;   //  position
	FLOAT			  psize;      //  point size
	DirectX::XMFLOAT4 color;      //  diffuse color
}	PointSpriteVertex;

//  Constant buffer structure
typedef struct {
	DirectX::XMMATRIX	matWorld;
	DirectX::XMMATRIX	matView;
	DirectX::XMMATRIX	matProj;
	FLOAT				AlphaThreshold;
}	PointSpriteConstantBuffer;

//  Instance class
struct PointSpriteDataContext;

#define MAX_POINTSPRITE_COUNT 128

class CD3D11PointSprite : public ID3D11PointSprite
{
public:
	CD3D11PointSprite(void);
	virtual ~CD3D11PointSprite(void);
	virtual void SetTexture(INT no, TCHAR *pFilename) override;
	virtual void Render(ID3D11DeviceContext *pContext, DirectX::XMFLOAT3 *position, FLOAT psize, DirectX::XMFLOAT4 *pcolor, INT texNo = 0) override;
	virtual void Flush(ID3D11DeviceContext *pContext) override;
	virtual INT	GetNumTextures() override;
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;

	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld) override;
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView)  override;
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection) override;

	//  keep this 16-byte aligned
	void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	void *operator new[](size_t size){
		return _mm_malloc(size,16);
	}
	void operator delete(void *p){
		return _mm_free(p);
	}
	static void CompileShaderCodes();	//	
	static void RemoveShaderCodes();	//	

protected:
	virtual HRESULT RestoreClassObjects();
	virtual HRESULT ReleaseClassObjects();
	virtual HRESULT RestoreInstanceObjects();
	virtual HRESULT ReleaseInstanceObjects();

	PointSpriteDataContext   *m_pDataContext;

	ID3D11DeviceContext	*m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	ID3D11SamplerState	*m_pTextureSamplerState;
	ID3D11BlendState	*m_pBlendState;
	ID3D11BlendState	*m_pBlendStateAdd;
	ID3D11Buffer		*m_pVertexBuffer;

	INT					m_iActiveTextureNo;

	static ID3D11Buffer			*m_pConstantBuffer;
	static ID3D11VertexShader	*m_pVertexShader;
	static ID3D11GeometryShader	*m_pGeometryShader;
	static ID3D11PixelShader	*m_pPixelShader;
	static ID3D11PixelShader	*m_pNoTexPixelShader;
	static ID3D11InputLayout	*m_pInputLayout;

	DirectX::XMMATRIX	m_matProj;
	DirectX::XMMATRIX	m_matWorld;
	DirectX::XMMATRIX	m_matView;
	
	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;
	static BYTE  *m_pGeometryShaderCode;
	static DWORD m_dwGeometryShaderCodeSize;
	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static BYTE  *m_pNoTexPixelShaderCode;
	static DWORD m_dwNoTexPixelShaderCodeSize;
	static INT	 m_iShaderReferenceCount;
};



ID3D11PointSprite *ID3D11PointSprite::CreateInstance(){
	return new CD3D11PointSprite();
}

ID3D11PointSprite::~ID3D11PointSprite(){
}

//  テクスチャ管理用データ
//  stl の展開を局所化するためローカルデータ構造としておく
struct PointSpriteDataContext{
	std::vector<PointSpriteTexture *>	pSpriteTextures;
	PointSpriteVertex vertices[MAX_POINTSPRITE_COUNT];
	INT	numVertices;
};


class D3D11PointSpriteShaderBuilder{
public:
	D3D11PointSpriteShaderBuilder(){
		CD3D11PointSprite::CompileShaderCodes();
	}
	~D3D11PointSpriteShaderBuilder(){
		CD3D11PointSprite::RemoveShaderCodes();
	}
}	d3d11PointSpriteInitializer;


CD3D11PointSprite::CD3D11PointSprite(void)
{
	m_pDataContext = new PointSpriteDataContext;
	m_pDataContext->numVertices = 0;
	m_pDeviceContext = NULL;
	m_pDevice = NULL;

	m_pTextureSamplerState = NULL;
	m_pBlendState = NULL;
	m_pBlendStateAdd = NULL;
	m_pVertexBuffer = NULL;
	m_iActiveTextureNo = -1;

	m_iShaderReferenceCount++;
}

CD3D11PointSprite::~CD3D11PointSprite(void)
{
	--m_iShaderReferenceCount;
	if (m_iShaderReferenceCount <= 0){
		m_iShaderReferenceCount = 0;
		ReleaseClassObjects();
	}
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pDeviceContext);

	std::vector<PointSpriteTexture *>::iterator it;
	it = m_pDataContext->pSpriteTextures.begin();
	while(it != m_pDataContext->pSpriteTextures.end()){
		SAFE_DELETE(*it);
		++it;
	}
	SAFE_DELETE(m_pDataContext);
}

//
//  method : SetTexture
//  @param:
//		no : texture no
//		pFilename : filename
//
void CD3D11PointSprite::SetTexture(INT no, TCHAR *pFilename){
	int count = m_pDataContext->pSpriteTextures.size();
	int len;
	PointSpriteTexture *pTextureNode;
	if (no >= count){
		int add = count - no + 1;
		for ( int i = 0; i < add ; ++i){
			m_pDataContext->pSpriteTextures.push_back(NULL);
		}
	}
	pTextureNode = m_pDataContext->pSpriteTextures[no];
	if (pTextureNode == NULL){
		pTextureNode = new PointSpriteTexture;
		m_pDataContext->pSpriteTextures[no] = pTextureNode;
	}else{
		SAFE_DELETE_ARRAY(pTextureNode->pFilename);
		SAFE_RELEASE(pTextureNode->pTexture);
		SAFE_RELEASE(pTextureNode->pTextureShaderResourceView);
	}
	len = _tcslen(pFilename) + 1;
	len = (len + 3)&0xfffffffc;
	pTextureNode->pFilename = new TCHAR[len];
	_tcscpy_s(pTextureNode->pFilename,len,pFilename);
}

INT CD3D11PointSprite::GetNumTextures(){
	INT num = 0;
	if (m_pDataContext){
		num = m_pDataContext->pSpriteTextures.size();
	}
	return num;
}


void CD3D11PointSprite::Render(ID3D11DeviceContext *pContext, DirectX::XMFLOAT3 *pposition, FLOAT psize, DirectX::XMFLOAT4 *pcolor, INT texNo){
	if (m_pDataContext->numVertices >= MAX_POINTSPRITE_COUNT || m_iActiveTextureNo != texNo){
		Flush(pContext);
	}
	m_iActiveTextureNo = texNo;
	PointSpriteVertex *pVertex = &m_pDataContext->vertices[m_pDataContext->numVertices];
	pVertex->color = *pcolor;
	pVertex->psize = psize;
	pVertex->position = *pposition;
	++m_pDataContext->numVertices;
}

//  描画バッファのフラッシュ
void CD3D11PointSprite::Flush(ID3D11DeviceContext *pContext){

	if (m_pDataContext->numVertices > 0){
		PointSpriteConstantBuffer cb;
		cb.matWorld  = m_matWorld;
		cb.matView  = m_matView;
		cb.matProj  = m_matProj;
		cb.AlphaThreshold = 0.0f;

		// サブリソースを更新.
		pContext->UpdateSubresource( m_pConstantBuffer, 0, NULL, &cb, 0, 0 );

		INT no = m_iActiveTextureNo;
		PointSpriteTexture		*pSpriteTexture = NULL;
		ID3D11Texture2D			*pTexture = NULL;
		ID3D11ShaderResourceView *pTextureShaderResourceView = NULL;
		if (no >= 0 && no < (INT)m_pDataContext->pSpriteTextures.size()){
			pSpriteTexture = m_pDataContext->pSpriteTextures[no];
			if (pSpriteTexture != NULL){
				pTexture = pSpriteTexture->pTexture;
				pTextureShaderResourceView = pSpriteTexture->pTextureShaderResourceView;
			}
		}
		if (pSpriteTexture != NULL){
			// ジオメトリシェーダに定数バッファを設定.
			pContext->GSSetConstantBuffers( 0, 1, &m_pConstantBuffer );

			//  頂点バッファの設定
			{
				D3D11_BOX box;
				box.left = 0;
				box.top = 0;
				box.front = 0;
				box.right = m_pDataContext->numVertices * sizeof(PointSpriteVertex);
				box.bottom = 1;
				box.back = 1;
				pContext->UpdateSubresource( m_pVertexBuffer, 0, &box, m_pDataContext->vertices, 0, 0 );
			}

			// 入力アセンブラに頂点バッファを設定.
			UINT stride = sizeof( PointSpriteVertex );
			UINT offset = 0;
			pContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

			// 入力アセンブラに入力レイアウトを設定.
			pContext->IASetInputLayout( m_pInputLayout );

			// プリミティブの種類を設定.
			pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

			ID3D11ShaderResourceView* ppShaderResourceViews[] = { pTextureShaderResourceView, 0 };
			ID3D11SamplerState	*ppSamplerStates[] = { m_pTextureSamplerState, 0 };
			pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
			pContext->PSSetSamplers( 0, 1, ppSamplerStates );

			//　シェーダを設定して描画.
			pContext->VSSetShader( m_pVertexShader, NULL, 0 );
			pContext->GSSetShader( m_pGeometryShader, NULL, 0 );
			if (pTextureShaderResourceView == NULL){
				pContext->PSSetShader( m_pNoTexPixelShader,  NULL, 0 );	//  テクスチャ無し
			}else{
				pContext->PSSetShader( m_pPixelShader,  NULL, 0 );		//  テクスチャ有り
			}

			float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			pContext->OMSetBlendState( m_pBlendStateAdd, blendFactor, 0xffffffff );

			pContext->Draw(m_pDataContext->numVertices,0);

			ppSamplerStates[0] = NULL;
			ppShaderResourceViews[0] = NULL;
			pContext->PSSetSamplers( 0, 1, ppSamplerStates );
			pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);

			//  ブレンドステートを元に戻す
			pContext->OMSetBlendState( NULL, blendFactor, 0xffffffff );
		}
		m_pDataContext->numVertices = 0;
	}
}



HRESULT CD3D11PointSprite::RestoreClassObjects(){
	HRESULT hr = S_OK;
	if (m_pVertexShader != NULL && m_pGeometryShader != NULL && m_pPixelShader != NULL && m_pNoTexPixelShader != NULL && m_pInputLayout != NULL && m_pConstantBuffer != NULL){
		return S_OK;
	}
	SAFE_RELEASE(this->m_pVertexShader);
	SAFE_RELEASE(this->m_pGeometryShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pNoTexPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);
	SAFE_RELEASE(this->m_pConstantBuffer);

	//  Compile shaders
	hr = m_pDevice->CreateVertexShader(m_pVertexShaderCode,m_dwVertexShaderCodeSize,NULL,&m_pVertexShader);
	if (FAILED(hr))
		goto EXIT;

	// 入力レイアウトの定義.
	D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {
			"POSITION", 
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 
		},
	    {
			"PSIZE", 
			0, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 
		},
	    {
			"DIFFUSE", 
			0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 
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
	if ( FAILED( hr ) )
	{
		return hr;
	}

	//  ジオメトリシェーダの生成
	hr = m_pDevice->CreateGeometryShader(m_pGeometryShaderCode,m_dwGeometryShaderCodeSize,NULL,&m_pGeometryShader);
	if (FAILED(hr))
		goto EXIT;


	//  ピクセルシェーダの生成
	hr = m_pDevice->CreatePixelShader(m_pPixelShaderCode,m_dwPixelShaderCodeSize,NULL,&m_pPixelShader);
	if (FAILED(hr))
		goto EXIT;

	//  ピクセルシェーダの生成
	hr = m_pDevice->CreatePixelShader(m_pNoTexPixelShaderCode,m_dwNoTexPixelShaderCodeSize,NULL,&m_pNoTexPixelShader);
	if (FAILED(hr))
		goto EXIT;

	// 定数バッファの生成.
	{
		// 定数バッファの設定.
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.ByteWidth        = sizeof( PointSpriteConstantBuffer );
		bd.Usage            = D3D11_USAGE_DEFAULT;
		bd.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags   = 0;

		// 定数バッファを生成.
		hr = m_pDevice->CreateBuffer( &bd, NULL, &m_pConstantBuffer );
		if ( FAILED( hr ) )
		{
			return	hr;
		}
	}
	hr = S_OK;
EXIT:
	return hr;
}

HRESULT CD3D11PointSprite::RestoreInstanceObjects(){
	HRESULT hr = S_OK;

	if (m_pDataContext){
		std::vector<PointSpriteTexture*>::iterator it;
		it = m_pDataContext->pSpriteTextures.begin();
		while (it != m_pDataContext->pSpriteTextures.end()){
			if (*it != NULL){
				ID3D11Texture2D		*pTexture = NULL;
				ID3D11ShaderResourceView	*pTextureShaderResourceView = NULL;
				hr = CTextureLoader::CreateTextureFromFile(m_pDeviceContext,
					(*it)->pFilename,   &pTexture,
					&(*it)->dwSrcWidth, &(*it)->dwSrcHeight,FillMode::None
				);
				// シェーダリソースビューを生成.
				if (SUCCEEDED(hr)){
					D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
					D3D11_TEXTURE2D_DESC	texDesc;
					ZeroMemory( &srvd, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
					pTexture->GetDesc(&texDesc);
					srvd.Format                     = texDesc.Format;
  					srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvd.Texture2D.MipLevels = texDesc.MipLevels;
					srvd.Texture2D.MostDetailedMip = 0;
					hr = m_pDevice->CreateShaderResourceView( pTexture, &srvd, &pTextureShaderResourceView);
				}
				if (FAILED(hr)){
					SAFE_RELEASE(pTexture);
					SAFE_RELEASE(pTextureShaderResourceView);
				}
				SAFE_RELEASE((*it)->pTexture);
				SAFE_RELEASE((*it)->pTextureShaderResourceView);
				(*it)->pTexture = pTexture;
				(*it)->pTextureShaderResourceView = pTextureShaderResourceView;
			}
			++it;
		}
	}

	//  テクスチャサンプラ―のセットアップ
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
	// 頂点バッファの設定.
	{
		// 頂点の定義.
  
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.Usage          = D3D11_USAGE_DEFAULT;
		bd.ByteWidth      = sizeof( this->m_pDataContext->vertices );
		bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// サブリソースの設定.
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory( &initData, sizeof( D3D11_SUBRESOURCE_DATA ) );
		initData.pSysMem = this->m_pDataContext->vertices;

		// 頂点バッファの生成.
		hr = m_pDevice->CreateBuffer( &bd, &initData, &m_pVertexBuffer );
		if ( FAILED( hr ) )
		{
			return	hr;
		}
	}
	{
		//  ブレンドステートの生成
		//  半透明
		D3D11_BLEND_DESC BlendDesc;
		ZeroMemory( &BlendDesc, sizeof( BlendDesc ) );
		BlendDesc.AlphaToCoverageEnable = FALSE;
		BlendDesc.IndependentBlendEnable = FALSE;
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		for (int i = 1 ; i < 8 ; ++i){
			BlendDesc.RenderTarget[i] = BlendDesc.RenderTarget[0];
		}
		hr = m_pDevice->CreateBlendState( &BlendDesc, &m_pBlendState );
		if ( FAILED( hr ) )
		{
			return	hr;
		}
		//  加算合成
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		for (int i = 1 ; i < 8 ; ++i){
			BlendDesc.RenderTarget[i] = BlendDesc.RenderTarget[0];
		}
		hr = m_pDevice->CreateBlendState( &BlendDesc, &m_pBlendStateAdd );
		if ( FAILED( hr ) )
		{
			return	hr;
		}
	}
	return hr;
}
HRESULT CD3D11PointSprite::ReleaseInstanceObjects(){
	HRESULT hr = S_OK;
	SAFE_RELEASE(m_pTextureSamplerState);
	SAFE_RELEASE(m_pBlendState);
	SAFE_RELEASE(m_pBlendStateAdd);
	SAFE_RELEASE(m_pVertexBuffer);

	return hr;
}
HRESULT CD3D11PointSprite::ReleaseClassObjects(){
	SAFE_RELEASE(this->m_pVertexShader);
	SAFE_RELEASE(this->m_pGeometryShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pNoTexPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);
	SAFE_RELEASE(this->m_pConstantBuffer);

	return S_OK;
}


//
//	Restore device dependent objects.
//
HRESULT CD3D11PointSprite::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
	ID3DBlob	*pShader = NULL;
	HRESULT hr = E_FAIL;

	pContext->AddRef();

	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	m_pDeviceContext = pContext;
	m_pDeviceContext->GetDevice(&m_pDevice);
	
	RestoreClassObjects();
	hr = RestoreInstanceObjects();
	return hr;
}

//
//	Release device dependent objects.
//
HRESULT CD3D11PointSprite::ReleaseDeviceObjects(){
	ReleaseInstanceObjects();
	ReleaseClassObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	return S_OK;
}

void	CD3D11PointSprite::SetWorldMatrix(DirectX::XMMATRIX *pMatWorld){
	m_matWorld = *pMatWorld;
}

void	CD3D11PointSprite::SetViewMatrix(DirectX::XMMATRIX  *pMatView){
	m_matView = *pMatView;
}

void	CD3D11PointSprite::SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection){
	m_matProj = *pMatProjection;
}



#ifndef COMPILE_AND_SAVE_SHADER
#include "d3d11pointspritevertexshadercode.inc"
#include "d3d11pointspritegeometryshadercode.inc"
#include "d3d11pointspritepixelshadercode.inc"
#include "d3d11pointspritenotexpixelshadercode.inc"
#else
#define FILENAME "res\\PointSprite.fx"
#endif
void CD3D11PointSprite::CompileShaderCodes(){

	RemoveShaderCodes();

#ifndef COMPILE_AND_SAVE_SHADER
	m_pVertexShaderCode = (BYTE*)D3D11PointSpriteVertexShaderCode;
	m_pGeometryShaderCode = (BYTE*)D3D11PointSpriteGeometryShaderCode;
	m_pPixelShaderCode = (BYTE*)D3D11PointSpritePixelShaderCode;
	m_pNoTexPixelShaderCode = (BYTE*)D3D11PointSpriteNoTexPixelShaderCode;

	m_dwVertexShaderCodeSize = _countof(D3D11PointSpriteVertexShaderCode);
	m_dwGeometryShaderCodeSize = _countof(D3D11PointSpriteGeometryShaderCode);
	m_dwPixelShaderCodeSize = _countof(D3D11PointSpritePixelShaderCode);
	m_dwNoTexPixelShaderCodeSize = _countof(D3D11PointSpriteNoTexPixelShaderCode);
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
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"VSFunc","vs_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pVertexShaderCode = new BYTE[len];
	memcpy(m_pVertexShaderCode,pCode->GetBufferPointer(),len);
	m_dwVertexShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"GSFunc","gs_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pGeometryShaderCode = new BYTE[len];
	memcpy(m_pGeometryShaderCode,pCode->GetBufferPointer(),len);
	m_dwGeometryShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"PSFunc","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pPixelShaderCode = new BYTE[len];
	memcpy(m_pPixelShaderCode,pCode->GetBufferPointer(),len);
	m_dwPixelShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"PSFuncNoTex","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pNoTexPixelShaderCode = new BYTE[len];
	memcpy(m_pNoTexPixelShaderCode,pCode->GetBufferPointer(),len);
	m_dwNoTexPixelShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	SaveShaderCode(m_pVertexShaderCode,m_dwVertexShaderCodeSize,_T("d3d11pointspritevertexshadercode.inc"),"D3D11PointSpriteVertexShaderCode");
	SaveShaderCode(m_pGeometryShaderCode,m_dwGeometryShaderCodeSize,_T("d3d11pointspritegeometryshadercode.inc"),"D3D11PointSpriteGeometryShaderCode");
	SaveShaderCode(m_pPixelShaderCode,m_dwPixelShaderCodeSize,_T("d3d11pointspritepixelshadercode.inc"),"D3D11PointSpritePixelShaderCode");
	SaveShaderCode(m_pNoTexPixelShaderCode,m_dwNoTexPixelShaderCodeSize,_T("d3d11pointspritenotexpixelshadercode.inc"),"D3D11PointSpriteNoTexPixelShaderCode");

	hr = S_OK;
EXIT:
	if (FAILED(hr)){
		RemoveShaderCodes();
	}
	SAFE_DELETE_ARRAY(pSourceCode);
#endif
}

void CD3D11PointSprite::RemoveShaderCodes(){
#ifdef COMPILE_AND_SAVE_SHADER
	SAFE_DELETE_ARRAY(m_pVertexShaderCode);
	SAFE_DELETE_ARRAY(m_pGeometryShaderCode);
	SAFE_DELETE_ARRAY(m_pPixelShaderCode);
	SAFE_DELETE_ARRAY(m_pNoTexPixelShaderCode);
#endif
	m_dwVertexShaderCodeSize = 0L;
	m_dwGeometryShaderCodeSize = 0L;
	m_dwPixelShaderCodeSize = 0L;
	m_dwNoTexPixelShaderCodeSize = 0L;
}



BYTE  *CD3D11PointSprite::m_pVertexShaderCode = NULL;
BYTE  *CD3D11PointSprite::m_pGeometryShaderCode = NULL;
BYTE  *CD3D11PointSprite::m_pPixelShaderCode = NULL;
BYTE  *CD3D11PointSprite::m_pNoTexPixelShaderCode = NULL;
DWORD CD3D11PointSprite::m_dwVertexShaderCodeSize = 0;
DWORD CD3D11PointSprite::m_dwGeometryShaderCodeSize = 0;
DWORD CD3D11PointSprite::m_dwPixelShaderCodeSize = 0;
DWORD CD3D11PointSprite::m_dwNoTexPixelShaderCodeSize = 0;
INT	  CD3D11PointSprite::m_iShaderReferenceCount = 0;

ID3D11Buffer			*CD3D11PointSprite::m_pConstantBuffer = NULL;
ID3D11VertexShader		*CD3D11PointSprite::m_pVertexShader = NULL;
ID3D11GeometryShader	*CD3D11PointSprite::m_pGeometryShader = NULL;
ID3D11PixelShader		*CD3D11PointSprite::m_pPixelShader = NULL;
ID3D11PixelShader		*CD3D11PointSprite::m_pNoTexPixelShader = NULL;
ID3D11InputLayout		*CD3D11PointSprite::m_pInputLayout = NULL;

