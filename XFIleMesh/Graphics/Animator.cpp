//
//	class CAnimator
//
//	@Brief : plays animation data
//
#include "stdafx.h"

#include <list>
#include <vector>
#include <D3D11.h>
#include <DirectXMath.h>
#include "Mesh.h"
#include "Animation.h"
#include "Animator.h"

#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }

//
//	local types
//



__declspec(align(16)) struct BoneParameters{
	Animation	*pAnimation;
	double		dblParamFrame;
	INT			iFrameId;
	DirectX::XMVECTOR	quatRotation;
	DirectX::XMVECTOR	vecPosition;
	DirectX::XMVECTOR	vecScaling;
	BoneParameters(){
		pAnimation = NULL;
		dblParamFrame = 0.0;
		iFrameId = -1;
		quatRotation = DirectX::XMQuaternionIdentity();
		vecPosition  = DirectX::XMVectorSet(0.f,0.f,0.f,0.f);
		vecScaling   = DirectX::XMVectorSet(1.f,1.f,1.f,1.f);
	}
	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	inline void *operator new[](size_t size){
		return _mm_malloc(size,16);
	}
	inline void operator delete(void *p){
		return _mm_free(p);
	}
};

struct AnimationTrack
{
	double	dblTrackTime;	//	current time
	double	dblTrackFrame;	//	frame no.
	double	dblSpeed;		//	animation speed
	double  dblWeight;      //  weight 0-1.0
	AnimationSet	*pAnimationSet;
	INT numActiveBoneParameters;
	INT numTotalBoneParameters;
	BOOL	bActive;
	BOOL	bLoop;
	std::list<BoneParameters*>	*ppActiveBoneParameters;
	std::list<BoneParameters*>	*ppInactiveBoneParameters;
	AnimationTrack(){
		dblTrackTime = 0.0;
		dblTrackFrame = 0.0;
		dblSpeed = 1.0;
		dblWeight = 0.0;
		bActive = false;
		bLoop = true;
		pAnimationSet = NULL;
		numActiveBoneParameters = 0;
		numTotalBoneParameters  = 0;
		ppActiveBoneParameters = new std::list<BoneParameters *>();
		ppInactiveBoneParameters = new std::list<BoneParameters *>();
	}
	~AnimationTrack(){
		if (ppActiveBoneParameters != NULL){
			std::list<BoneParameters*>::iterator it = ppActiveBoneParameters->begin();
			while(it != ppActiveBoneParameters->end()){
				if (*it != NULL){
					(*it)->pAnimation = NULL;
					delete (*it);
					*it = NULL;
					it = ppActiveBoneParameters->erase(it);
				}
			}
			delete ppActiveBoneParameters;
			ppActiveBoneParameters = NULL;
		}
		if (ppInactiveBoneParameters != NULL){
			std::list<BoneParameters*>::iterator it = ppInactiveBoneParameters->begin();
			while(it != ppInactiveBoneParameters->end()){
				if (*it != NULL){
					(*it)->pAnimation = NULL;
					delete (*it);
					*it = NULL;
					it = ppInactiveBoneParameters->erase(it);
				}
			}
			delete ppInactiveBoneParameters;
			ppInactiveBoneParameters = NULL;
		}
	}
};

#define	NUM_ACTIVE_TRACK	2

struct AnimationMixerTrack{
	INT		iFrameId;
	INT		iCount;
	BoneParameters	*ppBoneParameters[NUM_ACTIVE_TRACK];
	double	dblWeight[NUM_ACTIVE_TRACK];
};

struct AnimationContext{
	AnimationTrack *pActiveTrack[NUM_ACTIVE_TRACK];
	double	dblAnimationWeight[NUM_ACTIVE_TRACK];
	INT numAnimationSets;
	std::vector<AnimationSet*>	*ppAnimationSets;
	MeshFrame	*pFrameRoot;
	INT			numFramesInMeshHierarchy;
	INT			lastFrameLength;
	AnimationMixerTrack	*pFrameMotions;
	AnimationContext(){
		pActiveTrack[0] = new AnimationTrack();
		pActiveTrack[1] = new AnimationTrack();
		numAnimationSets = 0;
		ppAnimationSets = new std::vector<AnimationSet *>();
		pFrameRoot = NULL;
		pFrameMotions = NULL;
		lastFrameLength = 0;
	}
	~AnimationContext(){
		for (int i = 0; i < _countof(pActiveTrack) ; ++i){
			if (pActiveTrack[i] != NULL){
				pActiveTrack[i]->pAnimationSet = NULL;
				delete pActiveTrack[i];
				pActiveTrack[i] = NULL;
			}
		}
		if (ppAnimationSets != NULL){
			std::vector<AnimationSet*>::iterator it = ppAnimationSets->begin();
			while(it != ppAnimationSets->end() ){
				*it = NULL;
				++it;
			}
			delete ppAnimationSets;
			ppAnimationSets = NULL;
		}
		pFrameRoot = NULL;
		SAFE_DELETE_ARRAY(pFrameMotions);
	}
};




