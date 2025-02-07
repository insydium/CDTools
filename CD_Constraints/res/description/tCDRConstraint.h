#ifndef _tCDRConstraint_H_
#define _tCDRConstraint_H_

enum
{
	RC_PURCHASE			= 1000,
	
	RC_OFFSETS				= 1009,
	
	RC_AB_MIX				= 1012,
		
	RC_STRENGTH			= 1013,
	RC_SHOW_LINES			= 1014,
	RC_LINE_COLOR			= 1015,

	RC_AXIS_X				= 1016,
	RC_OFFSET_X			= 1017,
	RC_AXIS_Y				= 1018,
	RC_OFFSET_Y			= 1019,
	RC_AXIS_Z				= 1020,
	RC_OFFSET_Z			= 1021,
	
	RC_LOCAL_ROT			= 1022,
	RC_USE_AB_MIX			= 1023,
	
	RC_ADD_ROT				= 1031,
	RC_SUB_ROT				= 1032,
	
	RC_INTERPOLATION		= 1040,
		RC_AVERAGE			= 1041,
		RC_SHORTEST		= 1042,					
	
	RC_ID_TARGET			= 1050,
	
	RC_LINK_GROUP			= 2000,
	RC_LINE_ADD			= 3000,

	RC_TARGET				= 4000,
	
	RC_ROT_MIX				= 5000

};

#endif