//
//	class CD3D11Font
//	Desc: Ank Font on Direct3D11
//		Debug purpose
//
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include "D3D11Font.h"

#include "D3DContext.h"

//#define SAFE_RELEASE(o) if(o){  (o)->Release(); o = NULL; }

//  inner structure. do not expose

__declspec(align(16)) class CD3D11Font : public ID3D11Font
{
public:
	CD3D11Font(ID3D11DeviceContext *pContext);
	virtual ~CD3D11Font(void);
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;
	
	virtual void	Render(ID3D11DeviceContext *pContext);
	HRESULT CreateFontTexture();

	virtual void DrawAnkText(ID3D11DeviceContext *pContext,const TCHAR *pString, DirectX::XMFLOAT4 color ,FLOAT x, FLOAT y) override;
	virtual void DrawChar(ID3D11DeviceContext *pContext, char ankCode, DirectX::XMFLOAT4 color, FLOAT *px, FLOAT *py) override;

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

	ID3D11DeviceContext	*m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	ID3D11Buffer		*m_pVertexBuffer;
	ID3D11Texture2D		*m_pTexture;
	ID3D11ShaderResourceView	*m_pTextureShaderResourceView;
	ID3D11SamplerState	*m_pTextureSamplerState;
	ID3D11BlendState	*m_pBlendState;


	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;
	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static INT	 m_iShaderReferenceCount;

	static ID3D11VertexShader	*m_pVertexShader;
	static ID3D11PixelShader	*m_pPixelShader;
	static ID3D11InputLayout	*m_pInputLayout;
	static ID3D11Buffer			*m_pConstantBuffer;

	DWORD				m_dwFontFlags;
	DWORD				m_dwFontHeight;

	DirectX::XMMATRIX	m_matProj;

	DWORD	m_dwTexWidth;
	DWORD	m_dwTexHeight;
	TCHAR	*m_strFontName;
	BOOL	m_bInitialized;
    FLOAT   m_fTexCoords[128-32][4];
};



BYTE  *CD3D11Font::m_pVertexShaderCode = NULL;
DWORD CD3D11Font::m_dwVertexShaderCodeSize = 0L;
BYTE  *CD3D11Font::m_pPixelShaderCode = NULL;
DWORD CD3D11Font::m_dwPixelShaderCodeSize = 0L;

ID3D11VertexShader	*CD3D11Font::m_pVertexShader = NULL;
ID3D11PixelShader	*CD3D11Font::m_pPixelShader = NULL;
ID3D11InputLayout	*CD3D11Font::m_pInputLayout = NULL;
ID3D11Buffer		*CD3D11Font::m_pConstantBuffer = NULL;
INT					CD3D11Font::m_iShaderReferenceCount = 0;

class D3D11FontShaderBuilder{
public:
	D3D11FontShaderBuilder(){
		CD3D11Font::CompileShaderCodes();
	}
	~D3D11FontShaderBuilder(){
		CD3D11Font::RemoveShaderCodes();
	}
}	d3d11FontInitializer;



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
    matrix  Proj;   //  projection matrix\n\
	float4  Color;  //  font color\n\
};\n\
\n\
\n\
//-----------------------------------------------------------------------------------\n\
// VSInput structure\n\
//-----------------------------------------------------------------------------------\n\
struct VSInput\n\
{\n\
    float3 Position : POSITION;     //	position\n\
	float2 texCoord : TEXCOORD0;	//  texture coordinates\n\
};\n\
\n\
//-----------------------------------------------------------------------------------\n\
// GSPSInput structure\n\
//-----------------------------------------------------------------------------------\n\
struct GSPSInput\n\
{\n\
    float4 Position : SV_POSITION;  //  position\n\
	float2 texCoord : TEXCOORD0;	//  texture coordinates\n\
	float4 diffuse  : COLOR;        //  font color\n\
};\n\
\n\
//-----------------------------------------------------------------------------------\n\
//	Vertex Shader Entry point\n\
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
//	Pixel shader entry point\n\
//------------------------------------------------------------------------------------\n\
float4 PSFunc( GSPSInput psin ) : SV_TARGET0\n\
{\n\
	float4 pixel = diffuseTexture.Sample(textureSampler, psin.texCoord);\n\
    pixel = psin.diffuse * pixel;\n\
	return pixel;\n\
}\n\
\n";


