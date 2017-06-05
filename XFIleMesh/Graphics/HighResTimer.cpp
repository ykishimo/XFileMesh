//
//	@file HighResTimer.cpp
//  @brief Implementation of CHighResTimer.
//
//  @note High resolution timer with 64bit performance counter.
//	
//	@Reference URL
//	http://mrkk.ciao.jp/d3d2d/HighResTimer.html
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
