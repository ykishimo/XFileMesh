#include "stdafx.h"
#include <crtdbg.h>
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D2D1.h>
#include "Mesh.h"
#include "BoneRenderer.h"

#include "D3DContext.h"

#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }
#define SAFE_RELEASE(o) if(o){  (o)->Release(); o = NULL; }

class BoneRendererShaderBuilder{
public:
	BoneRendererShaderBuilder(){
		CBoneRenderer::CompileShaderCodes();
	}
	~BoneRendererShaderBuilder(){
		CBoneRenderer::RemoveShaderCodes();
	}
}	boneRendererInitializer;


BYTE  *CBoneRenderer::m_pVertexShaderCode = NULL;
DWORD CBoneRenderer::m_dwVertexShaderCodeSize = 0L;
BYTE  *CBoneRenderer::m_pPixelShaderCode = NULL;
DWORD CBoneRenderer::m_dwPixelShaderCodeSize = 0L;

ID3D11VertexShader	*CBoneRenderer::m_pVertexShader = NULL;
ID3D11PixelShader	*CBoneRenderer::m_pPixelShader = NULL;
ID3D11InputLayout	*CBoneRenderer::m_pInputLayout = NULL;
ID3D11Buffer		*CBoneRenderer::m_pConstantBuffer = NULL;
INT					CBoneRenderer::m_iShaderReferenceCount = 0;


//
//	VS Constant Buffer
//
typedef struct
{
    DirectX::XMMATRIX	World;      //  World Matrix
    DirectX::XMMATRIX	View;       //  View Matrix
    DirectX::XMMATRIX	Proj;       //  Projection Matrix
    DirectX::XMFLOAT4	diffuse;    //  line color
} BoneRendererConstantData;


CBoneRenderer::CBoneRenderer(MeshFrame *pFrameRoot)
{
	InitFields();
	this->m_pFrameRoot = pFrameRoot;
	++m_iShaderReferenceCount;
}


CBoneRenderer::~CBoneRenderer(void)
{
	--m_iShaderReferenceCount;
	if (m_iShaderReferenceCount <= 0){
		m_iShaderReferenceCount = 0;
		ReleaseClassObjects();
	}
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
}

void CBoneRenderer::InitFields(){
	m_pDeviceContext = NULL;
	m_pDevice = NULL;
	m_pVertexBuffer = NULL;

	m_matProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2,16.0f/9.0f,0.1f,1000.0f);
	m_matWorld      = DirectX::XMMatrixIdentity();
	{
		DirectX::XMVECTOR camPos    = DirectX::XMVectorSet( 0.0f, 0.0f, -1.75f, 0.0f );
		DirectX::XMVECTOR camTarget = DirectX::XMVectorSet( 0.0f, 0.0f,  0.0f, 0.0f );
		DirectX::XMVECTOR camUpward = DirectX::XMVectorSet( 0.0f, 1.0f,  0.0f, 0.0f );
		m_matView  = DirectX::XMMatrixLookAtLH(camPos,camTarget,camUpward);
	}
}


