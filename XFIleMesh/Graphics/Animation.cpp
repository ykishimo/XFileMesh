//
//	Mesh Animation data modle
//
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <DirectXMath.h>
#include "Mesh.h"
#include "Animation.h"

#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }
#define SAFE_RELEASE(o) if(o){  (o)->Release(); o = NULL; }


Animation::Animation(){
	pSlaveFrameName = NULL;
	pRotations = NULL;
	pPositions = NULL;
	pScalings = NULL;
	pSlaveMeshFrame = NULL;
}

Animation::~Animation(){
	SAFE_DELETE(pSlaveFrameName);
	SAFE_DELETE(pRotations);
	SAFE_DELETE(pPositions);
	SAFE_DELETE(pScalings);
	pSlaveMeshFrame = NULL;
}


AnimationSet::AnimationSet(){
	this->pName = NULL;
	this->numAnimations = 0;
	this->totalFrames = 0;
	this->ppAnimations = NULL;
}

AnimationSet::~AnimationSet(){
	SAFE_DELETE(pName);
	if (ppAnimations != NULL){
		for (int i = 0; i < numAnimations ; ++i){
			SAFE_DELETE(ppAnimations[i]);
		}
		delete [] ppAnimations;
		ppAnimations = NULL;
	}
}