//
//	ctor for CAnimator
//
CAnimator::CAnimator(MeshFrame *pRoot)
{
	pAnimationContext = NULL;
	pAnimationContext = new AnimationContext;
	pAnimationContext->pFrameRoot = pRoot;
	INT numFrames = CountFramesInHierarchy(pRoot);
	pAnimationContext->numFramesInMeshHierarchy = numFrames;
	if (numFrames > 0){
		AnimationMixerTrack *pFrameMotions = new AnimationMixerTrack[numFrames];
		for (int i = 0; i < numFrames ; ++i){
			pFrameMotions[i].iFrameId = i;
			pFrameMotions[i].iCount = 0;
			for (int j = 0;  j < NUM_ACTIVE_TRACK ; ++j){
				pFrameMotions[i].ppBoneParameters[j] = NULL;
				pFrameMotions[i].dblWeight[j] = 0.0;
			}
		}
		pAnimationContext->pFrameMotions = pFrameMotions;
	}
}


CAnimator::~CAnimator(void)
{
	SAFE_DELETE(pAnimationContext);
}

//
//  prototype
//
static void AdjustTrackAnimation(AnimationContext *pAnimationContext, INT trackNo);

//
//	SetTrackWeight
//	@param
//		trackNo : track no.
//		weight  : track weight	(0.0 - 1.0)
//	@return
//		none
//
void CAnimator::SetTrackWeight(INT trackNo, DOUBLE weight){
	if (trackNo < 0 || trackNo >= NUM_ACTIVE_TRACK)
		return;
	if (pAnimationContext->pActiveTrack == NULL)
		return;
	AnimationTrack *pTrack = pAnimationContext->pActiveTrack[trackNo];
	if (pTrack == NULL)
		return;
	pTrack->dblWeight = weight;
}


//
//	SetTrackTime
//	@param:
//		trackNo: track no.
//		time   : time
//  @return
//      current time (restricted 0 -- duration).
//
DOUBLE	CAnimator::SetTrackTime(INT trackNo, DOUBLE time){
	if (trackNo < 0 || trackNo >= NUM_ACTIVE_TRACK)
		return 0.0;
	if (pAnimationContext->pActiveTrack == NULL)
		return 0.0;
	AnimationTrack *pTrack = pAnimationContext->pActiveTrack[trackNo];
	if (pTrack == NULL)
		return 0.0;
	if (!pTrack->bActive)
		return	0.0f;
	AnimationSet *pAnimationSet = pAnimationContext->pActiveTrack[trackNo]->pAnimationSet;
	if (pAnimationSet == NULL)
		return 0.0;

	//	convert frames to duration;
	//  consider the duration of the last frame.
	double duration = ((double)(pAnimationSet->totalFrames+pAnimationContext->lastFrameLength)) / pTrack->dblSpeed;

	double tmptime;
	if (pTrack->bLoop){
		
		tmptime = fabs(time);
		INT denom = (int)(tmptime / duration);

		tmptime = tmptime - duration * denom;
		if (time < 0){
			tmptime = -tmptime;
			tmptime += duration;
		}
	}else{
		tmptime = min( max(0,time), duration );
	}
	pTrack->dblTrackTime = tmptime;
	pTrack->dblTrackFrame = tmptime * pTrack->dblSpeed;
	AdjustTrackAnimation(pAnimationContext, trackNo);
	return tmptime;
}

//
//	GetTrackDuration
//	@param:
//		trackNo   : track no (currently 0 or 1)
//		pDuration : pointer to store duration.
//  @return
//		true  : ok
//		false : ng
//
BOOL	CAnimator::GetTrackDuration(INT trackNo, DOUBLE *pDuration){

	if (trackNo < 0 || trackNo >= NUM_ACTIVE_TRACK)
		return false;
	
	if (pAnimationContext->pActiveTrack == NULL)
		return false;
	AnimationTrack *pTrack = pAnimationContext->pActiveTrack[trackNo];
	if (pTrack == NULL)
		return false;
	if (!pTrack->bActive)
		return false;
	AnimationSet *pAnimationSet = pAnimationContext->pActiveTrack[trackNo]->pAnimationSet;
	if (pAnimationSet == NULL)
		return false;

	//	convert frames to duration;
	//  consider the duration of the last frame.
	double duration = ((double)(pAnimationSet->totalFrames+pAnimationContext->lastFrameLength)) / pTrack->dblSpeed;

	if (pDuration != NULL)
		*pDuration = duration;

	return true;
}

