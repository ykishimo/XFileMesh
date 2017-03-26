//
//	Declaration of animation data
//
#ifndef __ANIMATION_H__
#define __ANIMATION_H__

//
//	AnimationSet structure
//

//
//	KeyFrames
//
__declspec(align(16)) struct QuaternionKeyFrame{
	INT	iFrame;
	DirectX::XMVECTOR	keyQuaternion;
	void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	void *operator new[](size_t size){
		return _mm_malloc(size,16);
	}
	void operator delete(void *p){
		return _mm_free(p);
	}
};
__declspec(align(16)) struct Vector3KeyFrame{
	INT	iFrame;
	DirectX::XMFLOAT3	keyVector3;
	void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	void *operator new[](size_t size){
		return _mm_malloc(size,16);
	}
	void operator delete(void *p){
		return _mm_free(p);
	}
};

struct MeshFrame;
__declspec(align(16)) struct Animation{
public:
	CHAR *pSlaveFrameName;
	QuaternionKeyFrame	*pRotations;
	INT					numRotationKeys;
	Vector3KeyFrame		*pPositions;
	INT					numPositionKeys;
	Vector3KeyFrame		*pScalings;
	INT					numScalingKeys;
	MeshFrame			*pSlaveMeshFrame;
	Animation();
	~Animation();
};

struct AnimationSet{
	CHAR *pName;
	Animation **ppAnimations;
	INT	numAnimations;
	INT	totalFrames;
	AnimationSet();
	~AnimationSet();
};
#endif

