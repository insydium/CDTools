/*
 *  CDJSFuncLib.h
 *  CDJointSkin
 *
 *  Created by Dan Libisch on 11/20/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#ifndef _CDJSFuncLib_H_
#define _CDJSFuncLib_H_

#include "c4d.h"
#include "lib_modeling.h"

#include "CDMStructures.h"
#include "CDJSStructures.h"

CDMRefPoint* CDGetSkinReference(BaseTag *tag);
CDMRefPoint* CDGetDeformReference(BaseTag *tag);
Bool CDGetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool CDSetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool CDRemapClusterWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map);
Bool CDGetJointWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG pCnt);
Bool CDSetJointWeight(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt);
Bool CDGetSkinJointColors(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt);
CDSkinVertex* CDGetSkinWeights(BaseTag *tag);

// INTERNAL
const LONG ID_CDJSFUNCLIB		= 1031426;

struct CDJSFuncLib : public C4DLibrary
{
	CDMRefPoint* (*CDGetSkinReference)(BaseTag *tag);
	CDMRefPoint* (*CDGetDeformReference)(BaseTag *tag);
	Bool (*CDGetClusterWeight)(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool (*CDSetClusterWeight)(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
	Bool (*CDRemapClusterWeights)(BaseTag *tag, BaseContainer *tData, TranslationMaps *map);
	Bool (*CDGetJointWeight)(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG pCnt);
	Bool (*CDSetJointWeight)(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt);
	Bool (*CDGetSkinJointColors)(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt);
	CDSkinVertex* (*CDGetSkinWeights)(BaseTag *tag);
};
// INTERNAL


#endif