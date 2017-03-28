#include "stdafx.h"
#include <D3D11.h>
#include <directxmath.h>
#include <D3Dcompiler.h>
#include <list>
#include <vector>

#include "D3DContext.h"
#include "TextureLoader.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#ifdef COMPILE_AND_SAVE_SHADER
#pragma comment(lib,"d3dcompiler.lib")
void SaveShaderCode(BYTE *pData, DWORD dwSize, TCHAR *pFilename, char *pFuncName){
	FILE *fp;
	size_t len = _tcslen(pFilename);
	len += _tcslen(_T("Graphics\\"));
	TCHAR *pFilename2 = new TCHAR[++len];
	_tcscpy_s(pFilename2,len,_T("Graphics\\"));
	_tcscat_s(pFilename2,len,pFilename);

	_tfopen_s(&fp,pFilename2,_T("w"));
	SAFE_DELETE_ARRAY(pFilename2);
	int		rest = dwSize;
	if (fp != NULL){
		fprintf_s(fp,"const unsigned char %s[]={\n",pFuncName);
		while(rest > 0){
			int col = 0;
			fprintf_s(fp,"\t");
			while(col < 8 && rest > 0){
				fprintf_s(fp,"0x%02x", *pData++);
				++col;
				--rest;
				if (rest>0){
					fprintf_s(fp,", ");
				}
			}
			fprintf_s(fp,"\n");
		}
		fprintf_s(fp,"};\n");
		fclose(fp);
	}
}

//
//	Complie shader from memory
//	@param
//		pCode : source code on memory
//		dwSize : size of source code
//		szEntryPoint : entry point function name
//		szShaderModel : shader model name (ex. ps_4_0 etc)
//
#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)

HRESULT CompileShaderFromMemory(BYTE* pCode, DWORD dwSize, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob **ppBlobOut){
    // リターンコードを初期化.
    HRESULT hr = S_OK;

    // コンパイルフラグ.
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(DEBUG) || defined(_DEBUG)
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif//defiend(DEBUG) || defined(_DEBUG)

#if defined(NDEBUG) || defined(_NDEBUG)
    //dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif//defined(NDEBUG) || defined(_NDEBUG)

    ID3DBlob* pErrorBlob = NULL;

	// シェーダをコンパイル.
	hr = D3DCompile(pCode,dwSize,NULL,NULL,D3D_COMPILE_STANDARD_FILE_INCLUDE,
		szEntryPoint,szShaderModel,dwShaderFlags,0,ppBlobOut,&pErrorBlob);

	// エラーチェック.
    if ( FAILED( hr ) )
    {
        // エラーメッセージを出力.
        if ( pErrorBlob != NULL )
        { OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() ); }
    }

    // 解放処理.
    if ( pErrorBlob )
    {
        pErrorBlob->Release();
        pErrorBlob = NULL;
    }

    // リターンコードを返却.
	return hr;
}


#endif



typedef struct _OutputDesc{
	DWORD				numFormats;
	DXGI_FORMAT			*m_pAvailableFormats;
	DWORD				*m_pNumModes;
	DXGI_MODE_DESC		**m_ppModes;
}	OutputDesc;

typedef struct _AdapterDesc {
	_AdapterDesc(){
		m_dwNumOutputs = 0L;
		m_pOutputDesc = NULL;
	}
	~_AdapterDesc(){
		OutputDesc *pOutput;
		for (UINT i = 0; i < m_dwNumOutputs ; ++i){
			pOutput = &m_pOutputDesc[i];
			if (pOutput->m_ppModes != NULL){
				for (UINT j = 0; j < pOutput->numFormats ; ++j){
					SAFE_DELETE_ARRAY(pOutput->m_ppModes[j]);
				}
			}
			SAFE_DELETE_ARRAY(pOutput->m_ppModes);
			SAFE_DELETE_ARRAY(pOutput->m_pAvailableFormats);
			SAFE_DELETE_ARRAY(pOutput->m_pNumModes);
		}
		SAFE_DELETE(m_pOutputDesc)
	}
	DXGI_ADAPTER_DESC	m_descAdapterDesc;
	DWORD				m_dwNumOutputs;
	OutputDesc			*m_pOutputDesc;
}	AdapterDesc;

//  implementation class
class CD3DContext : public ID3DContext
{
public:
	CD3DContext(HWND hWnd,BOOL bWindowed=true);
	virtual ~CD3DContext(void);
	virtual HRESULT InitD3D();

