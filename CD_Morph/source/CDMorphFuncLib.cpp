/*
 *  CDMorphFuncLib.cpp
 *  CD_Morph
 *
 *  Created by Dan Libisch on 11/20/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#include "CDMorphFuncLib.h"

CDMorphFuncLib* cdmFLib_Cache = NULL;

CDMorphFuncLib* CheckCDMorphFuncLib(LONG offset)
{
	return (CDMorphFuncLib*) CheckLib(ID_CDMORPHFUNCLIB, offset, (C4DLibrary**) &cdmFLib_Cache);
}

CDMRefPoint* CDGetMorphReference(BaseTag *tag)
{
	CDMorphFuncLib* cdmFLib = CheckCDMorphFuncLib(LIBOFFSET(CDMorphFuncLib, CDGetMorphReference)); 
	if (!cdmFLib || !cdmFLib->CDGetMorphReference) return FALSE;
	
	return cdmFLib->CDGetMorphReference(tag);
}

CDMRefPoint* CDGetMorphCache(BaseTag *tag)
{
	CDMorphFuncLib* cdmFLib = CheckCDMorphFuncLib(LIBOFFSET(CDMorphFuncLib, CDGetMorphCache)); 
	if (!cdmFLib || !cdmFLib->CDGetMorphCache) return FALSE;
	
	return cdmFLib->CDGetMorphCache(tag);
}

CDMPoint* CDGetMorphPoints(BaseTag *tag)
{
	CDMorphFuncLib* cdmFLib = CheckCDMorphFuncLib(LIBOFFSET(CDMorphFuncLib, CDGetMorphPoints)); 
	if (!cdmFLib || !cdmFLib->CDGetMorphPoints) return FALSE;
	
	return cdmFLib->CDGetMorphPoints(tag);
}

CDMSpPoint* CDGetMorphSphericalPoints(BaseTag *tag)
{
	CDMorphFuncLib* cdmFLib = CheckCDMorphFuncLib(LIBOFFSET(CDMorphFuncLib, CDGetMorphSphericalPoints)); 
	if (!cdmFLib || !cdmFLib->CDGetMorphSphericalPoints) return FALSE;
	
	return cdmFLib->CDGetMorphSphericalPoints(tag);
}