//
//	method GetNumAnimations
//	@param:none
//	@return: num animations
//
INT	CAnimator::GetNumAnimations(){
	if (pAnimationContext == NULL)
		return 0;
	return  pAnimationContext->numAnimationSets;
}

//
//	method GetAnimationDuration
//	@param:
//		animNo   : animation no (0 to num of added animations)
//		pDuration : pointer to store duration.
//		speed    : playback speed (default=1.0)
//  @return
//		true  : ok
//		false : ng
//
BOOL	CAnimator::GetAnimationDuration(INT animNo, DOUBLE *pDuration, DOUBLE speed){
	if (animNo < 0 || animNo >= pAnimationContext->numAnimationSets)
		return false;

	AnimationSet *pAnimationSet = pAnimationContext->ppAnimationSets->at(animNo);
	if (pAnimationSet == NULL)
		return false;

	//	convert frames to duration;
	//  consider the duration of the last frame.
	double duration = ((double)(pAnimationSet->totalFrames+pAnimationContext->lastFrameLength)) / speed;

	if (pDuration != NULL)
		*pDuration = duration;

	return true;
}


//
//	method AddAnimationSets
//
HRESULT CAnimator::AddAnimationSets(AnimationSet **ppAnimationSet, INT num){
	HRESULT hr = E_FAIL;
	int i,j, numAnimations, id;
	AnimationSet *pTmpAnimationSet;
	Animation *pTmpAnimation;
	int maxAnimationId = -1;
	int numFrames;
	for (i = 0; i < num ; ++i){
		pTmpAnimationSet = ppAnimationSet[i];
		numAnimations = pTmpAnimationSet->numAnimations;
		for (j = 0; j < numAnimations ; ++j){
			pTmpAnimation = pTmpAnimationSet->ppAnimations[j];
			id = pTmpAnimation->pSlaveMeshFrame->iFrameId;
			if (id > maxAnimationId){
				maxAnimationId = id;
			}
		}
		pAnimationContext->ppAnimationSets->push_back(pTmpAnimationSet);
	}
	pAnimationContext->numAnimationSets = (INT)pAnimationContext->ppAnimationSets->size();
	numFrames = maxAnimationId + 1;
	for (i = 0; i < NUM_ACTIVE_TRACK ; ++i){
		if (pAnimationContext->pActiveTrack[i]->numTotalBoneParameters < numFrames){
			int addCount = numFrames - pAnimationContext->pActiveTrack[i]->numTotalBoneParameters;
			for (j = 0; j < addCount ; ++j){
				BoneParameters *pBoneParam = new BoneParameters;
				pAnimationContext->pActiveTrack[i]->ppInactiveBoneParameters->push_back(pBoneParam);
			}
			pAnimationContext->pActiveTrack[i]->numTotalBoneParameters += addCount;
		}
	}
	return hr;
}

void CAnimator::SetTrackAnimation(INT trackNo, INT animNo, DOUBLE weight, BOOL loop){
	if (trackNo >= NUM_ACTIVE_TRACK || trackNo < 0){
		return;
	}
	AnimationTrack *pTrack;
	pTrack = pAnimationContext->pActiveTrack[trackNo];
	pTrack->bActive = false;
	pTrack->pAnimationSet = NULL;
	pTrack->dblWeight = 0.0f;
	if (animNo < 0 || animNo >= pAnimationContext->numAnimationSets)
		return;

	pTrack->bLoop = loop;
	pTrack->bActive = true;
	pTrack->pAnimationSet = pAnimationContext->ppAnimationSets->at(animNo);
	pTrack->dblSpeed = 1.0f;
	pTrack->dblTrackTime = 0.f;
	pTrack->dblTrackFrame = 0.f;
	pTrack->dblWeight = weight;
}

//
//	SetTrackSpeed
//
void CAnimator::SetTrackSpeed(INT trackNo, DOUBLE speed){
	if (trackNo >= NUM_ACTIVE_TRACK)
		return;
	AnimationTrack *pTrack;
	pTrack = pAnimationContext->pActiveTrack[trackNo];
	if (pTrack != NULL){
		if (pTrack->bActive){
			pTrack->dblSpeed = speed;
		}
	}
}

//
//	function AdjustTrackAnimation
//	
static void GetBoneRotation(BoneParameters	*pBoneParam,Animation *pAnimation, DOUBLE frame, DOUBLE duration, BOOL bLoop);
static void GetBonePosition(BoneParameters	*pBoneParam,Animation *pAnimation, DOUBLE frame, DOUBLE duration, BOOL bLoop);
static void GetBoneScaling (BoneParameters	*pBoneParam,Animation *pAnimation, DOUBLE frame, DOUBLE duration, BOOL bLoop);

