#ifndef _MaterialNode_H_
#define _MaterialNode_H_

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

struct MaterialNode
{
public:
	BaseMaterial	*mat;
	String			name;
	
	MaterialNode()
	{
		mat = NULL;
		name = "";
	}
	
	MaterialNode(BaseMaterial *m, String n)
	{
		mat = m;
		name = n;
	}
};


#endif