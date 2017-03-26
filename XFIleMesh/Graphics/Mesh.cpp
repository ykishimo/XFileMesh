//
//	Mesh data modle
//
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <DirectXMath.h>
#include "Mesh.h"

#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }

using namespace DirectX;
static void AdjustFrameMatricesSub(MeshFrame *pFrame, XMMATRIX *pMatParent);

//
//  ctor for mesh
//
Mesh::Mesh(){
	this->dwFVF = 0;
	this->numIndices = 0;
	this->numVertices = 0;
	this->numAttributes = 0;
	this->pIndices = NULL;
	this->pVertices = NULL;
	this->pMaterialAttributeRange = NULL;
}
//
//  destructor for mesh
//
Mesh::~Mesh(){
	SAFE_DELETE_ARRAY(pVertices);
	SAFE_DELETE_ARRAY(pIndices);
	SAFE_DELETE_ARRAY(pMaterialAttributeRange);
}

//
//  ctor for MeshContainer
//
MeshContainer::MeshContainer(){
	pName = NULL;
	pMaterials = NULL;
	pMeshData = NULL;
	ppBoneMatrixPointers = NULL;
	pBoneOffsetMatrices = NULL;
	pNextMeshContainer = NULL;
	numBones = 0;
}

//
//  destructor for MeshContainer
//
MeshContainer::~MeshContainer(){
	SAFE_DELETE(this->pNextMeshContainer);
	SAFE_DELETE(this->pName);
	if (pMaterials != NULL){
		for (DWORD i = 0; i < this->dwNumMaterials ; ++i){
			SAFE_DELETE(pMaterials[i].pName);
			SAFE_DELETE(pMaterials[i].pTextureFilename);
		}
		delete[] pMaterials;
		pMaterials = NULL;
	}
	SAFE_DELETE(this->pMeshData);
	SAFE_DELETE_ARRAY(ppBoneMatrixPointers);
	//SAFE_DELETE_ARRAY(pBoneOffsetMatrices);
	if (pBoneOffsetMatrices != NULL){
		_mm_free(pBoneOffsetMatrices);
		pBoneOffsetMatrices = NULL;
	}

}

//
//  ctor for Material
//
Material::Material(){
	this->pName = NULL;
	this->pTextureFilename = NULL;
	this->pTextureData = NULL;
}

//
//  destructor for Material
//
Material::~Material(){
	SAFE_DELETE(this->pName);
	SAFE_DELETE(this->pTextureFilename);
}

//
//  ctor for MeshFrame
//
MeshFrame::MeshFrame(){
	this->pFrameFirstChild=NULL;
	this->pFrameSibling=NULL;
	this->pName=NULL;
	this->pMeshContainer=NULL;
	this->pName=NULL;
	this->iFrameId = -1;
}
//
//  destructor for MeshFrame
//
MeshFrame::~MeshFrame(){
	SAFE_DELETE(this->pFrameFirstChild);
	SAFE_DELETE(this->pFrameSibling);
	SAFE_DELETE(this->pName);
	SAFE_DELETE(this->pMeshContainer);
	SAFE_DELETE(this->pName);
}



//
//	Get the size of an vertex
//
DWORD GetFVFSize(DWORD fvf){
	DWORD size = 0;
	if (0 != (fvf & Mesh::FVF_XYZ) ){
		size += sizeof(FLOAT) * 3;
	}
	if (0 != (fvf & Mesh::FVF_NORMAL)){
		size += sizeof(FLOAT) * 3;
	}
	switch( fvf & Mesh::FVF_TEXMASK){
	case	Mesh::FVF_TEX1:
		size += sizeof(FLOAT)*2;	break;
	case	Mesh::FVF_TEX2:
		size += sizeof(FLOAT)*4;	break;
	case	Mesh::FVF_TEX3:
		size += sizeof(FLOAT)*6;	break;
	}
	switch( fvf & Mesh::FVF_WEIGHT_MASK){
	case	Mesh::FVF_WEIGHT1:
		size += sizeof(FLOAT);		break;
	case	Mesh::FVF_WEIGHT2:
		size += sizeof(FLOAT)*2;	break;
	case	Mesh::FVF_WEIGHT3:
		size += sizeof(FLOAT)*3;	break;
	case	Mesh::FVF_WEIGHT4:
		size += sizeof(FLOAT)*4;	break;
	}
	if (0 != (fvf & Mesh::FVF_INDEXB4)){
		size += sizeof(BYTE)*4;
	}
	return size;
}

