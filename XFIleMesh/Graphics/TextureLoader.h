#pragma once
struct IWICImagingFactory;

typedef enum _FillMode{
	None = 0,
	Linear = 1
} FillMode;

class CTextureLoader
{
private:
	CTextureLoader(void);
	virtual ~CTextureLoader(void);

public:
	static void Initialize();
	static CTextureLoader *GetInstance();
	static void Destroy();
	static HRESULT CreateTextureFromFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight, FillMode dwFillmode = FillMode::None);
	static HRESULT CreateD2D1BitmapFromFile(ID2D1RenderTarget *pRenderTarget,TCHAR *pFilename, ID2D1Bitmap **ppBitmap);
protected:
	IWICImagingFactory *m_pFactory;
private:
	static CTextureLoader *m_pInstance;
};

