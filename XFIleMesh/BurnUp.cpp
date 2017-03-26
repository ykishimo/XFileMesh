#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include "D3DContext.h"
#include "BurnUp.h"
#include "D3D11PointSprite.h"
#include <list>

using namespace DirectX;

//
//    Particle B
//    ���シ�鉊�̃p�[�e�B�N��
//
typedef    struct    _FIRE_PARTICLE_B{
    DirectX::XMVECTOR p;
    FLOAT			  t;
    DirectX::XMVECTOR v;
	//  keep this 16-byte aligned
	void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	void *operator new[](size_t size){
		return _mm_malloc(size,16);
	}
	void operator delete(void *p){
		return _mm_free(p);
	}
}   FIRE_PARTICLE_B, *LPFIRE_PARTICLE_B;

#define PARTICLE_START_FO_B     15      /*    �p�[�e�B�N���t�F�[�h�A�E�g�J�n�^�C��    */
#define PARTICLE_LIFETIME_B     24      /*    �p�[�e�B�N����������      */
#define PARTICLE_SPEED_B        0.20f    /*    �p�[�e�B�N���㏸���x      */
#define PARTICLE_SPREAD_SPEED_B 0.125f   /*    �p�[�e�B�N�����������x    */
#define PARTICLE_EMIT_INTERVAL  0.2f    /*    �p�[�e�B�N�������Ԋu      */
#define BURNUP_LIFETIME        1200.0f   /*    ����G�t�F�N�g��������    */

class ID3D11PointSprite;

class CBurnUp : public IBurnUp
{
public:
    CBurnUp(DirectX::XMFLOAT3 *pVecPos){
		m_dwNumParticles = 0;
		m_dwPhase = 0;
		m_fBufferedTime = 0;
		m_vecPos = *pVecPos;
		m_fTime = 0;
	}
    virtual ~CBurnUp(void){
		std::list<LPFIRE_PARTICLE_B>::iterator it = m_listFireB.begin();
		while (it != m_listFireB.end()){
			SAFE_DELETE(*it);
			it = m_listFireB.erase(it);
		}
	}
    virtual BOOL Update(FLOAT timeElapsed) override
	{
		BOOL    bResult = FALSE;

		if (timeElapsed > 2.0f)
			timeElapsed = 2.0f;
		//    �p�[�e�B�N���̃C�e���[�V����
		XMVECTOR vecUp = XMLoadFloat3(&XMFLOAT3(0,1.0f,0));
		LPFIRE_PARTICLE_B    pTmp;
		std::list<LPFIRE_PARTICLE_B>::iterator it = m_listFireB.begin();
		while(it != m_listFireB.end()){
			pTmp = *it;
			pTmp->t += timeElapsed;
			pTmp->v *= pow(0.9f,timeElapsed);    //    �������̑��x�͌���������
			pTmp->p += pTmp->v * timeElapsed;    //    ������Ɉړ�
			pTmp->p += vecUp * (PARTICLE_SPEED_B * timeElapsed);
			if (pTmp->t >= PARTICLE_LIFETIME_B){    //    ������������폜����B
				SAFE_DELETE(*it);
				it = m_listFireB.erase(it);
				m_dwNumParticles --;
			}else
				++it;
		}
		FLOAT    fTmp = 0.0f;
		m_fBufferedTime += timeElapsed;

		switch(m_dwPhase){
			case    0:
				m_fTime += timeElapsed;
				//    �p�[�e�B�N���𔭐������鏈��
				while (m_fBufferedTime >= PARTICLE_EMIT_INTERVAL){
					m_fBufferedTime -= PARTICLE_EMIT_INTERVAL;
					pTmp = new FIRE_PARTICLE_B;
                
					//    ��U�����𗐐��Ō���
					fTmp = (FLOAT)rand() / (FLOAT)RAND_MAX;
					fTmp = XM_PI * 2.0f * fTmp;
					pTmp->v = XMLoadFloat3(&XMFLOAT3(cosf(fTmp),0,sinf(fTmp)));

					//    ��U���x�𗐐��Ō���
					fTmp = (FLOAT)rand() / (FLOAT)RAND_MAX;
					fTmp = PARTICLE_SPREAD_SPEED_B * fTmp;
					pTmp->v *= fTmp;

					//    ���̃p�����[�^�̏����ݒ�
					pTmp->t = m_fBufferedTime;
					pTmp->p = XMLoadFloat3(&m_vecPos);        //    �ʒu��ݒ�

					//    �����N���X�g�֒ǉ�
					m_listFireB.push_back(pTmp);

					m_dwNumParticles++;        //    �p�[�e�B�N������ǉ�
				}
				if (m_fTime >= BURNUP_LIFETIME)    //    ����I��
					m_dwPhase++;
				break;
			case    1:
				//    �p�[�e�B�N���������Ȃ�����I��
				if (m_listFireB.size() == 0)
					bResult = TRUE;
				break;
		}

		return    bResult;

	};
    virtual void Render(ID3D11DeviceContext *pContext, ID3D11PointSprite *pSprite) override
	{
		//    ����Β萔
		static const FLOAT    fInvFadeBase = 1.0f / (PARTICLE_LIFETIME_B - PARTICLE_START_FO_B);
		static const FLOAT    fInvFadeLifetime = 1.0f / PARTICLE_LIFETIME_B;

		//    �ϐ�
		XMFLOAT4    diffuse;
		FLOAT    fTmp, fSize, alpha;

		if (pSprite == NULL)
			return;
		
		std::list<LPFIRE_PARTICLE_B>::iterator it = m_listFireB.begin();
		LPFIRE_PARTICLE_B    pTmp;
		while(it != m_listFireB.end()){
			pTmp = *it;
			alpha = 1.0f;
			if (pTmp->t > PARTICLE_START_FO_B){
				fTmp = pTmp->t - PARTICLE_START_FO_B;
				fTmp *= fInvFadeBase;
				fTmp = 1.0f - fTmp;
				alpha = fTmp;
			}
			fSize = 0.6f * (1.0f - pTmp->t * fInvFadeLifetime);

			diffuse = XMFLOAT4(1.0f,1.0f,1.0f,alpha);

			pSprite->Render(pContext,(XMFLOAT3*)pTmp->p.m128_f32,fSize,&diffuse);
			++it;
		}
		pSprite->Flush(pContext);

	};
	//  keep this 16-byte aligned
	void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	void *operator new[](size_t size){
		return _mm_malloc(size,16);
	}
	void operator delete(void *p){
		return _mm_free(p);
	}
protected:
    DirectX::XMFLOAT3     m_vecPos;
    std::list<LPFIRE_PARTICLE_B> m_listFireB;
    FLOAT           m_fBufferedTime;
    FLOAT           m_fTime;
    DWORD           m_dwPhase;
    DWORD           m_dwNumParticles;
};

IBurnUp *IBurnUp::Create(DirectX::XMFLOAT3 *pVecPos){
	return new CBurnUp(pVecPos);
}

IBurnUp::~IBurnUp(){
}
