//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#ifndef _CDSymmetry_H_
#define _CDSymmetry_H_

// c4d includes
#include "c4d_tools.h"
#include "ge_matrix.h"
#include "ge_math.h"
#include "lib_modeling.h"

// CD includes
#include "CDCompatibility.h"
#include "CDSelLog.h"
#include "CDaabb.h"
#include "CDMessages.h"
#include "CDGeneral.h"
#include "CDReg.h"
#include "CDUserArea.h"

// CDSY includes
#include "CDSymmetryTag.h"
#include "CDSymFuncLib.h"


//#define EPSILON 0.00001

// CD Symmetry ID
#define ID_CDSYMMETRYTOOLS			1023295

//#define ID_CDSYMLOGOBAR				1023298

// Tag ID's
#define ID_CDSYMMETRYTAG			1020413
#define ID_CDCENTERLOCKTAG			1024790

// Object ID's
#define ID_CDDYNSYMMETRY			1024166

// Command ID's
#define ID_ABOUTCDSYMMETRY	 		1023297
#define ID_CDMIRRORSELECTION		1022981
#define ID_CDADDSYMMETRYTAG			1022274
#define ID_CDSYMMETRYTOTAG			1023527
#define ID_CDTAGTOSYMMETRY			1023720
#define ID_CDACTIVATENEGATIVE		1024198
#define ID_CDACTIVATEPOSITIVE		1024199
#define ID_CDSYMMETRYUPDATE			1024691
#define ID_CDSYMMETRYASSIGN			1022979
#define ID_MAKESYMMETRICAL			1024817

// Tool ID's
#define ID_CDSYMMETRYSELECT			1022980

// Message ID's
#define ID_CDSYMMESSAGE				1023296

#define C4D_SERIAL_SIZE				12
#define CDSY_SERIAL_SIZE			27

// knife tool modeling command data ID's
#define OLD_MDATA_KNIFE_P1 					2110 // VECTOR
#define OLD_MDATA_KNIFE_V1 					2111 // VECTOR
#define OLD_MDATA_KNIFE_P2 					2112 // VECTOR
#define OLD_MDATA_KNIFE_V2 					2113 // VECTOR
#define OLD_MDATA_KNIFE_RESTRICT 			2114 //Bool
#define OLD_MDATA_KNIFE_ANGLE 				2115 // Real

// CD Center Lock tag
enum
{	
	CL_SYMMETRY_AXIS				= 1001,
	CL_TOLERANCE					= 1002,
	CL_LOCK_ON						= 1003
};

struct CDSData
{
	CDSData(){list=NULL;}
	AtomArray *list;
};

// user area classes
class CDSymmetryAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

class CDSymOptionsUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	LONG					sx;
	
public:
	virtual Bool GetMinSize(LONG& w, LONG& h);
	virtual void Sized(LONG w, LONG h);
	
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

// common Functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);

void CDOptimize(BaseDocument *doc, BaseObject *op, Real t, Bool pt, Bool ply, Bool un, LONG mode);

void StoreBrigeToolData(BaseDocument *doc);
void RestoreBrigeToolData(BaseDocument *doc);

LONG GetEdgeSymmetry(BaseObject *op, LONG *sym, LONG ind);
LONG GetPolySymmetry(BaseObject *op, LONG *sym, LONG ind);
void ConvertToPointSelection(BaseSelect *bs, BaseSelect *ptS, BaseObject *op, LONG mode);

BaseObject* GetDynSymParentObject(BaseObject *op);
void CacheToMesh(BaseDocument *doc, BaseObject *pr, BaseObject *op);

// sn functions
Bool RegisterCDSY(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);

#endif