	//  public methods
	virtual ID3D11Device *GetDevice() override;
	virtual ID3D11DeviceContext *GetDeviceContext() override;
	virtual IDXGISwapChain *GetSwapChain() override;
	virtual ID3D11RenderTargetView *GetRenderTargetView() override;
	virtual ID3D11DepthStencilView *GetDepthStencilView() override;
	virtual ID3D11Texture2D *GetBackBuffer() override;
	virtual ID3D11ShaderResourceView	*GetRenderTargetShaderResourceView() override;
	virtual ID3D11Texture2D				*GetpDepthStencilTexture() override;
	virtual ID3D11ShaderResourceView	*GetDepthStencilShaderResourceView() override;
	virtual const TCHAR *GetDriverTypeText() override;
	virtual const TCHAR *GetFeatureLevelText() override;
	virtual void  GetCurrentViewport(D3D11_VIEWPORT *pViewport) override;
	virtual void  ClearRenderTargetView(float ClearColor[4]) override;
	virtual void  ClearDepthStencilView(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
	virtual HRESULT Present(UINT SyncInterval, UINT Flags) override;
	virtual void  SetDepthEnable(BOOL bDepth, BOOL bWrite) override;
protected:
	virtual HRESULT CreateDeviceContextAndSwapChain();
	virtual BOOL FindMode(DWORD width, DWORD height, DWORD adapter, DWORD output, DWORD *pWidth, DWORD *pHeight, DXGI_FORMAT *pFromat);
	virtual void BuildModes();

	ID3D11Device			*m_pDevice;
	ID3D11DeviceContext		*m_pContext;
	IDXGISwapChain			*m_pSwapChain;
	ID3D11RenderTargetView	*m_pRenderTargetView;
	ID3D11DepthStencilView	*m_pDepthStencilView;
	ID3D11Texture2D			*m_pBackBuffer;
	ID3D11ShaderResourceView	*m_pRenderTargetShaderResourceView;
	ID3D11Texture2D				*m_pDepthStencilTexture;
	ID3D11ShaderResourceView	*m_pDepthStencilShaderResourceView;

	D3D_FEATURE_LEVEL	m_flFeatureLevel;
	D3D_DRIVER_TYPE		m_dtDriverType;
	D3D11_VIEWPORT		m_vpViewport;
	DWORD				m_dwNumAdapters;
	AdapterDesc			*m_pAdapters;

	BOOL				UpdateDepthStencilState();
	D3D11_DEPTH_STENCIL_DESC m_dsdCurrentDepthStencilState;
	BOOL				m_bDepthEnable;
	BOOL				m_bDepthWriteEnable;

	HWND m_hWnd;
	BOOL m_bWindowed;
};


//  local functions
static HRESULT GetBackBufferSize(IDXGISwapChain *pSwapChain, DWORD *pWidth, DWORD *pHeight);
static HRESULT CreateDepthStencilView(
    ID3D11Device *pDevice,
    INT          width,
    INT          height,
    DXGI_FORMAT  fmtDepthStencil,
    ID3D11DepthStencilView    **ppDepthStencilView,
    ID3D11Texture2D           **ppDepthStencilTexture,
    ID3D11ShaderResourceView  **ppDepthStencilShaderResourceView
);
static HRESULT CreateRenderTargetView(
    ID3D11Device             *pDevice,
    IDXGISwapChain           *pSwapChain,
    ID3D11RenderTargetView   **ppRenderTargetView,
    ID3D11ShaderResourceView **ppRenderTargetShaderResourceView
);
/*
static HRESULT CreateDeviceContextAndSwapChain(
    HWND                  hWnd,
    ID3D11Device          **ppDevice,        
    ID3D11DeviceContext   **ppContext,    
    IDXGISwapChain        **ppSwapChain,
    D3D_DRIVER_TYPE       *pDriverType,
    D3D_FEATURE_LEVEL     *pFeatureLevel
);
*/

void CD3DContext::BuildModes(){
	std::vector<AdapterDesc*>	vecAdapters;
	AdapterDesc *pTmpAdapter;
	OutputDesc	*pTmpOutput = NULL;
	std::vector<OutputDesc*>	vecOutputDesc;

	IDXGIFactory *pFactory = NULL;

	SAFE_DELETE_ARRAY(m_pAdapters);
	if (SUCCEEDED(CreateDXGIFactory(IID_IDXGIFactory,(void**)&pFactory))){
		IDXGIAdapter *pAdapter = NULL;
		UINT adapterNo = 0;
		while(true){
			DXGI_FORMAT fmtCandidates[]={
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_FORMAT_B5G6R5_UNORM  ,
				DXGI_FORMAT_B5G5R5A1_UNORM
			};
			if (SUCCEEDED(pFactory->EnumAdapters(adapterNo,&pAdapter))){
				if (pAdapter != NULL){
					UINT uiPort = 0;
					IDXGIOutput *pOutput = NULL;
					pTmpAdapter = new AdapterDesc();
					pAdapter->GetDesc(&pTmpAdapter->m_descAdapterDesc);
					pTmpAdapter->m_dwNumOutputs = 0;
					while(true){
						if (SUCCEEDED(pAdapter->EnumOutputs(uiPort,&pOutput))){
							if (pOutput != NULL){
								UINT numModesSum,numModes[_countof(fmtCandidates)];
								UINT countModes;
								DXGI_MODE_DESC *pModes;
								pTmpOutput = new OutputDesc();
								pTmpOutput->numFormats = 0L;
								pTmpOutput->m_pAvailableFormats = NULL;
								pTmpOutput->m_ppModes = NULL;
								pTmpOutput->m_pNumModes = 0L;
								numModesSum = 0;
								countModes = 0;
								for (int fmt = 0; fmt < _countof(fmtCandidates) ; ++fmt){
									if (SUCCEEDED(pOutput->GetDisplayModeList(fmtCandidates[fmt],0,&numModes[fmt],NULL))){
										numModesSum += numModes[fmt];
										if (numModes[fmt] > 0){
											++countModes;
										}
									}
								}
								pTmpOutput->numFormats = countModes;
								pTmpOutput->m_pNumModes = new DWORD[countModes];
								if (numModesSum > 0){
									//  if there is at least one mode
									UINT current = 0;			
									UINT modeNo = 0;
									pTmpOutput->m_ppModes = new DXGI_MODE_DESC*[countModes];
									pTmpOutput->m_pAvailableFormats = new DXGI_FORMAT[countModes];
									for (int fmt = 0; fmt < _countof(fmtCandidates) ; ++fmt){
										if (numModes[fmt] > 0){
											UINT num = (UINT)numModes[fmt];
											pTmpOutput->m_pAvailableFormats[modeNo] = fmtCandidates[fmt];
											pModes = new DXGI_MODE_DESC[num];								
											pTmpOutput->m_pNumModes[modeNo] = (DWORD)num;
											pOutput->GetDisplayModeList(fmtCandidates[fmt],0,&num,pModes);
											pTmpOutput->m_ppModes[modeNo++] = pModes;
											pModes = NULL;
										}
									}
									vecOutputDesc.push_back(pTmpOutput);
									pTmpOutput = NULL;
								}else{
									SAFE_DELETE(pTmpOutput);
								}
								pOutput->Release();
								pOutput = NULL;
								++uiPort;
								continue;
							}
						}
						break;
					}
					if (uiPort > 0){
						size_t numOutputs = vecOutputDesc.size();
						if (numOutputs > 0){
							OutputDesc *pOutput = NULL;
							pTmpAdapter->m_pOutputDesc = new OutputDesc[numOutputs];
							pTmpAdapter->m_dwNumOutputs = numOutputs;
							for (UINT i = 0; i < numOutputs ; ++i){
								pOutput = vecOutputDesc.at(i);
								if (pOutput != NULL){
									pTmpAdapter->m_pOutputDesc[i] = *pOutput;
									pOutput->m_pAvailableFormats = NULL;
									pOutput->m_ppModes = NULL;
									delete pOutput;
									vecOutputDesc.at(i) = NULL;
								}
							}
							vecOutputDesc.clear();
							vecAdapters.push_back(pTmpAdapter);
							pTmpAdapter = NULL;
						}
					}
					SAFE_DELETE(pTmpAdapter);
					pAdapter->Release();
					pAdapter = NULL;
					++adapterNo;
					continue;
				}
			}
			break;
		}
		if (vecAdapters.size() > 0){
			size_t	numAdapters = vecAdapters.size();
			m_dwNumAdapters = numAdapters;
			this->m_pAdapters = new AdapterDesc[numAdapters];
			for (UINT i = 0; i < numAdapters ; ++i){
				AdapterDesc *pAdapter = vecAdapters.at(i);
				m_pAdapters[i] = *pAdapter;	//  this copy is a bad one
				pAdapter->m_pOutputDesc = NULL;
				pAdapter->m_dwNumOutputs = 0;
				delete pAdapter;
				vecAdapters.at(i) = NULL;
			}
		}
		SAFE_RELEASE(pFactory);
	}
}

//  Factory
HRESULT CreateD3DContext(ID3DContext **ppContext, HWND hWnd, BOOL bWindowed){

	CD3DContext *pContext = new CD3DContext(hWnd,bWindowed);
	HRESULT hr;
	if (pContext){
		hr = pContext->InitD3D();
		if (FAILED(hr)){
			SAFE_DELETE(pContext);
			return E_FAIL;
		}
	}
	*ppContext = pContext;
	return S_OK;
}

//  pure virtual destructor needs entity
ID3DContext::~ID3DContext(){
}

void PrepareDefaultDepthStencilDesc(D3D11_DEPTH_STENCIL_DESC *pdsd){
	pdsd->DepthEnable = TRUE;
	pdsd->DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	pdsd->DepthFunc = D3D11_COMPARISON_LESS;
	pdsd->StencilEnable = FALSE;
	pdsd->StencilReadMask = 0xFF;
	pdsd->StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	pdsd->FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	pdsd->FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	pdsd->FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	pdsd->FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	pdsd->BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	pdsd->BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	pdsd->BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	pdsd->BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
}


CD3DContext::CD3DContext(HWND hWnd, BOOL bWindowed)
{
	m_pDevice = NULL;
	m_pContext = NULL;
	m_pSwapChain = NULL;
	m_pRenderTargetView = NULL;
	m_pDepthStencilView = NULL;
	m_pBackBuffer = NULL;
	m_pRenderTargetShaderResourceView = NULL;
	m_pDepthStencilTexture = NULL;
	m_pDepthStencilShaderResourceView = NULL;
	m_pAdapters = NULL;
	m_dwNumAdapters = 0;
	m_flFeatureLevel = D3D_FEATURE_LEVEL_9_1;
	m_dtDriverType = D3D_DRIVER_TYPE_UNKNOWN;
	m_bDepthEnable = true;
	m_bDepthWriteEnable = true;

	PrepareDefaultDepthStencilDesc(&m_dsdCurrentDepthStencilState);

	m_hWnd = hWnd;
	m_bWindowed = bWindowed;
}


CD3DContext::~CD3DContext(void)
{
    if (m_pContext){
        m_pContext->ClearState();
        m_pContext->Flush();
    }

    SAFE_RELEASE(m_pRenderTargetShaderResourceView);
    SAFE_RELEASE(m_pDepthStencilTexture);
    SAFE_RELEASE(m_pDepthStencilShaderResourceView);
    SAFE_RELEASE(m_pRenderTargetView);
    SAFE_RELEASE(m_pDepthStencilView);
    SAFE_RELEASE(m_pSwapChain);
#ifdef _DEBUG
	if (m_pContext){
		ULONG ref = m_pContext->Release();
		m_pContext = NULL;
	}
	if (m_pDevice){
		ULONG ref = m_pDevice->Release();
		m_pDevice = NULL;
	}
#else
    SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pDevice);
#endif
	SAFE_DELETE_ARRAY(m_pAdapters);
	CTextureLoader::Destroy();
}

