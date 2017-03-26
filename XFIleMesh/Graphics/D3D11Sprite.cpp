//
//	class CD3D11Sprite
//	Desc: This class implements 2D sprites on Direct3D11
//	
#include "stdafx.h"
#include <d3d11.h>
#include <directxmath.h>
#include "D3D11Sprite.h"
#include "TextureLoader.h"
#include <vector>
#include "D3DContext.h"
//  

#ifdef COMPILE_AND_SAVE_SHADER

static char shadercode[]="\
//-----------------------------------------------------------------------------------\n\
//  texture sampler\n\
//-----------------------------------------------------------------------------------\n\
SamplerState textureSampler : register(s0);\n\
Texture2D	diffuseTexture : register(t0);\n\
\n\
\n\
//-----------------------------------------------------------------------------------\n\
// Constant buffer\n\
//-----------------------------------------------------------------------------------\n\
cbuffer VSConstantBuffer : register( b0 )\n\
{\n\
    matrix  Proj;   //  射影行列\n\
	float4  Color;  //  モジュレ―トする色\n\
};\n\
\n\
\n\
//-----------------------------------------------------------------------------------\n\
// VSInput structure\n\
//-----------------------------------------------------------------------------------\n\
struct VSInput\n\
{\n\
    float3 Position : POSITION;     //	位置座標\n\
	float2 texCoord : TEXCOORD0;	//  テクスチャ座標\n\
};\n\
\n\
//-----------------------------------------------------------------------------------\n\
// GSPSInput structure\n\
//-----------------------------------------------------------------------------------\n\
struct GSPSInput\n\
{\n\
    float4 Position : SV_POSITION;  //  位置座標\n\
	float2 texCoord : TEXCOORD0;	//  テクスチャ座標\n\
	float4 diffuse  : COLOR;        //  文字色\n\
};\n\
\n\
//-----------------------------------------------------------------------------------\n\
//	頂点シェーダエントリーポイント\n\
//-----------------------------------------------------------------------------------\n\
GSPSInput VSFunc( VSInput input )\n\
{\n\
    GSPSInput output = (GSPSInput)0;\n\
\n\
    // 入力データをfloat4 へ変換.\n\
	float4 pos = float4( input.Position, 1.0f );\n\
\n\
	// 射影空間に変換.\n\
    float4 projPos  = mul( Proj,  pos );\n\
\n\
	output.Position = projPos;\n\
	output.texCoord = input.texCoord;\n\
	output.diffuse  = Color;\n\
    return output;\n\
}\n\
\n\
//------------------------------------------------------------------------------------\n\
//	ピクセルシェーダエントリーポイント\n\
//------------------------------------------------------------------------------------\n\
float4 PSFunc( GSPSInput psin ) : SV_TARGET0\n\
{\n\
	float4 pixel = diffuseTexture.Sample(textureSampler, psin.texCoord);\n\
    pixel = psin.diffuse * pixel;\n\
	return pixel;\n\
}\n\
float4 PSFuncNoTex( GSPSInput psin ) : SV_TARGET0\n\
{\n\
	return psin.diffuse;\n\
}\n\
\n";
#endif


//  テクスチャ管理用のノード構造体
struct SpriteTexture{
	TCHAR				*pFilename;
	ID3D11Texture2D		*pTexture;
	ID3D11ShaderResourceView	*pTextureShaderResourceView;
	DWORD				dwSrcWidth;
	DWORD				dwSrcHeight;

	SpriteTexture(){
		pFilename = NULL;
		pTexture = NULL;
		pTextureShaderResourceView = NULL;
	}
	~SpriteTexture(){
		SAFE_DELETE(pFilename);
		SAFE_RELEASE(pTexture);
		SAFE_RELEASE(pTextureShaderResourceView);
	}
};

//  テクスチャ管理用データ
//  stl の展開を局所化するためローカルデータ構造としておく
struct SpriteDataContext{
	std::vector<SpriteTexture *>	pSpriteTextures;
};

//  Vertex Structure
typedef struct {
	DirectX::XMFLOAT3 position;     //  位置座標
	DirectX::XMFLOAT2 texture;      //  テクスチャ座標
}	SpriteVertex;

//  Constant buffer structure
typedef struct {
	DirectX::XMMATRIX	matProj;
	DirectX::XMFLOAT4	color;
}	SpriteConstantBuffer;


