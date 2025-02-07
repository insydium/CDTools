/*
 *  CDTypes.h
 *
 *  Created by Cactus Dan on 2/28/14.
 *  Copyright 2014 Libisch Graphic Design. All rights reserved.
 *
 */

#if API_VERSION < 15000
	typedef LONG			CDLong;
	typedef Real			CDFloat;
	typedef LReal			CDDouble;
#else
	typedef Int32			CDLong;
	typedef Float			CDFloat;
	typedef Float64			CDDouble;
#endif