HRESULT CBoneRenderer::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
	HRESULT	hr = E_FAIL;
	//  Prepare device
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	pContext->AddRef();

	//ReleaseDeviceObjects();
	m_pDeviceContext = pContext;
	m_pDeviceContext->GetDevice(&m_pDevice);

	hr = RestoreClassObjects();

	return RestoreInstanceObjects();
}
HRESULT CBoneRenderer::RestoreClassObjects(){
	HRESULT hr = E_FAIL;
	if (m_pVertexShader != NULL && m_pPixelShader != NULL && m_pInputLayout != NULL && m_pConstantBuffer != NULL){
		return S_OK;
	}

	if (m_pVertexShaderCode == NULL || m_pPixelShaderCode == NULL)
		return E_FAIL;

	hr = m_pDevice->CreateVertexShader(m_pVertexShaderCode,m_dwVertexShaderCodeSize,NULL,&m_pVertexShader);
	if (FAILED(hr))
		goto EXIT;

	// 入力レイアウトの定義.
	static D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {
			"POSITION", 
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 
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


	hr = m_pDevice->CreatePixelShader(m_pPixelShaderCode,m_dwPixelShaderCodeSize,NULL,&m_pPixelShader);

	// 定数バッファの生成.
	{
		// 定数バッファの設定.
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.ByteWidth        = sizeof( BoneRendererConstantData );
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
	//hr = PrepareShaders();
	//hr = S_OK;
EXIT:
	return hr;
}


HRESULT CBoneRenderer::RestoreInstanceObjects(){
	//	
	HRESULT hr;
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
	bd.Usage          = D3D11_USAGE_DEFAULT;
	bd.ByteWidth      = sizeof( m_pVertexSysMem );
	bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory( &initData, sizeof( D3D11_SUBRESOURCE_DATA ) );
	initData.pSysMem = (VOID*)m_pVertexSysMem;
	// 頂点バッファの生成.
	SAFE_RELEASE(m_pVertexBuffer);

	hr = m_pDevice->CreateBuffer( &bd, &initData, &m_pVertexBuffer );
	if ( FAILED( hr ) )
	{
		return	hr;
	}
	return hr;
}

HRESULT CBoneRenderer::ReleaseInstanceObjects(){
	SAFE_RELEASE(m_pVertexBuffer);
	return S_OK;
}

HRESULT CBoneRenderer::ReleaseClassObjects(){

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pConstantBuffer);
	return S_OK;
}
HRESULT CBoneRenderer::ReleaseDeviceObjects(){
	ReleaseInstanceObjects();
	ReleaseClassObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	return	S_OK;
}

//
//	Prepare shaders
//
#ifndef COMPILE_AND_SAVE_SHADER
#include "bonerenderervertexixelshadecode.inc"
#include "bonerendererpixelshadecode.inc"
#else
#define FILENAME "res\\bonerenderer.fx"
#endif

void CBoneRenderer::CompileShaderCodes(){

	RemoveShaderCodes();

#ifndef COMPILE_AND_SAVE_SHADER
	m_pVertexShaderCode = (BYTE*)BoneRendererVertexShaderCode;
	m_pPixelShaderCode = (BYTE*)BoneRendererPixelShaderCode;
	m_dwVertexShaderCodeSize = _countof(BoneRendererVertexShaderCode);
	m_dwPixelShaderCodeSize = _countof(BoneRendererPixelShaderCode);
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
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"BRVSFunc","vs_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pVertexShaderCode = new BYTE[len];
	memcpy(m_pVertexShaderCode,pCode->GetBufferPointer(),len);
	m_dwVertexShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"BRPSFunc","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	len = pCode->GetBufferSize();
	m_pPixelShaderCode = new BYTE[len];
	memcpy(m_pPixelShaderCode,pCode->GetBufferPointer(),len);
	m_dwPixelShaderCodeSize = (DWORD)len;
	SAFE_RELEASE(pCode);

#ifdef COMPILE_AND_SAVE_SHADER
	SaveShaderCode(m_pVertexShaderCode,m_dwVertexShaderCodeSize,_T("bonerenderervertexixelshadecode.inc"),"BoneRendererVertexShaderCode");
	SaveShaderCode(m_pPixelShaderCode,m_dwPixelShaderCodeSize,_T("bonerendererpixelshadecode.inc"),"BoneRendererPixelShaderCode");
#endif

	hr = S_OK;
EXIT:
	if (FAILED(hr)){
		RemoveShaderCodes();
	}
	SAFE_DELETE_ARRAY(pSourceCode);
#endif
}

void CBoneRenderer::RemoveShaderCodes(){
#ifdef COMPILE_AND_SAVE_SHADER
	SAFE_DELETE_ARRAY(m_pVertexShaderCode);
	SAFE_DELETE_ARRAY(m_pPixelShaderCode);
#endif
	m_dwVertexShaderCodeSize = 0L;
	m_dwPixelShaderCodeSize = 0L;
}

#if 0
HRESULT CBoneRenderer::PrepareShaders(){
	HRESULT hr = E_FAIL;
	BYTE *pSourceCode = NULL;
	struct _stat tmpStat;

	if (-1 !=_tstat(_T(FILENAME),&tmpStat)){
		pSourceCode = new BYTE[tmpStat.st_size+3&0xfffffffc];
		FILE *fp;
		_tfopen_s(&fp,_T(FILENAME),_T("rb"));
		fread((void*)pSourceCode,1,tmpStat.st_size,fp);
		fclose(fp);
	}else
		return E_FAIL;

	SAFE_RELEASE(this->m_pVertexShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);

	//  Compile shaders
	ID3DBlob *pCode = NULL;
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"BRVSFunc","vs_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;

	hr = m_pDevice->CreateVertexShader(pCode->GetBufferPointer(),pCode->GetBufferSize(),NULL,&m_pVertexShader);
	if (FAILED(hr))
		goto EXIT;

	// 入力レイアウトの定義.
	static D3D11_INPUT_ELEMENT_DESC layout[] = {
	    {
			"POSITION", 
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 
		}
	};
	UINT numElements = ARRAYSIZE( layout );
	// 入力レイアウトを生成.
	hr = m_pDevice->CreateInputLayout( 
	    layout,
	    numElements,
	    pCode->GetBufferPointer(),
	    pCode->GetBufferSize(),
	    &m_pInputLayout
	);
	SAFE_RELEASE(pCode);
	if (FAILED(hr))
		goto EXIT;

	//  ピクセルシェーダの生成
	hr = CompileShaderFromMemory(pSourceCode,(DWORD)tmpStat.st_size,"BRPSFunc","ps_4_0",&pCode);
	if (FAILED(hr))
		goto EXIT;
	hr = m_pDevice->CreatePixelShader(pCode->GetBufferPointer(),pCode->GetBufferSize(),NULL,&m_pPixelShader);
	SAFE_RELEASE(pCode);
	if (FAILED(hr))
		goto EXIT;

	// 定数バッファの生成.
	{
		// 定数バッファの設定.
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( D3D11_BUFFER_DESC ) );
		bd.ByteWidth        = sizeof( BoneRendererConstantData );
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
	delete [] pSourceCode;
	return hr;
}
#endif

//
//  Render Bones
//	@params:
//		pContext : ID3D11Context
//
void CBoneRenderer::Render(ID3D11DeviceContext *pContext){
	if (pContext != this->m_pDeviceContext)
		return;

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { 0, 0 };
	ID3D11SamplerState	*ppSamplerStates[] = { 0, 0 };
	m_pDeviceContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
	m_pDeviceContext->PSSetSamplers( 0, 1, ppSamplerStates );

	// 入力アセンブラに入力レイアウトを設定.
	m_pDeviceContext->IASetInputLayout( m_pInputLayout );

	// プリミティブの種類を設定.
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );

	//　シェーダを設定
	m_pDeviceContext->VSSetShader( m_pVertexShader,   NULL, 0 );
	m_pDeviceContext->GSSetShader( NULL, NULL, 0 );
	m_pDeviceContext->PSSetShader( m_pPixelShader,    NULL, 0 );

	MeshFrame *pFrame = this->m_pFrameRoot;
	if (pFrame == NULL)
		return;

	if (pFrame->pFrameSibling != NULL){
		RenderSub(pFrame->pFrameSibling,NULL);
	}

	if (pFrame->pFrameFirstChild != NULL){
		RenderSub(pFrame->pFrameFirstChild,&pFrame->CombinedMatrix);
	}
	//　シェーダを設定
	m_pDeviceContext->VSSetShader( NULL, NULL, 0 );
	m_pDeviceContext->GSSetShader( NULL, NULL, 0 );
	m_pDeviceContext->PSSetShader( NULL, NULL, 0 );
}