struct AnkFontVertex
{
	DirectX::XMFLOAT3 position;     //  position
	DirectX::XMFLOAT2 texture;      //  texture coordinates
};

typedef struct{
	DirectX::XMMATRIX	matProj;
	DirectX::XMFLOAT4	color;
}	AnkFontConstantBuffer;

ID3D11Font *ID3D11Font::CreateAnkFont(ID3D11DeviceContext *pContext){
	return new CD3D11Font(pContext);
}

//  pure virtual destructor needs entity
ID3D11Font::~ID3D11Font(){
}

CD3D11Font::CD3D11Font(ID3D11DeviceContext *pContext)
{
	//  いったんすべて NULL で初期化
	m_pDeviceContext = NULL;
	m_pDevice = NULL;
	m_pTexture = NULL;
	m_pTextureShaderResourceView = NULL;
	m_pTextureSamplerState = NULL;
	m_pVertexShader = NULL;
	m_pPixelShader = NULL;
	m_pInputLayout = NULL;
	m_pBlendState = NULL;
	m_pVertexBuffer = NULL;
	m_pConstantBuffer = NULL;
	
	//  デバッグ用デフォルト
	m_strFontName = _T("ＭＳ ゴシック");
	m_dwFontHeight = 20;
	m_dwFontFlags = 0;

	m_bInitialized = false;

	RestoreDeviceObjects(pContext);

	++m_iShaderReferenceCount;
}


CD3D11Font::~CD3D11Font(void)
{
	--m_iShaderReferenceCount;
	if (m_iShaderReferenceCount <= 0){
		m_iShaderReferenceCount = 0;
		ReleaseClassObjects();
	}
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pDeviceContext);
}

HRESULT CD3D11Font::RestoreClassObjects(){
	HRESULT hr = E_FAIL;

	if (m_pVertexShader != NULL && m_pPixelShader != NULL && m_pInputLayout != NULL && m_pConstantBuffer != NULL){
		return S_OK;
	}

	SAFE_RELEASE(this->m_pVertexShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);
	SAFE_RELEASE(this->m_pConstantBuffer);

	//  Compile shaders
	hr = m_pDevice->CreateVertexShader(m_pVertexShaderCode,m_dwVertexShaderCodeSize,NULL,&m_pVertexShader);
	if (FAILED(hr))
		goto EXIT;

	// 入力レイアウトの定義.
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
	if (FAILED(hr))
		goto EXIT;

	//  ピクセルシェーダの生成
	hr = m_pDevice->CreatePixelShader(m_pPixelShaderCode,m_dwPixelShaderCodeSize,NULL,&m_pPixelShader);
	if (FAILED(hr))
		goto EXIT;


	{
		// 定数バッファの設定.
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.ByteWidth        = sizeof( AnkFontConstantBuffer );
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

HRESULT CD3D11Font::ReleaseClassObjects(){

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pConstantBuffer);
	return S_OK;
}

HRESULT CD3D11Font::RestoreInstanceObjects(){
	//	
	HRESULT hr = CreateFontTexture();

	if (FAILED(hr))
		return hr;

    // シェーダリソースビューを生成.
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		D3D11_TEXTURE2D_DESC	texDesc;
		ZeroMemory( &srvd, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
		m_pTexture->GetDesc(&texDesc);
		srvd.Format                     = texDesc.Format;
  		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MipLevels = texDesc.MipLevels;
		srvd.Texture2D.MostDetailedMip = 0;
		hr = m_pDevice->CreateShaderResourceView( m_pTexture, &srvd, &m_pTextureShaderResourceView);
		if (FAILED(hr))
			return hr;
	}

	//  サンプラ―のセットアップ
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
			AnkFontVertex vertices[] = {
				{  DirectX::XMFLOAT3( -0.5f,  0.5f, 0.0f ),  DirectX::XMFLOAT2(  0.0f,  0.0f ) },
				{  DirectX::XMFLOAT3(  0.5f,  0.5f, 0.0f ),  DirectX::XMFLOAT2(  1.0f,  0.0f ) },
				{  DirectX::XMFLOAT3( -0.5f, -0.5f, 0.0f ),  DirectX::XMFLOAT2(  0.0f,  1.0f ) },
				{  DirectX::XMFLOAT3(  0.5f, -0.5f, 0.0f ),  DirectX::XMFLOAT2(  1.0f,  1.0f ) },
			};
  
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.Usage          = D3D11_USAGE_DEFAULT;
		bd.ByteWidth      = sizeof( AnkFontVertex ) * 4;
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
	return hr;
}

HRESULT CD3D11Font::ReleaseInstanceObjects(){
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pTextureShaderResourceView);
	SAFE_RELEASE(m_pTextureSamplerState);
	SAFE_RELEASE(m_pBlendState);
	m_bInitialized = false;

	return S_OK;
}