static void AdjustTrackAnimation(AnimationContext *pAnimationContext, INT trackNo){
	AnimationTrack	*pAnimationTrack = pAnimationContext->pActiveTrack[trackNo];
	AnimationSet	*pAnimationSet;
	Animation		*pAnimation;
	std::list<BoneParameters *>::iterator itBone;
	if (pAnimationTrack == NULL)
		return;
	pAnimationSet = pAnimationTrack->pAnimationSet;
	if (pAnimationSet == NULL)
		return;
	
	itBone = pAnimationTrack->ppActiveBoneParameters->begin();
	while(itBone != pAnimationTrack->ppActiveBoneParameters->end()){
		pAnimationTrack->ppInactiveBoneParameters->push_back(*itBone);
		itBone = pAnimationTrack->ppActiveBoneParameters->erase(itBone);
	}
	for (int i = 0; i < pAnimationSet->numAnimations; ++i){
		BoneParameters *pBoneParam;
		double dblFrame, totalFrames;
		BOOL	bLoop;
		pAnimation = pAnimationSet->ppAnimations[i];
		itBone = pAnimationTrack->ppInactiveBoneParameters->begin();
		if (itBone == pAnimationTrack->ppInactiveBoneParameters->end())
			break;	//  something's wrong there's no free BoneParameters
		pBoneParam = *itBone;
		pAnimationTrack->ppInactiveBoneParameters->erase(itBone);
		pBoneParam->pAnimation = pAnimation;
		if (pAnimation->pSlaveMeshFrame != NULL)
			pBoneParam->iFrameId = pAnimation->pSlaveMeshFrame->iFrameId;
		else
			pBoneParam->iFrameId = -1;

		dblFrame = pAnimationTrack->dblTrackFrame;
		pBoneParam->dblParamFrame = dblFrame;
		//  consider the duration of the last frame.
		totalFrames = (DOUBLE)(pAnimationSet->totalFrames+pAnimationContext->lastFrameLength);
		bLoop = pAnimationTrack->bLoop;

		//  Read the rotation data into pBoneParam.
		GetBoneRotation(pBoneParam, pAnimation, dblFrame, totalFrames, bLoop);

		//	Read the position data into pBoneParam.
		GetBonePosition(pBoneParam, pAnimation, dblFrame, totalFrames, bLoop);

		//	Read the position data into pBoneParam.
		GetBoneScaling(pBoneParam, pAnimation, dblFrame, totalFrames,  bLoop);

		pAnimationTrack->ppActiveBoneParameters->push_back(pBoneParam);
		
	}
}