struct SpriteDataContext;
class CD3D11Sprite : public ID3D11Sprite
{
public:
	CD3D11Sprite(void);
	virtual ~CD3D11Sprite(void);
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;
	virtual void	SetTexture(INT no, TCHAR *pFilename) override;
	virtual void    Render(ID3D11DeviceContext *pContext, FLOAT x, FLOAT y, FLOAT w, FLOAT h, INT no) override;
	virtual void    Render(ID3D11DeviceContext *pContext, FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT tx, FLOAT ty, FLOAT tw, FLOAT th, INT no) override;
	virtual void	SetDiffuse(FLOAT r, FLOAT g, FLOAT b, FLOAT a) override;
	virtual INT		GetNumTextures() override;

	//  keep 16-byte aligned
	void *operator new(size_t size){
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

	void InitFields();
	SpriteDataContext   *m_pDataContext;

	ID3D11DeviceContext	*m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	ID3D11SamplerState	*m_pTextureSamplerState;
	ID3D11BlendState	*m_pBlendState;
	ID3D11Buffer		*m_pVertexBuffer;
	
	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;
	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static BYTE  *m_pNoTexPixelShaderCode;
	static DWORD m_dwNoTexPixelShaderCodeSize;
	static INT	 m_iShaderReferenceCount;

	static ID3D11Buffer		*m_pConstantBuffer;
	static ID3D11VertexShader	*m_pVertexShader;
	static ID3D11PixelShader	*m_pPixelShader;
	static ID3D11PixelShader	*m_pNoTexPixelShader;
	static ID3D11InputLayout	*m_pInputLayout;

	DirectX::XMMATRIX	m_matProj;
	DirectX::XMFLOAT4   m_vecDiffuse;

	BOOL m_bInitialized;
};

class D3D11SpriteShaderBuilder{
public:
	D3D11SpriteShaderBuilder(){
		CD3D11Sprite::CompileShaderCodes();
	}
	~D3D11SpriteShaderBuilder(){
		CD3D11Sprite::RemoveShaderCodes();
	}
}	d3d11SpriteInitializer;



BYTE  *CD3D11Sprite::m_pVertexShaderCode;
DWORD CD3D11Sprite::m_dwVertexShaderCodeSize;
BYTE  *CD3D11Sprite::m_pPixelShaderCode;
DWORD CD3D11Sprite::m_dwPixelShaderCodeSize;
BYTE  *CD3D11Sprite::m_pNoTexPixelShaderCode;
DWORD CD3D11Sprite::m_dwNoTexPixelShaderCodeSize;
INT	  CD3D11Sprite::m_iShaderReferenceCount;

ID3D11Buffer		*CD3D11Sprite::m_pConstantBuffer;
ID3D11VertexShader	*CD3D11Sprite::m_pVertexShader;
ID3D11PixelShader	*CD3D11Sprite::m_pPixelShader;
ID3D11PixelShader	*CD3D11Sprite::m_pNoTexPixelShader;
ID3D11InputLayout	*CD3D11Sprite::m_pInputLayout;

ID3D11Sprite *ID3D11Sprite::CreateInstance(){
	return new CD3D11Sprite();
}

ID3D11Sprite::~ID3D11Sprite()
{
}


//
//  constructor
//
CD3D11Sprite::CD3D11Sprite(void)
{
	InitFields();
	m_pDataContext = new SpriteDataContext;
	m_bInitialized = false;
	++m_iShaderReferenceCount;
}

//
//  destructor
//
CD3D11Sprite::~CD3D11Sprite(void)
{
	--m_iShaderReferenceCount;
	if (m_iShaderReferenceCount <= 0){
		m_iShaderReferenceCount = 0;
		ReleaseClassObjects();
	}
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pDeviceContext);

	std::vector<SpriteTexture *>::iterator it;
	it = m_pDataContext->pSpriteTextures.begin();
	while(it != m_pDataContext->pSpriteTextures.end()){
		SAFE_DELETE(*it);
		++it;
	}
	SAFE_DELETE(m_pDataContext);
}

//  全フィールドの初期化
void CD3D11Sprite::InitFields(){

	m_pDataContext = NULL;
	m_pDeviceContext = NULL;
	m_pDevice = NULL;
	m_pTextureSamplerState = NULL;
	m_pBlendState = NULL;
	m_pVertexBuffer = NULL;

}