//
//	Adjust combined matrix
//
VOID AdjustFrameMatrices(MeshFrame *pFrameRoot, DirectX::XMMATRIX *pMatWorld){
	static XMMATRIX matIdentity = XMMatrixIdentity();
	if (pMatWorld == NULL){
		AdjustFrameMatricesSub(pFrameRoot,&matIdentity);
	}else
		AdjustFrameMatricesSub(pFrameRoot,pMatWorld);
}

static void AdjustFrameMatricesSub(MeshFrame *pFrame, XMMATRIX *pMatParent){
	{
		pFrame->CombinedMatrix = DirectX::XMMatrixMultiply(pFrame->FrameMatrix, *pMatParent);
	}
	if (pFrame->pFrameSibling){
		AdjustFrameMatricesSub(pFrame->pFrameSibling,pMatParent);
	}
	if (pFrame->pFrameFirstChild){
		AdjustFrameMatricesSub(pFrame->pFrameFirstChild,&pFrame->CombinedMatrix);
	}
}


//
//	Adjust vertex format
//
static HRESULT AdjustMeshVertexFormatSub(MeshFrame *pFrame, DWORD dwFVF, DWORD dwConvertMask);

//  @param
//    pFrameRoot : Root frame
//    dwFVF      : Resulted fvf
//    dwConvertMask : if 0xffffffffL convert all
//
HRESULT AdjustMeshVertexFormatFVF(MeshFrame *pFrameRoot, DWORD dwFVF, DWORD dwConvertMask){
	HRESULT hr = E_FAIL;
	hr = AdjustMeshVertexFormatSub(pFrameRoot, dwFVF, dwConvertMask);
	return hr;
}

