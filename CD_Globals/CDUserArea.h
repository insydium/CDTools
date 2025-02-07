//
//  CDUserArea.h
//	R9 to R12+ backward compatible tag data class
//  Created by Dan Libisch on 8/3/13.
//

#ifndef _CDUSERAREA_H_
#define _CDUSERAREA_H_

#include "c4d.h"
#include "c4d_gui.h"

#include "CDCompatibility.h"

class CDUserArea : public GeUserArea
{
public:
	CDUserArea();
	virtual ~CDUserArea();

	virtual void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
	
#if API_VERSION < 9800
	virtual void Draw(LONG x1,LONG y1,LONG x2,LONG y2)
	{ CDDraw(x1, y1, x2, y2); }
#else
	virtual void DrawMsg(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg)
	{ CDDraw(x1, y1, x2, y2, msg); }
#endif
};


#endif
