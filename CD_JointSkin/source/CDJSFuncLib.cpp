/*
 *  CDJSFuncLib.cpp
 *  CDJointSkin
 *
 *  Created by Dan Libisch on 11/20/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#include "CDJSFuncLib.h"

CDJSFuncLib* cdjsFLib_Cache = NULL;

CDJSFuncLib* CheckCDJSFuncLib(LONG offset)
{
	return (CDJSFuncLib*) CheckLib(ID_CDJSFUNCLIB, offset, (C4DLibrary**) &cdjsFLib_Cache);
}

CDMRefPoint* CDGetSkinReference(BaseTag *tag)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDGetSkinReference)); 
	if (!cdjsFLib || !cdjsFLib->CDGetSkinReference) return FALSE;
	
	return cdjsFLib->CDGetSkinReference(tag);
}

CDMRefPoint* CDGetDeformReference(BaseTag *tag)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDGetDeformReference)); 
	if (!cdjsFLib || !cdjsFLib->CDGetDeformReference) return FALSE;
	
	return cdjsFLib->CDGetDeformReference(tag);
}

Bool CDGetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDGetClusterWeight)); 
	if (!cdjsFLib || !cdjsFLib->CDGetClusterWeight) return FALSE;
	
	return cdjsFLib->CDGetClusterWeight(tag, tData, tWeight, tCnt);
}

Bool CDSetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDSetClusterWeight)); 
	if (!cdjsFLib || !cdjsFLib->CDSetClusterWeight) return FALSE;
	
	return cdjsFLib->CDSetClusterWeight(tag, tData, tWeight, tCnt);
}

Bool CDRemapClusterWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDRemapClusterWeights)); 
	if (!cdjsFLib || !cdjsFLib->CDRemapClusterWeights) return FALSE;
	
	return cdjsFLib->CDRemapClusterWeights(tag, tData, map);
}

Bool CDGetJointWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG pCnt)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDGetJointWeight)); 
	if (!cdjsFLib || !cdjsFLib->CDGetJointWeight) return FALSE;
	
	return cdjsFLib->CDGetJointWeight(tag, tData, tWeight, pCnt);
}

Bool CDSetJointWeight(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDSetJointWeight)); 
	if (!cdjsFLib || !cdjsFLib->CDSetJointWeight) return FALSE;
	
	return cdjsFLib->CDSetJointWeight(tag, doc, tData, tWeight, pCnt);
}

Bool CDGetSkinJointColors(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDGetSkinJointColors)); 
	if (!cdjsFLib || !cdjsFLib->CDGetSkinJointColors) return FALSE;
	
	return cdjsFLib->CDGetSkinJointColors(tag, doc, tData, jColor, pCnt);
}

CDSkinVertex* CDGetSkinWeights(BaseTag *tag)
{
	CDJSFuncLib* cdjsFLib = CheckCDJSFuncLib(LIBOFFSET(CDJSFuncLib, CDGetSkinWeights)); 
	if (!cdjsFLib || !cdjsFLib->CDGetSkinWeights) return FALSE;
	
	return cdjsFLib->CDGetSkinWeights(tag);
}
