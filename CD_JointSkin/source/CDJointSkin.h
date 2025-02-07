#ifndef _CDJointSkin_H_
#define _CDJointSkin_H_


//c4d includes
#include "ge_matrix.h"
#include "c4d_tools.h"
#include "ge_math.h"
#include "lib_modeling.h"
#include "customgui_priority.h"

// CD includes
#include "CDSelLog.h"
#include "CDQuaternion.h"
#include "CDMessages.h"
#include "CDGeneral.h"
#include "CDUserArea.h"
#include "CDReg.h"

// CDJS includes
#include "CDJSStructures.h"
#include "CDJSFuncLib.h"

// CDM includes
//#include "CDMStructures.h"
#include "CDMorphRef.h"
#include "CDMorphFuncLib.h"

// CDSY includes
#include "CDSymmetryTag.h"
#include "CDSymFuncLib.h"

// compatibility includes
#include "CDCompatibility.h"
#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

// Serial number constants
#define C4D_SERIAL_SIZE				12
#define CDJS_SERIAL_SIZE			27

// CD Joints & Skin ID
#define ID_CDJOINTSANDSKIN			1019258

// Tag ID's
#define ID_CDCLUSTERTAGPLUGIN		1018751
#define ID_CDSKINPLUGIN				1019251
#define ID_CDSKINREFPLUGIN			1019291
#define ID_CDBINDPOSETAG			1019257

// Command ID's
#define ID_ABOUTCDJOINTSKIN 		1019259
#define ID_ADDSKINCOMMAND			1019252
#define ID_CDJOINTMIRRORPLUGIN 		1019256
#define ID_CDORIENTJOINTS	 		1019301
#define ID_CDADDJOINTS	 			1019329
#define ID_CDDELETEJOINTS	 		1019330
#define ID_CDMIRRORSKINWEIGHT	 	1019333
#define ID_CDMIRRORSKINCLUSTER	 	1019380
#define ID_CDCONVERTTOCDJOINTS		1019481
#define ID_CDCONVERTFROMCDJOINTS	1019564
#define ID_CDCONVERTTOCDSKIN		1019562
#define ID_CDCONVERTFROMCDSKIN		1019563
#define ID_CDFREEZETRANSFORMATION	1019577
#define ID_CDLINKOBJECTS			1021092
#define ID_CDUNLINKOBJECTS			1021093
#define ID_CDJOINTXRAYTOGGLE		1021350
#define ID_CDCOLORIZEJOINTS			1021351
#define ID_CDJOINTSTOPOLYGONS		1021352
#define ID_CDADDSKINCLUSTER			1021379
#define ID_CDJOINTPROXYTOGGLE		1021381
#define ID_CDREMOVESKIN				1021921
#define ID_CDJOINTDISPLAYSIZE		1022228
#define ID_CDJOINTMIRRORASSIGN		1022269
#define ID_CDNORMALIZEALLWEIGHT		1022789
#define ID_CDTOGGLEPAINTMODE		1024028
#define ID_CDMERGESKIN				1024185
#define ID_CDFREEZESKINSTATE		1024295
#define ID_CDJOINTENVTOGGLE			1024336
#define ID_CDREROOTJOINTS			1024612
#define ID_CDTRANSFERSKIN			1028464

// Tool ID's
#define ID_CDJOINTTOOLPLUGIN		1019255
#define ID_SKINWEIGHTTOOL			1019253

// Object ID's
#define ID_CDJOINTOBJECT			1019254
#define ID_CDRIGGUIDE				1026646

// Scene Hook ID's
#define ID_CDJSSCENEHOOK			1021336

// Message ID's
#define ID_JSMESSAGEPLUGIN			1019424

// External ID's
#define ID_CDIKTOOLS				1016043
#define ID_ABOUTCDIKTOOLS			1016034

#define ID_CDMORPHREFPLUGIN			1017273
#define ID_CDMORPHTAGPLUGIN			1017237
#define ID_CDMORPHSLIDERPLUGIN		1017387
#define ID_CDMIRRORPOINTSTAG		1020413

#define ID_CDHANDPLUGIN				1016815
#define ID_CDFINGERPLUGIN			1015930
#define ID_CDTHUMBPLUGIN			1016004
#define ID_CDLIMBIKPLUGIN			1015855
#define ID_CDQUADLEGPLUGIN			1018184
#define ID_CDMECHLIMBPLUGIN			1023910
#define ID_CDSPLINEIKPLUGIN			1015961
#define ID_CDSPINALPLUGIN			1016706
#define ID_CDDUALTARGETPLUGIN		1015908

#define ID_CDCLOTHTAG				1030926
#define ID_CDROPETAG				1030927

// Auto Key ID's
enum
{
	IDM_A_POS				= 12417,
	IDM_A_SIZE				= 12418,
	IDM_A_DIR				= 12419,
	IDM_A_PLA				= 12421,
	IDM_A_PARAMETER			= 12422,
	IDM_AUTOKEYS			= 12425
};

// user area classes
class CDJSAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	virtual void CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg=NULL);
	
};

class CDJSOptionsUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	LONG					sx;
	
public:
	virtual Bool GetMinSize(LONG& w, LONG& h);
	virtual void Sized(LONG w, LONG h);
	
	virtual void CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg=NULL);
	
};

// GraphView call function for retrieving the port ID of a description parameter
//#define GvCall(op,fnc) (((GvOperatorData*)op)->*((OPERATORPLUGIN*)C4DOS.Bl->RetrieveTableX((NodeData*)op,1))->fnc)

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);

int CompareWeights(const void *w1, const void *w2);
void RecalculateReference(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM);
LONG GetUserDataCount(BaseObject *op);


// sn functions
Bool RegisterCDJS(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);

#endif