HRESULT CD3DContext::InitD3D(){
	HRESULT hr;
   DWORD        width, height;     //  Screen width, Screen height

   //  Modeリストの作成
   BuildModes();

    //  デバイス、デバイスコンテキストそしてスワップチェインを生成
    //hr = CreateDeviceContextAndSwapChain(m_hWnd,&m_pDevice,&m_pContext,&m_pSwapChain,&m_dtDriverType,&m_flFeatureLevel);
    hr = CreateDeviceContextAndSwapChain();
    if ( FAILED( hr ) )
        goto ERROR_EXIT;

    //  RenderTaget を生成
    hr = CreateRenderTargetView(m_pDevice,m_pSwapChain,&m_pRenderTargetView,&m_pRenderTargetShaderResourceView);
    if ( FAILED( hr ) )
        goto ERROR_EXIT;

    //  深度ステンシルバッファ生成のため Backbuffer のサイズを取得
    hr = GetBackBufferSize(m_pSwapChain,&width, &height);
    if ( FAILED( hr ) )
        goto ERROR_EXIT;

    //  サイズとフォーマットを指定して、深度ステンシルバッファを生成
    hr = CreateDepthStencilView(m_pDevice,width,height,DXGI_FORMAT_D16_UNORM,&m_pDepthStencilView,&m_pDepthStencilTexture,&m_pDepthStencilShaderResourceView);
    if ( FAILED( hr ) )
        goto ERROR_EXIT;

    //  デバイスコンテキストにレンダーターゲットと深度ステンシルを設定
    m_pContext->OMSetRenderTargets( 1, &m_pRenderTargetView,m_pDepthStencilView);

	//  SetDepthStencilState
	{
		ID3D11DepthStencilState	*pDepthStencilState = NULL;
		UpdateDepthStencilState();
		m_pDevice->CreateDepthStencilState(&m_dsdCurrentDepthStencilState,&pDepthStencilState);
		m_pContext->OMSetDepthStencilState(pDepthStencilState,0);
		SAFE_RELEASE(pDepthStencilState);
	}
    // ビューポートの設定.
    D3D11_VIEWPORT vp;
    vp.Width    = (FLOAT)width;
    vp.Height   = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;

    // デバイスコンテキストにビューポートを設定.
    m_pContext->RSSetViewports( 1, &vp );

ERROR_EXIT:
	return hr;
}

