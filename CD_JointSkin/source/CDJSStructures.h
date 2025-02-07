#ifndef _CDJSStructures_H_
#define _CDJSStructures_H_

// c4d includes
#include "c4d.h"

// CD includes
#include "CDCompatibility.h"
#include "CDArray.h"

struct CDClusterWeight
{
public:
	LONG		ind;	// vertex index
	Real		w;		// vertex weight
};

struct CDSkinVertex
{
public:
	Real				taw;	// total accumulated weight affecting vertex
	LONG				jn;		// number of joints with weight > 0
	CDArray<Real>		jw;		// pointer to joint weight array
	CDArray<LONG>		jPlus;	// pointer to joint index array
	//Real				*jw;	// pointer to joint weight array
	//LONG				*jPlus;	// pointer to joint index array
};

struct CDJTransM
{
	Matrix			m;
	Bool			base;
	Real			stretch;
	Real			squash;
	Bool			useSpline;
	Real			splStart;
	Real			splEnd;
	Real			length;
	Real			volume;
	SplineData		*spline;
	BaseObject		*jnt;
	Matrix			trnsM;
};

struct CDJSData
{
	CDJSData(){list=NULL;}
	AtomArray *list;
};


#endif