HRESULT CD3D11Sprite::RestoreClassObjects(){
	HRESULT hr = E_FAIL;

	if (m_pVertexShader != NULL && m_pPixelShader != NULL && m_pNoTexPixelShader != NULL && m_pInputLayout != NULL && m_pConstantBuffer != NULL){
		return S_OK;
	}

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
	D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {
			"POSITION", 
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 
		},
	    {
			"TEXCOORD", 
			0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 
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
		bd.ByteWidth        = sizeof( SpriteConstantBuffer );
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

HRESULT CD3D11Sprite::ReleaseClassObjects(){

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pNoTexPixelShader);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pConstantBuffer);
	return S_OK;
}

HRESULT CD3D11Sprite::RestoreInstanceObjects(){
	//	
	HRESULT hr;

	if (m_pDataContext){
		std::vector<SpriteTexture*>::iterator it;
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
		SpriteVertex vertices[] = {
			{  DirectX::XMFLOAT3( -0.5f,  0.5f, 0.0f ),  DirectX::XMFLOAT2(  0.0f,  0.0f ) },
			{  DirectX::XMFLOAT3(  0.5f,  0.5f, 0.0f ),  DirectX::XMFLOAT2(  1.0f,  0.0f ) },
			{  DirectX::XMFLOAT3( -0.5f, -0.5f, 0.0f ),  DirectX::XMFLOAT2(  0.0f,  1.0f ) },
			{  DirectX::XMFLOAT3(  0.5f, -0.5f, 0.0f ),  DirectX::XMFLOAT2(  1.0f,  1.0f ) },
		};
  
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.Usage          = D3D11_USAGE_DEFAULT;
		bd.ByteWidth      = sizeof( SpriteVertex ) * 4;
		bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// サブリソースの設定.
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory( &initData, sizeof( D3D11_SUBRESOURCE_DATA ) );
		initData.pSysMem = vertices;

		// 頂点バッファの生成.
		hr = m_pDevice->CreateBuffer( &bd, &initData, &m_pVertexBuffer );
		if ( FAILED( hr ) )
		{
			return	hr;
		}
	}
	{
		//  アルファ抜きブレンドステートの生成
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
	}
	// 行列の設定.
	{
		//  non scaled ortho matrix (2D)
		m_matProj = DirectX::XMMatrixIdentity();
		m_matProj.r[0].m128_f32[0] = 1.0f;
		m_matProj.r[1].m128_f32[1] = 1.0f;
		m_matProj.r[2].m128_f32[2] = 0.0f;
		m_matProj.r[2].m128_f32[3] = 1.0f;
		m_matProj.r[3].m128_f32[2] = 0.0f;
		m_matProj.r[3].m128_f32[3] = 0.0f;
	}
	m_bInitialized = true;
	return hr;
}

HRESULT CD3D11Sprite::ReleaseInstanceObjects(){
	if (m_pDataContext){
		std::vector<SpriteTexture*>::iterator it;
		it = m_pDataContext->pSpriteTextures.begin();
		while (it != m_pDataContext->pSpriteTextures.end()){
			if (*it != NULL){
				SAFE_RELEASE((*it)->pTexture);
				SAFE_RELEASE((*it)->pTextureShaderResourceView);
			}
			++it;
		}
	}
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pTextureSamplerState);
	SAFE_RELEASE(m_pBlendState);
	m_bInitialized = false;

	return S_OK;
}
//
//	Release device dependent objects.
//
HRESULT CD3D11Sprite::ReleaseDeviceObjects(){
	ReleaseInstanceObjects();
	ReleaseClassObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	return S_OK;
}

//
//	Restore device dependent objects.
//
HRESULT CD3D11Sprite::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
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