ID3D11Device *CD3DContext::GetDevice(){
	if (m_pDevice){
		m_pDevice->AddRef();
	}
	return m_pDevice;
}

ID3D11DeviceContext *CD3DContext::GetDeviceContext(){
	if (m_pContext){
		m_pContext->AddRef();
	}
	return m_pContext;
}

IDXGISwapChain *CD3DContext::GetSwapChain(){
	if (m_pSwapChain)
		m_pSwapChain->AddRef();
	return m_pSwapChain;
}

ID3D11RenderTargetView *CD3DContext::GetRenderTargetView(){
	if (m_pRenderTargetView)
		m_pRenderTargetView->AddRef();
	return m_pRenderTargetView;
}

ID3D11DepthStencilView *CD3DContext::GetDepthStencilView(){
	if (m_pDepthStencilView)
		m_pDepthStencilView->AddRef();
	return m_pDepthStencilView;
}

ID3D11Texture2D *CD3DContext::GetBackBuffer(){
	if (m_pBackBuffer)
		m_pBackBuffer->AddRef();
	return m_pBackBuffer;
}

ID3D11ShaderResourceView	*CD3DContext::GetRenderTargetShaderResourceView(){
	if (m_pRenderTargetShaderResourceView)
		m_pRenderTargetShaderResourceView->AddRef();
	return m_pRenderTargetShaderResourceView;;
}