HRESULT CD3D11Font::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
	ID3DBlob	*pShader = NULL;
	HRESULT hr;

	m_bInitialized = false;

	pContext->AddRef();
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pDeviceContext);
	m_pDeviceContext = pContext;
	m_pDeviceContext->GetDevice(&m_pDevice);
	
	RestoreClassObjects();
	hr = RestoreInstanceObjects();
#if 0

	//  頂点シェーダの生成
	hr = CompileShaderFromMemory((BYTE*)shadercode,sizeof(shadercode),"VSFunc","vs_4_0",&pShader);
	if (FAILED(hr)){
		return	hr;
	}
	hr = m_pDevice->CreateVertexShader(pShader->GetBufferPointer(),pShader->GetBufferSize(),NULL,&m_pVertexShader);
	if (FAILED(hr)){
		return	hr;
	}


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
	    pShader->GetBufferPointer(),
	    pShader->GetBufferSize(),
	    &m_pInputLayout
	);
	SAFE_RELEASE(pShader);
	if ( FAILED( hr ) )
	{
		return hr;
	}

	// 入力アセンブラに入力レイアウトを設定.
	m_pDeviceContext->IASetInputLayout( m_pInputLayout );

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory((BYTE*)shadercode,sizeof(shadercode),"PSFunc","ps_4_0",&pShader);
	if (FAILED(hr))
		return hr;
	hr = m_pDevice->CreatePixelShader(pShader->GetBufferPointer(),pShader->GetBufferSize(),NULL,&m_pPixelShader);
	SAFE_RELEASE(pShader);
	if (FAILED(hr))
		return	hr;

	// 定数バッファの生成.
	{
		// 定数バッファの設定.
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.ByteWidth        = sizeof( AnkFontConstantBuffer );
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

#endif


	m_bInitialized = true;
	return S_OK;
}

