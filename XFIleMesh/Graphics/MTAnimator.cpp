//
//	class CMTAnimator
//
//! @Brief: class to animate CSkinnedMesh without cloning it.
//
#include "stdafx.h"
#include <D3D11.h>
#include <DirectXMath.h>
#include "SkinnedMesh.h"
#include "Animation.h"
#include "Animator.h"
#include "MTAnimator.h"

//////////////////////////////////////////////////////////////////////
//  ctor/destructor
//////////////////////////////////////////////////////////////////////

CMTAnimator::CMTAnimator(ISkinnedMesh *pMesh)
{
	m_pMesh = pMesh;
	Reset();
}

CMTAnimator::~CMTAnimator()
{
}

//-----------------------------------------------------------
//	Name : Reset
//	Desc : アニメーションを初期状態に戻す
//-----------------------------------------------------------
void	CMTAnimator::Reset(){
	m_iActiveTrack[0] = 0;
	m_iActiveTrack[1] = -1;
	m_fTrackTime[0] = 0;
	m_fTrackTime[1] = 0;
	m_dwFlags[0] = MULTITRACK_ANIM_LOOP;
	m_dwFlags[1] = MULTITRACK_ANIM_LOOP;
	m_dwAnimationPhase = MULTITRACK_ANIM_PHASE_RUN;
	m_fPriorityBlend = 0.0f;
	m_fBlendDiff = 0.0f;
	m_fSpeed = 1.0f;
	m_matWorld = DirectX::XMMatrixIdentity();
}

//-----------------------------------------------------------
//	Name : GetNumTracks
//	Desc : マルチトラックアニメーションのトラック数を返す
//-----------------------------------------------------------
DWORD	CMTAnimator::GetNumAnimationSets(){
	DWORD	dwNumAnimations = 0;
	IAnimator *pAnimator = NULL;
	if (m_pMesh != NULL){
		pAnimator = m_pMesh->GetAnimator();
		if (pAnimator == NULL)
			return 0;
		dwNumAnimations = pAnimator->GetNumAnimations();
	}
	return	dwNumAnimations;
}

