#ifndef _CDAlignCam_H_
#define _CDAlignCam_H_

#include "CDCompatibility.h"

#define ID_CDALIGNTOCAMERA			1023658
#define ID_CDATCMESSAGE				1023673

struct CDListData
{
	CDListData() { list = NULL; }
	AtomArray *list;
};

#endif