ID3D11Texture2D				*CD3DContext::GetpDepthStencilTexture(){
	if (m_pDepthStencilTexture)
		m_pDepthStencilTexture->AddRef();
	return m_pDepthStencilTexture;
}
ID3D11ShaderResourceView	*CD3DContext::GetDepthStencilShaderResourceView(){
	if (m_pDepthStencilShaderResourceView)
		m_pDepthStencilShaderResourceView->AddRef();
	return m_pDepthStencilShaderResourceView;
}

void	CD3DContext::GetCurrentViewport(D3D11_VIEWPORT *pViewport){
	*pViewport = m_vpViewport;
}

void CD3DContext::ClearRenderTargetView(float ClearColor[4]){
	m_pContext->ClearRenderTargetView(m_pRenderTargetView,ClearColor);
}
void CD3DContext::ClearDepthStencilView(UINT ClearFlags, FLOAT Depth, UINT8 Stencil){
	m_pContext->ClearDepthStencilView(m_pDepthStencilView,ClearFlags,Depth,Stencil);
}

HRESULT CD3DContext::Present(UINT SyncInterval, UINT Flags){
	return m_pSwapChain->Present(SyncInterval, Flags); 
}

void CD3DContext::SetDepthEnable(BOOL bDepth,BOOL bWrite){
    //  デバイスコンテキストにレンダーターゲットと深度ステンシルを設定
	//   OMSetDepthStencilState を使った方が正解！
	m_bDepthEnable = bDepth;
	m_bDepthWriteEnable = bWrite;
	if (UpdateDepthStencilState()){
		ID3D11DepthStencilState	*pDepthStencilState = NULL;
		m_pDevice->CreateDepthStencilState(&m_dsdCurrentDepthStencilState,&pDepthStencilState);
		m_pContext->OMSetDepthStencilState(pDepthStencilState,0);
		SAFE_RELEASE(pDepthStencilState);
	}
}

