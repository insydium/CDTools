#ifndef _TakeNode_H_
#define _TakeNode_H_

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

struct TakeNode
{
public:
	String		name;
	LONG		start;
	LONG		end;
	
	TakeNode()
	{
		name = "";
		start = 0;
		end = 0;
	}
	
	TakeNode(String n, LONG s, LONG e)
	{
		name = n;
		start = s;
		end = e;
	}

};


#endif