#ifndef COMPILE_AND_SAVE_SHADER
#include "d3d11spritevertexshadercode.inc"
#include "d3d11spritepixelshadercode.inc"
#include "d3d11spritenotexpixelshadercode.inc"
#endif
void CD3D11Sprite::CompileShaderCodes(){

	RemoveShaderCodes();

#ifndef COMPILE_AND_SAVE_SHADER
	m_pVertexShaderCode = (BYTE*)D3D11SpriteVertexShaderCode;
	m_pPixelShaderCode = (BYTE*)D3D11SpritePixelShaderCode;
	m_pNoTexPixelShaderCode = (BYTE*)D3D11SpriteNoTexPixelShaderCode;
	m_dwVertexShaderCodeSize = _countof(D3D11SpriteVertexShaderCode);
	m_dwPixelShaderCodeSize = _countof(D3D11SpritePixelShaderCode);
	m_dwNoTexPixelShaderCodeSize = _countof(D3D11SpriteNoTexPixelShaderCode);
#else
	HRESULT	hr;

	BYTE *pSourceCode = (BYTE*)shadercode;

	//  Compile shaders
	ID3DBlob *pCode = NULL;
	size_t len;
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)_countof(shadercode),"VSFunc","vs_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pVertexShaderCode = new BYTE[len];
	memcpy(m_pVertexShaderCode,pCode->GetBufferPointer(),len);
	m_dwVertexShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)_countof(shadercode),"PSFunc","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pPixelShaderCode = new BYTE[len];
	memcpy(m_pPixelShaderCode,pCode->GetBufferPointer(),len);
	m_dwPixelShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)_countof(shadercode),"PSFuncNoTex","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pNoTexPixelShaderCode = new BYTE[len];
	memcpy(m_pNoTexPixelShaderCode,pCode->GetBufferPointer(),len);
	m_dwNoTexPixelShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	SaveShaderCode(m_pVertexShaderCode,m_dwVertexShaderCodeSize,_T("d3d11spritevertexshadercode.inc"),"D3D11SpriteVertexShaderCode");
	SaveShaderCode(m_pPixelShaderCode,m_dwPixelShaderCodeSize,_T("d3d11spritepixelshadercode.inc"),"D3D11SpritePixelShaderCode");
	SaveShaderCode(m_pNoTexPixelShaderCode,m_dwNoTexPixelShaderCodeSize,_T("d3d11spritenotexpixelshadercode.inc"),"D3D11SpriteNoTexPixelShaderCode");

	hr = S_OK;
EXIT:
	if (FAILED(hr)){
		RemoveShaderCodes();
	}
	//SAFE_DELETE_ARRAY(pSourceCode);
#endif
}

void CD3D11Sprite::RemoveShaderCodes(){
#ifdef COMPILE_AND_SAVE_SHADER
	SAFE_DELETE_ARRAY(m_pVertexShaderCode);
	SAFE_DELETE_ARRAY(m_pPixelShaderCode);
	SAFE_DELETE_ARRAY(m_pNoTexPixelShaderCode);
#endif
	m_dwVertexShaderCodeSize = 0L;
	m_dwPixelShaderCodeSize = 0L;
}



