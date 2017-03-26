#pragma once

//	�t�@�C�������f�R�[�h���邽�߂̃N���X
//	���X���b�h�Z�[�t�łȂ�
class CFilenameDecoder
{
public:
	CFilenameDecoder(TCHAR	*pFilename,BOOL removeDotDirectory = false);
	virtual ~CFilenameDecoder(void);
	void	GetPath(DWORD *length, TCHAR *pBuffer);
	void	GetFilename(DWORD *length, TCHAR *pBuffer);
	void	GetFullname(DWORD *length, TCHAR *pBuffer);
protected:
	INT		Decode(TCHAR *pPath, VOID *pRootNode);
	void	CreatePathString(VOID *pRootNode,INT numNodes);
	INT		InterpretTheDotDirectory(VOID *pRootNode, INT numNodes);
	TCHAR	*m_strPath;
	TCHAR	*m_strFilename;
};

