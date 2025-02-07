#ifndef _CDMStructures_H_
#define _CDMStructures_H_

// c4d includes
#include "c4d.h"

//Point storage struct for morph ref tags
struct CDMRefPoint
{
public:
	Real		x;
	Real		y;
	Real		z;
	
	Vector GetVector(void) { return Vector(x,y,z); }
	void SetVector(const Vector v) { x = v.x; y = v.y; z = v.z; }
};

// Point storage struct for morph tags
struct CDMPoint
{
public:
	LONG		ind;
	Real		x;
	Real		y;
	Real		z;
	
	Vector GetVector(void) { return Vector(x,y,z); }
	void SetVector(const Vector v) { x = v.x; y = v.y; z = v.z; }
	
	Vector GetDelta(Vector v) { return Vector(v.x+x, v.y+y, v.z+z); }
	void SetDelta(const Vector v, const Vector r) { x = v.x-r.x; y = v.y-r.y; z = v.z-r.z; }
};

struct CDMSpPoint
{
public:
	CDMPoint		normal;
	CDMPoint		center;
};

struct CDMData
{
	CDMData(){list=NULL;}
	AtomArray *list;
};


#endif