void CBoneRenderer::RenderSub(MeshFrame *pFrame, DirectX::XMMATRIX *pParent){
	if (pParent != NULL){
		FLOAT *p = pParent->r[3].m128_f32;
		m_pVertexSysMem[0].x = *p++;
		m_pVertexSysMem[0].y = *p++;
		m_pVertexSysMem[0].z = *p++;
		p = pFrame->CombinedMatrix.r[3].m128_f32;
		m_pVertexSysMem[1].x = *p++;
		m_pVertexSysMem[1].y = *p++;
		m_pVertexSysMem[1].z = *p++;

		//	ラインを描画する
		BoneRendererConstantData	brConstants;
		m_pDeviceContext->UpdateSubresource(this->m_pVertexBuffer,0,NULL,m_pVertexSysMem,0,0);
		brConstants.World = DirectX::XMMatrixIdentity();
		brConstants.Proj  = this->m_matProjection;
		brConstants.View  = this->m_matView;
		brConstants.diffuse.x = 1.0;
		brConstants.diffuse.y = 1.0;
		brConstants.diffuse.z = 1.0;
		brConstants.diffuse.w = 1.0;

		// 頂点シェーダに定数バッファを設定.
		m_pDeviceContext->UpdateSubresource( m_pConstantBuffer, 0, NULL, &brConstants, 0, 0 );
		m_pDeviceContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );

		//  頂点バッファを登録
		UINT stride = sizeof( DirectX::XMFLOAT3 );
		UINT offset = 0;
		m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

		m_pDeviceContext->Draw(2,0);	//  Draw 2 vertices.
	}
	if (pFrame->pFrameSibling){
		RenderSub(pFrame->pFrameSibling,pParent);
	}
	if (pFrame->pFrameFirstChild){
		RenderSub(pFrame->pFrameFirstChild,&pFrame->CombinedMatrix);
	}
}

void	CBoneRenderer::SetWorldMatrix(DirectX::XMMATRIX *pMatWorld){
	m_matWorld = *pMatWorld;
}

void	CBoneRenderer::SetViewMatrix(DirectX::XMMATRIX  *pMatView){
	m_matView = *pMatView;
}

void	CBoneRenderer::SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection){
	m_matProjection = *pMatProjection;
}
