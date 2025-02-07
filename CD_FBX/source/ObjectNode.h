#ifndef _ObjectNode_H_
#define _ObjectNode_H_

// c++ includes

// c4d includes
#include "ge_matrix.h"
#include "ge_math.h"

// CD includes
#include "CDCompatibility.h"
#include "CDArray.h"

// fbx sdk includes
#include "fbxsdk.h"

enum // node types
{
	NODE_TYPE_JOINT					= 0,
	NODE_TYPE_MESH,
	NODE_TYPE_LIGHT,
	NODE_TYPE_CAMERA,
	NODE_TYPE_CAMERA_PARENT,
	NODE_TYPE_NULL,
	NODE_TYPE_SPLINE
};

// structures
struct ImportProps
{
public:
	Bool pos;
	Bool rot;
	Bool sca;
	
	ImportProps()
	{
		pos = false;
		rot = false;
		sca = false;
	}
};

struct ObjectNode
{
public:
	LONG			nodeType;
	
	BaseObject		*object;
	KFbxNode		*node;
	
	KFbxXMatrix		matrix;
	Matrix			rotM;
	
	KFbxCluster		*cluster;
	
	Bool			skinned;
	Bool			pla;
	
	ImportProps		prop;
	KFbxVector4		oldRot;
	
	ObjectNode()
	{
		nodeType = -1;
		object = NULL;
		node = NULL;
		matrix = KFbxXMatrix();
		rotM = Matrix();
		cluster = NULL;
		skinned = false;
		pla = false;
		oldRot = KFbxVector4();
	}
	
	ObjectNode(LONG type, BaseObject *op, KFbxNode *n, KFbxXMatrix kM, Bool sk)
	{
		nodeType = type;
		object = op;
		node = n;
		matrix = kM;
		cluster = NULL;
		skinned = sk;
		pla = false;
		oldRot = KFbxVector4(0.0,0.0,0.0);
		rotM = Matrix();
	}
};

class ObjectNodeArray : public CDArray<ObjectNode>
{
public:
	ObjectNodeArray() {}
	~ObjectNodeArray() {}
	
	LONG GetIndex(BaseObject *op)
	{
		for(LONG i=0; i<data.size(); i++)
		{
			if(op == data[i].object) return i;
		}
		
		return -1;
	}
	
	LONG GetIndex(KFbxNode *nd)
	{
		for(LONG i=0; i<data.size(); i++)
		{
			if(nd == data[i].node) return i;
		}
		
		return -1;
	}
};


#endif