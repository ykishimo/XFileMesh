//  Mesh collider
//  Now under constructing.
//
#include "stdafx.h"
#include <crtdbg.h>
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D2D1.h>
#include <list>
#include <vector>
#include "XFileParser.h"

#include "Mesh.h"
#include "Animation.h"
#include "SimpleMesh.h"
#include "Animator.h"
#include "AnimatedMesh.h"
#include "D3D11TextureDB.h"

#include "D3DContext.h"

#include "MeshCollider.h"

using namespace DirectX;
//  pure virtual destructor
IMeshCollider::~IMeshCollider(){
}

//  Factory
IMeshCollider * IMeshCollider::CreateInstance(TCHAR *pFilename){
	CMeshCollider *pCollider = new CMeshCollider(pFilename);
	if (pCollider && pCollider->GetRootFrame() == NULL){
		delete pCollider;
		pCollider = NULL;
	}
	return pCollider;
}

__declspec(align(16)) struct COLLISIONVERTEX{
	DirectX::XMFLOAT3 position;   //  position
	DirectX::XMFLOAT3 normal;     //  normal
	DirectX::XMFLOAT2 texture;    //  texture coord

};

__declspec(align(16)) struct	COLLISIONINFO{
	XMVECTOR	n;		//	面法線
	FLOAT		amount;	//	食い込み量
	XMVECTOR	positions[3];
	XMVECTOR	normals[3];
	XMVECTOR	nearest;
	void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	void *operator new[](size_t size) {
		return _mm_malloc(size, 16);
	}
		void operator delete(void *p) {
		return _mm_free(p);
	}
};

//  local data structure
struct frameiterator{
	std::list<MeshFrame*> frames;
};

//  prototypes
static void	CalcShortestDistance(
	DirectX::XMVECTOR *vecPos, 
	DirectX::XMVECTOR *vecP1, DirectX::XMVECTOR *vecP2, 
	DirectX::XMVECTOR *vecN1, DirectX::XMVECTOR *vecN2, 
	float *pDist, DirectX::XMVECTOR *pPosOut, DirectX::XMVECTOR *pNormal);

static BOOL	ProbeTheTriangleDistance(
	XMVECTOR *positions,XMVECTOR *normals, 
	FLOAT *pDist, XMVECTOR *pProbePos, 
	XMFLOAT3 *pBoxMin, XMFLOAT3 *pBoxMax, 
	FLOAT fRadius, XMVECTOR *pNormal,XMVECTOR *pNearestPos);


static void CalcShortestDistanceAndAltitude(DirectX::XMFLOAT3 *vecPos, 
	DirectX::XMVECTOR *pPos1, DirectX::XMVECTOR *pPos2,
	DirectX::XMVECTOR *pNormal1, DirectX::XMVECTOR *pNormal2,
	float *pAlt, float *pDist, DirectX::XMVECTOR *pNormal);

static BOOL FitLineToBox(
	DirectX::XMVECTOR *pPos1, DirectX::XMVECTOR *pPos2,
	DirectX::XMVECTOR *pNormal1, DirectX::XMVECTOR *pNormal2,
	DirectX::XMFLOAT3 *pVecMin, DirectX::XMFLOAT3 *pVecMax
);

static BOOL	ProbeTheTriangleHeight(
	DirectX::XMVECTOR *pVertices,DirectX::XMVECTOR *pNormals, 
	FLOAT *pDist, FLOAT *pAlt, 
	DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax,
	DirectX::XMFLOAT3 *pPosition,DirectX::XMVECTOR *pVecNormal
);

static BOOL	ProbeTheTriangleHeightOneSide(
	DirectX::XMVECTOR *pVertices,DirectX::XMVECTOR *pNormals, 
	FLOAT *pDist, FLOAT *pAlt, 
	DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax,
	DirectX::XMFLOAT3 *pPosition,DirectX::XMVECTOR *pVecNormal
);

//
//  ctor for simple mesh renderer
//
CMeshCollider::CMeshCollider(TCHAR *pFilename)
{
	AnimationSet **ppAnimationSets = NULL;
	DWORD        dwNumAnimations;
	MeshFrame *pFrame = NULL;

	//  Initialize fields fields
	m_dwNumFrames = 0;
	m_ppMeshFrames = NULL;
	m_vecMin.x = FLT_MAX;
	m_vecMin.y = FLT_MAX;
	m_vecMin.z = FLT_MAX;

	m_vecMax.x = -FLT_MAX;
	m_vecMax.y = -FLT_MAX;
	m_vecMax.z = -FLT_MAX;
	
	m_matWorld = XMMatrixIdentity();

	if (SUCCEEDED( CXFileParser::CreateMesh(pFilename,&pFrame,&ppAnimationSets,&dwNumAnimations))){
		//  Ignore animations
		if (ppAnimationSets != NULL){
			for (DWORD i = 0; i < dwNumAnimations ; ++i){
				delete ppAnimationSets[i];
			}
			delete ppAnimationSets;
			ppAnimationSets = NULL;
		}
		MeshFrame *pOld = m_smrRenderer.ReplaceRootFrame(pFrame);
		SAFE_DELETE(pOld)
	}
	if (FAILED(m_smrRenderer.AdjustVertexFormat())){
		DeleteObjects();
	}else{
		frameiterator tmpFrames;
		MeshFrame *pRoot = m_smrRenderer.GetRootFrame();
		if (pRoot != NULL){
			EnflatFrames(pRoot,&tmpFrames);
			this->m_dwNumFrames = tmpFrames.frames.size();
			if (m_dwNumFrames > 0){
				int i = 0;
				this->m_ppMeshFrames = new MeshFrame*[m_dwNumFrames];
				std::list<MeshFrame *>::iterator it = tmpFrames.frames.begin();
				while(it != tmpFrames.frames.end()){
					this->m_ppMeshFrames[i++] = *it;
					*it = NULL;
					it = tmpFrames.frames.erase(it);
				}
			}
			PrepareBoundingAABB();
		}

	}
}

void CMeshCollider::PrepareBoundingAABB(){
	XMMATRIX matWorld = XMMatrixIdentity();
	if (this->m_ppMeshFrames == NULL)
		return;
	m_smrRenderer.SetWorldMatrix(&matWorld);
	AdjustFrameMatrices(m_smrRenderer.GetRootFrame());

	MeshFrame *pFrame;
	MeshContainer *pMeshContainer;
	Mesh *pMesh;
	COLLISIONVERTEX *pVertices;
	for (int i = 0; i < (int)m_dwNumFrames ; ++i){
		pFrame = m_ppMeshFrames[i];
		pMeshContainer = pFrame->pMeshContainer;
		pMesh = pMeshContainer->pMeshData;
		pVertices = (COLLISIONVERTEX*)pMesh->pVertices;
		for (int j = 0; j < pMesh->numVertices ; ++j){
			FLOAT x, y, z;
			XMVECTOR vecTmp = XMLoadFloat3(&pVertices->position);
			vecTmp = XMVector3TransformCoord(vecTmp,pFrame->CombinedMatrix);
			x = XMVectorGetX(vecTmp);
			y = XMVectorGetY(vecTmp);
			z = XMVectorGetZ(vecTmp);
			m_vecMin.x = min(m_vecMin.x, x);
			m_vecMin.y = min(m_vecMin.y, y);
			m_vecMin.z = min(m_vecMin.z, z);
			m_vecMax.x = max(m_vecMax.x, x);
			m_vecMax.y = max(m_vecMax.y, y);
			m_vecMax.z = max(m_vecMax.z, z);
			++pVertices;
		}
	}
}
void CMeshCollider::EnflatFrames(MeshFrame *pFrame, frameiterator *pFrames){
	if (pFrame == NULL)
		return;
	if (pFrame->pFrameSibling != NULL){
		EnflatFrames(pFrame->pFrameSibling,pFrames);
	}
	if (pFrame->pFrameFirstChild != NULL){
		EnflatFrames(pFrame->pFrameFirstChild,pFrames);
	}
	if (pFrame->pMeshContainer != NULL){
		pFrames->frames.push_back(pFrame);
	}
}
CMeshCollider::~CMeshCollider(void)
{
	DeleteObjects();
	SAFE_DELETE(m_ppMeshFrames);
}
void CMeshCollider::DeleteObjects(){

}

void CMeshCollider::GetBoundingAABB( XMVECTOR *pMin, XMVECTOR *pMax){
	*pMin = XMLoadFloat3(&m_vecMin);
	*pMax = XMLoadFloat3(&m_vecMax);
}

void CMeshCollider::GetBoundingAABBFloat3( XMFLOAT3 *pMin, XMFLOAT3 *pMax){
	*pMin = m_vecMin;
	*pMax = m_vecMax;
}


//  check the collision between 2 AABBs
static inline BOOL Collide2Box(
	XMFLOAT3 *pVecMin1, XMFLOAT3 *pVecMax1,
	XMFLOAT3 *pVecMin2, XMFLOAT3 *pVecMax2
){
	if (pVecMax2->x < pVecMin1->x)
		return false;
	if (pVecMax2->y < pVecMin1->y)
		return false;
	if (pVecMax2->z < pVecMin1->z)
		return false;
	if (pVecMin2->x > pVecMax1->x)
		return false;
	if (pVecMin2->y > pVecMax1->y)
		return false;
	if (pVecMin2->z > pVecMax1->z)
		return false;
	return true;
}
	
BOOL	CMeshCollider::ProbeTheWallSinkDepth(XMFLOAT3 *pVec, FLOAT fRadius, XMFLOAT3 *pVecNormal, FLOAT *pDepth){
	pVecNormal->x = 0;
	pVecNormal->y = 0;
	pVecNormal->z = 0;
	return ProbeTheWallSinkDepthBase(pVec,fRadius,pVecNormal,pDepth);
}

BOOL	CMeshCollider::ProbeTheWallSinkDepthWithMotion(XMFLOAT3 *pVec, FLOAT fRadius, XMFLOAT3 *pVecNormal, FLOAT *pDepth){
	if ( 0 == XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(pVecNormal))))
		return false;
	return ProbeTheWallSinkDepthBase(pVec,fRadius,pVecNormal,pDepth);
}


