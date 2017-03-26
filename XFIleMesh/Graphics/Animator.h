#pragma once

//  Moved to IMesh.h
#if 0
//
//	class Animator
//
struct AnimationSet;

class IAnimator{
public:
//	static IAnimator *CreateAnimator(MeshFrame *pRoot);
	static IAnimator *CreateInstance(MeshFrame *pRoot);
	virtual ~IAnimator() = 0;
	virtual HRESULT	AddAnimationSets(AnimationSet **ppAnimationSet, INT num) = 0;
	virtual void	SetTrackAnimation(INT trackNo, INT animNo, DOUBLE weight, BOOL loop) = 0;
	virtual void	SetTrackWeight(INT trackNo, DOUBLE weight) = 0;
	virtual void	SetTrackSpeed(INT trackNo, DOUBLE speed) = 0;
	virtual DOUBLE	SetTrackTime(INT trackNo, DOUBLE time) = 0;
	virtual void	AdvanceTime(DOUBLE time) = 0;
	virtual void	AdjustAnimation() = 0;	//	Finalize animation and apply to mesh
	virtual INT		GetNumAnimations() = 0;
	virtual BOOL	GetTrackDuration(INT trackNo, DOUBLE *pDuration) = 0;
	virtual BOOL	GetAnimationDuration(INT animNo, DOUBLE *pDuration, DOUBLE speed = 1.0) = 0;
};
#else
#include "IMesh.h"
#endif

struct AnimationContext;
class CAnimator : public IAnimator
{
public:
	CAnimator(MeshFrame *pRoot);
	virtual ~CAnimator(void);
	virtual HRESULT	AddAnimationSets(AnimationSet **ppAnimationSet, INT num);
	virtual void	SetTrackAnimation(INT trackNo, INT animNo, DOUBLE weight, BOOL loop);
	virtual void	SetTrackWeight(INT trackNo, DOUBLE weight);
	virtual void	SetTrackSpeed(INT trackNo, DOUBLE speed);
	virtual DOUBLE	SetTrackTime(INT trackNo, DOUBLE time);
	virtual void	AdvanceTime(DOUBLE time);
	virtual void	AdjustAnimation();	//	Finalize animation and apply to mesh
	virtual INT		GetNumAnimations();
	virtual BOOL	GetTrackDuration(INT trackNo, DOUBLE *pDuration);
	virtual BOOL	GetAnimationDuration(INT animNo, DOUBLE *pDuration, DOUBLE speed = 1.0);
	virtual void	SetLastFrameLength(int length) override;
private:
	AnimationContext	*pAnimationContext;
};

