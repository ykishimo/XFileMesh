#pragma once
#include "IMesh.h"
struct MeshFrame;
class CMeshFrameTracker : public IMeshFrameTracker
{
public:
	CMeshFrameTracker(MeshFrame *pFrame);
	virtual ~CMeshFrameTracker(void);
	void	GetTransform(DirectX::XMMATRIX *pMatrix) override;
	static MeshFrame *FindFrame(MeshFrame *pFrame, CHAR *pName);
protected:
	MeshFrame *m_pFrame;
};