BOOL CMeshCollider::ProbeTheGroundAltitude(XMFLOAT3 *pPosition, XMFLOAT3 *pBoxMin, XMFLOAT3 *pBoxMax, XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist){

//  Probe the ground altitude (Basic method)
	DirectX::XMFLOAT3	vecMin, vecMax;
	DirectX::XMVECTOR	vecNormal, vecNormalTmp;
	FLOAT	fDist,fMinDist,fAlt, fAltTmp;
	BOOL	bRet = false;

	vecNormalTmp = DirectX::XMLoadFloat3(pVecNormal);
	
	GetBoundingAABBFloat3(&vecMin, &vecMax);
	
	//	与えられた箱とこの地形のバウンダリボックスとの
	//	衝突判定
	if (!Collide2Box(&vecMin,&vecMax,pBoxMin,pBoxMax)){
		return false;
	}

	fMinDist = FLT_MAX;
	int num;
	//	メッシュから個別にポリゴンを抽出する。
	for (int i = 0; i < (int)m_dwNumFrames ; ++i){
		int index = 0, k;
		COLLISIONVERTEX *pVertices;
		XMVECTOR positions[3], normals[3];
		MeshFrame *pFrame = m_ppMeshFrames[i];
		XMMATRIX matWorld;

		if (pFrame == NULL)
			continue;
		MeshContainer *pContainer = pFrame->pMeshContainer;
		if (pContainer == NULL)
			continue;
		Mesh *pMesh = pContainer->pMeshData;
		if (pMesh == NULL)
			continue;

		//  Prepare world matrix
		matWorld = m_matWorld * pFrame->CombinedMatrix;

		index = 0;
		pVertices = (COLLISIONVERTEX*)pMesh->pVertices;
		int iNumIndices = pMesh->numIndices * 3;
		while(index < iNumIndices){
			k = pMesh->pIndices[index++];
			positions[0] = XMLoadFloat3(&pVertices[k].position);
			normals[0]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[1] = XMLoadFloat3(&pVertices[k].position);
			normals[1]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[2] = XMLoadFloat3(&pVertices[k].position);
			normals[2]   = XMLoadFloat3(&pVertices[k].normal);

			//  transform'em into world coordinate system
			for (int i = 0; i < 3 ; ++i){
				positions[i] = XMVector3TransformCoord(positions[i],matWorld);
				normals[i]   = XMVector3TransformNormal(normals[i], matWorld);
			}
			{
				float minx = FLT_MAX;
				float maxx = FLT_MIN;
				float minz = FLT_MAX;
				float maxz = FLT_MIN;
				for (int i = 0; i < 3 ; ++i){
					positions[i] = XMVector3TransformCoord(positions[i],matWorld);
					normals[i]   = XMVector3TransformNormal(normals[i], matWorld);
					minx = min(minx,XMVectorGetX(positions[i]));
					minz = min(minx,XMVectorGetZ(positions[i]));
					maxx = max(maxx,XMVectorGetX(positions[i]));
					maxz = max(maxx,XMVectorGetZ(positions[i]));
				}
			}
			if (ProbeTheTriangleHeight(positions,normals,&fDist,&fAltTmp,pBoxMin,pBoxMax,pPosition,&vecNormalTmp)){
				if (fDist == 0){
					bRet = true;
					fMinDist = 0.0f;
					fAlt = fAltTmp;
					vecNormal = vecNormalTmp;
					break;
				}
				if (fDist < fMinDist){
					bRet = true;
					fMinDist = fDist;
					fAlt = fAltTmp;
					vecNormal = vecNormalTmp;
				}
			}
		}
		num = index;
	}
	if (bRet){
		*pAlt = fAlt;
		*pDist = fMinDist;
		pVecNormal->x = XMVectorGetX(vecNormal);
		pVecNormal->y = XMVectorGetY(vecNormal);
		pVecNormal->z = XMVectorGetZ(vecNormal);
	}
	return	bRet;
}

