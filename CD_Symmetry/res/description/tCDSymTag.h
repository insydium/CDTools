#ifndef _tCDSymTag_H_
#define _tCDSymTag_H_

enum
{	
	SYM_USE_SYMMETRY			= 1000,
	SYM_SET_SYMMETRY			= 1001,
	SYM_RELEASE_SYMMETRY		= 1002,
	SYM_TOLERANCE				= 1003,
	
	SYM_SYMMETRY_AXIS			= 2000,
		SYM_MX					= 2001,
		SYM_MY					= 2002,
		SYM_MZ					= 2003,
		
	SYM_RESTRICT_SYM			= 2010,
	SYM_RESTRICT_AXIS			= 2011,
		SYM_POSITIVE			= 2012,
		SYM_NEGATIVE			= 2013,
		
	SYM_LOCK_CENTER				= 2020,

	SYM_SHOW_GUIDE				= 2030,
	SYM_GUIDE_SIZE				= 2031,
	SYM_GUIDE_COLOR				= 2032,

	SYM_PURCHASE				= 2100

};

#endif