//
//	Get the rotation of the bone
//
void GetBoneRotation(BoneParameters	*pBoneParam,Animation *pAnimation, DOUBLE dblFrame, DOUBLE dblDuration, BOOL bLoop){
	if (pAnimation->numRotationKeys == 0){
		pBoneParam->quatRotation = DirectX::XMQuaternionIdentity();
		return;
	}else if (pAnimation->numRotationKeys == 1){
		pBoneParam->quatRotation = pAnimation->pRotations[0].keyQuaternion;
		return;
	}
	
	int imin = 0, imax = pAnimation->numRotationKeys;
	int imid;
	int numRotationKeys = imax;
	double tmpTime;
	if (dblFrame >= pAnimation->pRotations[numRotationKeys - 1].iFrame){
		//  if the frame no is greater than the last frame
		//	do this
		if (!bLoop){
			pBoneParam->quatRotation = pAnimation->pRotations[numRotationKeys - 1].keyQuaternion;
			return;
		}else{
			double frame0, frame1;
			frame0 = pAnimation->pRotations[numRotationKeys - 1].iFrame;
			frame1 = pAnimation->pRotations[0].iFrame + dblDuration;
			if (frame0 >= frame1){
				pBoneParam->quatRotation = pAnimation->pRotations[numRotationKeys - 1].keyQuaternion;
			}else{
				DirectX::XMVECTOR	quat0, quat1;
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);
				quat0 = pAnimation->pRotations[numRotationKeys - 1].keyQuaternion;
				quat1 = pAnimation->pRotations[0].keyQuaternion;
				pBoneParam->quatRotation = DirectX::XMQuaternionSlerp(quat0, quat1, (FLOAT)tmp);
			}
			return;
		}
	}else if (dblFrame <= pAnimation->pRotations[0].iFrame){
		//  if the frame no is lower than the first frame
		//	do this
		if (!bLoop){
			pBoneParam->quatRotation = pAnimation->pRotations[0].keyQuaternion;
			return;
		}else{
			double frame0, frame1;
			frame0 = pAnimation->pRotations[numRotationKeys - 1].iFrame - dblDuration;
			frame1 = pAnimation->pRotations[0].iFrame;
			if (frame0 >= frame1){
				pBoneParam->quatRotation = pAnimation->pRotations[0].keyQuaternion;
			}else{
				DirectX::XMVECTOR	quat0, quat1;
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);
				quat0 = pAnimation->pRotations[numRotationKeys - 1].keyQuaternion;
				quat1 = pAnimation->pRotations[0].keyQuaternion;
				pBoneParam->quatRotation = DirectX::XMQuaternionSlerp(quat0, quat1, (FLOAT)tmp);
			}
			return;
		}
	}else{
		//  search with binary search
		//	and read the nearest rotation data
		while (imax > imin){
			imid = (imin + imax)>>1;
			tmpTime = (double)(pAnimation->pRotations[imid].iFrame);
			if (dblFrame == tmpTime){
				break;
			}else if (dblFrame < tmpTime){
				imax = imid;
			}else{
				imin = imid+1;
			}
		}
		if (imax <= imin){
			imid = imax;
		}
		imid = min(imid,numRotationKeys-1);
		tmpTime = pAnimation->pRotations[imid].iFrame;
		if (dblFrame == tmpTime){
			pBoneParam->quatRotation = pAnimation->pRotations[imid].keyQuaternion;
		}else{
			int index0, index1;
			if (tmpTime < dblFrame){
				index1 = imid;
				do{
					++index1;
				}while(pAnimation->pRotations[index1].iFrame < dblFrame &&index1 < numRotationKeys);
				index0 = index1 - 1;
			}else{
				index0 = imid;
				do{
					--index0;
				}while(pAnimation->pRotations[index0].iFrame > dblFrame && index0 > 0);
				index1 = index0 + 1;
			}
			DirectX::XMVECTOR	quat0, quat1;
			double	frame0, frame1;
			frame0 = (double)pAnimation->pRotations[index0].iFrame;
			frame1 = (double)pAnimation->pRotations[index1].iFrame;
			if (frame0 == frame1){
				pBoneParam->quatRotation = pAnimation->pRotations[index0].keyQuaternion;
			}else{
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);
				quat0 = pAnimation->pRotations[index0].keyQuaternion;
				quat1 = pAnimation->pRotations[index1].keyQuaternion;
				pBoneParam->quatRotation = DirectX::XMQuaternionSlerp(quat0, quat1, (FLOAT)tmp);
			}
		}
	}
}

