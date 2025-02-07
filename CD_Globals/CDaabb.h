
#ifndef _CDaabb_H_
#define _CDaabb_H_

#include "c4d_tools.h"
#include "ge_matrix.h"
#include "ge_math.h"

class CDAABB
{
public:
	Vector	min; //Minimum AABB value
	Vector	max; //Maximum AABB value
	
	void Empty(); //Empty the polygon AABB
	void AddPoint(const Vector &p); //Expand the AABB to include point
	
	Bool PointInBounds(const Vector &p); // Test if point is withing the AABB bounds
	Bool IntersectionTest(CDAABB *b);
	Vector ClosestPoint(const Vector &p) const; //Return the closest point on the box to another point
	Vector GetCenterPoint(void);
	
};


#endif

