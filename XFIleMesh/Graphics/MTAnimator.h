//
//	@file MTAnimator.h
//	@brief an declaration of CMTAnimator
//
#pragma once

#define	MULTITRACK_ANIM_PHASE_RUN		1
#define	MULTITRACK_ANIM_PHASE_TRANSIT	2

//	フラグ
#define	MULTITRACK_ANIM_LOOP	1
#define	MULTITRACK_ANIM_STOPPED	2

class ISkinnedMesh;

//
// @class CMTAnimator
//
// @Brief: Multi track animator
//
__declspec(align(16)) class CMTAnimator
{
public:
	CMTAnimator(ISkinnedMesh *pMesh);
	virtual ~CMTAnimator();
	virtual	DWORD	GetNumAnimationSets();
	virtual	void	Update(FLOAT fTime, DirectX::XMMATRIX *matWorld);
	virtual	void	SetActiveAnimationSet(INT tno, FLOAT fTime, BOOL bLoop = TRUE);
	virtual	void	SetSpeed(FLOAT fSpeed);
	virtual	void	Render(ID3D11DeviceContext *pContext);
	virtual void    UpdateBones();
	virtual DOUBLE	GetCurrentPosition();

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
protected:
	static	DOUBLE	DecDouble(DOUBLE v);
	virtual	void	AdjustTracks();
	virtual void Reset();
	ISkinnedMesh *m_pMesh;
	DWORD	m_dwAnimationPhase;
	INT		m_iActiveTrack[2];
	DOUBLE	m_fTrackTime[2];	//	
	DWORD	m_dwFlags[2];		//	フラグ
	FLOAT	m_fPriorityBlend;
	FLOAT	m_fBlendDiff;
	FLOAT	m_fSpeed;
	DirectX::XMMATRIX m_matWorld;
};