void GetBonePosition(BoneParameters	*pBoneParam,Animation *pAnimation, DOUBLE dblFrame,DOUBLE dblDuration, BOOL bLoop){

	Vector3KeyFrame *pPositions = pAnimation->pPositions;

	if (pAnimation->numPositionKeys == 0){
		pBoneParam->vecPosition = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(0,0,0));
		return;
	}else if (pAnimation->numPositionKeys == 1){
		pBoneParam->vecPosition = DirectX::XMLoadFloat3(&pPositions[0].keyVector3);
		return;
	}

	int imin = 0, imax = pAnimation->numPositionKeys;
	int imid;
	int numPositionKeys = imax;
	double tmpTime;
	if (dblFrame >= pPositions[numPositionKeys - 1].iFrame){

		//  if the frame is after the last frame
		//	do this
		if (!bLoop){
			pBoneParam->vecPosition = DirectX::XMLoadFloat3(&pPositions[numPositionKeys - 1].keyVector3);
			return;
		}else{
			double frame0, frame1;
			frame0 = pPositions[numPositionKeys - 1].iFrame;
			frame1 = pPositions[0].iFrame + dblDuration;
			if (frame0 >= frame1){
				pBoneParam->vecPosition = DirectX::XMLoadFloat3(&pPositions[numPositionKeys - 1].keyVector3);
			}else{
				DirectX::XMVECTOR	vec0, vec1;
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);

				vec0 = DirectX::XMLoadFloat3(&pPositions[numPositionKeys - 1].keyVector3);
				vec1 = DirectX::XMLoadFloat3(&pPositions[0].keyVector3);
				vec0 = DirectX::XMVectorScale(vec0,1.0f-(FLOAT)tmp);
				vec1 = DirectX::XMVectorScale(vec1,(FLOAT)tmp);
				pBoneParam->vecPosition = DirectX::XMVectorAdd(vec0,vec1);
			}
			return;
		}
	}else if (dblFrame <= pPositions[0].iFrame){
		//  if the frame is before the first frame
		//	do this
		if (!bLoop){
			pBoneParam->vecPosition = DirectX::XMLoadFloat3(&pPositions[0].keyVector3);
			return;
		}else{
			double frame0, frame1;
			frame0 = pPositions[numPositionKeys - 1].iFrame - dblDuration;
			frame1 = pPositions[0].iFrame;
			if (frame0 >= frame1){
				pBoneParam->vecPosition = DirectX::XMLoadFloat3(&pPositions[0].keyVector3);
				return;
			}else{
				DirectX::XMVECTOR	vec0, vec1;
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);

				vec0 = DirectX::XMLoadFloat3(&pPositions[numPositionKeys - 1].keyVector3);
				vec1 = DirectX::XMLoadFloat3(&pPositions[0].keyVector3);
				vec0 = DirectX::XMVectorScale(vec0,(FLOAT)tmp);
				vec1 = DirectX::XMVectorScale(vec1,1.0f-(FLOAT)tmp);
				pBoneParam->vecPosition = DirectX::XMVectorAdd(vec0,vec1);
			}
			return;
		}
	}else{
		//  if the frame is 
		//  between the first frame and the last frame
		//  do binary search with and read the nearest position data
		while (imax > imin){
			imid = (imin + imax)>>1;
			tmpTime = (double)(pPositions[imid].iFrame);
			if (dblFrame == tmpTime){
				break;
			}else if (dblFrame < tmpTime){
				imax = imid;
			}else{
				imin = imid+1;
			}
		}
		if (imax <= imin){
			imid = imax;
		}
		imid = min(imid,numPositionKeys-1);
		tmpTime = pPositions[imid].iFrame;
		if (dblFrame == tmpTime){
			pBoneParam->vecPosition = DirectX::XMLoadFloat3(&pPositions[imid].keyVector3);
		}else{
			int index0, index1;
			if (tmpTime < dblFrame){
				//  imid is always premious to the last element because the last frame is bigeer than dblFrame.
				index1 = imid;
				do{
					++index1;
				}while(pPositions[index1].iFrame < dblFrame &&index1 < numPositionKeys);
				index0 = index1 - 1;
			}else{
				//  imid is always after the first element because the first frame is smaller than dblFrame
				index0 = imid;
				do{
					--index0;
				}while(pPositions[index0].iFrame > dblFrame && index0 > 0);
				index1 = index0 + 1;
			}
			DirectX::XMVECTOR	vec0, vec1;
			double	frame0, frame1;
			frame0 = (double)pPositions[index0].iFrame;
			frame1 = (double)pPositions[index1].iFrame;
			if (frame0 == frame1){
				pBoneParam->vecPosition = DirectX::XMLoadFloat3(&pPositions[index0].keyVector3);
			}else{
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);
				vec0 = DirectX::XMLoadFloat3(&pPositions[index0].keyVector3);
				vec1 = DirectX::XMLoadFloat3(&pPositions[index1].keyVector3);
				vec0 = DirectX::XMVectorScale(vec0,1.0f-(FLOAT)tmp);
				vec1 = DirectX::XMVectorScale(vec1,(FLOAT)tmp);
				pBoneParam->vecPosition = DirectX::XMVectorAdd(vec0,vec1);
			}
		}
	}
}