//
//	ANK フォント用テクスチャの生成
//
HRESULT CD3D11Font::CreateFontTexture(){
	HDC		hDC = NULL;
	HBITMAP hbmBitmap = NULL;
	HFONT hFont = NULL;
    hDC       = CreateCompatibleDC( NULL );
	HRESULT hr;
	INT	nHeight;

	DWORD dwBold   = (m_dwFontFlags&D3DFONT_BOLD)   ? FW_BOLD : FW_NORMAL;
    DWORD dwItalic = (m_dwFontFlags&D3DFONT_ITALIC) ? TRUE    : FALSE;


	if (m_dwFontFlags & D3DFONT_SIZE_IN_PIXELS){
		nHeight = (INT)(m_dwFontHeight);
	}else{
		__int64	tmp64 = m_dwFontHeight;
		tmp64 *= (INT)(GetDeviceCaps(hDC, LOGPIXELSY));
		nHeight    = (INT)(tmp64 / 72);	//	フォント高 * dpi * testScale / 72
	}

	// Large fonts need larger textures
    if( nHeight > 64 )
        m_dwTexWidth = m_dwTexHeight = 1024;
    else if( nHeight > 32 )
        m_dwTexWidth = m_dwTexHeight = 512;
    else
        m_dwTexWidth  = m_dwTexHeight = 256;

    D3D11_TEXTURE2D_DESC td;
    ZeroMemory( &td, sizeof( D3D11_TEXTURE2D_DESC ) );
    td.Width                = m_dwTexWidth;
    td.Height               = m_dwTexHeight;
    td.MipLevels            = 1;
    td.ArraySize            = 1;
    td.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count     = 1;	//	MULTI SAMPLE COUNT
    td.SampleDesc.Quality   = 0;	//	MULtI SAMPLE QUALITY
    td.Usage                = D3D11_USAGE_DYNAMIC;	//  Make it writeable
    td.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
    td.MiscFlags            = 0;
  
    // 2Dテクスチャの生成
    hr = m_pDevice->CreateTexture2D( &td, NULL, &this->m_pTexture );
    if ( FAILED( hr ) )
		return hr;

	
	DWORD*      pBitmapBits;
	BITMAPINFO bmi;
	ZeroMemory( &bmi.bmiHeader,  sizeof(BITMAPINFOHEADER) );
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       =  (int)m_dwTexWidth;
	bmi.bmiHeader.biHeight      = -(int)m_dwTexHeight;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount    = 32;

	// ビットマップの作成
	hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS,
										(VOID**)&pBitmapBits, NULL, 0 );
	SetMapMode( hDC, MM_TEXT );

	//	ANTIALIASED_QUALITYを指定してフォントを生成する。
	//	アンチエイリアスを効かせたフォントを得ようとするが、
	//	アンチエイリアスがかかるとは保証されない。
	hFont    = CreateFont( -nHeight, 0, 0, 0, dwBold, dwItalic,
						FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
						CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
						VARIABLE_PITCH, m_strFontName );
	if( NULL==hFont ){
		hr = E_FAIL;
	}else{
		SelectObject( hDC, hbmBitmap );
		SelectObject( hDC, hFont );

		// テキストプロパティーの設定
		SetTextColor( hDC, RGB(255,255,255) );
		SetBkColor(   hDC, 0x00000000 );
		SetTextAlign( hDC, TA_TOP );

		//	texture coordinatesを保存しながら、出力可能な文字をビットマップに
		//	出力してゆく。
		DWORD x = 0;
		DWORD y = 0;
		TCHAR str[2] = _T("x");
		SIZE size;

		for( TCHAR c=32; c<127; c++ )
		{
			str[0] = c;
			GetTextExtentPoint32( hDC, str, 1, &size );

			if( (DWORD)(x+size.cx+1) > m_dwTexWidth )
			{
				x  = 0;
				y += size.cy+1;
			}

			ExtTextOut( hDC, x+0, y+0, ETO_OPAQUE, NULL, str, 1, NULL );

			m_fTexCoords[c-32][0] = ((FLOAT)(x+0))/m_dwTexWidth;
			m_fTexCoords[c-32][1] = ((FLOAT)(y+0))/m_dwTexHeight;
			m_fTexCoords[c-32][2] = ((FLOAT)(x+0+size.cx))/m_dwTexWidth;
			m_fTexCoords[c-32][3] = ((FLOAT)(y+0+size.cy))/m_dwTexHeight;

			x += size.cx+1;
		}

		// Lock the surface and write the alpha values for the set pixels
		D3D11_MAPPED_SUBRESOURCE hMappedResource;
		hr = m_pDeviceContext->Map( 
			m_pTexture,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&hMappedResource );
		if (SUCCEEDED(hr)){
			// ここで書き込む
			BYTE* pBits = (BYTE*)hMappedResource.pData;

			BYTE bAlpha; // 4-bit measure of pixel intensity

			for( y=0; y < m_dwTexHeight; y++ )
			{
				DWORD* pDst32 = (DWORD*)(pBits+(y*hMappedResource.RowPitch));
				for( x=0; x < m_dwTexWidth; x++ )
				{
					bAlpha = (BYTE)(pBitmapBits[m_dwTexWidth*y + x] & 0xff);
					if (bAlpha > 0){
						*pDst32++ = 0x00ffffff | (bAlpha<<24);
					}
					else
					{
						*pDst32++ = 0x00000000;
					}
				}
			}
			m_pDeviceContext->Unmap(m_pTexture,0);
		}
	}

	if (hbmBitmap)
	    DeleteObject( hbmBitmap );
	if (hFont)
		DeleteObject( hFont );
	if (hDC)
	    DeleteDC( hDC );

	return S_OK;
}

