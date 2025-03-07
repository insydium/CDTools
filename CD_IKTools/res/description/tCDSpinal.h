#ifndef _tCDSpinal_H_
#define _tCDSpinal_H_

enum
{	
	SPN_PURCHASE				= 1000,
	
	SPN_USE						= 1001,
	
	SPN_POLE_AXIS				= 1002,
		SPN_POLE_X				= 1003,
		SPN_POLE_Y				= 1004,
		SPN_POLE_Z				= 1005,
		SPN_POLE_NX				= 1006,
		SPN_POLE_NY				= 1007,
		SPN_POLE_NZ				= 1008,

	SPN_BONES_IN_GROUP			= 1010,

	SPN_CONNECT_BONES			= 1012,
	SPN_CONNECT_NEXT			= 1013,

	SPN_BASE_LINK				= 1015,
	SPN_BASE_DEPTH				= 1016,
	SPN_TIP_LINK				= 1017,
	SPN_TIP_DEPTH				= 1018,
	SPN_TIP_TWIST				= 1019,
	SPN_BASE_TWIST				= 1020,
	
	SPN_SHOW_LINES				= 1021,
	SPN_LINE_COLOR				= 1022,
	SPN_BLEND					= 1023,
	
	SPN_CONTROL_GROUP			= 2000,
	SPN_USE_TARGET_TWIST		= 2001,
	SPN_INCLUDE_TIP_TWIST		= 2002,
	SPN_INTERPOLATION			= 2003,
		SPN_SHORTEST			= 2004,
		SPN_AVERAGE				= 2005,
		SPN_LONGEST				= 2006,
	
	SPN_STRETCH_GROUP			= 3000,
	SPN_SET_LENGTH				= 3001,
	SPN_CLEAR_LENGTH			= 3002,
	
	SPN_SQUASH_N_STRETCH		= 3004,
	SPN_CLAMP_SQUASH			= 3005,
	SPN_SQUASH_DIST				= 3006,
	SPN_SET_SQUASH_DIST			= 3007,
	SPN_CLAMP_STRETCH			= 3008,
	SPN_STRETCH_DIST			= 3009,
	SPN_SET_STRETCH_DIST		= 3010,
	
	SPN_CHANGE_VOLUME			= 3013,
	SPN_USE_BIAS_CURVE			= 3014,
	SPN_BIAS_CURVE				= 3015,
	SPN_MIX_VOLUME				= 3016,
	SPN_DEPTH_S_N_S				= 3017,
	SPN_SQUASH_DEPTH			= 3018,
	SPN_STRETCH_DEPTH			= 3019,
	SPN_RESET_LENGTH			= 3020,
	SPN_VOLUME_STRENGTH			= 3022
	
};

#endif



