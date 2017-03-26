//
//	HighResTimer.cpp: CHighResTimer �N���X�̃C���v�������e�[�V����
//
//		64 �r�b�g�V�X�e���J�E���^��p���������x�^�C�}�[��
//		�񋟂���B
//	
//	�ڍׂ́A���L�t�q�k���Q��
//	http://www5b.biglobe.ne.jp/~u-hei/d3d2d/HighResTimer.html
//
//
#include "stdafx.h"
#include "HighResTimer.h"

CHighResTimer::CHighResTimer()
{
    Reset();
}

CHighResTimer::~CHighResTimer()
{

}

void    CHighResTimer::Reset()
{
    LARGE_INTEGER   liFreq;
	double			i60 = 60.0;
    ::QueryPerformanceFrequency(&liFreq);
	m_dbFrequency = i60 / ((double)liFreq.QuadPart);
    ::QueryPerformanceCounter(&m_liCounter);
}

double    CHighResTimer::GetElapsedTime()
{
    LARGE_INTEGER  tmp;
    double          frame;
	::QueryPerformanceCounter(&tmp);
	
	frame = (double)(tmp.QuadPart - m_liCounter.QuadPart);
	frame *= m_dbFrequency;
	return frame;
}

double    CHighResTimer::GetElapsedTimeAndReset()
{
    LARGE_INTEGER  tmp;
    double         frame;
    ::QueryPerformanceCounter(&tmp);

	frame = (double)(tmp.QuadPart - m_liCounter.QuadPart);
	frame *= m_dbFrequency;
	
	m_liCounter.QuadPart = tmp.QuadPart    ;
    return    frame;
}