void GetBoneScaling(BoneParameters	*pBoneParam,Animation *pAnimation, DOUBLE dblFrame, DOUBLE dblDuration, BOOL bLoop){

	Vector3KeyFrame *pScalings = pAnimation->pScalings;

	if (pAnimation->numScalingKeys == 0){
		pBoneParam->vecScaling = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(1,1,1));
		return;
	}else if (pAnimation->numScalingKeys == 1){
		pBoneParam->vecScaling = DirectX::XMLoadFloat3(&pScalings[0].keyVector3);
		return;
	}
	
	//  search with binary search
	//	and read the nearest scaling data

	int imin = 0, imax = pAnimation->numScalingKeys;
	int imid;
	int numScalingKeys = imax;
	double tmpTime;
	if (dblFrame >= pScalings[numScalingKeys - 1].iFrame){

		//  if the frame is after the last frame
		//	do this
		if (!bLoop){
			pBoneParam->vecScaling = DirectX::XMLoadFloat3(&pScalings[numScalingKeys - 1].keyVector3);
			return;
		}else{
			double frame0, frame1;
			frame0 = pScalings[numScalingKeys - 1].iFrame;
			frame1 = pScalings[0].iFrame + dblDuration;
			if (frame0 >= frame1){
				pBoneParam->vecScaling = DirectX::XMLoadFloat3(&pScalings[numScalingKeys - 1].keyVector3);
			}else{
				DirectX::XMVECTOR	vec0, vec1;
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);

				vec0 = DirectX::XMLoadFloat3(&pScalings[numScalingKeys - 1].keyVector3);
				vec1 = DirectX::XMLoadFloat3(&pScalings[0].keyVector3);
				vec0 = DirectX::XMVectorScale(vec0,1.0f-(FLOAT)tmp);
				vec1 = DirectX::XMVectorScale(vec1,(FLOAT)tmp);
				pBoneParam->vecScaling = DirectX::XMVectorAdd(vec0,vec1);
			}
			return;
		}
	}else if (dblFrame <= pScalings[0].iFrame){
		//  if the frame is before the first frame
		//	do this
		if (!bLoop){
			pBoneParam->vecScaling = DirectX::XMLoadFloat3(&pScalings[0].keyVector3);
			return;
		}else{
			double frame0, frame1;
			frame0 = pScalings[numScalingKeys - 1].iFrame - dblDuration;
			frame1 = pScalings[0].iFrame;
			if (frame0 >= frame1){
				pBoneParam->vecScaling = DirectX::XMLoadFloat3(&pScalings[0].keyVector3);
				return;
			}else{
				DirectX::XMVECTOR	vec0, vec1;
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);

				vec0 = DirectX::XMLoadFloat3(&pScalings[numScalingKeys - 1].keyVector3);
				vec1 = DirectX::XMLoadFloat3(&pScalings[0].keyVector3);
				vec0 = DirectX::XMVectorScale(vec0,(FLOAT)tmp);
				vec1 = DirectX::XMVectorScale(vec1,1.0f-(FLOAT)tmp);
				pBoneParam->vecScaling = DirectX::XMVectorAdd(vec0,vec1);
			}
			return;
		}
	}else{
		//  search with binary search
		//	and read the nearest scaling data
		while (imax > imin){
			imid = (imin + imax)>>1;
			tmpTime = (double)(pScalings[imid].iFrame);
			if (dblFrame == tmpTime){
				break;
			}else if (dblFrame < tmpTime){
				imax = imid;
			}else{
				imin = imid+1;
			}
		}
		if (imax <= imin){
			imid = imax;
		}
		imid = min(imid,numScalingKeys-1);
		tmpTime = pScalings[imid].iFrame;
		if (dblFrame == tmpTime){
			pBoneParam->vecScaling = DirectX::XMLoadFloat3(&pScalings[imid].keyVector3);
		}else{
			int index0, index1;
			if (tmpTime < dblFrame){
				index1 = imid;
				do{
					++index1;
				}while(pScalings[index1].iFrame < dblFrame &&index1 < numScalingKeys);
				index0 = index1 - 1;
			}else{
				index0 = imid;
				do{
					--index0;
				}while(pScalings[index0].iFrame > dblFrame && index0 > 0);
				index1 = index0 + 1;
			}
			DirectX::XMVECTOR	vec0, vec1;
			double	frame0, frame1;
			frame0 = (double)pScalings[index0].iFrame;
			frame1 = (double)pScalings[index1].iFrame;
			if (frame0 == frame1){
				pBoneParam->vecScaling = DirectX::XMLoadFloat3(&pScalings[index0].keyVector3);
			}else{
				double tmp = dblFrame - frame0;
				tmp /= (frame1 - frame0);
				vec0 = DirectX::XMLoadFloat3(&pScalings[index0].keyVector3);
				vec1 = DirectX::XMLoadFloat3(&pScalings[index1].keyVector3);
				vec0 = DirectX::XMVectorScale(vec0,1.0f-(FLOAT)tmp);
				vec1 = DirectX::XMVectorScale(vec1,(FLOAT)tmp);
				pBoneParam->vecScaling = DirectX::XMVectorAdd(vec0,vec1);
			}
		}
	}
}

//
//	Advance time
//
void CAnimator::AdvanceTime(DOUBLE time){
	for (int i = 0; i < NUM_ACTIVE_TRACK ; ++i){
		AnimationTrack *pTrack = pAnimationContext->pActiveTrack[i];
		SetTrackTime(i,pTrack->dblTrackTime + time);
	}
}