HRESULT CD3D11Font::ReleaseDeviceObjects(){

	ReleaseClassObjects();
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	m_bInitialized = false;
	return	S_OK;
}

#ifndef COMPILE_AND_SAVE_SHADER
#include "d3d11fontvertexixelshadecode.inc"
#include "d3d11fontpixelshadercode.inc"
#endif
void CD3D11Font::CompileShaderCodes(){

	RemoveShaderCodes();

#ifndef COMPILE_AND_SAVE_SHADER
	m_pVertexShaderCode = (BYTE*)D3D11FontVertexShaderCode;
	m_pPixelShaderCode = (BYTE*)D3D11FontPixelShaderCode;
	m_dwVertexShaderCodeSize = _countof(D3D11FontVertexShaderCode);
	m_dwPixelShaderCodeSize = _countof(D3D11FontPixelShaderCode);
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

	SaveShaderCode(m_pVertexShaderCode,m_dwVertexShaderCodeSize,_T("d3d11fontvertexixelshadecode.inc"),"D3D11FontVertexShaderCode");
	SaveShaderCode(m_pPixelShaderCode,m_dwPixelShaderCodeSize,_T("d3d11fontpixelshadercode.inc"),"D3D11FontPixelShaderCode");

	hr = S_OK;
EXIT:
	if (FAILED(hr)){
		RemoveShaderCodes();
	}
	//SAFE_DELETE_ARRAY(pSourceCode);
#endif
}

void CD3D11Font::RemoveShaderCodes(){
#ifdef COMPILE_AND_SAVE_SHADER
	SAFE_DELETE_ARRAY(m_pVertexShaderCode);
	SAFE_DELETE_ARRAY(m_pPixelShaderCode);
#endif
	m_dwVertexShaderCodeSize = 0L;
	m_dwPixelShaderCodeSize = 0L;
}

void	CD3D11Font::Render(ID3D11DeviceContext *pContext){

	if (!m_bInitialized)
		return;

	//　シェーダを設定して描画.
	pContext->VSSetShader( m_pVertexShader, NULL, 0 );
	pContext->GSSetShader( NULL,			NULL, 0 );
	pContext->PSSetShader( m_pPixelShader,  NULL, 0 );

	// 定数バッファの設定.
	AnkFontConstantBuffer cb;
	cb.matProj  = m_matProj;
	//memcpy((void*)&cb.matProj,(void*)&m_matProj,sizeof(DirectX::XMMATRIX));
	cb.color = DirectX::XMFLOAT4(1.0,1.0,1.0,1.0);
	// サブリソースを更新.
	pContext->UpdateSubresource( m_pConstantBuffer, 0, NULL, &cb, 0, 0 );

	// ジオメトリシェーダに定数バッファを設定.
	pContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );

	// 入力アセンブラに頂点バッファを設定.
	UINT stride = sizeof( AnkFontVertex );
	UINT offset = 0;
	pContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

	// プリミティブの種類を設定.
	pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { m_pTextureShaderResourceView, 0 };
	ID3D11SamplerState	*ppSamplerStates[] = { m_pTextureSamplerState, 0 };
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
	pContext->PSSetSamplers( 0, 1, ppSamplerStates );

	pContext->Draw(4,0);	//  Draw 4 vertices.

	ppSamplerStates[0] = NULL;
	ppShaderResourceViews[0] = NULL;
	pContext->PSSetSamplers( 0, 1, ppSamplerStates );
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
}

