#ifndef _tCDClothCollider_H_
#define _tCDClothCollider_H_

enum
{
	CLD_COLLIDER_TYPE		= 1000,
		CLD_PLANE			= 0,
		CLD_SPHERE			= 1,
		CLD_JOINT			= 2,
		CLD_POLYGON			= 3,
	
	CLD_RADIUS				= 1001,
	
	CLD_PLANE_NORM			= 1002,
		CLD_NORM_XP			= 0,
		CLD_NORM_XN			= 1,
		CLD_NORM_YP			= 2,
		CLD_NORM_YN			= 3,
		CLD_NORM_ZP			= 4,
		CLD_NORM_ZN			= 5,
	
	CLD_OFFSET				= 1003,
	
	CLD_SELECTION_GROUP		= 1100,
	CLD_SET_SELECTION		= 1101,
	CLD_EDIT_SELECTION		= 1102,
	CLD_RESTORE_SELECTION	= 1103,
	
	CLD_PURCHASE				= 2100
};

#endif



