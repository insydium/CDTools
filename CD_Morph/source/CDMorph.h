//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch

#ifndef _CDMorph_H_
#define _CDMorph_H_

// c4d includes
#include "c4d.h"
#include "lib_modeling.h"

// CD includes
#include "CDaabb.h"
#include "CDSelLog.h"
#include "CDMessages.h"
#include "CDGeneral.h"
#include "CDCompatibility.h"
#include "CDReg.h"
#include "CDUserArea.h"

// CDM includes
#include "CDMorphRef.h"
#include "CDMorphTag.h"
#include "CDMStructures.h"
#include "CDMorphFuncLib.h"

// CDSY includes
#include "CDSymmetryTag.h"
#include "CDSymFuncLib.h"

// CDJS includes
#include "CDJSStructures.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"
#include "CDJSFuncLib.h"

// compatibility includes
#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

// CD Morph ID
#define ID_CDMORPH					1017235

//#define ID_CDMLOGOBAR				1022487	// No longer used.
//#define ID_ADDMORPHREFERENCE		1017557	// No longer used.

// Tag ID's
#define ID_CDMORPHMIXPLUGIN			1017236
#define ID_CDMORPHTAGPLUGIN			1017237
#define ID_CDMORPHREFPLUGIN			1017273
#define ID_CDMORPHSLIDERPLUGIN		1017387
#define ID_CDSYMMETRYTAG			1020413

// Command ID's
#define ID_ABOUTCDMORPH 			1017238
#define ID_SETMORPHSELECTION 		1017558
#define ID_SELECTDIFFERENCE 		1017559
#define ID_SETHOMEPOSITION 			1017560
#define ID_CDCREATEMTAGPLUGIN		1018644
#define ID_CDMIRRORMTAGPLUGIN		1018645
#define ID_CDCREATEMOBJPLUGIN		1018646
#define ID_CDMMIXTOOBJPLUGIN		1018983
#define ID_CDMMIXTOTAGPLUGIN		1021018
#define ID_CDMMIXSUBTOTAG			1021552
#define ID_CDMORPHCONVERT			1023868
#define ID_CDMORPHSPLIT				1025581

// Tool ID's
//#define	ID_CDMORPHBLEND				1025582  // not used

// Scene Hook ID's
#define ID_CDMSCENEHOOK				1025585

// Message ID's
#define ID_CDMRMESSAGEPLUGIN		1019519

// External ID's
#define ID_CDSKINPLUGIN				1019251
#define ID_CDJOINTOBJECT			1019254
#define ID_CDSKINREFPLUGIN			1019291
#define ID_CDZEROTRANSTAG			1022137
#define ID_CDSYMMETRYASSIGN			1022979
#define ID_ABOUTCDSYMMETRY	 		1023297

#define C4D_SERIAL_SIZE				12
#define CDM_SERIAL_SIZE			27

// user area classes
class CDMorphAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

class CDMOptionsUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	LONG					sx;
	
public:
	virtual Bool GetMinSize(LONG& w, LONG& h);
	virtual void Sized(LONG w, LONG h);
	
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);
void ConvertToPointSelection(BaseSelect *bs, BaseSelect *ptS, BaseObject *op, LONG mode);

Vector CDMCalcFaceNormal(CDMRefPoint *refadr, const CPolygon &ply);
Vector CDMCalcPolygonCenter(CDMRefPoint *refadr, const CPolygon &ply);
Vector CDMCalcPointNormal(BaseObject *op, CDMRefPoint *refadr, LONG ind);

// sn functions
Bool RegisterCDM(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);


#endif

