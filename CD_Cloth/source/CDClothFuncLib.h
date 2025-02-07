/*
 *  CDClothFuncLib.h
 *  CDCloth
 *
 *  Created by Dan Libisch on 11/19/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */
#ifndef _CDClothFuncLib_H_
#define _CDClothFuncLib_H_

#include "c4d.h"

#include "CDAABB.h"
class CDClothCollider;

Bool CDClothGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool CDClothSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool CDRopeGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool CDRopeSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt);
CDAABB* CDColliderGetBounds(BaseTag *tag);
//Vector CDGetNearestSurfacePoint(BaseTag *tag, BaseObject *op, Vector pt, Vector &n);
Vector CDGetNearestSurfacePoint(CDClothCollider *cldTag, BaseObject *op, Vector pt, Vector &n);

// INTERNAL
const LONG ID_CDCLOTHFUNCLIB		= 1031423;

struct CDClothFuncLib : public C4DLibrary
{
	Bool (*CDClothGetWeight)(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool (*CDClothSetWeight)(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool (*CDRopeGetWeight)(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool (*CDRopeSetWeight)(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt);
	CDAABB* (*CDColliderGetBounds)(BaseTag *tag);
	//Vector (*CDGetNearestSurfacePoint)(BaseTag *tag, BaseObject *op, Vector pt, Vector &n);
	Vector (*CDGetNearestSurfacePoint)(CDClothCollider *cldTag, BaseObject *op, Vector pt, Vector &n);
};
// INTERNAL


#endif