static BOOL	ProbeTheTriangleHeight(
			XMVECTOR *pVertices, XMVECTOR *pNormals,
			FLOAT *pDist, FLOAT *pAlt, 
			XMFLOAT3 *pBoxMin,  XMFLOAT3 *pBoxMax,
			XMFLOAT3 *pPosition,XMVECTOR *pVecNormal
){
	//  calculate the boundary aabb of the triangle
	XMFLOAT3 vecMin, vecMax;
	FLOAT	fAlt;
	FLOAT	fDist, fDistMin;
	vecMin = XMFLOAT3( FLT_MAX, FLT_MAX, FLT_MAX);
	vecMax = XMFLOAT3(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	XMVECTOR *pCoord = pVertices;
	for (int i = 0; i < 3 ; ++i){
		FLOAT x = XMVectorGetX(*pCoord);
		FLOAT y = XMVectorGetY(*pCoord);
		FLOAT z = XMVectorGetZ(*pCoord++);
		vecMin.x = min(vecMin.x, x);
		vecMin.y = min(vecMin.y, y);
		vecMin.z = min(vecMin.z, z);
		vecMax.x = max(vecMax.x, x);
		vecMax.y = max(vecMax.y, y);
		vecMax.z = max(vecMax.z, z);
	}
	if (!Collide2Box(pBoxMin,pBoxMax,&vecMin,&vecMax))
		return false;

	FLOAT		len1, len2, len3, dx, dz;

	//  first press the triangle on the x-z plane
	//  ( make y zero )
	//  then measure the lenths of all edges.
	dx = XMVectorGetX(pVertices[1]) - XMVectorGetX(pVertices[0]);
	dz = XMVectorGetZ(pVertices[1]) - XMVectorGetZ(pVertices[0]);
	len1 = dx * dx + dz * dz;
	dx = XMVectorGetX(pVertices[2]) - XMVectorGetX(pVertices[1]);
	dz = XMVectorGetZ(pVertices[2]) - XMVectorGetZ(pVertices[1]);
	len2 = dx * dx + dz * dz;
	dx = XMVectorGetX(pVertices[0]) - XMVectorGetX(pVertices[2]);
	dz = XMVectorGetZ(pVertices[0]) - XMVectorGetZ(pVertices[2]);
	len3 = dx * dx + dz * dz;

	//	you can stand on the polygon only in case of it has enough area.
	if ((fabs(len1) > FLT_MIN)&&(fabs(len2) > FLT_MIN)&&(fabs(len3) > FLT_MIN)){
		FLOAT x, y, z;
		XMVECTOR vecDir1, vecDir2, vecTmp;
		BOOL bF0 = false, bF1 = false, bF2 = false;
		x = pPosition->x;
		y = pPosition->y;
		z = pPosition->z;
		vecDir1 = XMLoadFloat3(pPosition) - pVertices[0];
		vecDir2 = pVertices[1] - pVertices[0];
		XMVectorSetY(vecDir1,0.0f);
		XMVectorSetY(vecDir2,0.0f);
		vecTmp = XMVector3Cross(vecDir1,vecDir2);
		if (XMVectorGetY(vecTmp) > -FLT_MIN){
			bF0 = true;
		}

		vecDir1 = XMLoadFloat3(pPosition) - pVertices[1];
		vecDir2 = pVertices[2] - pVertices[1];
		XMVectorSetY(vecDir1,0.0f);
		XMVectorSetY(vecDir2,0.0f);
		vecTmp = XMVector3Cross(vecDir1,vecDir2);
		if (XMVectorGetY(vecTmp) > -FLT_MIN){
			bF1=TRUE;
		}

		vecDir1 = XMLoadFloat3(pPosition) - pVertices[2];
		vecDir2 = pVertices[0] - pVertices[2];
		XMVectorSetY(vecDir1,0.0f);
		XMVectorSetY(vecDir2,0.0f);
		vecTmp = XMVector3Cross(vecDir1,vecDir2);
		if (XMVectorGetY(vecTmp) > -FLT_MIN){
			bF2=TRUE;
		}

		fDistMin = FLT_MAX;
		//  if the position is inside the triangle when you see it on x-z plane.
		if (!bF0 && !bF1 && !bF2){
			double	t,u,inv;
			FLOAT	px = x - XMVectorGetX(pVertices[0]);
			FLOAT	pz = z - XMVectorGetZ(pVertices[0]);
			vecDir1 = pVertices[1] - pVertices[0];
			vecDir2 = pVertices[2] - pVertices[0];
			FLOAT	x1 = XMVectorGetX(vecDir1);
			FLOAT	z1 = XMVectorGetZ(vecDir1);
			FLOAT	x2 = XMVectorGetX(vecDir2);
			FLOAT	z2 = XMVectorGetZ(vecDir2);

			//  Evaluate t and u when px and pz is assumed as below
			//  px = t*x1 + u*x2
			//  pz = t*z1 + u*z2
			//  we use the Cramer's rule to evaluate them
			inv = (x1*z2 - x2*z1);
			if (fabs(inv) > FLT_MIN){
				inv = 1.0f / inv;	//  normalize the reversed normal
				t = inv * (px*z2 - pz*x2);
				u = inv * (x1*pz - px*z1);

				XMVECTOR vecPos, vecNormal;
				//	Calculate the position
				vecPos = vecDir1*(FLOAT)t + vecDir2*(FLOAT)u;
				vecPos += pVertices[0];

				//	Calculate the normal
				vecNormal = pNormals[0] * (1-(FLOAT)t) + pNormals[1] * (FLOAT)t;
				vecNormal += pNormals[0] * (1-(FLOAT)u) + pNormals[2] * (FLOAT)u;
				vecNormal = XMVector3Normalize(vecNormal);

				*pAlt = (FLOAT)XMVectorGetY(vecPos);
				*pVecNormal = vecNormal;
				*pDist = 0;

				return	true;
			}
		}

	}
	{
		//  When the position is not on the triangle 
		fDistMin = FLT_MAX;
		XMVECTOR pos1, pos2;
		XMVECTOR normal1, normal2;
		XMVECTOR vecNormal;
		FLOAT fTmp;

		//  line 0 to 1
		pos1 = pVertices[1];
		pos2 = pVertices[0];
		normal1 = pNormals[1];
		normal2 = pNormals[0];
		if (FitLineToBox(&pos1, &pos2, &normal1, &normal2, &vecMin, &vecMax)){
			CalcShortestDistanceAndAltitude(pPosition,&pos1, &pos2,&normal1,&normal2, &fTmp, &fDist,&vecNormal);
			if (fDist < fDistMin){
				fDistMin = fDist;
				fAlt = fTmp;
				*pVecNormal = vecNormal;
			}
		}
		//  line 1 to 2
		pos1 = pVertices[2];
		pos2 = pVertices[1];
		normal1 = pNormals[2];
		normal2 = pNormals[1];
		if (FitLineToBox(&pos1, &pos2, &normal1, &normal2, &vecMin, &vecMax)){
			CalcShortestDistanceAndAltitude(pPosition,&pos1, &pos2,&normal1,&normal2, &fTmp, &fDist,&vecNormal);
			if (fDist < fDistMin){
				fDistMin = fDist;
				fAlt = fTmp;
				*pVecNormal = vecNormal;
			}
		}
		//  line 2 to 0
		pos1 = pVertices[0];
		pos2 = pVertices[2];
		normal1 = pNormals[0];
		normal2 = pNormals[2];
		if (FitLineToBox(&pos1, &pos2, &normal1, &normal2, &vecMin, &vecMax)){
			CalcShortestDistanceAndAltitude(pPosition,&pos1, &pos2,&normal1,&normal2, &fTmp, &fDist,&vecNormal);
			if (fDist < fDistMin){
				fDistMin = fDist;
				fAlt = fTmp;
				*pVecNormal = vecNormal;
			}
		}
		if (fDistMin < FLT_MAX){

			fDist = (FLOAT)sqrt(fDistMin);
			*pDist = fDist;
			*pAlt = fAlt;
			return	TRUE;
		}
	}
	return false;
}

static BOOL	ProbeTheTriangleHeightOneSide(
			XMVECTOR *pVertices, XMVECTOR *pNormals,
			FLOAT *pDist, FLOAT *pAlt, 
			XMFLOAT3 *pBoxMin,  XMFLOAT3 *pBoxMax,
			XMFLOAT3 *pPosition,XMVECTOR *pVecNormal
){
	//  calculate the boundary aabb of the triangle
	XMFLOAT3 vecMin, vecMax;
	FLOAT	fAlt;
	FLOAT	fDist, fDistMin;
	vecMin = XMFLOAT3( FLT_MAX, FLT_MAX, FLT_MAX);
	vecMax = XMFLOAT3(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	XMVECTOR *pCoord = pVertices;
	for (int i = 0; i < 3 ; ++i){
		FLOAT x = XMVectorGetX(*pCoord);
		FLOAT y = XMVectorGetY(*pCoord);
		FLOAT z = XMVectorGetZ(*pCoord++);
		vecMin.x = min(vecMin.x, x);
		vecMin.y = min(vecMin.y, y);
		vecMin.z = min(vecMin.z, z);
		vecMax.x = max(vecMax.x, x);
		vecMax.y = max(vecMax.y, y);
		vecMax.z = max(vecMax.z, z);
	}
	if (!Collide2Box(pBoxMin,pBoxMax,&vecMin,&vecMax))
		return false;

	{
		//	裏向きのポリゴンとは当たり判定を行わない。
		XMVECTOR	vec1, vec2,vec3;
  		vec1 = pVertices[2] - pVertices[1];
		vec2 = pVertices[0] - pVertices[1];
		XMVectorSetY(vec1,0.f);
		XMVectorSetY(vec2,0.f);

		vec3 = XMVector3Cross(vec1,vec2);
		if (XMVectorGetY(vec3) < -FLT_MIN)
			return	FALSE;
	}
	FLOAT		len1, len2, len3, dx, dz;
	//	ポリゴンを、真上から、XZ 平面に投影した時の
	//	各辺の長さを算出。
	dx = XMVectorGetX(pVertices[1]) - XMVectorGetX(pVertices[0]);
	dz = XMVectorGetZ(pVertices[1]) - XMVectorGetZ(pVertices[0]);
	len1 = dx * dx + dz * dz;
	dx = XMVectorGetX(pVertices[2]) - XMVectorGetX(pVertices[1]);
	dz = XMVectorGetZ(pVertices[2]) - XMVectorGetZ(pVertices[1]);
	len2 = dx * dx + dz * dz;
	dx = XMVectorGetX(pVertices[0]) - XMVectorGetX(pVertices[2]);
	dz = XMVectorGetZ(pVertices[0]) - XMVectorGetZ(pVertices[2]);
	len3 = dx * dx + dz * dz;

	//	you can stand on the polygon only in case of it has enough area.
	if ((fabs(len1) > FLT_MIN)&&(fabs(len2) > FLT_MIN)&&(fabs(len3) > FLT_MIN)){
		FLOAT x, y, z;
		XMVECTOR vecDir1, vecDir2, vecTmp;
		BOOL bF0 = false, bF1 = false, bF2 = false;
		x = pPosition->x;
		y = pPosition->y;
		z = pPosition->z;
		vecDir1 = XMLoadFloat3(pPosition) - pVertices[0];
		vecDir2 = pVertices[1] - pVertices[0];
		XMVectorSetY(vecDir1,0.0f);
		XMVectorSetY(vecDir2,0.0f);
		vecTmp = XMVector3Cross(vecDir1,vecDir2);
		if (XMVectorGetY(vecTmp) > -FLT_MIN){
			bF0 = true;
		}

		vecDir1 = XMLoadFloat3(pPosition) - pVertices[1];
		vecDir2 = pVertices[2] - pVertices[1];
		XMVectorSetY(vecDir1,0.0f);
		XMVectorSetY(vecDir2,0.0f);
		vecTmp = XMVector3Cross(vecDir1,vecDir2);
		if (XMVectorGetY(vecTmp) > -FLT_MIN){
			bF1=TRUE;
		}

		vecDir1 = XMLoadFloat3(pPosition) - pVertices[2];
		vecDir2 = pVertices[0] - pVertices[2];
		XMVectorSetY(vecDir1,0.0f);
		XMVectorSetY(vecDir2,0.0f);
		vecTmp = XMVector3Cross(vecDir1,vecDir2);
		if (XMVectorGetY(vecTmp) > -FLT_MIN){
			bF2=TRUE;
		}

		fDistMin = FLT_MAX;
		//  if the position is inside the triangle when you see it on x-z plane.
		if (!bF0 && !bF1 && !bF2){
			double	t,u,inv;
			FLOAT	px = x - XMVectorGetX(pVertices[0]);
			FLOAT	pz = z - XMVectorGetZ(pVertices[0]);
			vecDir1 = pVertices[1] - pVertices[0];
			vecDir2 = pVertices[2] - pVertices[0];
			FLOAT	x1 = XMVectorGetX(vecDir1);
			FLOAT	z1 = XMVectorGetZ(vecDir1);
			FLOAT	x2 = XMVectorGetX(vecDir2);
			FLOAT	z2 = XMVectorGetZ(vecDir2);

			//  Evaluate t and u when px and pz is assumed as below
			//  px = t*x1 + u*x2
			//  pz = t*z1 + u*z2
			//  we use the Cramer's rule to evaluate them
			inv = (x1*z2 - x2*z1);
			if (fabs(inv) > FLT_MIN){
				inv = 1.0f / inv;	//  normalize the reversed normal
				t = inv * (px*z2 - pz*x2);
				u = inv * (x1*pz - px*z1);

				XMVECTOR vecPos, vecNormal;
				//	Calculate the position
				vecPos = vecDir1*(FLOAT)t + vecDir2*(FLOAT)u;
				vecPos += pVertices[0];

				//	Calculate the normal
				vecNormal = pNormals[0] * (1-(FLOAT)t) + pNormals[1] * (FLOAT)t;
				vecNormal += pNormals[0] * (1-(FLOAT)u) + pNormals[2] * (FLOAT)u;
				vecNormal = XMVector3Normalize(vecNormal);

				*pAlt = (FLOAT)XMVectorGetY(vecPos);
				*pVecNormal = vecNormal;
				*pDist = 0;

				return	true;
			}
		}

	}
	{
		//  When the position is not on the triangle 
		fDistMin = FLT_MAX;
		XMVECTOR pos1, pos2;
		XMVECTOR normal1, normal2;
		XMVECTOR vecNormal;
		FLOAT fTmp;

		//  line 0 to 1
		pos1 = pVertices[1];
		pos2 = pVertices[0];
		normal1 = pNormals[1];
		normal2 = pNormals[0];
		if (FitLineToBox(&pos1, &pos2, &normal1, &normal2, &vecMin, &vecMax)){
			CalcShortestDistanceAndAltitude(pPosition,&pos1, &pos2,&normal1,&normal2, &fTmp, &fDist,&vecNormal);
			if (fDist < fDistMin){
				fDistMin = fDist;
				fAlt = fTmp;
				*pVecNormal = vecNormal;
			}
		}
		//  line 1 to 2
		pos1 = pVertices[2];
		pos2 = pVertices[1];
		normal1 = pNormals[2];
		normal2 = pNormals[1];
		if (FitLineToBox(&pos1, &pos2, &normal1, &normal2, &vecMin, &vecMax)){
			CalcShortestDistanceAndAltitude(pPosition,&pos1, &pos2,&normal1,&normal2, &fTmp, &fDist,&vecNormal);
			if (fDist < fDistMin){
				fDistMin = fDist;
				fAlt = fTmp;
				*pVecNormal = vecNormal;
			}
		}
		//  line 2 to 0
		pos1 = pVertices[0];
		pos2 = pVertices[2];
		normal1 = pNormals[0];
		normal2 = pNormals[2];
		if (FitLineToBox(&pos1, &pos2, &normal1, &normal2, &vecMin, &vecMax)){
			CalcShortestDistanceAndAltitude(pPosition,&pos1, &pos2,&normal1,&normal2, &fTmp, &fDist,&vecNormal);
			if (fDist < fDistMin){
				fDistMin = fDist;
				fAlt = fTmp;
				*pVecNormal = vecNormal;
			}
		}
		if (fDistMin < FLT_MAX){
			fDist = (FLOAT)sqrt(fDistMin);
			*pDist = fDist;
			*pAlt = fAlt;
			return	TRUE;
		}
	}
	return false;
}

//  Intersect the segment with an AABB
static BOOL FitLineToBox(
	DirectX::XMVECTOR *pPos1, DirectX::XMVECTOR *pPos2,
	DirectX::XMVECTOR *pNormal1, DirectX::XMVECTOR *pNormal2,
	DirectX::XMFLOAT3 *pVecMin, DirectX::XMFLOAT3 *pVecMax
){
	BOOL	b1, b2;
	b1 = true;	b2 = true;
	XMVECTOR	vp1 = *pPos1, vp2 = *pPos2;
	XMVECTOR	vn1 = *pNormal1, vn2 = *pNormal2;
	XMVECTOR	vtmp;
	double	ftmp;
	double	w,h,d;	//	width, height, depth

	//	Clip with minimum x plane
	b1 = XMVectorGetX(vp1) < pVecMin->x;
	b2 = XMVectorGetX(vp2) < pVecMax->x;
	if (b1 && b2)
		return	false;
	if (b1 != b2){
		if (b1){
			ftmp = (pVecMin->x-XMVectorGetX(vp1)) / (XMVectorGetX(vp2)-XMVectorGetX(vp1));
			vp1 += (vp2 -vp1) * (FLOAT)ftmp;
		}else{
			ftmp = (pVecMin->x - XMVectorGetX(vp2)) / (XMVectorGetX(vp1) - XMVectorGetX(vp2));
			vp2 += (vp1 - vp2) * (FLOAT)ftmp;
		}
	}
	//  clip with maximum x plane
	b1 = XMVectorGetX(vp1) > pVecMax->x;
	b2 = XMVectorGetX(vp2) > pVecMax->x;
	if (b1 && b2)
		return	false;

	if (b1!=b2){
		if (b1){
			ftmp = (pVecMax->x - XMVectorGetX(vp1)) / (XMVectorGetX(vp2) - XMVectorGetX(vp1));
			vp1 += (vp2 - vp1) * (FLOAT)ftmp;
		}else{
			ftmp = (pVecMax->x - XMVectorGetX(vp2)) / (XMVectorGetX(vp1) - XMVectorGetX(vp2));
			vp2 += (vp1 - vp2) * (FLOAT)ftmp;
		}
	}

	//	Clip with minimum y plane
	b1 = XMVectorGetY(vp1) < pVecMin->y;
	b2 = XMVectorGetY(vp2) < pVecMin->y;
	if (b1 && b2)
		return	false;

	if (b1!=b2){
		if (b1){
			ftmp = (pVecMin->y - XMVectorGetY(vp1)) / (XMVectorGetY(vp2) - XMVectorGetY(vp1));
			vp1 += (vp2 - vp1) * (FLOAT)ftmp;
		}else{
			ftmp = (pVecMin->y - XMVectorGetY(vp2)) / (XMVectorGetY(vp1) - XMVectorGetY(vp2));
			vp2 += (vp1 - vp2) * (FLOAT)ftmp;
		}
	}

	//  clip with maximum y plane
	b1 = XMVectorGetY(vp1) > pVecMax->y;
	b2 = XMVectorGetY(vp2) > pVecMax->y;
	if (b1 && b2)
		return	false;

	if (b1!=b2){
		if (b1){
			ftmp = (pVecMax->y - XMVectorGetY(vp1)) / (XMVectorGetY(vp2) - XMVectorGetY(vp1));
			vp1 += (vp2 - vp1) * (FLOAT)ftmp;
		}else{
			ftmp = (pVecMax->y - XMVectorGetY(vp2)) / (XMVectorGetY(vp1) - XMVectorGetY(vp2));
			vp2 += (vp1 - vp2) * (FLOAT)ftmp;
		}
	}

	//	Clip with minimum z plane
	b1 = XMVectorGetZ(vp1) < pVecMin->z;
	b2 = XMVectorGetZ(vp2) < pVecMin->z;
	if (b1 && b2)
		return	false;

	if (b1!=b2){
		if (b1){
			ftmp = (pVecMin->z - XMVectorGetZ(vp1)) / (XMVectorGetZ(vp2) - XMVectorGetZ(vp1));
			vp1 += (vp2 - vp1) * (FLOAT)ftmp;
		}else{
			ftmp = (pVecMin->z - XMVectorGetZ(vp2)) / (XMVectorGetZ(vp1) - XMVectorGetZ(vp2));
			vp2 += (vp1 - vp2) * (FLOAT)ftmp;
		}
	}

	//  clip with maximum z plane
	b1 = XMVectorGetZ(vp1) > pVecMax->z;
	b2 = XMVectorGetZ(vp2) > pVecMax->z;
	if (b1 && b2)
		return	false;

	if (b1!=b2){
		if (b1){
			ftmp = (pVecMax->z - XMVectorGetZ(vp1)) / (XMVectorGetZ(vp2) - XMVectorGetZ(vp1));
			vp1 += (vp2 - vp1) * (FLOAT)ftmp;
		}else{
			ftmp = (pVecMax->z - XMVectorGetZ(vp2)) / (XMVectorGetZ(vp1) - XMVectorGetZ(vp2));
			vp2 += (vp1 - vp2) * (FLOAT)ftmp;
		}
	}

	XMVECTOR vecTmp = *pPos2 - *pPos1;
	w = XMVectorGetX(vecTmp);
	h = XMVectorGetY(vecTmp);
	d = XMVectorGetZ(vecTmp);
	w *= w;	h *= h;	d *= d;
	if (w > h && w > d){
		//	x -- clip the segment with y-z plane
		w = 1.0 / w;
		ftmp = (XMVectorGetX(vp1) - XMVectorGetX(*pPos1)) * w;
		vtmp = *pNormal2 * (FLOAT)ftmp + *pNormal1 * (FLOAT)(1.0 - ftmp);
		vn1 = XMVector3Normalize(vtmp);

		ftmp = (XMVectorGetX(vp2) - XMVectorGetX(*pPos1)) * w;
		vtmp = *pNormal2 * (FLOAT)ftmp + *pNormal1 * (FLOAT)(1.0 - ftmp);
		vn2 = XMVector3Normalize(vtmp);

	}else if (h > d && h > w){
		//	y  -- clip the segment with x-z plane
		h = 1.0 / h;
		ftmp = (XMVectorGetY(vp1) - XMVectorGetY(*pPos1)) * h;
		vtmp = *pNormal2 * (FLOAT)ftmp + *pNormal1 * (FLOAT)(1.0 - ftmp);
		vn1 = XMVector3Normalize(vtmp);

		ftmp = (XMVectorGetY(vp2) - XMVectorGetY(*pPos1)) * h;
		vtmp = *pNormal2 * (FLOAT)ftmp + *pNormal1 * (FLOAT)(1.0 - ftmp);
		vn2 = XMVector3Normalize(vtmp);

	}else{
		//	z -- clip the segment with x-y plane
		d = 1.0 / d;
		ftmp = (XMVectorGetZ(vp1) - XMVectorGetZ(*pPos1)) * d;
		vtmp = *pNormal2 * (FLOAT)ftmp + *pNormal1 * (FLOAT)(1.0 - ftmp);
		vn1 = XMVector3Normalize(vtmp);

		ftmp = (XMVectorGetZ(vp2) - XMVectorGetZ(*pPos1)) * d;
		vtmp = *pNormal2 * (FLOAT)ftmp + *pNormal1 * (FLOAT)(1.0 - ftmp);
		vn2 = XMVector3Normalize(vtmp);
	}
	//  out put the vertices
	*pPos1 = vp1;		*pNormal1 = vn1;
	*pPos2 = vp2;		*pNormal2 = vn2;
	return true;
}

static void CalcShortestDistanceAndAltitude(DirectX::XMFLOAT3 *vecPos, 
	DirectX::XMVECTOR *pPos1, DirectX::XMVECTOR *pPos2,
	DirectX::XMVECTOR *pNormal1, DirectX::XMVECTOR *pNormal2,
	float *pAlt, float *pDist, DirectX::XMVECTOR *pNormal)
{
	double	t;
	double	dx1, dy1, dz1, dx2, dz2;
	double	len, len1, len2;	
	double	px, py, pz;

	XMVECTOR vecTmp = *pPos2 - *pPos1;
	dx1 = XMVectorGetX(vecTmp);
	dy1 = XMVectorGetY(vecTmp);
	dz1 = XMVectorGetZ(vecTmp);

	dx2 = XMVectorGetX(*pPos1) - vecPos->x;
//	dy2 = XMVectorGetY(*pPos1) - vecPos->y;
	dz2 = XMVectorGetZ(*pPos1) - vecPos->z;

	len = dx1*dx1 + dz1*dz1;
	if (len < FLT_MIN){
		*pDist = (float)(dx2*dx2 + dz2 * dz2);
		*pAlt = XMVectorGetX(*pPos1);
		*pNormal = XMVector3Normalize(*pNormal1);
		return;
	}

	//	the differential formula of the distance between 
	//  the given position and the any position on the segment
	//	and calculate the position where the fomula returns
	//  0. you can find the nearest position on the segment.
	//  
	t = -(dx1 * dx2 + dz1*dz2);
	t /= len;
	if ((t >= 0.0f) && (t <= 1.0f)){
		//	線上の点が最小距離だった場合

		//	距離が最小となる線分P1-P2 上の点を求める

		px = XMVectorGetX(*pPos1) + (dx1*t);
		py = XMVectorGetY(*pPos1) + (dy1*t);
		pz = XMVectorGetZ(*pPos1) + (dz1*t);

		//	求められた点との距離を算出
		dx1 = px - vecPos->x;		//	求められた点との
		dz1 = pz - vecPos->z;		//	距離を算出
		len = dx1*dx1 + dz1*dz1;
		*pDist = (float)len;		//	呼び出し元へ
		*pAlt = (float)py;			//	値を返す。
		*pNormal = XMVector3Normalize(*pNormal1*(float)(1.0-t) + (*pNormal2 * (float)t));
		return;
	}else{
		//	いずれかの頂点が最小距離である場合。
		dx1  = XMVectorGetX(*pPos2) - vecPos->x;
		dz1  = XMVectorGetX(*pPos2) - vecPos->z;
		len1  = dx2*dx2 + dz2*dz2;	//	P1 までの距離
		len2 = dx1*dx1 + dz1*dz1;	//	P2 までの距離
		if ( len2 < len1 ){
			//	P2までの距離と、P2 の高度を返す
			*pNormal = XMVector3Normalize(*pNormal2);
			*pDist = (float)len2;
			*pAlt = XMVectorGetY(*pPos2);
			return;
		}
		//	P1 までの距離と、P1 の高度を返す
		*pNormal = XMVector3Normalize(*pNormal1);
		*pDist = (float)len1;
		*pAlt = XMVectorGetY(*pPos1);
	}
	return;
}

BOOL	CMeshCollider::ProbeTheGroundAltitudeOneSide(XMFLOAT3 *pPosition, XMFLOAT3 *pBoxMin, XMFLOAT3 *pBoxMax, XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist){
	//  Probe the ground altitude (OneSide)
	DirectX::XMFLOAT3	vecMin, vecMax;
	DirectX::XMVECTOR	vecNormal, vecNormalTmp;
	FLOAT	fDist,fMinDist,fAlt, fAltTmp;
	BOOL	bRet = false;

	vecNormalTmp = DirectX::XMLoadFloat3(pVecNormal);
	
	GetBoundingAABBFloat3(&vecMin, &vecMax);
	
	//	与えられた箱とこの地形のバウンダリボックスとの
	//	衝突判定
	if (!Collide2Box(&vecMin,&vecMax,pBoxMin,pBoxMax)){
		return false;
	}

	fMinDist = FLT_MAX;
	//	メッシュから個別にポリゴンを抽出する。
	for (int i = 0; i < (int)m_dwNumFrames ; ++i){
		int index = 0, k;
		COLLISIONVERTEX *pVertices;
		XMVECTOR positions[3], normals[3];
		MeshFrame *pFrame = m_ppMeshFrames[i];
		XMMATRIX matWorld;

		if (pFrame == NULL)
			continue;
		MeshContainer *pContainer = pFrame->pMeshContainer;
		if (pContainer == NULL)
			continue;
		Mesh *pMesh = pContainer->pMeshData;
		if (pMesh == NULL)
			continue;

		//  Prepare world matrix
		matWorld = m_matWorld * pFrame->CombinedMatrix;

		index = 0;
		pVertices = (COLLISIONVERTEX*)pMesh->pVertices;
		int iNumIndices = pMesh->numIndices * 3;
		while(index < iNumIndices){
			k = pMesh->pIndices[index++];
			positions[0] = XMLoadFloat3(&pVertices[k].position);
			normals[0]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[1] = XMLoadFloat3(&pVertices[k].position);
			normals[1]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[2] = XMLoadFloat3(&pVertices[k].position);
			normals[2]   = XMLoadFloat3(&pVertices[k].normal);

			//  transform'em into world coordinate system
			for (int i = 0; i < 3 ; ++i){
				positions[i] = XMVector3TransformCoord(positions[i],matWorld);
				normals[i]   = XMVector3TransformNormal(normals[i], matWorld);
			}

			if (ProbeTheTriangleHeightOneSide(positions,normals,&fDist,&fAltTmp,pBoxMin,pBoxMax,pPosition,&vecNormalTmp)){
				if (fDist == 0){
					bRet = true;
					fMinDist = 0.0f;
					fAlt = fAltTmp;
					vecNormal = vecNormalTmp;
					break;
				}
				if (fDist < fMinDist){
					bRet = true;
					fMinDist = fDist;
					fAlt = fAltTmp;
					vecNormal = vecNormalTmp;
				}
			}
		}
	}
	if (bRet){
		*pAlt = fAlt;
		*pDist = fMinDist;
		pVecNormal->x = XMVectorGetX(vecNormal);
		pVecNormal->y = XMVectorGetY(vecNormal);
		pVecNormal->z = XMVectorGetZ(vecNormal);
	}
	return	bRet;
}

BOOL	CMeshCollider::ProbeTheGroundAltitudeVerticallyNearest(XMFLOAT3 *pPosition, XMFLOAT3 *pBoxMin, XMFLOAT3 *pBoxMax, XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist){
	//  Probe the ground altitude (OneSide)
	DirectX::XMFLOAT3	vecMin, vecMax;
	DirectX::XMVECTOR	vecNormal, vecNormalTmp;
	FLOAT	fDist,fAlt, fAltTmp;
	FLOAT   fNearest;
	FLOAT	fDistRange,w,d;
	BOOL	bRet = false;
	
	vecNormalTmp = DirectX::XMLoadFloat3(pVecNormal);
	
	GetBoundingAABBFloat3(&vecMin, &vecMax);
	
	//	check if the box and the bounding box intersect
	if (!Collide2Box(&vecMin,&vecMax,pBoxMin,pBoxMax)){
		return false;
	}
	w = (vecMax.x - vecMin.x) * .5f;
	w *= w;
	d = (vecMax.z - vecMin.z) * .5f;
	d *= d;
	fDistRange = w + d;
	vecNormalTmp = XMLoadFloat3(pVecNormal);

	fNearest = FLT_MAX;
	//	iterate each polygon on the mesh.
	for (int i = 0; i < (int)m_dwNumFrames ; ++i){
		int index = 0, k;
		COLLISIONVERTEX *pVertices;
		XMVECTOR positions[3], normals[3];
		MeshFrame *pFrame = m_ppMeshFrames[i];
		XMMATRIX matWorld;

		if (pFrame == NULL)
			continue;
		MeshContainer *pContainer = pFrame->pMeshContainer;
		if (pContainer == NULL)
			continue;
		Mesh *pMesh = pContainer->pMeshData;
		if (pMesh == NULL)
			continue;

		//  Prepare world matrix
		matWorld = m_matWorld * pFrame->CombinedMatrix;

		index = 0;
		pVertices = (COLLISIONVERTEX*)pMesh->pVertices;
		int iNumIndices = pMesh->numIndices * 3;
		while(index < iNumIndices){
			k = pMesh->pIndices[index++];
			positions[0] = XMLoadFloat3(&pVertices[k].position);
			normals[0]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[1] = XMLoadFloat3(&pVertices[k].position);
			normals[1]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[2] = XMLoadFloat3(&pVertices[k].position);
			normals[2]   = XMLoadFloat3(&pVertices[k].normal);

			//  transform'em into world coordinate system
			for (int i = 0; i < 3 ; ++i){
				positions[i] = XMVector3TransformCoord(positions[i],matWorld);
				normals[i]   = XMVector3TransformNormal(normals[i], matWorld);
			}

			if (ProbeTheTriangleHeightOneSide(positions,normals,&fDist,&fAltTmp,pBoxMin,pBoxMax,pPosition,&vecNormalTmp)){
				FLOAT fTmp = fAltTmp - pPosition->y;
				fTmp *= fTmp;
				if (fDist < fDistRange && fTmp < fNearest){
					fNearest = fTmp;
					fAlt = fAltTmp;
					vecNormal = vecNormalTmp;
					bRet = true;
				}
			}
		}
	}
	if (bRet){
		*pAlt = fAlt;
		*pDist = fNearest;
		pVecNormal->x = XMVectorGetX(vecNormal);
		pVecNormal->y = XMVectorGetY(vecNormal);
		pVecNormal->z = XMVectorGetZ(vecNormal);
	}
	return	bRet;
}

//  Name: CalcTriangleBoundingBox
static void inline CalcTriangleBoundingBox(XMVECTOR *pVertices, XMFLOAT3 *pVecMin, XMFLOAT3 *pVecMax){
	XMFLOAT3 vecMin = XMFLOAT3( FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 vecMax = XMFLOAT3(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	FLOAT x,y,z;
	//  iterate the vertices
	for (int i = 0; i < 3 ; ++i){

		x = XMVectorGetX(*pVertices);
		y = XMVectorGetY(*pVertices);
		z = XMVectorGetZ(*pVertices);

		vecMin.x = min(vecMin.x, x);
		vecMin.y = min(vecMin.y, y);
		vecMin.z = min(vecMin.z, z);

		vecMax.x = max(vecMax.x, x);
		vecMax.y = max(vecMax.y, y);
		vecMax.z = max(vecMax.z, z);
		++pVertices;
	}
	*pVecMin = vecMin;
	*pVecMax = vecMax;
}

//  Function: CalcGoBackDir
static BOOL CalcGoBackDir(std::vector<COLLISIONINFO*>*pCollisionInfos, XMVECTOR *pVecPos, XMVECTOR *pVecMotion, XMVECTOR *pVecNormal, FLOAT fRadius){
	BOOL bRet = true;
	int iNumCollisionInfo = pCollisionInfos->size();
	XMFLOAT3 vecDirMin = XMFLOAT3( FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 vecDirMax = XMFLOAT3(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	XMVECTOR vecNormal;
	COLLISIONINFO *p;
	FLOAT x, y , z;

	//  scan all collision infos and create the medium of the normal
	for (int i = 0 ; i < iNumCollisionInfo; ++i){
		p = pCollisionInfos->at(0);
		x = XMVectorGetX(p->n);
		y = XMVectorGetY(p->n);
		z = XMVectorGetZ(p->n);
		vecDirMin.x = min(vecDirMin.x,x);
		vecDirMin.y = min(vecDirMin.y,y);
		vecDirMin.z = min(vecDirMin.z,z);
		vecDirMax.x = max(vecDirMax.x,x);
		vecDirMax.y = max(vecDirMax.y,y);
		vecDirMax.z = max(vecDirMax.z,z);
	}
	x = (vecDirMin.x+vecDirMax.x)*0.5f;	
	y = (vecDirMin.y+vecDirMax.y)*0.5f;	
	z = (vecDirMin.z+vecDirMax.z)*0.5f;	
	if ((x*x+y*y+z*z) > FLT_MIN){
		//vecNormal = XMVector3Normalize(XMLoadFloat3(&XMFLOAT3(x,y,z)));
		vecNormal = XMVector3Normalize(XMVectorSet(x,y,z,0));
		//	Check the reliability
		//	if the probe stacks in polygons.
		FLOAT l = XMVectorGetX(XMVector3Length(*pVecMotion));
		if (l > FLT_MIN){
			XMVECTOR vecReverse = *pVecMotion / (-l);
			FLOAT fDot;
			XMVECTOR	vecN1, vecN2;
			XMVECTOR	vecM1, vecM2;
			XMVECTOR	vecP1, vecP2;
			XMVECTOR    vecVertical, vecTmp;
			BOOL b1, b2;
			for (int i = 0; i < iNumCollisionInfo ; ++i){
				p = pCollisionInfos->at(i);
				vecN1 = p->n;
				vecP1 = p->nearest;
				vecM1 = XMVector3Normalize(*pVecPos - vecP1);
				for (int j = i+1 ; j < iNumCollisionInfo ; ++j){
					//  check the angle of 2 vectors
					p = pCollisionInfos->at(j);
					vecN2 = p->n;
					vecP2 = p->nearest;
					vecM2 = XMVector3Normalize(*pVecPos - vecP1);
					vecVertical = XMVector3Cross(vecN1, vecN2);
					l = XMVectorGetX(XMVector3Length(vecVertical));
					if (l > FLT_MIN){
						fDot = XMVectorGetX(XMVector3Dot(vecN1,vecN2));
						if (fDot < FLT_MIN){
							//  if the angle is more than 90 degrees
							vecVertical /= l;
							vecM2 = XMVector3Normalize(vecM2);
							if (XMVectorGetX(XMVector3Dot(vecM1,vecM2)) < 0.5f){
								//  if the angle made with 2 nearest positions and 
								//  the center of the probe is more than 60 degrees 
								//	we assume the probe is stacked in polygons
								vecTmp = XMVector3Cross(vecReverse,vecM1);
								b1 = (
									XMVectorGetX(XMVector3Dot(vecTmp,vecVertical)) > 0
								);
								vecTmp = XMVector3Cross(vecReverse,vecM2);
								b2 = (
									XMVectorGetX(XMVector3Dot(vecTmp,vecVertical)) > 0
								);
								if (b1 != b2){	//	面法線が進行方向の逆方向ベクトルを挟んでいる
									bRet = false;	//	
								}
							}
						}
					}
				}
			}

		}
	}else{
		//  if the medium of the normal is almost 0 length
		//  select the nearest polygon to decide go back dir.
		FLOAT	fMax = -FLT_MAX;
		for (int i = 0; i < iNumCollisionInfo ; ++i){
			p = pCollisionInfos->at(i);
			if (p->amount > fMax){
				vecNormal = p->n;
				fMax = p->amount;
			}
			++p;
		}
		bRet = false;
	}
	*pVecNormal = vecNormal;
	return bRet;
}

//====================================================================
//  Function : AdjustDepth
//
// @brief : now we got the direction to go back we have to decide the
//        amount to go back.
//
// @param :
//   pCollisionInfo : (in) collection of COLLISIONINFO contains polygons which intersects the probe
//   pPosition      : (in) central position of the probe
//   fRadius        : (in) radius of the probe
//   pVecNormal     : (in) go back dir
//   *pDepth        : (out) result amount
// @return          : true ... the result is reliable 
//                  : false ... the result is doubtful.
//====================================================================
BOOL AdjustDepth(std::vector<COLLISIONINFO*> *pCollisionInfo, XMVECTOR *pPosition, FLOAT fRadius, XMVECTOR *pVecNormal, FLOAT *pDepth){
	COLLISIONINFO *p;
	FLOAT	fMove=-FLT_MAX;
	FLOAT	fMoveSynth = 0;
	XMVECTOR	vecNearest;
	FLOAT	fDot, fTmp;
	XMVECTOR	vecNormal = *pVecNormal;
	int	iNumCollisionInfo = pCollisionInfo->size();
	BOOL	bRet = true;	//	reliability

	for (int i = 0; i < iNumCollisionInfo ; ++i){
		p = pCollisionInfo->at(i);
		fDot = XMVectorGetX(XMVector3Dot(vecNormal,p->n));
		if (fDot> FLT_MIN){  //  Ignore a polygon crossed right angle or reversed
			fMove = p->amount / fDot;
			if (fMove > fRadius * 2.0f)
				fMove = fRadius * 2.0f;
			//  Predict new position
			XMVECTOR vecNewPos, vecRewind;
			vecNewPos = *pPosition + (fMove * vecNormal);
			vecNearest = p->nearest;
			vecRewind = -vecNormal;	//	reverse of go back dir
			//  Assume
			//  T:nearest position P:position of the probe
			//  V:direction to rewind D: P - T
			//  
			//	Evaluate
			//  V^2*t^2+2DVt+(D^2-r^2) = 0 
			//
			//  means you go to the position whose distance between P and T is
			//  equal to probe's radius.
			//
			double a, b, c, d, t, t1, t2;
			a = XMVectorGetX(XMVector3LengthSq(vecRewind));
			c = XMVectorGetX(XMVector3LengthSq(vecNewPos - vecNearest));
			c = c - fRadius*fRadius;
			b = 2.0f * XMVectorGetX(XMVector3Dot((vecNewPos - vecNearest), vecRewind));
			d = b*b-4.0*a*c;
			if (d < 0){
				//	解が無い時はこのポリゴンはスキップ
				fTmp = 0;
			}else{
				d = sqrt(d);
				t1 = -b+d;
				t2 = -b-d;
				t1 /= 2.0 * a;
				t2 /= 2.0 * a;
				if (t1 >= 0 && t2 >= 0){
					t = min(t1,t2);
					fTmp = (float)t;
				}else{
					//	解ありでこうなる可能性は考えにくい
					fTmp = 0;
				}
			}
			if (fMove >= fTmp)
				fMove -= fTmp;
			if (fMove > fMoveSynth){
				fMoveSynth = fMove;
			}
		}
	}
	*pDepth = fMoveSynth;
	return bRet;
}

static void MatrixInverseRotation(XMMATRIX *pOut, XMVECTOR *pVecX,XMVECTOR *pVecY, XMVECTOR *pVecZ){
	*pOut = XMMatrixSet(
		XMVectorGetX(*pVecX),	XMVectorGetX(*pVecY),	XMVectorGetX(*pVecZ),	0.0f,
		XMVectorGetY(*pVecX),	XMVectorGetY(*pVecY),	XMVectorGetY(*pVecZ),	0.0f,
		XMVectorGetZ(*pVecX),	XMVectorGetZ(*pVecY),	XMVectorGetZ(*pVecZ),	0.0f,
		0.0f,					0.0f,					0.0f,					1.0f
	);
}

// Probe the triangle distance with segment
BOOL	ProbeTheTriangleDistanceWithSegment(XMVECTOR *pPositions, FLOAT *pDist, XMVECTOR *pVec1, XMVECTOR *pVec2, FLOAT fRadius){
	XMVECTOR vecPos;
	XMVECTOR vecPos1, vecPos2;
	XMVECTOR positions[3];
	FLOAT		len1, len2, len3;
	vecPos1 = *pVec1;
	vecPos2 = *pVec2;

	//  calc the length of each edge
	len1 = XMVectorGetX(XMVector3LengthSq(pPositions[1]-pPositions[0]));
	len2 = XMVectorGetY(XMVector3LengthSq(pPositions[2]-pPositions[1]));
	len3 = XMVectorGetY(XMVector3LengthSq(pPositions[0]-pPositions[2]));

	//  if the triangle has enough area
	if ((fabs(len1) >= FLT_MIN)&&(fabs(len2) >= FLT_MIN)&&(fabs(len3) >= FLT_MIN)){
		//  make the p0-p1-p2 triangle
		//  and find the longest edge 
		//  then make the edge to p0-p1
		if ((len2 >= len1) && (len2 >= len3)){
			positions[0] = pPositions[1];
			positions[1] = pPositions[2];
			positions[2] = pPositions[0];
		}else if ((len3 >= len1)&&(len3>=len2)){
			positions[2] = pPositions[1];
			positions[1] = pPositions[0];
			positions[0] = pPositions[2];
		}else{
			positions[2] = pPositions[2];
			positions[1] = pPositions[1];
			positions[0] = pPositions[0];
		}

		//	caculate the rotation matrix 
		//  that makes the edge p0-p1 and Z axis
		//	in parallel.
		XMMATRIX	matRotation;
		XMVECTOR	vecOrd;
		XMVECTOR	vecZ, vecX, vecY;
		vecZ = XMVector3Normalize(positions[1] - positions[0]);
		vecY = XMVector3Normalize(XMVector3Cross(vecZ,positions[2] - positions[0]));
		vecX = XMVector3Cross(vecY,vecZ);
		MatrixInverseRotation(&matRotation,&vecX,&vecY,&vecZ);
		//  
		vecOrd = positions[0];
		vecPos1 = XMVector3TransformCoord(vecPos1 - vecOrd,matRotation);
		vecPos2 = XMVector3TransformCoord(vecPos2 - vecOrd,matRotation);

		//	check with x - z plane (y = 0)
		if ((XMVectorGetY(vecPos1) > fRadius)&&(XMVectorGetY(vecPos2) > fRadius))
			return	false;

		if ((XMVectorGetY(vecPos1) < -fRadius)&&(XMVectorGetY(vecPos2) < -fRadius))
			return	false;

		if ((XMVectorGetY(vecPos1) * XMVectorGetY(vecPos2))<=0){
			//	if vecPos1 and vecPos2 is across the x-z plane.
			FLOAT t;
			if (fabs(XMVectorGetY(vecPos1) - XMVectorGetY(vecPos2)) > FLT_MIN){
				//	t = (0 - XMVectorGetY(vecPos1)) / (XMVectorGetY(vecPos2) - XMVectorGetY(vecPos1));
				t = XMVectorGetY(vecPos1) / (XMVectorGetY(vecPos1) - XMVectorGetY(vecPos2));
				//  get the position where y equals 0.
				vecPos = (1-t) * vecPos1 + t * vecPos2;
			}else{
				vecPos = vecPos1;
			}
		}else if (fabs(XMVectorGetY(vecPos1)) < fabs(XMVectorGetY(vecPos2))){
			vecPos = vecPos1;
		}else{
			vecPos = vecPos2;
		}

		//  assume point p0 as origin
		//  and transform all vertices pressed 
		//  on x-z plane 
		//  node: do not transform normals
		for (int i = 1; i < 3 ; ++i){
			XMVECTOR vecTmp = positions[i] - vecOrd;
			positions[i] = XMVector3TransformCoord(positions[i],matRotation);
		}
		positions[0] = XMVectorSet(0,0,0,0);	//XMLoadFloat3(&XMFLOAT3(0,0,0));
		FLOAT x, y, z;
		BOOL bF0=false, bF1 = false, bF2 = false;
		x = XMVectorGetX(vecPos);
		y = XMVectorGetY(vecPos);
		z = XMVectorGetZ(vecPos);
		XMVECTOR vecDir1 = vecPos;
		XMVECTOR vecDir2 = positions[1] - positions[0];
		//  XMVECTOR vecTmp = XMVector3Cross(vecDir1, vecDir2);
		//  FLOAT fCross = XMVectorGetY(vecTmp);
		FLOAT fCross = XMVectorGetZ(vecDir1) * XMVectorGetX(vecDir2);
		fCross -= XMVectorGetX(vecDir1) * XMVectorGetZ(vecDir2);
		if (fCross > -FLT_MIN){
			bF0 = true;
		}
		vecDir1 = vecPos - positions[1];
		vecDir2 = positions[2] - positions[1];
		fCross = XMVectorGetZ(vecDir1) * XMVectorGetX(vecDir2);
		fCross -= XMVectorGetX(vecDir1) * XMVectorGetZ(vecDir2);
		if (fCross > -FLT_MIN){
			bF1 = true;
		}
		vecDir1 = vecPos - positions[2];
		vecDir2 = positions[0] - positions[2];
		fCross = XMVectorGetZ(vecDir1) * XMVectorGetX(vecDir2);
		fCross -= XMVectorGetX(vecDir1) * XMVectorGetZ(vecDir2);
		if (fCross > -FLT_MIN){
			bF0 = true;
		}
		if (!bF0 && !bF1 && !bF2){
			//  if the position is on the triangle
			*pDist = XMVectorGetY(vecPos);
			return true;
		}
		FLOAT fDist = FLT_MAX;
		FLOAT fTmp;
		XMVECTOR vecTmp;
		if (bF0){
			CalcShortestDistance(&vecPos,&positions[1],&positions[0],
				NULL,NULL,&fTmp,&vecTmp,NULL);
			if (fTmp < fDist)
				fDist = fTmp;
		}
		if (bF1){
			CalcShortestDistance(&vecPos,&positions[2],&positions[1],
				NULL,NULL,&fTmp,&vecTmp,NULL);
			if (fTmp < fDist)
				fDist = fTmp;
		}
		if (bF2){
			CalcShortestDistance(&vecPos,&positions[0],&positions[2],
				NULL,NULL,&fTmp,&vecTmp,NULL);
			if (fTmp < fDist)
				fDist = fTmp;
		}
		if (fDist < FLT_MAX){
			fDist = (FLOAT)sqrt(fDist);
			if (fDist > fRadius)
				return false;
			*pDist = fDist;
			return true;
		}
	}
//TRIANGLE_HAS_NO_AREA:
	return false;
}

//-------------------------------------------------------------
//	Name: ProbeTheTriangleDistanceWithSegment2
//  Desc: 指定された線分とポリゴンとの当たり判定。
//		引数の座標系は、ワールド座標系に統一されて
//		いなければならない。
//	pTri[3]  三角ポリゴン
//	pvec1    線分の起点
//  pvec2    線分の終点
//	fErrorCapacity  ポリゴンエッジでの許容誤差（これが無いと、ポリゴンの継ぎ目で抜ける）
//  pVecOut   衝突点
//  pT        線分上の衝突点までの比率  vecOut = vec1 + (vec2 - vec1)*t
//-------------------------------------------------------------
BOOL	ProbeTheTriangleDistanceWithSegment2(XMVECTOR *positions, XMVECTOR *pvec1, XMVECTOR *pvec2, FLOAT fErrorCapacity, XMVECTOR *pVecOut, FLOAT *pT)
{
    //  平面を意味する方程式
    //  V = V1 + (V2-V1)u + (V3-V1)v　と
    //  直線の方程式
    //  V = Vp + Vr*t  より
    //  方程式　(V2-V1)u + (V3 - V1)v + (-Vr)t = Vp - V1
    //  の u, v, t について解を得る事で、交点を算出する。

    double D,D1,D2,D3;
    
    FLOAT    a,b,c,d;
    FLOAT    e,f,g,h;
    FLOAT    i,j,k,l;
	XMVECTOR	vr = *pvec2 - *pvec1;
#if 0
    a = XMVectorGetX(positions[1]) - XMVectorGetX(positions[0]);//pTri[1].p.x - pTri[0].p.x;
    b = XMVectorGetX(positions[2]) - XMVectorGetX(positions[0]);//pTri[2].p.x - pTri[0].p.x;
    c = -XMVectorGetX(vr);//-vr.x;
    d = XMVectorGetX(*pvec1) - XMVectorGetX(positions[0]);//pvec1->x - pTri[0].p.x;
							  
    e = XMVectorGetY(positions[1]) - XMVectorGetY(positions[0]);//pTri[1].p.y - pTri[0].p.y;
    f = XMVectorGetY(positions[2]) - XMVectorGetY(positions[0]);//pTri[2].p.y - pTri[0].p.y;
    g = -XMVectorGetY(vr);//vr.y;
    h = XMVectorGetY(*pvec1) - XMVectorGetY(positions[0]);//pvec1->y - pTri[0].p.y;

    i = XMVectorGetZ(positions[1]) - XMVectorGetY(positions[0]);//pTri[1].p.z - pTri[0].p.z;
    j = XMVectorGetZ(positions[2]) - XMVectorGetY(positions[0]);//pTri[2].p.z - pTri[0].p.z;
    k = -XMVectorGetZ(vr);//-vr.z;
    l = XMVectorGetZ(*pvec1) - XMVectorGetZ(positions[0]);//pvec1->z - pTri[0].p.z;
#else
	XMVECTOR vTmp;

	vTmp = positions[1] - positions[0];
	a = XMVectorGetX(vTmp);
	e = XMVectorGetY(vTmp);
	i = XMVectorGetZ(vTmp);

	vTmp = positions[2] - positions[0];
	b = XMVectorGetX(vTmp);
	f = XMVectorGetY(vTmp);
	j = XMVectorGetZ(vTmp);

	c = -XMVectorGetX(vr);
	g = -XMVectorGetY(vr);
	k = -XMVectorGetZ(vr);

	vTmp = *pvec1 - positions[0];
	d = XMVectorGetX(vTmp);
	h = XMVectorGetY(vTmp);
	l = XMVectorGetZ(vTmp);

#endif

	D = a*f*k + e*j*c + b*g*i - c*f*i - b*e*k - a*j*g;
    if (fabs(D) < FLT_MIN){
        //  if ther is no answer
		*pT = 0;
		*pVecOut = *pvec1;
		return    false;
    }

	D1 = d*f*k + h*j*c + b*g*l - c*f*l - b*h*k - d*j*g;
	D2 = a*h*k + e*l*c + d*g*i - c*h*i - d*e*k - a*l*g;
	D3 = a*f*l + e*j*d + b*h*i - d*f*i - b*e*l - a*j*h;

	D = 1.0f/D;

	FLOAT	u,v,t;
	u = (FLOAT)(D1 * D);
    v = (FLOAT)(D2 * D);
	t = (FLOAT)(D3 * D);

	XMVECTOR	vecOut= XMVectorSet(a*u+b*v,e*u+f*v,i*u+j*v,0);	//XMLoadFloat3(&XMFLOAT3(a*u+b*v,e*u+f*v,i*u+j*v));
	vecOut += positions[0];//pTri[0].p;

	*pT = t;
	*pVecOut = vecOut;
	if ( t < 0 || t > 1.0f)
		return	false;
	if ( u >= 0 && v >= 0 && (u+v) <= 1.0f){
		return	true;
	}
	if ( u < 0 && v < 0){
		if (XMVectorGetX(XMVector3LengthSq((vecOut - positions[0]))) < fErrorCapacity){
			return	true;
		}
	}else if ( u > 1.0f && v < 0.0f){
		if (XMVectorGetX(XMVector3LengthSq((vecOut - positions[1]))) < fErrorCapacity){
			return	true;
		}
	}else if ( u < 0.0f && v > 1.0f){
		if (XMVectorGetX(XMVector3LengthSq((vecOut - positions[2]))) < fErrorCapacity){
			return	true;
		}
	}else if ( u >= 0.0f && v < 0.0f){

		XMVECTOR	vecTmp = positions[1] - positions[0];//pTri[1].p - pTri[0].p;
		vecOut -= positions[0];//pTri[0].p;
		//	幾何的に見えて、実は微分式で答えを出している
		FLOAT	n = XMVectorGetX(XMVector3Dot(vecTmp, vecOut));
		n /= XMVectorGetX(XMVector3LengthSq(vecTmp));
		if (XMVectorGetX(XMVector3LengthSq((vecOut - n*vecTmp))) < fErrorCapacity)
			return true;

	}else if ( u < 0.0f && v >= 0.0f){

		XMVECTOR	vecTmp = positions[2] - positions[0];//pTri[2].p - pTri[0].p;
		vecOut -= positions[0];//pTri[0].p;
		//	幾何的に見えて、実は微分式で答えを出している
		FLOAT	n = XMVectorGetX(XMVector3Dot(vecTmp, vecOut));
		n /= XMVectorGetX(XMVector3LengthSq(vecTmp));
		if (XMVectorGetX(XMVector3LengthSq((vecOut - n*vecTmp))) < fErrorCapacity)
			return true;

	}else{

		XMVECTOR	vecTmp = positions[2] - positions[1];//pTri[2].p - pTri[1].p;
		vecOut -= positions[1];//pTri[1].p;

		//  Get the nearest position with differential equation.
		//  not from the geometric function.
		//	幾何的に見えて、実は微分式で答えを出している
		FLOAT	n = XMVectorGetX(XMVector3Dot(vecTmp, vecOut));
		n /= XMVectorGetX(XMVector3LengthSq(vecTmp));
		if (XMVectorGetX(XMVector3LengthSq((vecOut - n*vecTmp))) < fErrorCapacity)
			return true;

	}
	return false;
}

//  Check collision with a segment
//
BOOL	CMeshCollider::CheckCollisionWithSegment(XMFLOAT3 *pVec1, XMFLOAT3 *pVec2, FLOAT fRadius){
	XMFLOAT3	vecMin, vecMax;
	XMFLOAT3	vecBoxMin, vecBoxMax;
	BOOL	bRet = false;
	XMVECTOR vecPos1 = XMLoadFloat3(pVec1);
	XMVECTOR vecPos2 = XMLoadFloat3(pVec2);
	FLOAT fDist = FLT_MAX;
	vecBoxMin.x = min(pVec1->x,pVec2->x) - fRadius;
	vecBoxMin.y = min(pVec1->y,pVec2->y) - fRadius;
	vecBoxMin.z = min(pVec1->z,pVec2->z) - fRadius;
	vecBoxMax.x = max(pVec1->x,pVec2->x) + fRadius;
	vecBoxMax.y = max(pVec1->y,pVec2->y) + fRadius;
	vecBoxMax.z = max(pVec1->z,pVec2->z) + fRadius;

	GetBoundingAABBFloat3(&vecMin,&vecMax);

	if (!Collide2Box(&vecMin,&vecMax,&vecBoxMin,&vecBoxMax))
		return false;

	//	iterate each polygon on the mesh.
	for (int i = 0; i < (int)m_dwNumFrames ; ++i){
		int index = 0, k;
		COLLISIONVERTEX *pVertices;
		XMVECTOR positions[3], normals[3];	//  the pick-upped triangle
		MeshFrame *pFrame = m_ppMeshFrames[i];
		XMMATRIX matWorld;

		if (pFrame == NULL)
			continue;
		MeshContainer *pContainer = pFrame->pMeshContainer;
		if (pContainer == NULL)
			continue;
		Mesh *pMesh = pContainer->pMeshData;
		if (pMesh == NULL)
			continue;

		//  Prepare world matrix
		matWorld = m_matWorld * pFrame->CombinedMatrix;

		index = 0;
		pVertices = (COLLISIONVERTEX*)pMesh->pVertices;
		int iNumIndices = pMesh->numIndices * 3;
		while(index < iNumIndices){
			k = pMesh->pIndices[index++];
			positions[0] = XMLoadFloat3(&pVertices[k].position);
			normals[0]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[1] = XMLoadFloat3(&pVertices[k].position);
			normals[1]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[2] = XMLoadFloat3(&pVertices[k].position);
			normals[2]   = XMLoadFloat3(&pVertices[k].normal);

			//  transform'em into world coordinate system
			for (int i = 0; i < 3 ; ++i){
				positions[i] = XMVector3TransformCoord(positions[i],matWorld);
				normals[i]   = XMVector3TransformNormal(normals[i], matWorld);
			}
			CalcTriangleBoundingBox(positions,&vecMin,&vecMax);
			if (!Collide2Box(&vecMin,&vecMax,&vecBoxMin,&vecBoxMax))
				continue;

			//  Check the collision with triangle
			if (ProbeTheTriangleDistanceWithSegment(positions, &fDist,&vecPos1,&vecPos2,fRadius)){
				bRet = true;
				break;
			}
		}
	}

	return bRet;
}
BOOL	CMeshCollider::CheckCollisionWithSegment(XMFLOAT3 *pVec1, XMFLOAT3 *pVec2, FLOAT fErrorCapacity, XMFLOAT3 *pVecOut){
	XMFLOAT3	vecMin, vecMax;
	XMFLOAT3	vecBoxMin, vecBoxMax;
	BOOL	bRet = false;
	XMVECTOR vecPos1 = XMLoadFloat3(pVec1);
	XMVECTOR vecPos2 = XMLoadFloat3(pVec2);
	XMVECTOR vecOut;
	FLOAT fT, fTmp;

	vecBoxMin.x = min(pVec1->x,pVec2->x) - fErrorCapacity;
	vecBoxMin.y = min(pVec1->y,pVec2->y) - fErrorCapacity;
	vecBoxMin.z = min(pVec1->z,pVec2->z) - fErrorCapacity;
	vecBoxMax.x = max(pVec1->x,pVec2->x) + fErrorCapacity;
	vecBoxMax.y = max(pVec1->y,pVec2->y) + fErrorCapacity;
	vecBoxMax.z = max(pVec1->z,pVec2->z) + fErrorCapacity;

	GetBoundingAABBFloat3(&vecMin,&vecMax);

	if (!Collide2Box(&vecMin,&vecMax,&vecBoxMin,&vecBoxMax))
		return false;

	//  square the error capacity
	fErrorCapacity *= fErrorCapacity;
	fT = FLT_MAX;
	//	iterate each polygon on the mesh.
	for (int i = 0; i < (int)m_dwNumFrames ; ++i){
		int index = 0, k;
		COLLISIONVERTEX *pVertices;
		XMVECTOR positions[3], normals[3];	//  the pick-upped triangle
		MeshFrame *pFrame = m_ppMeshFrames[i];
		XMMATRIX matWorld;

		if (pFrame == NULL)
			continue;
		MeshContainer *pContainer = pFrame->pMeshContainer;
		if (pContainer == NULL)
			continue;
		Mesh *pMesh = pContainer->pMeshData;
		if (pMesh == NULL)
			continue;

		//  Prepare world matrix
		matWorld = m_matWorld * pFrame->CombinedMatrix;

		index = 0;
		pVertices = (COLLISIONVERTEX*)pMesh->pVertices;
		int iNumIndices = pMesh->numIndices * 3;
		while(index < iNumIndices){
			k = pMesh->pIndices[index++];
			positions[0] = XMLoadFloat3(&pVertices[k].position);
			normals[0]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[1] = XMLoadFloat3(&pVertices[k].position);
			normals[1]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[2] = XMLoadFloat3(&pVertices[k].position);
			normals[2]   = XMLoadFloat3(&pVertices[k].normal);

			//  transform'em into world coordinate system
			for (int i = 0; i < 3 ; ++i){
				positions[i] = XMVector3TransformCoord(positions[i],matWorld);
				normals[i]   = XMVector3TransformNormal(normals[i], matWorld);
			}
			CalcTriangleBoundingBox(positions,&vecMin,&vecMax);
			if (!Collide2Box(&vecMin,&vecMax,&vecBoxMin,&vecBoxMax))
				continue;

			//  Check the collision with triangle
			if (ProbeTheTriangleDistanceWithSegment2(positions, &vecPos1,&vecPos2,
				fErrorCapacity, &vecOut,&fTmp)){
				if (fTmp < fT){
					pVecOut->x = XMVectorGetX(vecOut);
					pVecOut->y = XMVectorGetY(vecOut);
					pVecOut->z = XMVectorGetZ(vecOut);
					fT = fTmp;
				}
				bRet = true;
			}
		}
	}

	return bRet;
}



//-------------------------------------------------------------
//	Name: ProbeTheWallSinkDepthBase
//  @param 
//		pPosition : the position of the probe
//		fRadius   : the radius of the probe
//      pVecNormal: (in) the motion of the probe (out)the direction to go back
//		pDepth    : (out) the depth of the probe sank into the terrain
//  return : false : no collision / true : hit
//  
//-------------------------------------------------------------
BOOL CMeshCollider::ProbeTheWallSinkDepthBase(
	XMFLOAT3 *pPosition,FLOAT fRadius,XMFLOAT3 *pVecNormal,FLOAT *pDepth)
{
	XMVECTOR	vecNormal, vecPosition;
	XMFLOAT3	vecBoxMin, vecBoxMax;
	XMFLOAT3	vecMin, vecMax;
	FLOAT	    fMoveSynth = 0;
	int	        iNumCollisionInfo = 0;
	BOOL bRet = false;

	std::vector<COLLISIONINFO*> collisioninfos;
	COLLISIONINFO* pCollision;
	MeshFrame *pRoot;
	pRoot = m_smrRenderer.GetRootFrame();
	if (pRoot == NULL)
		return	false;

	vecPosition = XMLoadFloat3(pPosition);

	vecBoxMin = XMFLOAT3(pPosition->x - fRadius,pPosition->y - fRadius,pPosition->z - fRadius);
	vecBoxMax = XMFLOAT3(pPosition->x + fRadius,pPosition->y + fRadius,pPosition->z + fRadius);

	GetBoundingAABBFloat3(&vecMin, &vecMax);

	//	At first check the trrain's boundary box
	if (!Collide2Box(&vecMin,&vecMax,&vecBoxMin,&vecBoxMax)){
		return	false;
	}

	//	iterate each polygon on the mesh.
	for (int i = 0; i < (int)m_dwNumFrames ; ++i){
		int index = 0, k;
		COLLISIONVERTEX *pVertices;
		XMVECTOR positions[3], normals[3];	//  the pick-upped triangle
		MeshFrame *pFrame = m_ppMeshFrames[i];
		XMMATRIX matWorld;

		if (pFrame == NULL)
			continue;
		MeshContainer *pContainer = pFrame->pMeshContainer;
		if (pContainer == NULL)
			continue;
		Mesh *pMesh = pContainer->pMeshData;
		if (pMesh == NULL)
			continue;

		//  Prepare world matrix
		matWorld = m_matWorld * pFrame->CombinedMatrix;

		index = 0;
		pVertices = (COLLISIONVERTEX*)pMesh->pVertices;
		int iNumIndices = pMesh->numIndices * 3;
		while(index < iNumIndices){
			k = pMesh->pIndices[index++];
			positions[0] = XMLoadFloat3(&pVertices[k].position);
			normals[0]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[1] = XMLoadFloat3(&pVertices[k].position);
			normals[1]   = XMLoadFloat3(&pVertices[k].normal);
			k = pMesh->pIndices[index++];
			positions[2] = XMLoadFloat3(&pVertices[k].position);
			normals[2]   = XMLoadFloat3(&pVertices[k].normal);

			//  transform'em into world coordinate system
			for (int i = 0; i < 3 ; ++i){
				positions[i] = XMVector3TransformCoord(positions[i],matWorld);
				normals[i]   = XMVector3TransformNormal(normals[i], matWorld);
			}
			CalcTriangleBoundingBox(positions,&vecMin,&vecMax);
			if (Collide2Box(&vecMin,&vecMax,&vecBoxMin,&vecBoxMax)){
				FLOAT fDist;
				XMVECTOR vecNormalTmp, vec4Tmp, vecTmp;
				if (ProbeTheTriangleDistance(positions,normals,&fDist,
					&vecPosition,&vecBoxMin,&vecBoxMax,fRadius,&vecNormalTmp,&vec4Tmp)){

					vecTmp = vec4Tmp - vecPosition;
					if (XMVectorGetX(XMVector3Dot(vecNormalTmp, vecTmp)) < FLT_MIN){
						pCollision = new COLLISIONINFO;
						pCollision->n = vecNormalTmp;
						pCollision->amount = fRadius - fDist;
						pCollision->nearest = vec4Tmp;
						for (int i = 0; i < 3 ; ++i){
							pCollision->normals[i] = normals[i];
							pCollision->positions[i] = positions[i];
						}
						collisioninfos.push_back(pCollision);
						pCollision = NULL;
					}
				}
			}
		}
	}
	if (collisioninfos.size() == 0)
		return false;
	if (collisioninfos.size() == 1){
		pCollision = collisioninfos[0];
		vecNormal = pCollision->n;
		pVecNormal->x = XMVectorGetX(vecNormal);
		pVecNormal->y = XMVectorGetY(vecNormal);
		pVecNormal->z = XMVectorGetZ(vecNormal);
		*pDepth = pCollision->amount;
		collisioninfos[0] = NULL;
		delete pCollision;
		return true;
	}

	//  if the probe hits more than 2 polygons.
	{
		XMVECTOR vecMotion = XMLoadFloat3(pVecNormal);
		XMVECTOR vecPosition = XMLoadFloat3(pPosition);
		FLOAT l = XMVectorGetX(XMVector3Length(vecMotion));
		if (!CalcGoBackDir(&collisioninfos,&vecPosition,&vecMotion,&vecNormal,fRadius)){

			//  if it is impossible to calculate 
			//  just go back to the ordinal position.
			vecNormal = (-vecMotion) * (1.0f/l);
			pVecNormal->x = XMVectorGetX(vecNormal);;
			pVecNormal->y = XMVectorGetY(vecNormal);;
			pVecNormal->z = XMVectorGetZ(vecNormal);;
		}
		if (l > FLT_MIN && !AdjustDepth(&collisioninfos,&vecPosition, fRadius, &vecNormal, &fMoveSynth)){
			//  just go back to previous position
			vecNormal = (-vecMotion) * (1.0f/l);
			pVecNormal->x = XMVectorGetX(vecNormal);;
			pVecNormal->y = XMVectorGetY(vecNormal);;
			pVecNormal->z = XMVectorGetZ(vecNormal);;
			*pDepth = l;
			bRet = true;
			goto EXIT_COLLISION;
		}
		*pDepth = fMoveSynth;
		pVecNormal->x = XMVectorGetX(vecNormal);;
		pVecNormal->y = XMVectorGetY(vecNormal);;
		pVecNormal->z = XMVectorGetZ(vecNormal);;
		bRet = true;
		if (fabs(fMoveSynth) < FLT_MIN)
			bRet = false;
	}
EXIT_COLLISION:
	for (int i = 0; i < (int)collisioninfos.size() ; ++i){
		SAFE_DELETE(collisioninfos[i]);
	}
	return bRet;
#if 0
	//	インデックスバッファの形式を確認。
	LPDIRECT3DINDEXBUFFER9	pIB = NULL;
	GetIndexBuffer(&pIB);
	hr = pIB->GetDesc(&ixDesc);
	SAFE_RELEASE(pIB);

	vecNormal = D3DXVECTOR3(0,0,0);

	if (ixDesc.Format == D3DFMT_INDEX16){
		iNumCollisionInfo = CreateCollisionInfo16(pVec,fRadius);
	}else if (ixDesc.Format != D3DFMT_INDEX32){
		return	false;
	}

	//	ヒット判定
	if (iNumCollisionInfo <= 0)
		return	false;

	if (iNumCollisionInfo == 1){
		*pVecNormal = m_pCollisionInfo->n;
		*pDepth = m_pCollisionInfo->amount;
		return	true;
	}

	//	複数のポリゴンにヒットする時

	//	戻す方向を算出
	if (!CalcGoBackDir(pVec, pVecNormal,&vecNormal,fRadius)){
		//	計算できないときは、移動前に戻す
		vecNormal = -*pVecNormal;
		*pVecNormal = vecNormal/l;
		//*pDepth = l;
		//return	true;
	}

	//	壁に対して戻す量の最終調整
	if (l > FLT_MIN && !AdjustDepth(pVec, fRadius, &vecNormal,&fMoveSynth)){
		//	計算できないときは、移動前に戻す
		vecNormal = -*pVecNormal;
		*pVecNormal = vecNormal/l;
		*pDepth = l;
		return	true;
	}

	*pDepth = fMoveSynth;
	*pVecNormal = vecNormal;
	if (fabs(fMoveSynth) < FLT_MIN)
		return	false;
	return	true;
#endif
}

BOOL	ProbeTheTriangleDistance(
	XMVECTOR *positions,XMVECTOR *normals, 
	FLOAT *pDist, XMVECTOR *pProbePos, 
	XMFLOAT3 *pBoxMin, XMFLOAT3 *pBoxMax, 
	FLOAT fRadius, XMVECTOR *pNormal,XMVECTOR *pNearestPos)
{
	XMVECTOR	vecTmp, vecPos;
	FLOAT		len1, len2, len3;
	FLOAT		fDist,fTmp;
	BOOL	bF0=false, bF1=false, bF2=false;
	XMVECTOR	vecNormal, vecDir1, vecDir2, vecNearest;

	vecPos = *pProbePos;

	//	Calculate the length of polygon's edge.
	len1 = XMVectorGetX(XMVector3LengthSq(positions[1] - positions[0]));
	len2 = XMVectorGetX(XMVector3LengthSq(positions[2] - positions[1]));
	len3 = XMVectorGetX(XMVector3LengthSq(positions[0] - positions[2]));

	//  do evaluate if the polygon is large enough.
	if ((fabs(len1) >= FLT_MIN)&&(fabs(len2) >= FLT_MIN)&&(fabs(len3) >= FLT_MIN)){

		//	calculate the face normal
		vecDir1 = positions[1] - positions[0];
		vecDir2 = positions[2] - positions[0];
		vecNormal = XMVector3Cross(vecDir1,vecDir2);

		//	面法線が算出できないは、最短距離点はポリゴンの辺上
		if (XMVectorGetX(XMVector3LengthSq(vecNormal)) <= FLT_MIN)
			goto	TRIANGLE_HAS_NO_AREA;

		//	面法線を正規化
		vecNormal = XMVector3Normalize(vecNormal);

		vecDir1 = vecPos - positions[0];
		vecDir2 = vecNormal;
		fDist = XMVectorGetX(XMVector3Dot(vecDir1,vecDir2));
		if (fDist < -FLT_MIN)
			return	false;
		if (fDist > fRadius)
			return	false;

		//	平面上の最近点を算出
		*pNearestPos = vecNearest = vecPos - (vecNormal * fDist);

		vecDir1 = vecNearest - positions[0];
		vecDir2 = positions[1] - positions[0];
		vecTmp = XMVector3Cross(vecDir1, vecDir2);
		fTmp = XMVectorGetX(XMVector3Dot(vecTmp,vecNormal));
		if (fTmp > -FLT_MIN){
			bF0=TRUE;
		}

		vecDir1 = vecNearest - positions[1];
		vecDir2 = positions[2] - positions[1];
		vecTmp = XMVector3Cross(vecDir1, vecDir2);
		fTmp = XMVectorGetX(XMVector3Dot(vecTmp,vecNormal));
		if (fTmp > -FLT_MIN){
			bF1=TRUE;
		}

		vecDir1 = vecNearest - positions[2];
		vecDir2 = positions[0] - positions[2];
		vecTmp = XMVector3Cross(vecDir1, vecDir2);
		fTmp = XMVectorGetX(XMVector3Dot(vecTmp,vecNormal));
		if (fTmp > -FLT_MIN){
			bF2=TRUE;
		}

		if (!bF0 && !bF1 && !bF2){
			*pDist = fDist;
			*pNormal = vecNormal;
			return	TRUE;
		}
		fDist = FLT_MAX;
		if (bF0){
			CalcShortestDistance(&vecPos,&positions[1], &positions[0], &normals[1], &normals[0], &fTmp,&vecTmp,pNormal);
			if (fTmp < fDist){
				fDist = fTmp;
				*pNearestPos = vecTmp;
			}
		}
		if (bF1){
			CalcShortestDistance(&vecPos,&positions[2], &positions[1], &normals[2], &normals[1], &fTmp,&vecTmp,pNormal);
			if (fTmp < fDist){
				fDist = fTmp;
				*pNearestPos = vecTmp;
			}
		}
		if (bF2){
			CalcShortestDistance(&vecPos,&positions[0], &positions[2], &normals[0], &normals[2], &fTmp,&vecTmp,pNormal);
			if (fTmp < fDist){
				fDist = fTmp;
				*pNearestPos = vecTmp;
			}
		}
		if (fDist < FLT_MAX){
			fDist = (FLOAT)sqrt(fDist);
			if (fDist > fRadius)
				return	FALSE;
			*pNormal = vecNormal;
			*pDist = fDist;
			return	TRUE;
		}
	}
TRIANGLE_HAS_NO_AREA:
	//	線分ポリゴンもここに来る
	FLOAT	fDistTmp, fDistMin;
	XMVECTOR	vecNormalTmp;
	CalcShortestDistance(&vecPos,&positions[1], &positions[0], &normals[1], &normals[0], &fDistMin,pNormal,pNearestPos);
	CalcShortestDistance(&vecPos,&positions[2], &positions[1], &normals[2], &normals[1], &fDistTmp,&vecNormalTmp,&vecTmp);
	if (fDistTmp < fDistMin){
		*pNormal = vecNormalTmp;
		*pNearestPos = vecTmp;
		fDistMin = fDistTmp;
	}
	CalcShortestDistance(&vecPos,&positions[0], &positions[2], &normals[0], &normals[2], &fDistTmp,&vecNormalTmp,&vecTmp);
	if (fDistTmp < fDistMin){
		*pNormal = vecNormalTmp;
		*pNearestPos = vecTmp;
		fDistMin = fDistTmp;
	}

	fDist = sqrt(fDistMin);
	if (fDist > fRadius)
		return	false;
	*pDist = fDist;
	*pNormal = XMVector3Normalize(*pNormal);
	return	true;
}

//  calc shortest distance from a position to the place on the segment
static void	CalcShortestDistance(
	XMVECTOR *pVecPos, 
	XMVECTOR *pVecP1, XMVECTOR *pVecP2, 
	XMVECTOR *pVecN1, XMVECTOR *pVecN2, 
	float *pDist, XMVECTOR *pPosOut, XMVECTOR *pNormal
){
	float	t;
	//double	dx1, dy1, dz1, dx2, dy2, dz2;
	float	len, len1, len2;	
	//double	px, py, pz;
	XMVECTOR vecD1 = *pVecP2  - *pVecP1;
	XMVECTOR vecD2 = *pVecPos - *pVecP1;

	len = XMVectorGetX(XMVector3LengthSq(vecD1));
	if (len < FLT_MIN){
		*pDist = XMVectorGetX(XMVector3LengthSq(vecD2));
		*pPosOut = *pVecP1;
		if (pNormal)
			*pNormal = XMVector3Normalize(*pVecN1);
	}

	//  find the nearest position on the segment from pVecPos  
	t = XMVectorGetX(XMVector3Dot(vecD1,vecD2));
	t /= len;
	if ((t >= 0.0f) && (t <= 1.0f)){

		//	if the nearest position is on the segment
		XMVECTOR posResult;

		//	calculate the exact position
		posResult = (*pVecP1) + (vecD1*(FLOAT)t);
		*pPosOut = posResult;

		//	calculate the distance
		len = XMVectorGetX(XMVector3LengthSq(posResult - *pVecPos));		
		*pDist = (FLOAT)len;
		if (pNormal)
			*pNormal = XMVector3Normalize(
				(*pVecN2) * t + (*pVecN1) * (1.0f-t));
		return;
	}else{
		//	いずれかの頂点が最小距離である場合。
		len1 = XMVectorGetX(XMVector3LengthSq(*pVecP1-*pVecPos));
		len2 = XMVectorGetX(XMVector3LengthSq(*pVecP2-*pVecPos));
		if ( len2 < len1 ){
			//	P2までの距離と、P2 の高度を返す
			*pDist = (float)len2;
			*pPosOut = *pVecP2;
			if (pNormal)
				*pNormal = XMVector3Normalize(*pVecN2);
			return;
		}
		//	P1 までの距離と、P1 の高度を返す
		*pDist = (float)len1;
		*pPosOut = *pVecP1;
		if (pNormal)
			*pNormal = XMVector3Normalize(*pVecN1);
		return;
	}
}


//  Inclusion of CSimpleMeshRenderer

BOOL	CMeshCollider::IsPrepared(){
	return m_smrRenderer.IsPrepared();
}
void	CMeshCollider::Render(ID3D11DeviceContext *pContext){
	m_smrRenderer.Render(pContext);
}
void	CMeshCollider::SetWorldMatrix(DirectX::XMMATRIX *pMatWorld){
	m_matWorld = *pMatWorld;
	m_smrRenderer.SetWorldMatrix(pMatWorld);
}
void	CMeshCollider::SetViewMatrix(DirectX::XMMATRIX  *pMatView){
	m_smrRenderer.SetViewMatrix(pMatView);
}
void	CMeshCollider::SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection){
	m_smrRenderer.SetProjectionMatrix(pMatProjection);
}

void	CMeshCollider::SetLightDir(DirectX::XMFLOAT3 *pVecDir){
	m_smrRenderer.SetLightDir(pVecDir);
}
void	CMeshCollider::SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse){
	m_smrRenderer.SetLightDiffuse(pVecDiffuse);
}
void	CMeshCollider::SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient){
	m_smrRenderer.SetLightAmbient(pVecAmbient);
}

MeshFrame	*CMeshCollider::GetRootFrame(){
	return	m_smrRenderer.GetRootFrame();
}

HRESULT CMeshCollider::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
	return m_smrRenderer.RestoreDeviceObjects(pContext);
}

HRESULT CMeshCollider::ReleaseDeviceObjects(){
	return m_smrRenderer.ReleaseDeviceObjects();
}
