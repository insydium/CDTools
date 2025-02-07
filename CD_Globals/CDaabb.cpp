// 	Cactus Dan's AABB class
//	Copyright 2008 by Cactus Dan Libisch

#include "CDAABB.h"
#include "CDCompatibility.h"

void CDAABB::Empty()
{
	min.x = CDMAXREAL;
	min.y = CDMAXREAL;
	min.z = CDMAXREAL;

	max.x = CDMINREAL;
	max.y = CDMINREAL;
	max.z = CDMINREAL;
}

void CDAABB::AddPoint(const Vector &p)
{
	if(p.x < min.x) min.x = p.x;
	if(p.y < min.y) min.y = p.y;
	if(p.z < min.z) min.z = p.z;

	if(p.x > max.x) max.x = p.x;
	if(p.y > max.y) max.y = p.y;
	if(p.z > max.z) max.z = p.z;
}

Bool CDAABB::PointInBounds(const Vector &p)
{
	return p.x>min.x && p.y>min.y && p.z>min.z && p.x<max.x && p.y<max.y && p.z<max.z;
}

Bool CDAABB::IntersectionTest(CDAABB *b)
{
	if(max.x < b->min.x || min.x > b->max.x) return false;
	if(max.y < b->min.y || min.y > b->max.y) return false;
	if(max.z < b->min.z || min.z > b->max.z) return false;
	
	return true;
}

Vector CDAABB::ClosestPoint(const Vector &p) const
{
	Vector rPt;
	
	if(p.x < min.x) rPt.x = min.x;
	else if(p.x > max.x) rPt.x = max.x;
	else rPt.x = p.x;
	
	if(p.y < min.y) rPt.y = min.y;
	else if(p.y > max.y) rPt.y = max.y;
	else rPt.y = p.y;

	if(p.z < min.z) rPt.z = min.z;
	else if(p.z > max.z) rPt.z = max.z;
	else rPt.z = p.z;

	return rPt;	
}

Vector CDAABB::GetCenterPoint(void)
{
	Vector center;
	
	center.x = min.x + (max.x - min.x) * 0.5;
	center.y = min.y + (max.y - min.y) * 0.5;
	center.z = min.z + (max.z - min.z) * 0.5;
	
	return center;
}

