#ifndef _CDMessages_H_
#define _CDMessages_H_

#if API_VERSION > 14999
	#include "CDLegacy.h"
#endif

struct CDPoseCopyData
{
	LONG			pInd;
	BaseTag 		*hTag;
};

struct CDTransMatrixData
{
	Matrix		nM;
	Matrix		oM;
};

#define CD_MSG_UPDATE						1019461		// generic update
#define CD_MSG_SCALE_CHANGE					1019589		// 
#define CD_MSG_FREEZE_TRANSFORMATION		1019462		// send scale vector
#define CD_MSG_POSE_COPY					1023403		// send CDPoseCopyData
#define CD_MSG_RECALC_REFERENCE				1024116		// send CDTransMatrixData
#define CD_MSG_NORMALIZE_WEIGHT				1024336
#define CD_MSG_SELECTION_UPDATE				1024747		//				
#define CD_MSG_SYMMETRY_UPDATE				1024816
#define CD_MSG_FINGER_THUMB_UPDATE			1025521

#endif