#ifndef _NgonUVW_H_
#define _NgonUVW_H_

// c++ includes
#include <vector>
using std::vector;

// c4d includes
#include "ge_matrix.h"
#include "ge_math.h"

// CD includes
#include "CDCompatibility.h"

// fbx sdk includes
#include "fbxsdk.h"

struct NgonUVW
{
public:
	LONG		kPolyID;
	LONG		ngonID;
	
	NgonUVW()
	{
		ngonID = -1;
		kPolyID = -1;
	}
	
	NgonUVW(LONG nid, LONG pid)
	{
		ngonID = nid;
		kPolyID = pid;
	}
};


#endif