//
//	Adjust animations and finish the current pose.
//
void CAnimator::AdjustAnimation(){
	AnimationMixerTrack *pMixerTrack = this->pAnimationContext->pFrameMotions;
	AnimationMixerTrack *pTmpMixerTrack;
	INT numFrames = pAnimationContext->numFramesInMeshHierarchy;

	if (pMixerTrack == NULL){
		return;
	}

	//  Clear mixer buffer
	for (int i = 0; i < numFrames ; ++i){
		pTmpMixerTrack = &pMixerTrack[i];
		pTmpMixerTrack->iCount = 0;
		for (int j = 0; j < NUM_ACTIVE_TRACK ; ++j){
			pTmpMixerTrack->ppBoneParameters[j] = NULL;
			pTmpMixerTrack->dblWeight[j] = 0.0;
		}
	}

	//  gather animation data
	for (int i = 0; i < NUM_ACTIVE_TRACK ; ++i){
		AnimationTrack	*pAnimationTrack = pAnimationContext->pActiveTrack[i];
		if (pAnimationTrack->bActive == false)
			continue;
		std::list<BoneParameters*>::iterator itBone = pAnimationTrack->ppActiveBoneParameters->begin();
		while (itBone != pAnimationTrack->ppActiveBoneParameters->end()){
			if (*itBone != NULL){
				INT iFrameId = (*itBone)->iFrameId;
				if (iFrameId >= 0){
					pTmpMixerTrack = &pMixerTrack[iFrameId];
					INT iCount = pTmpMixerTrack->iCount;
					if (iCount < NUM_ACTIVE_TRACK){
						pTmpMixerTrack->ppBoneParameters[iCount] = *itBone;
						pTmpMixerTrack->dblWeight[iCount] = pAnimationTrack->dblWeight;
						pTmpMixerTrack->iCount = ++iCount;
					}
				}
			}
			++itBone;
		}
	}

	//	apply weights
	for (int i = 0; i < numFrames ; ++i){
		pTmpMixerTrack = &pMixerTrack[i];
		DirectX::XMVECTOR	quatRotation;
		DirectX::XMVECTOR	vecPosition, vec1;
		DirectX::XMVECTOR	vecScaling;
		DOUBLE	weight[NUM_ACTIVE_TRACK];
		DOUBLE	fTmp;
		MeshFrame	*pFrame = NULL;
		switch(pTmpMixerTrack->iCount){
		case	0:
			continue;
		case	1:
			quatRotation = pTmpMixerTrack->ppBoneParameters[0]->quatRotation;
			vecPosition  = pTmpMixerTrack->ppBoneParameters[0]->vecPosition;
			vecScaling   = pTmpMixerTrack->ppBoneParameters[0]->vecScaling;

			break;
		default:
			//  interporate animations
			//  currently NUM_ACTIVE_TRACK must be 2
			//  because of my poor lerp algorithm.

			//  normalize the weights
			for (int i = 0; i < NUM_ACTIVE_TRACK ; ++i){
				weight[i] = 0.0f;
			}
			fTmp =0.0;
			for (int i = 0; i < pTmpMixerTrack->iCount ; ++i){
				fTmp += weight[i] = pTmpMixerTrack->dblWeight[i];
			}
			fTmp = 1.0 / fTmp;
			for (int i = 0; i < pTmpMixerTrack->iCount ; ++i){
				weight[i] *= fTmp;
			}

			quatRotation = pTmpMixerTrack->ppBoneParameters[0]->quatRotation;
			vecPosition  = pTmpMixerTrack->ppBoneParameters[0]->vecPosition;
			vecScaling   = pTmpMixerTrack->ppBoneParameters[0]->vecScaling;

			for (int i = 1 ; i < NUM_ACTIVE_TRACK; ++i){
				fTmp = weight[i];
				quatRotation = DirectX::XMQuaternionSlerp(quatRotation,pTmpMixerTrack->ppBoneParameters[i]->quatRotation,(FLOAT)(1.0-fTmp));
				vecPosition = DirectX::XMVectorScale(vecPosition,(FLOAT)(1.0 - fTmp));
				vec1        = DirectX::XMVectorScale(pTmpMixerTrack->ppBoneParameters[1]->vecPosition,(FLOAT)fTmp);
				vecPosition = DirectX::XMVectorAdd(vecPosition,vec1);
				vecScaling  = DirectX::XMVectorScale(vecScaling,(FLOAT)(1.0 - fTmp));
				vec1        = DirectX::XMVectorScale(pTmpMixerTrack->ppBoneParameters[1]->vecScaling,(FLOAT)fTmp);
				vecScaling  = DirectX::XMVectorAdd(vecScaling,vec1);
			}

			break;
		}
		//  put it into frames;
		pFrame = pTmpMixerTrack->ppBoneParameters[0]->pAnimation->pSlaveMeshFrame;

		if (pFrame != NULL){
			pFrame->FrameMatrix
				= DirectX::XMMatrixMultiply(
					DirectX::XMMatrixMultiply(
						DirectX::XMMatrixScalingFromVector(vecScaling),
						DirectX::XMMatrixTranspose( DirectX::XMMatrixRotationQuaternion(quatRotation))
					),  DirectX::XMMatrixTranslationFromVector(vecPosition)
				);
		}
	}
}

void CAnimator::SetLastFrameLength(INT length){
	this->pAnimationContext->lastFrameLength = length;
}