//
//  method : SetTexture
//  @param:
//		no : texture no
//		pFilename : filename
//
void CD3D11Sprite::SetTexture(INT no, TCHAR *pFilename){
	int count = m_pDataContext->pSpriteTextures.size();
	int len;
	SpriteTexture *pTextureNode;
	if (no >= count){
		int add = count - no + 1;
		for ( int i = 0; i < add ; ++i){
			m_pDataContext->pSpriteTextures.push_back(NULL);
		}
	}
	pTextureNode = m_pDataContext->pSpriteTextures[no];
	if (pTextureNode == NULL){
		pTextureNode = new SpriteTexture;
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

INT CD3D11Sprite::GetNumTextures(){
	INT num = 0;
	if (m_pDataContext){
		num = m_pDataContext->pSpriteTextures.size();
	}
	return num;
}

//
//  method : Render
//	@param : 
//		pContext : D3D11 context
//		x : position - x
//		y : position - y
//		w : sprite width
//		h : sprire height
//		no : texture no
//
void    CD3D11Sprite::Render(ID3D11DeviceContext *pContext, FLOAT x, FLOAT y, FLOAT w, FLOAT h, INT no){
	Render(pContext,x,y,w,h,0.0f,0.0f,FLT_MAX,FLT_MAX,no);
}

//
//  method : Render
//	@param : 
//		pContext : D3D11 context
//		x : position - x
//		y : position - y
//		w : sprite width
//		h : sprire height
//		tx : texture-coord.x
//		ty : texture-coord.y
//		tw : texture width
//		th : texture height
//		no : texture no
//
void    CD3D11Sprite::Render(ID3D11DeviceContext *pContext, FLOAT x, FLOAT y, FLOAT w, FLOAT h,
						FLOAT tx, FLOAT ty, FLOAT tw, FLOAT th, INT no){

	SpriteTexture			*pSpriteTexture = NULL;
	ID3D11Texture2D			*pTexture = NULL;
	ID3D11ShaderResourceView *pTextureShaderResourceView = NULL;

	if (m_pDataContext == NULL)
		return;

	if (no >= 0 && no < (INT)m_pDataContext->pSpriteTextures.size()){
		pSpriteTexture = m_pDataContext->pSpriteTextures[no];
		if (pSpriteTexture != NULL){
			pTexture = pSpriteTexture->pTexture;
			pTextureShaderResourceView = pSpriteTexture->pTextureShaderResourceView;
		}
	}
	if (pSpriteTexture == NULL)
		return;

	//　シェーダを設定して描画.
	pContext->VSSetShader( m_pVertexShader, NULL, 0 );
	pContext->GSSetShader( NULL,			NULL, 0 );
	if (pTextureShaderResourceView == NULL){
		pContext->PSSetShader( m_pNoTexPixelShader,  NULL, 0 );	//  テクスチャ無し
	}else{
		pContext->PSSetShader( m_pPixelShader,  NULL, 0 );		//  テクスチャ有り
	}

	// 定数バッファの設定.
	SpriteConstantBuffer cb;
	cb.matProj  = m_matProj;
	cb.color = m_vecDiffuse;

	// サブリソースを更新.
	pContext->UpdateSubresource( m_pConstantBuffer, 0, NULL, &cb, 0, 0 );

	// ジオメトリシェーダに定数バッファを設定.
	pContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );

	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	pContext->OMSetBlendState( m_pBlendState, blendFactor, 0xffffffff );

	// 頂点バッファの設定.
	{
		D3D11_VIEWPORT  vp;
		UINT			uiNumViewport = 1;
		m_pDeviceContext->RSGetViewports(&uiNumViewport,&vp);
		if (uiNumViewport != 1)
			return;
		//  transform 2D -> clipping space
		FLOAT l = x;
		FLOAT t = y;
		FLOAT r = l + w;
		FLOAT b = t + h;
		FLOAT z = 1.0f;
		FLOAT tx1, ty1;
		FLOAT tx2, ty2;

		l /= vp.Width*0.5f;
		t /= vp.Height*0.5f;
		r /= vp.Width*0.5f;
		b /= vp.Height*0.5f;
		l -= 1.0f;
		r -= 1.0f;
		t = 1.0f - t;
		b = 1.0f - b;

		tx1 = ty1 = 0.0f;
		tx2 = ty2 = tx1;

		if (pTexture != NULL){
			D3D11_TEXTURE2D_DESC desc;
			FLOAT texW, texH;
			pTexture->GetDesc(&desc);
			texW = (FLOAT)desc.Width;
			texH = (FLOAT)desc.Height;
			tx1 = tx / texW;
			ty1 = ty / texH;
			if (tw == FLT_MAX){
				tx2 = (FLOAT)pSpriteTexture->dwSrcWidth / texW;
				ty2 = (FLOAT)pSpriteTexture->dwSrcHeight / texH;
			}else{
				tx2 = tx1 + tw / texW;
				ty2 = ty1 + th / texH;
			}
		}

		// 頂点の定義.
		SpriteVertex vertices[] = {
			{  DirectX::XMFLOAT3( l, t, z ),  DirectX::XMFLOAT2(  tx1,  ty1 ) },
			{  DirectX::XMFLOAT3( r, t, z ),  DirectX::XMFLOAT2(  tx2,  ty1 ) },
			{  DirectX::XMFLOAT3( l, b, z ),  DirectX::XMFLOAT2(  tx1,  ty2 ) },
			{  DirectX::XMFLOAT3( r, b, z ),  DirectX::XMFLOAT2(  tx2,  ty2 ) },
		};
		pContext->UpdateSubresource( m_pVertexBuffer, 0, NULL, vertices, 0, 0 );
		
	}
	// 入力アセンブラに頂点バッファを設定.
	UINT stride = sizeof( SpriteVertex );
	UINT offset = 0;
	pContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

	// 入力アセンブラに入力レイアウトを設定.
	pContext->IASetInputLayout( m_pInputLayout );

	// プリミティブの種類を設定.
	pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { pTextureShaderResourceView, 0 };
	ID3D11SamplerState	*ppSamplerStates[] = { m_pTextureSamplerState, 0 };
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
	pContext->PSSetSamplers( 0, 1, ppSamplerStates );

	pContext->Draw(4,0);	//  Draw 4 vertices.

	ppSamplerStates[0] = NULL;
	ppShaderResourceViews[0] = NULL;
	pContext->PSSetSamplers( 0, 1, ppSamplerStates );
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);

	//  ブレンドステートを元に戻す
	pContext->OMSetBlendState( NULL, blendFactor, 0xffffffff );
}

//
//	Set diffuse color.
//
void	CD3D11Sprite::SetDiffuse(FLOAT r, FLOAT g, FLOAT b, FLOAT a){
	m_vecDiffuse.x = r;
	m_vecDiffuse.y = g;
	m_vecDiffuse.z = b;
	m_vecDiffuse.w = a;
}

