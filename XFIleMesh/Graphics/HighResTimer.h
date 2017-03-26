/*
*	class CHighResTimer
*	QueryPerformanceCounter ���g�p����
*	�����x�Ȍv���@�\��񋟂���N���X�B
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
