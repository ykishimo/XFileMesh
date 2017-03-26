#ifndef	__TextureDB_h__
#define	__TextureDB_h__

class	CTextureNode;
class	CD3D11TextureDB{
public:
	void	DeleteDecendants(CTextureNode *pNode);
	void	DeleteTheNode(CTextureNode *pNode);
	BOOL    RegisterNewKey(TCHAR *key, CTextureNode **pRet);
	HRESULT	RestoreDeviceObjects(ID3D11DeviceContext *pContext);
	static HRESULT RestoreNode(ID3D11DeviceContext *pContext, CTextureNode *pNode);
	HRESULT	InvalidateDeviceObjects();
	static	CD3D11TextureDB *GetInstance();
	static	void Destroy();
private:
	CD3D11TextureDB();
	virtual	~CD3D11TextureDB();

	static CD3D11TextureDB	*m_pInstance;
	HRESULT	RestoreObjectsOfDecendants(ID3D11DeviceContext *pContext,CTextureNode *pNode);
	HRESULT	InvalidateObjectsOfDecendants(CTextureNode *pNode);
	DWORD				dwUnique;
	DWORD				dwNumNodes;
	CTextureNode		*m_pNode;
	CTextureNode		*m_pSentinel;
};


class	CTextureNode{
public:
	CTextureNode(TCHAR *pStr,ID3D11Texture2D *pTex = NULL);
	void	StoreTexture(ID3D11Texture2D *pTex){
		m_pTexture = pTex;
	}
	void	Release();
	ID3D11Texture2D	*GetTexture(){
		return	m_pTexture;
	}
	ID3D11ShaderResourceView	*GetTextureShaderResourceView(){
		return	m_pTextureShaderResourceView;
	}
	TCHAR	*GetFileName(){
		return	m_strFilename;
	}
private:
	friend class CD3D11TextureDB;
	CD3D11TextureDB		*m_pTextureDB;
	CTextureNode	**m_ppParent;
	CTextureNode	*m_pLeft;
	CTextureNode	*m_pRight;
	int				m_iRefCount;
	TCHAR			*m_strFilename;
	ID3D11Texture2D	*m_pTexture;
	ID3D11ShaderResourceView *m_pTextureShaderResourceView;
};

#endif
