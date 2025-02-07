/*
 *  CDClothFuncLib.cpp
 *  CDCloth
 *
 *  Created by Dan Libisch on 11/19/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#include "CDClothFuncLib.h"

CDClothFuncLib* cdcFLib_Cache = NULL;

CDClothFuncLib* CheckCDClothFuncLib(LONG offset)
{
	return (CDClothFuncLib*) CheckLib(ID_CDCLOTHFUNCLIB, offset, (C4DLibrary**) &cdcFLib_Cache);
}

Bool CDClothGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClothFuncLib* cdcFLib = CheckCDClothFuncLib(LIBOFFSET(CDClothFuncLib, CDClothGetWeight)); 
	if (!cdcFLib || !cdcFLib->CDClothGetWeight) return FALSE;
	
	return cdcFLib->CDClothGetWeight(tag, tData, tWeight, tCnt);
}

Bool CDClothSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClothFuncLib* cdcFLib = CheckCDClothFuncLib(LIBOFFSET(CDClothFuncLib, CDClothSetWeight)); 
	if (!cdcFLib || !cdcFLib->CDClothSetWeight) return FALSE;
	
	return cdcFLib->CDClothSetWeight(tag, op, tData, tWeight, tCnt);
}

Bool CDRopeGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClothFuncLib* cdcFLib = CheckCDClothFuncLib(LIBOFFSET(CDClothFuncLib, CDRopeGetWeight)); 
	if (!cdcFLib || !cdcFLib->CDRopeGetWeight) return FALSE;
	
	return cdcFLib->CDRopeGetWeight(tag, tData, tWeight, tCnt);
}

Bool CDRopeSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClothFuncLib* cdcFLib = CheckCDClothFuncLib(LIBOFFSET(CDClothFuncLib, CDRopeSetWeight)); 
	if (!cdcFLib || !cdcFLib->CDRopeSetWeight) return FALSE;
	
	return cdcFLib->CDRopeSetWeight(tag, op, tData, tWeight, tCnt);
}

CDAABB* CDColliderGetBounds(BaseTag *tag)
{
	CDClothFuncLib* cdcFLib = CheckCDClothFuncLib(LIBOFFSET(CDClothFuncLib, CDColliderGetBounds)); 
	if (!cdcFLib || !cdcFLib->CDColliderGetBounds) return FALSE;
	
	return cdcFLib->CDColliderGetBounds(tag);
}

//Vector CDGetNearestSurfacePoint(BaseTag *tag, BaseObject *op, Vector pt, Vector &n)
Vector CDGetNearestSurfacePoint(CDClothCollider *cldTag, BaseObject *op, Vector pt, Vector &n)
{
	CDClothFuncLib* cdcFLib = CheckCDClothFuncLib(LIBOFFSET(CDClothFuncLib, CDGetNearestSurfacePoint)); 
	if (!cdcFLib || !cdcFLib->CDGetNearestSurfacePoint) return FALSE;
	
	return cdcFLib->CDGetNearestSurfacePoint(cldTag, op, pt, n);
}
