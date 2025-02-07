/*
 *  CDSymFuncLib.cpp
 *  CDSymmetry
 *
 *  Created by Dan Libisch on 11/20/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#include "CDSymFuncLib.h"

CDSymFuncLib* cdsyFLib_Cache = NULL;

CDSymFuncLib* CheckCDSymFuncLib(LONG offset)
{
	return (CDSymFuncLib*) CheckLib(ID_CDSYMFUNCLIB, offset, (C4DLibrary**) &cdsyFLib_Cache);
}

LONG* CDSymGetMirrorPoints(BaseTag *tag)
{
	CDSymFuncLib* cdsyFLib = CheckCDSymFuncLib(LIBOFFSET(CDSymFuncLib, CDSymGetMirrorPoints)); 
	if (!cdsyFLib || !cdsyFLib->CDSymGetMirrorPoints) return FALSE;
	
	return cdsyFLib->CDSymGetMirrorPoints(tag);
}