void CD3D11Font::DrawChar(ID3D11DeviceContext *pContext,  char ankCode, DirectX::XMFLOAT4 color, FLOAT *px, FLOAT *py){
	
	if (!m_bInitialized)
		return;

	if (m_pDeviceContext != pContext)
		return;


	char c = ankCode;
	if (c < ' ')
		return;
	FLOAT fTexWidth = (FLOAT)m_dwTexWidth;
	FLOAT fTexHeight = (FLOAT)m_dwTexHeight;
	FLOAT fTexWidthInv = 1.0f / fTexWidth;
	FLOAT fTexHeightInv = 1.0f / fTexHeight;

	FLOAT tx1 = m_fTexCoords[c-32][0];
    FLOAT ty1 = m_fTexCoords[c-32][1];
    FLOAT tx2 = m_fTexCoords[c-32][2];
    FLOAT ty2 = m_fTexCoords[c-32][3];
    FLOAT w = (tx2-tx1) * fTexWidth;
    FLOAT h = (ty2-ty1) * fTexHeight;


	D3D11_VIEWPORT  vp;
	UINT			uiNumViewport = 1;
	m_pDeviceContext->RSGetViewports(&uiNumViewport,&vp);
	if (uiNumViewport != 1)
		return;

	//  transform 2D -> clipping space
	FLOAT l = *px + vp.TopLeftX;
	FLOAT t = *py + vp.TopLeftY;
	FLOAT r = l + w;
	FLOAT b = t + h;
	FLOAT z = 1.0f;
	l /= vp.Width*0.5f;
	t /= vp.Height*0.5f;
	r /= vp.Width*0.5f;
	b /= vp.Height*0.5f;
	l -= 1.0f;
	r -= 1.0f;
	t = 1.0f - t;
	b = 1.0f - b;
	
	*px += w;

	//  Setup the vertex buffer
	{
		//  Definition of the vertices
		AnkFontVertex vertices[] = {
			{  DirectX::XMFLOAT3( l, t, z ),  DirectX::XMFLOAT2(  tx1,  ty1 ) },
			{  DirectX::XMFLOAT3( r, t, z ),  DirectX::XMFLOAT2(  tx2,  ty1 ) },
			{  DirectX::XMFLOAT3( l, b, z ),  DirectX::XMFLOAT2(  tx1,  ty2 ) },
			{  DirectX::XMFLOAT3( r, b, z ),  DirectX::XMFLOAT2(  tx2,  ty2 ) },
		};
		pContext->UpdateSubresource( m_pVertexBuffer, 0, NULL, vertices, 0, 0 );
		
	}

	//　install the shaders
	pContext->VSSetShader( m_pVertexShader,   NULL, 0 );
	pContext->GSSetShader( NULL, NULL, 0 );
	pContext->PSSetShader( m_pPixelShader,    NULL, 0 );

	// 定数バッファの設定.
	AnkFontConstantBuffer cb;
	cb.matProj  = m_matProj;
	//memcpy((void*)&cb.matProj,(void*)&m_matProj,sizeof(DirectX::XMMATRIX));
	cb.color = DirectX::XMFLOAT4(color.x,color.y,color.z,color.w);
	DirectX::XMMatrixTranspose(cb.matProj);
	// サブリソースを更新.
	pContext->UpdateSubresource( m_pConstantBuffer, 0, NULL, &cb, 0, 0 );

	// ジオメトリシェーダに定数バッファを設定.
	pContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );

	// 入力アセンブラに頂点バッファを設定.
	UINT stride = sizeof( AnkFontVertex );
	UINT offset = 0;
	pContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

	// 入力アセンブラに入力レイアウトを設定.
	pContext->IASetInputLayout( m_pInputLayout );

	// プリミティブの種類を設定.
	pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { m_pTextureShaderResourceView, 0 };
	ID3D11SamplerState	*ppSamplerStates[] = { m_pTextureSamplerState, 0 };
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
	pContext->PSSetSamplers( 0, 1, ppSamplerStates );

	pContext->Draw(4,0);	//  Draw 4 vertices.

	ppSamplerStates[0] = NULL;
	ppShaderResourceViews[0] = NULL;
	pContext->PSSetSamplers( 0, 1, ppSamplerStates );
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);

}

void CD3D11Font::DrawAnkText(ID3D11DeviceContext *pContext,const TCHAR *pString, DirectX::XMFLOAT4 color, FLOAT x, FLOAT y){
	FLOAT tmpx = x;
	FLOAT tmpy = y;

	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	pContext->OMSetBlendState( m_pBlendState, blendFactor, 0xffffffff );

	TCHAR *p = (TCHAR*)pString;
	while(*p != (TCHAR)0){
		DrawChar(pContext,(char)(*p & 0x7f),color, &tmpx,&tmpy);
		p++;
	}

	pContext->OMSetBlendState( NULL, blendFactor, 0xffffffff );
}
