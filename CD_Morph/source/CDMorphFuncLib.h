/*
 *  CDMorphFuncLib.h
 *  CD_Morph
 *
 *  Created by Dan Libisch on 11/20/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#ifndef _CDMorphFuncLib_H_
#define _CDMorphFuncLib_H_

#include "c4d.h"

#include "CDMStructures.h"

CDMRefPoint* CDGetMorphReference(BaseTag *tag);
CDMRefPoint* CDGetMorphCache(BaseTag *tag);
CDMPoint* CDGetMorphPoints(BaseTag *tag);
CDMSpPoint* CDGetMorphSphericalPoints(BaseTag *tag);

// INTERNAL
const LONG ID_CDMORPHFUNCLIB		= 1031425;

struct CDMorphFuncLib : public C4DLibrary
{
	CDMRefPoint* (*CDGetMorphReference)(BaseTag *tag);
	CDMRefPoint* (*CDGetMorphCache)(BaseTag *tag);
	CDMPoint* (*CDGetMorphPoints)(BaseTag *tag);
	CDMSpPoint* (*CDGetMorphSphericalPoints)(BaseTag *tag);
};
// INTERNAL


#endif