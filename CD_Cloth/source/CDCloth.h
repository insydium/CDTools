//	Cactus Dan's Cloth
//	Copyright 2013 by Cactus Dan Libisch
//	CDCloth.h

#ifndef _CDCLOTH_H_
#define _CDCLOTH_H_


//c4d includes
#include "ge_matrix.h"
#include "c4d_tools.h"
#include "ge_math.h"
#include "lib_modeling.h"
#include "customgui_priority.h"

// CD includes
#include "CDSelLog.h"
#include "CDQuaternion.h"
#include "CDaabb.h"
#include "CDGeneral.h"
#include "CDUserArea.h"
#include "CDReg.h"

#include "CDClothFuncLib.h"
#include "CDJSFuncLib.h"


// compatibility includes
#include "CDCompatibility.h"
#if API_VERSION < 9800
#include "R9Animation.h"
#else
#include "R10Animation.h"
#endif

// CD external includes
#include "CDJSStructures.h"

#define C4D_SERIAL_SIZE				12
#define CDCL_SERIAL_SIZE			27

// CD Cloth ID
#define ID_CDCLOTH					1030924

// Command ID's
#define ID_ABOUTCDCLOTH				1030925
#define ID_CDADDCLOTHCOMMAND		1030956
#define ID_CDREMOVECLOTH			1030962
#define ID_CDADDROPECOMMAND			1030957

// Tag ID's
#define ID_CDCLOTHTAG				1030926
#define ID_CDROPETAG				1030927
#define ID_CDCLOTHCOLLIDER			1030928

// Message ID's
#define ID_CDCLOTHMESSAGE			1030929

// External ID's
#define ID_SKINWEIGHTTOOL			1019253
#define ID_CDSKINPLUGIN				1019251
#define ID_CDCLUSTERTAGPLUGIN		1018751
#define ID_CDSKINREFPLUGIN			1019291
#define ID_CDJOINTOBJECT			1019254

#define ID_CDMORPHREFPLUGIN			1017273

struct CDCLData
{
	CDCLData(){list=NULL;}
	AtomArray *list;
};

// user area classes
class CDClothAboutUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
	
};

class CDClothOptionsUA : public CDUserArea
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


// sn functions
Bool RegisterCDCL(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);

#endif