BOOL CD3DContext::UpdateDepthStencilState(){
	BOOL	bChanged = false;
	if (m_dsdCurrentDepthStencilState.DepthEnable != m_bDepthEnable){
		m_dsdCurrentDepthStencilState.DepthEnable = m_bDepthEnable;
		bChanged = true;
	}
	if (m_bDepthWriteEnable){
		if (m_dsdCurrentDepthStencilState.DepthWriteMask != D3D11_DEPTH_WRITE_MASK_ALL){
			m_dsdCurrentDepthStencilState.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			bChanged = true;
		}
	}else{
		if (m_dsdCurrentDepthStencilState.DepthWriteMask != D3D11_DEPTH_WRITE_MASK_ZERO){
			m_dsdCurrentDepthStencilState.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			bChanged = true;
		}
	}
	return bChanged;
}

BOOL CD3DContext::FindMode(DWORD width, DWORD height, DWORD adapter, DWORD output,
						   DWORD *pWidth, DWORD *pHeight, DXGI_FORMAT *pFormat)
{
	if (adapter >= m_dwNumAdapters)
		return false;
	AdapterDesc *pAdapter = &m_pAdapters[adapter];
	if (output >= pAdapter->m_dwNumOutputs)
		return false;
	OutputDesc *pOutput = &pAdapter->m_pOutputDesc[output];
	INT nearest = INT_MAX;
	INT nearestMode = -1;
	for (UINT fmt = 0; fmt < pOutput->numFormats ; ++fmt){
		DXGI_FORMAT format = pOutput->m_pAvailableFormats[fmt];
		DXGI_MODE_DESC	*pMode = pOutput->m_ppModes[fmt];
		for (UINT mode = 0; mode < pOutput->m_pNumModes[fmt] ; ++mode){
			if (pMode[mode].Width == width && pMode[mode].Height == height){
				*pWidth = width;
				*pHeight = height;
				*pFormat = format;
				return true;
			}else{
				INT difference = pMode[mode].Height - height;
				if (abs(difference) < abs(nearest)){
					nearestMode = mode;
					nearest = difference;
				}
			}
		}
		if (nearestMode >= 0){
			*pWidth = pMode[nearestMode].Width;
			*pHeight = pMode[nearestMode].Height;
			*pFormat = pMode[nearestMode].Format;
			return true;
		}
	}
	return false;
}

//=============================================================================
//
//    Create DeviceContext and SwapChain
//
HRESULT CD3DContext::CreateDeviceContextAndSwapChain(
){
    HRESULT hr = E_FAIL;

    // デバイス生成フラグ.
    UINT createDeviceFlags = 0;

    // ドライバータイプ候補
    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTytpes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

    // 機能レベル候補
    static const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = sizeof( featureLevels ) / sizeof( featureLevels[0] );

	DXGI_FORMAT	dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT width, height;
	if (m_bWindowed){
		RECT rc;
		GetClientRect( m_hWnd, &rc );
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
	}else{
		RECT rc;
		HWND hWnd = GetDesktopWindow();
		GetWindowRect( hWnd, &rc );
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
		DWORD dwWidth, dwHeight;
		if (!FindMode((DWORD)width,(DWORD)height,0,0,&dwWidth,&dwHeight,&dxgiFormat)){
			m_bWindowed = false;
			GetClientRect( m_hWnd, &rc );
			width = rc.right - rc.left;
			height = rc.bottom - rc.top;
		}
		width = (UINT)dwWidth;
		height = (UINT)dwHeight;
	}
    // スワップチェインの構成設定.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof(DXGI_SWAP_CHAIN_DESC) );
    sd.BufferCount                          = 1;
    sd.BufferDesc.Width                     = width;
    sd.BufferDesc.Height                    = height;
    sd.BufferDesc.Format                    = dxgiFormat;
    sd.BufferDesc.RefreshRate.Numerator     = 60;
    sd.BufferDesc.RefreshRate.Denominator   = 1;
    sd.BufferUsage                          = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
    sd.OutputWindow                         = m_hWnd;
    sd.SampleDesc.Count                     = 1;
    sd.SampleDesc.Quality                   = 0;
    sd.Windowed                             = m_bWindowed;

    for( UINT idx = 0; idx < numDriverTytpes; ++idx ){
        // ドライバータイプ設定.
        D3D_DRIVER_TYPE DriverType = driverTypes[ idx ];
  
        // デバイスとスワップチェインの生成.
        hr = D3D11CreateDeviceAndSwapChain(
            NULL, 
            DriverType,
            NULL,
            createDeviceFlags,
            featureLevels,
            numFeatureLevels,
            D3D11_SDK_VERSION,
            &sd,
            &m_pSwapChain,
            &m_pDevice,
			&m_flFeatureLevel,
            &m_pContext 
        );
        m_dtDriverType = DriverType;

        // 成功したらループを脱出.
        if ( SUCCEEDED( hr ) )
            break; 
    }
    return hr;
}

