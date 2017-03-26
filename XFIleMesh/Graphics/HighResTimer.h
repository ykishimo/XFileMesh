/*
*	class CHighResTimer
*	QueryPerformanceCounter を使用した
*	高精度な計時機能を提供するクラス。
*
*/

#ifndef	__HIGHRESTIMER_H__
#define	__HIGHRESTIMER_H__

class CHighResTimer  
{
public:
	CHighResTimer();
	virtual ~CHighResTimer();
	void	Reset();
	double	GetElapsedTime();
	double	GetElapsedTimeAndReset();
private:
	double	m_dbFrequency;
	LARGE_INTEGER	m_liCounter;
};

#endif
