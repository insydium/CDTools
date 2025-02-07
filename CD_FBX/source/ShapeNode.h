#ifndef _ShapeNode_H_
#define _ShapeNode_H_

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

struct ShapeNode
{
	BaseTag		*mtag;
	KFbxShape	*shape;
	
	String		name;

	ShapeNode()
	{
		mtag = NULL;
		shape = NULL;
		name = "";
	}
	
	ShapeNode(BaseTag *t, KFbxShape *sh, String n)
	{
		mtag = t;
		shape = sh;
		name = n;
	}
};


#endif