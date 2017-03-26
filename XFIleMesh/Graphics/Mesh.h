//
//	Mesh data
//
#ifndef __MESH_H__

#define __MESH_H__

//struct MeshLoaderContext;


typedef struct _MaterialAttributeRange
{
    DWORD MaterialId;
    DWORD FaceStart;
    DWORD FaceCount;
} MaterialAttributeRange;



/*
__declspec(align(16)) struct Bone{
	CHAR	*pName;
	BOOL	bActive;
	INT		orderOnMesh;
	INT		orderOnHierarchy;
	DirectX::XMMATRIX	matOffset;	//  must aligned in 16 byte
	Bone();
	~Bone();

	//  keep this 16-byte aligned
	void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	void operator delete(void *p){
		return _mm_free(p);
	}
};
*/

struct Material{
	CHAR	*pName;
	DirectX::XMFLOAT4	vecDiffuse;
	DirectX::XMFLOAT4	vecEmmisive;
	DirectX::XMFLOAT4	vecSpecular;
	FLOAT	fPower;
	TCHAR	*pTextureFilename;
	VOID	*pTextureData;
	Material();
	~Material();
};

struct Mesh{
	VOID	*pVertices;
	USHORT	*pIndices;
	MaterialAttributeRange *pMaterialAttributeRange;
	INT		numVertices;
	INT		iVertexStride;
	INT		numIndices;
	INT		numAttributes;
	DWORD	dwFVF;
	static const DWORD FVF_XYZ     = 1;
	static const DWORD FVF_NORMAL  = 2;
	static const DWORD FVF_TEX0    = 0;
	static const DWORD FVF_TEX1    = 4;
	static const DWORD FVF_TEX2    = 8;
	static const DWORD FVF_TEX3    = 0xc;
	static const DWORD FVF_TEXMASK = 0xc;
	static const DWORD FVF_WEIGHT1 = 0x10;
	static const DWORD FVF_WEIGHT2 = 0x20;
	static const DWORD FVF_WEIGHT3 = 0x30;
	static const DWORD FVF_WEIGHT4 = 0x40;
	static const DWORD FVF_WEIGHT_MASK = 0x70;
	static const DWORD FVF_INDEXB4  = 0x80;
	Mesh();
	~Mesh();
};



struct MeshContainer{
	CHAR		*pName;
	Mesh		*pMeshData;
	Material	*pMaterials;
	DWORD       dwNumMaterials;
	INT			numBones;
	DirectX::XMMATRIX		**ppBoneMatrixPointers;
	DirectX::XMMATRIX		*pBoneOffsetMatrices;
	struct	MeshContainer	*pNextMeshContainer;

	MeshContainer();
	~MeshContainer();
};

__declspec(align(16)) struct MeshFrame{
	CHAR *pName;
	MeshContainer	*pMeshContainer;
	MeshFrame		*pFrameSibling;
	MeshFrame		*pFrameFirstChild;
	DirectX::XMMATRIX	FrameMatrix;
	DirectX::XMMATRIX	CombinedMatrix;
	INT				iFrameId;			//  Frame's serial no. it's incremental from zero.
	MeshFrame();
	~MeshFrame();
	//  keep this 16-byte aligned
	void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	void operator delete(void *p){
		return _mm_free(p);
	}
};



//======================================================
//  Prototypes
//======================================================
DWORD GetFVFSize(DWORD fvf);
VOID AdjustFrameMatrices(MeshFrame *pFrameRoot, DirectX::XMMATRIX *pMatWorld = NULL);
HRESULT AdjustMeshVertexFormatFVF(MeshFrame *pFrameRoot, DWORD dwFVF, DWORD dwConvertMask);
INT	CountFramesInHierarchy(MeshFrame *pRoot);
HRESULT AdjustMeshVertexFormatFVF(Mesh *pMesh, DWORD dwFVF);
#endif