//=============================================================================
//
//    Create Render Target
//    @param :
//        pDevice    : (in)Direct3D device object
//        pSwapChain : (in)device's swap chain
//        ppRenderTargetView : (out)Created render target view
//        ppRenderTargetShaderResourceView : (out)render target's shader resource view
//
HRESULT CreateRenderTargetView(
    ID3D11Device        *pDevice,
    IDXGISwapChain        *pSwapChain,
    ID3D11RenderTargetView    **ppRenderTargetView,
    ID3D11ShaderResourceView    **ppRenderTargetShaderResourceView
){
    ID3D11Texture2D    *pBackBuffer = NULL;
    HRESULT hr = E_FAIL;

    //  スワップチェインよりバックバッファを取得.
    hr = pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
    if ( FAILED( hr ) )
        goto ERROR_EXIT;

    //    バックバッファより、レンダーターゲットを生成.
    hr = pDevice->CreateRenderTargetView( pBackBuffer, NULL, ppRenderTargetView );
    if ( FAILED( hr ) )
        goto ERROR_EXIT;

    // レンダーターゲットのシェーダリソースビューを生成.
    hr = pDevice->CreateShaderResourceView( pBackBuffer, NULL, ppRenderTargetShaderResourceView );
    if ( FAILED( hr ) )
        goto ERROR_EXIT;

ERROR_EXIT:
    SAFE_RELEASE(pBackBuffer);    //    バックバッファを開放

    return hr;
}

//=============================================================================
//
//    Get back-buffer size from swap chain
//    @param :
//        pSwapChain : (in)swap chain
//        pWidth     : (out)back-buffer width
//        pHeight    : (out)back-buffer height]
//
HRESULT GetBackBufferSize(IDXGISwapChain *pSwapChain, DWORD *pWidth, DWORD *pHeight){
    ID3D11Texture2D    *pBackBuffer = NULL;
    HRESULT hr = E_FAIL;
    D3D11_TEXTURE2D_DESC desc;
    //  スワップチェインよりバックバッファを取得.
    hr = pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), 
                         (LPVOID*)&pBackBuffer );
    if ( FAILED( hr ) )
        goto ERROR_EXIT;
    pBackBuffer->GetDesc(&desc);

    *pWidth  = (DWORD)desc.Width;
    *pHeight = (DWORD)desc.Height;
    hr = S_OK;

ERROR_EXIT:
    SAFE_RELEASE(pBackBuffer);
    return hr;
}