HRESULT AdjustMeshVertexFormatFVF(Mesh *pMesh, DWORD dwFVF){
	HRESULT hr = E_FAIL;
	DWORD ordFVF;
	INT newVertexSize, bufferSize, numVertices;
	BYTE	*pNewVertices = NULL;

	if (pMesh == NULL)
		goto MESHCONVERT_END;
	
	ordFVF = pMesh->dwFVF;
	if (ordFVF == dwFVF)
		return S_OK;	//  no need to convert

	numVertices =  pMesh->numVertices;
	newVertexSize = GetFVFSize(ordFVF);
	bufferSize = newVertexSize * numVertices; 
	pNewVertices = (BYTE*) new DWORD[bufferSize + 3];

	INT ordPos = -1, ordNormal = -1, ordTex1 = -1;
	INT ordTex2 = -1, ordTex3 = -1;
	INT ordWeight1 = -1, ordWeight2 = -1, ordWeight3 = -1, ordWeight4 = -1;
	INT ordIndexB4 = -1;
	INT offset = 0;
	INT srcStride;
	BYTE *pByte,*pSrcByte;
	FLOAT *pfDst, *pfSrc;
	if (0 != (ordFVF & Mesh::FVF_XYZ)){
		ordPos = offset;
		offset += sizeof(FLOAT)*3;
	}
	switch(ordFVF & Mesh::FVF_WEIGHT_MASK){
	case	Mesh::FVF_WEIGHT1:
		ordWeight1 = offset;
		offset += sizeof(FLOAT);
		break;
	case	Mesh::FVF_WEIGHT2:
		ordWeight1 = offset;
		ordWeight2 = ordWeight1 + sizeof(FLOAT);
		offset += sizeof(FLOAT)*2;
		break;
	case	Mesh::FVF_WEIGHT3:
		ordWeight1 = offset;
		ordWeight2 = ordWeight1 + sizeof(FLOAT);
		ordWeight3 = ordWeight2 + sizeof(FLOAT);
		offset += sizeof(FLOAT)*3;
		break;
	case	Mesh::FVF_WEIGHT4:
		ordWeight1 = offset;
		ordWeight2 = ordWeight1 + sizeof(FLOAT);
		ordWeight3 = ordWeight2 + sizeof(FLOAT);
		ordWeight4 = ordWeight3 + sizeof(FLOAT);
		offset += sizeof(FLOAT)*4;
		break;
	}
	if (0 != (ordFVF & Mesh::FVF_INDEXB4)){
		ordIndexB4 = offset;
		offset += sizeof(DWORD);
	}
	if (0 != (ordFVF & Mesh::FVF_NORMAL)){
		ordNormal = offset;
		offset += sizeof(FLOAT)*3;
	}
	switch(ordFVF & Mesh::FVF_TEXMASK){
	case	Mesh::FVF_TEX1:
		ordTex1 = offset;
		offset += sizeof(FLOAT)*2;
		break;
	case	Mesh::FVF_TEX2:
		ordTex1 = offset;
		ordTex2 = ordTex1 + sizeof(FLOAT)*2;
		offset += sizeof(FLOAT)*2*2;
		break;
	case	Mesh::FVF_TEX3:
		ordTex1 = offset;
		ordTex2 = ordTex1 + sizeof(FLOAT)*2;
		ordTex3 = ordTex2 + sizeof(FLOAT)*2;
		offset += sizeof(FLOAT)*2*3;
		break;
	}

	srcStride = offset;
	pByte = pNewVertices;
	pSrcByte = (BYTE*)pMesh->pVertices;
	for (int i = 0 ; i < numVertices ; ++i){
		offset = 0;
		if (0 != (dwFVF & Mesh::FVF_XYZ)){
			pfDst = (FLOAT*)pByte;
			if (ordPos >= 0){
				pfSrc = (FLOAT*)(pSrcByte+ordPos);
				pfDst[0] = pfSrc[0];
				pfDst[1] = pfSrc[1];
				pfDst[2] = pfSrc[2];
			}else{
				pfDst[0] = 0;
				pfDst[1] = 0;
				pfDst[2] = 0;
			}
			offset += sizeof(FLOAT) * 3;
		}
		if (0 != (dwFVF & Mesh::FVF_WEIGHT_MASK)){
			if ((dwFVF & Mesh::FVF_WEIGHT_MASK) >= Mesh::FVF_WEIGHT1){
				pfDst = (FLOAT*)(pByte+offset);
				if (ordWeight1 >= 0){
					pfSrc = (FLOAT*)(pSrcByte + ordWeight1);
					pfDst[0] = pfSrc[0];
				}else{
					pfDst[0] = 1;
				}
				offset += sizeof(FLOAT);
			}
			if ((dwFVF & Mesh::FVF_WEIGHT_MASK) >= Mesh::FVF_WEIGHT2){
				pfDst = (FLOAT*)(pByte+offset);
				if (ordWeight2 >= 0){
					pfSrc = (FLOAT*)(pSrcByte + ordWeight2);
					pfDst[0] = pfSrc[0];
				}else{
					pfDst[0] = 0;
				}
				offset += sizeof(FLOAT);
			}
			if ((dwFVF & Mesh::FVF_WEIGHT_MASK) >= Mesh::FVF_WEIGHT3){
				pfDst = (FLOAT*)(pByte+offset);
				if (ordWeight3 >= 0){
					pfSrc = (FLOAT*)(pSrcByte + ordWeight3);
					pfDst[0] = pfSrc[0];
				}else{
					pfDst[0] = 0;
				}
				offset += sizeof(FLOAT);
			}
			if ((dwFVF & Mesh::FVF_WEIGHT_MASK) >= Mesh::FVF_WEIGHT4){
				pfDst = (FLOAT*)(pByte+offset);
				if (ordWeight4 >= 0){
					pfSrc = (FLOAT*)(pSrcByte + ordWeight4);
					pfDst[0] = pfSrc[0];
				}else{
					pfDst[0] = 0;
				}
				offset += sizeof(FLOAT);
			}

		}
		if (0 != (dwFVF & Mesh::FVF_INDEXB4)){
			DWORD *pdwDst = (DWORD*)(pByte+offset);
			if (ordIndexB4 >= 0){
				DWORD *pdwSrc = (DWORD*)(pSrcByte+ordIndexB4);
				*pdwDst = *pdwSrc;
			}else{
				*pdwDst = 0;
			}
			offset += sizeof(DWORD);
		}
		if (0 != (dwFVF & Mesh::FVF_NORMAL)){
			pfDst = (FLOAT*)(pByte+offset);
			if (ordNormal >= 0){
				pfSrc = (FLOAT*)(pSrcByte+ordNormal);
				pfDst[0] = pfSrc[0];
				pfDst[1] = pfSrc[1];
				pfDst[2] = pfSrc[2];
			}else{
				pfDst[0] = 0;
				pfDst[1] = 0;
				pfDst[2] = 0;
			}
			offset += sizeof(FLOAT) * 3;
		}
		if (0 != (dwFVF & Mesh::FVF_TEXMASK)){
			if ((dwFVF & Mesh::FVF_TEXMASK) >= Mesh::FVF_TEX1){
				pfDst = (FLOAT*)(pByte+offset);
				if (ordTex1 >= 0){
					pfSrc = (FLOAT*)(pSrcByte + ordTex1);
					pfDst[0] = pfSrc[0];
					pfDst[1] = pfSrc[1];
				}else{
					pfDst[0] = 0;
					pfDst[1] = 0;
				}
				offset += sizeof(FLOAT)*2;
			}
			if ((dwFVF & Mesh::FVF_TEXMASK) >= Mesh::FVF_TEX2){
				pfDst = (FLOAT*)(pByte+offset);
				if (ordTex2 >= 0){
					pfSrc = (FLOAT*)(pSrcByte + ordTex2);
					pfDst[0] = pfSrc[0];
					pfDst[1] = pfSrc[1];
				}else{
					pfDst[0] = 0;
					pfDst[1] = 0;
				}
				offset += sizeof(FLOAT)*2;
			}
			if ((dwFVF & Mesh::FVF_TEXMASK) >= Mesh::FVF_TEX3){
				pfDst = (FLOAT*)(pByte+offset);
				if (ordTex3 >= 0){
					pfSrc = (FLOAT*)(pSrcByte + ordTex3);
					pfDst[0] = pfSrc[0];
					pfDst[1] = pfSrc[1];
				}else{
					pfDst[0] = 0;
					pfDst[1] = 0;
				}
				offset += sizeof(FLOAT)*2;
			}
		}
		pByte += offset;
		pSrcByte += srcStride;
	}
	
	SAFE_DELETE(pMesh->pVertices);
	pMesh->pVertices = pNewVertices;
	pMesh->dwFVF = dwFVF;

MESHCONVERT_END:
	return hr;	
}

