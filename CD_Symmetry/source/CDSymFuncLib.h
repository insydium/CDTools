/*
 *  CDSymFuncLib.h
 *  CDSymmetry
 *
 *  Created by Dan Libisch on 11/20/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#ifndef _CDSymFuncLib_H_
#define _CDSymFuncLib_H_

#include "c4d.h"

LONG* CDSymGetMirrorPoints(BaseTag *tag);

// INTERNAL
const LONG ID_CDSYMFUNCLIB		= 1031427;

struct CDSymFuncLib : public C4DLibrary
{
	LONG* (*CDSymGetMirrorPoints)(BaseTag *tag);
};
// INTERNAL


#endif