//=============================================================================
//
//    create depth-stencil view
//    @param :
//        pDevice : (in)Direct3D device
//        width   : (in)screen width
//        height  : (in)screen height
//        fmtDepthStencil    : (in)depth stencil format.
//        ppDepthStencilView : (out)depth stencil buffer
//        ppDepthStencilTexture : (out)depth stencil surface texture
//        ppDepthStencilShaderResourceView : (out)depth-stencil's shader resource view
//        
HRESULT    CreateDepthStencilView(
    ID3D11Device        *pDevice,
    INT    width,
    INT height,
    DXGI_FORMAT fmtDepthStencil,
    ID3D11DepthStencilView        **ppDepthStencilView,
    ID3D11Texture2D                **ppDepthStencilTexture,
    ID3D11ShaderResourceView    **ppDepthStencilShaderResourceView
){
    DXGI_FORMAT textureFormat   = DXGI_FORMAT_R16_TYPELESS;
    DXGI_FORMAT resourceFormat  = DXGI_FORMAT_R16_UNORM;
    HRESULT hr = E_FAIL;

    //  深度ステンシルテクスチャとシェーダリソースビューのフォーマットを
    //  適切なものに変更.
    switch( fmtDepthStencil )
    {
    case DXGI_FORMAT_D16_UNORM:
        textureFormat  = DXGI_FORMAT_R16_TYPELESS;
        resourceFormat = DXGI_FORMAT_R16_UNORM;
        break;
  
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        textureFormat  = DXGI_FORMAT_R24G8_TYPELESS;
        resourceFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        break;
  
    case DXGI_FORMAT_D32_FLOAT:
        textureFormat  = DXGI_FORMAT_R32_TYPELESS;
        resourceFormat = DXGI_FORMAT_R32_FLOAT;
        break;
  
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        textureFormat  = DXGI_FORMAT_R32G8X24_TYPELESS;
        resourceFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        break;
    default:
        hr = E_FAIL;
        goto ERROR_EXIT;
    }
  
    //    深度ステンシルテクスチャの生成.
    D3D11_TEXTURE2D_DESC td;
    ZeroMemory( &td, sizeof( D3D11_TEXTURE2D_DESC ) );
    td.Width                = width;
    td.Height               = height;
    td.MipLevels            = 1;
    td.ArraySize            = 1;
    td.Format               = textureFormat;
    td.SampleDesc.Count     = 1;    //    MULTI SAMPLE COUNT
    td.SampleDesc.Quality   = 0;    //    MULtI SAMPLE QUALITY
    td.Usage                = D3D11_USAGE_DEFAULT;
    td.BindFlags            = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags       = 0;
    td.MiscFlags            = 0;
  
    // 深度ステンシルテクスチャの生成.
    hr = pDevice->CreateTexture2D( &td, NULL, ppDepthStencilTexture );
    if ( FAILED( hr ) )
        goto ERROR_EXIT;
  
    // 深度ステンシルビューの設定.
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
    ZeroMemory( &dsvd, sizeof( D3D11_DEPTH_STENCIL_VIEW_DESC ) );
    dsvd.Format         = fmtDepthStencil;
    if ( td.SampleDesc.Count == 0 )
    {
        dsvd.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvd.Texture2D.MipSlice = 0;
    }
    else
    {
        dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }
  
    // 深度ステンシルビューの生成.
    hr = pDevice->CreateDepthStencilView( *ppDepthStencilTexture, &dsvd, ppDepthStencilView );
    if ( FAILED( hr ) )
    {
      goto ERROR_EXIT;
    }

    // シェーダリソースビューの設定.
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    ZeroMemory( &srvd, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
    srvd.Format                     = resourceFormat;
  
    if ( td.SampleDesc.Count == 0 )
    {
        srvd.ViewDimension              = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MostDetailedMip  = 0;
        srvd.Texture2D.MipLevels        = 1;
    }
    else
    {
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    }
  
    // シェーダリソースビューを生成.
    hr = pDevice->CreateShaderResourceView( *ppDepthStencilTexture, &srvd, ppDepthStencilShaderResourceView );
    if ( FAILED( hr ) )
    { 
      goto ERROR_EXIT;
    }
ERROR_EXIT:
    return hr;
}


//=============================================================================
//
//  Debug utility
//  Get the name of driver type
//
const TCHAR *CD3DContext::GetDriverTypeText(){
    switch(m_dtDriverType){
    case    D3D_DRIVER_TYPE_UNKNOWN:
        return _T("D3D_DRIVER_TYPE_UNKNOWN");
    case    D3D_DRIVER_TYPE_HARDWARE:    
        return _T("D3D_DRIVER_TYPE_HARDWARE");
    case    D3D_DRIVER_TYPE_REFERENCE:
        return _T("D3D_DRIVER_TYPE_REFERENCE");
    case    D3D_DRIVER_TYPE_NULL:
        return _T("D3D_DRIVER_TYPE_NULL");
    case    D3D_DRIVER_TYPE_SOFTWARE:    
        return _T("D3D_DRIVER_TYPE_SOFTWARE");
    case    D3D_DRIVER_TYPE_WARP:        
        return _T("D3D_DRIVER_TYPE_WARP");
    }
    return NULL;
}

//=============================================================================
//
//  Debug utility
//  Get the name of feature level
//
const TCHAR *CD3DContext::GetFeatureLevelText(){
    switch(m_flFeatureLevel){
    case    D3D_FEATURE_LEVEL_9_1:    
        return _T("D3D_FEATURE_LEVEL_9_1");
    case    D3D_FEATURE_LEVEL_9_2:    
        return _T("D3D_FEATURE_LEVEL_9_2");
    case    D3D_FEATURE_LEVEL_9_3:
        return _T("D3D_FEATURE_LEVEL_9_3");
    case    D3D_FEATURE_LEVEL_10_0:
        return _T("D3D_FEATURE_LEVEL_10_0");
    case    D3D_FEATURE_LEVEL_10_1:
        return _T("D3D_FEATURE_LEVEL_10_1");
    case    D3D_FEATURE_LEVEL_11_0:
        return _T("D3D_FEATURE_LEVEL_11_0");
    }
    return NULL;
}