static HRESULT AdjustMeshVertexFormatSub(MeshFrame *pFrame, DWORD dwFVF, DWORD dwConvertMask){
	HRESULT hr=S_OK;
	
	if (pFrame->pMeshContainer != NULL){
		Mesh *pMesh = pFrame->pMeshContainer->pMeshData;
		DWORD dwOrd = pMesh->dwFVF;
		DWORD tempFVF = dwFVF & dwConvertMask;
		tempFVF |= (~dwConvertMask)& dwOrd;
		AdjustMeshVertexFormatFVF(pMesh,tempFVF);
	}
	if (pFrame->pFrameSibling){
		hr = AdjustMeshVertexFormatSub(pFrame->pFrameSibling,dwFVF,dwConvertMask);
		if (FAILED(hr))
			goto EXIT;
	}
	if (pFrame->pFrameFirstChild){
		hr = AdjustMeshVertexFormatSub(pFrame->pFrameFirstChild,dwFVF,dwConvertMask);
		if (FAILED(hr))
			goto EXIT;
	}

	hr = S_OK;
EXIT:
	return hr;
}

static INT CountFrameSub(MeshFrame *pFrame){
	INT count = 0;
	if (pFrame->pFrameSibling != NULL){
		count += CountFrameSub(pFrame->pFrameSibling);
	}
	if (pFrame->pFrameFirstChild != NULL){
		count += CountFrameSub(pFrame->pFrameFirstChild);
	}
	++count;
	return count;
}

INT CountFramesInHierarchy(MeshFrame *pRoot){
	return CountFrameSub(pRoot);	
}