//-----------------------------------------------------------
//	Name : Update
//	Desc : マルチトラックアニメーションを実行
//-----------------------------------------------------------
void	CMTAnimator::Update(FLOAT fTime, DirectX::XMMATRIX *pMatWorld){
	int	i;
	DOUBLE period;
	IAnimator *pAnim = NULL;
	AnimationSet *pAnimSet = NULL;

	m_matWorld = *pMatWorld;
	pAnim = m_pMesh->GetAnimator();
	if (pAnim != NULL){

		for ( i = 0; i < 2 ; ++i ){
			if (0 > m_iActiveTrack[i])
				continue;
			if ((m_dwFlags[i] & MULTITRACK_ANIM_STOPPED) != 0)
				continue;
			m_fTrackTime[i] += fTime * m_fSpeed;
			if (pAnim->GetAnimationDuration(m_iActiveTrack[i],&period,1.0)){
				if (m_fTrackTime[i] >= period){
					if (m_dwFlags[i]){

						INT	a = (INT)(m_fTrackTime[i] / period);
						m_fTrackTime[i] -= period * (double)a;
					}else{
						//	
						//	m_fTrackTime[i] = DecDouble(period);
						//	イマイチ不正確なアニメーションデータを吐き出すソフトがあるようだ、
						//	少し多めに戻しておく
						m_fTrackTime[i] = DecDouble(period);
						m_dwFlags[i] |= MULTITRACK_ANIM_STOPPED;
					}
				}
			}
		}
	}
	switch(this->m_dwAnimationPhase){
		case	MULTITRACK_ANIM_PHASE_TRANSIT:
			m_fPriorityBlend -= m_fBlendDiff * fTime;
			if (m_fPriorityBlend < FLT_MIN){
				m_iActiveTrack[1] = -1;
				m_fPriorityBlend = 0.0f;
				m_dwAnimationPhase = MULTITRACK_ANIM_PHASE_RUN;
			}
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------
//	Name : DecDouble
//	Desc : Double 型のスカラーデータをほんの少し０に近づける
//	Param:  v   : 倍精度(DOUBLE)数値
//-----------------------------------------------------------
DOUBLE	CMTAnimator::DecDouble(DOUBLE v){
	DOUBLE	result;
	unsigned __int64	ui64End =*(unsigned __int64*)&v;
	unsigned __int64	ui64Exp = ui64End;
	unsigned __int64	ui64Sign =ui64End;
	unsigned __int64	ui64FracMask = 0x000fffffffffffff;

	ui64Exp  &= 0x7ff0000000000000;	//	指数部取出し
	ui64Sign &= 0x8000000000000000;	//	符号取り出し

	if (ui64Exp <= 0x0010000000000000){
		result = 0.0;	//	極小値はゼロにクリップ
	}else{
		//	少しだけ減らす
		if ((ui64End & ui64FracMask) != 0){
			ui64End--;
		}else{
			ui64Exp -= 0x0010000000000000;
			ui64End = 0x000fffffffffffff | ui64Exp;
		}
		ui64End |= ui64Sign;
		result = *(double*)&ui64End;
	}
	return	result;
}
//-----------------------------------------------------------
//	Name : SetActiveAnimationSet
//	Desc : アニメーションさせるAnimationSetを指定する
//	Param:  tno   : AnimationSet番号
//			fTime : AnimationSetが入れ替わるまでの時間を指定。
//-----------------------------------------------------------
void	CMTAnimator::SetActiveAnimationSet(INT tno, FLOAT fTime, BOOL bLoop){
	if (fTime < FLT_MIN)
		fTime = 1.0f;
	m_iActiveTrack[1] = m_iActiveTrack[0];
	m_fTrackTime[1] = m_fTrackTime[0];
	m_dwFlags[1] = m_dwFlags[0];
	m_fPriorityBlend = 1.0f;
	m_iActiveTrack[0] = tno;
	m_fTrackTime[0] = 0.0;
	m_dwFlags[0] = (bLoop)?(MULTITRACK_ANIM_LOOP):(0);

	m_fBlendDiff = 1.0f / fTime;
	m_dwAnimationPhase = MULTITRACK_ANIM_PHASE_TRANSIT;
}

//-----------------------------------------------------------
//	Name : SetSpeed
//	Desc : アニメーションのスピード設定を行う
//-----------------------------------------------------------
void	CMTAnimator::SetSpeed(FLOAT fSpeed){
	m_fSpeed = fSpeed;
}

//-----------------------------------------------------------
//	Name : AdjustTracks
//	Desc : スキンメッシュ上のアニメーショントラックへ反映させる。
//-----------------------------------------------------------
void	CMTAnimator::AdjustTracks(){
	int	tno, i;
	DWORD	dwNumTracks;
	IAnimator	*pAnim = NULL;
	if (m_pMesh != NULL)
		pAnim = m_pMesh->GetAnimator();

	if (pAnim != NULL){
		dwNumTracks = pAnim->GetNumAnimations();
		for (i = 0 ; i < 2 ; ++i){
			tno = m_iActiveTrack[i];
			if (tno < 0){
				pAnim->SetTrackAnimation(i,-1,0,false);
			}else{
				if (tno > (INT)dwNumTracks)
					tno = -1;
				pAnim->SetTrackAnimation(i,tno,(i==0)?(1.0f-m_fPriorityBlend):(m_fPriorityBlend),
					0!=(m_dwFlags[i]&MULTITRACK_ANIM_LOOP));
				pAnim->SetTrackTime(i,m_fTrackTime[i]);
			}
		}
	}
	pAnim->AdjustAnimation();
}

//-----------------------------------------------------------
//	Name : Render
//	Desc : 登録されたスキンメッシュの描画
//-----------------------------------------------------------
void	CMTAnimator::Render(ID3D11DeviceContext *pContext){
	AdjustTracks();
	m_pMesh->AdjustPose(&m_matWorld);
	m_pMesh->Render(pContext);
}

//-----------------------------------------------------------
//	Name : UpdateMatrices
//	Desc : 登録されたスキンメッシュのボーンを描画せず更新
//-----------------------------------------------------------
void	CMTAnimator::UpdateBones(){
	AdjustTracks();
	m_pMesh->AdjustPose(&m_matWorld);
}

//-----------------------------------------------------------
//	Name : GetCurrentPosition
//	Desc : アニメーションの現在時間を返す。
//	Note :	アニメーションデータ内の絶対時間を返す為、
//			時間の単位は、モデリングソフトに依存する。
//-----------------------------------------------------------
DOUBLE	CMTAnimator::GetCurrentPosition(){
	return	m_fTrackTime[0];
}
