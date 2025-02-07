#ifndef _oCDDynSym_H_
#define _oCDDynSym_H_

enum
{
	DS_PURCHASE					= 1000,

	DS_SHOW_GUIDE				= 1001,
	DS_GUIDE_SIZE				= 1002,
	DS_GUIDE_COLOR				= 1003,
	
	DS_SYMMETRY_AXIS			= 1010,
		DS_MX					= 1011,
		DS_MY					= 1012,
		DS_MZ					= 1013,
		
	DS_TOLERANCE				= 1020,
	
	DS_ACTIVE_SYMMETRY			= 1030,
		DS_POSITIVE				= 1031,
		DS_NEGATIVE				= 1032,
		
	DS_AUTO_UPDATE				= 2000,
	DS_LOCK_CENTER				= 2001,
	DS_KEEP_SEL					= 2002,
	DS_KEEP_UVW					= 2003
};

#endif
