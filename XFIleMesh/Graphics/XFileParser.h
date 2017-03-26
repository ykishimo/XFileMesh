#pragma once

//#include "Mesh.h"
//#include "Animation.h"

struct FileReadingContext;
struct TempMesh;
struct TempAnimationSet;
struct MeshFrame;
struct MeshContainer;
struct AnimationSet;
struct Animation;
struct SkinWeights;
struct Vector3KeyFrame;
struct QuaternionKeyFrame;
struct Mesh;
struct Material;

class CXFileParser
{
public:
	static HRESULT CreateMesh(TCHAR *pFilename, MeshFrame **ppResult, AnimationSet ***pppAnimationSets, DWORD *pdwNumAnimationSets);

protected:
	CXFileParser(void);
	virtual ~CXFileParser(void);
	HRESULT	OpenRead(TCHAR *pFilename);
	void	Close();
	
	CHAR	*CheckNextToken();
	void	SkipToken();
	CHAR	*GetToken();
	CHAR	*GetTokenText();
	CHAR	*GetTokenBin();
	BOOL	GetNumericToken(CHAR **ppTextResult, FLOAT *pFloatResult, INT *pIntResult);
	BOOL	GetNumericTokenText(CHAR **ppTextResult, FLOAT *pFloatResult, INT *pIntResult);
	BOOL	GetNumericTokenBin(CHAR **ppTextResult, FLOAT *pFloatResult, INT *pIntResult);

	CHAR	*DecodeArrayToText();
	CHAR	*ReturnFloat(FLOAT a);		//
	CHAR	*ReturnInt(INT a);			//
	UINT	GetPos();					//
	void	SetPos(UINT pos);			//	
	UINT	GetSize();					//
	HRESULT	SkipToNextBrace();			//  
	HRESULT SkipTemplate();
	HRESULT CheckHeader();
	HRESULT DecompressMSZip();			//	Compressed XFile

	HRESULT	GetRootMesh(MeshFrame **ppMeshFrame);
	HRESULT	GetMeshFrame(MeshFrame **ppMeshFrame);
	HRESULT GetMatrix(DirectX::XMMATRIX *pMatrix);
	HRESULT GetMeshContainer(MeshContainer **ppMeshContainer);

	HRESULT Get3DVectors(DirectX::XMFLOAT3 **ppVectors, INT *pNumVectors);
	HRESULT Get2DVectors(DirectX::XMFLOAT2 **ppVectors, INT *pNumVectors);
	HRESULT GetTriangleIndices(INT **ppIndices, INT *pNumTriangles, INT **ppOriginalFaceReference, INT *pNumReference);
	HRESULT GetSkinWeights(SkinWeights **ppSkinWeights);
	HRESULT GetMeshMaterialList(TempMesh *pMeshData);
	HRESULT GetMaterial(Material **ppMaterial);
	HRESULT GetMeshGeometry(TempMesh *pMeshData);
	HRESULT GetMeshNormals(TempMesh *pMeshData);
	HRESULT GetMeshTextureCoords(TempMesh *pMeshData);
	HRESULT GetSkinMeshHeader(INT *pNumBones, INT *pNumMaxWeightsPerVertex, INT *pNumMaxWeightsPerFace);
	HRESULT GetAnimationSet(TempAnimationSet **ppAnimationSet);
	HRESULT GetAnimation(Animation **ppAnimation);
	HRESULT Get4DAnimationKeys(QuaternionKeyFrame **ppAnimationKeys,INT *pCount);
	HRESULT Get3DAnimationKeys(Vector3KeyFrame **ppAnimationKeys,INT *pCount);

	//
	HRESULT	EnflatLoadedFrames();
	HRESULT EnflatAnimations();
	HRESULT ConfirmAnimationSet(AnimationSet *pAnimationSet);

	HRESULT PrepareVertices(MeshContainer *pMeshContainer,TempMesh *pTMesh,Mesh *pMesh);
	HRESULT PrepareWeights(MeshContainer *pMeshContainer,TempMesh *pTMesh, BYTE *pBuffer, INT weightOffset, INT stride);
	HRESULT CheckBoneId(TempMesh *pTMesh);
	HRESULT SortFacesByMaterial(MeshContainer *pMeshContainer,TempMesh *pTMesh, Mesh *pMesh);
	HRESULT OrganizeMeshContainer(MeshContainer *pMeshContainer,TempMesh *pTMesh );
	HRESULT OrganizeLoadedMeshes();

protected:

	FILE	*m_pFp;
	TCHAR	*m_pFilePath;
	BOOL	m_bBinaryMode;
	BOOL	m_bOpenRead;
	BOOL	m_bDouble;
	BOOL	m_bArray;
	INT		m_iArrayCount;
	INT		m_iArrayType;
	BYTE	*m_pBuffer;
	DWORD	m_dwSize;
	INT		m_iDepth;
	CHAR	*m_pCurrent;
	CHAR	*m_pNextPosition;
	CHAR	*m_pTemporary;
	INT		m_iTemporarySize;

	MeshFrame		*m_pFrameRoot;

	MeshFrame		**m_ppFlattenFramePointers;
	INT				m_iNumFrames;
	AnimationSet	**m_ppAnimationSets;
	INT				m_iNumAnimationSets;
	FileReadingContext	*m_pFileReadingContext;	//  Local data
};

