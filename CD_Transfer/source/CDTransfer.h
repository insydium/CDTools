#ifndef _CDTransfer_H_
#define _CDTransfer_H_

// c4d includes
#include "c4d_tools.h"
#include "ge_matrix.h"
#include "ge_math.h"
#include "lib_modeling.h"
#include "gvconst.h"
#include "gvcondition.h"
#include "gvdynamic.h"
#include "gvobject.h"
#include "modelingids.h"

// CD includes
#include "CDCompatibility.h"
#include "CDQuaternion.h"
#include "CDSelLog.h"
#include "CDaabb.h"
#include "CDMessages.h"
#include "CDGeneral.h"
#include "CDReg.h"
//#include "CDDebug.h"
#include "CDUserArea.h"

// compatibility includes
#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

//#define EPSILON 0.00001

// CD Transfer ID's
#define ID_CDTRANSFERTOOLS			1022163
//#define ID_CDTRLOGOBAR				1022482

// Tag ID's
#define ID_CDZEROTRANSTAG			1022137
#define ID_CDTRANSELECTEDTAG		1022164
//#define ID_CDTRANSACTIVATEDTAG		1027350

// Command ID's
#define ID_ABOUTCDTRANSFER	 		1022171
#define ID_ZEROGLOBALROTATION	 	1015896
#define ID_ZEROLOCALROTATION	 	1022172
#define ID_CDTRANSFERCOM			1022165
#define ID_CDTRANSELCOM				1022166
#define ID_CDADDTRANSELECTED		1022167
#define ID_CDADDZEROTRANS			1022168
#define ID_CDCOORDINATES			1022170
#define ID_CDFREEZETRANSFORMATION	1019577
#define ID_CDSELECTALLSAME			1022234
#define ID_CDMIRRORTRANSCOM			1022248
#define ID_CDREPLACETRANSCOM		1022249
#define ID_CDSWAPTRANSFERCOM		1022352
#define ID_LINKOBJECTS				1021092
#define ID_UNLINKOBJECTS			1021093
#define ID_CDGOALTRANSCOM			1022394
#define ID_SETZEROHOMEPOSITION		1022483
#define ID_CDAIMTRANSFERCOM			1022575
#define ID_CDTRANSFERANIM			1024935
#define ID_CDTRANSFERUSERDATA		1024974
#define ID_CDTRANSFERAXIS			1025261

// Message ID's
#define ID_CDTRANSMESSAGE			1022169

// Scene Hook ID
#define ID_CDTSCENEHOOK				1024112

// External ID's
#define ID_ABOUTCDIKTOOLS			1016034
#define ID_ABOUTCDJOINTSKIN 		1019259
#define ID_CDJOINTOBJECT			1019254
#define ID_CDMORPHREFPLUGIN			1017273
#define ID_CDSKINREFPLUGIN			1019291
#define ID_CDSKINPLUGIN				1019251
#define ID_CDBINDPOSETAG			1019257
#define ID_CDDUALTARGETPLUGIN		1015908
#define ID_CDACONSTRAINTPLUGIN		1017948

#define C4D_SERIAL_SIZE				12
#define CDTR_SERIAL_SIZE			27

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

enum // common ID's from tags
{
	// from CD Skin Reference
	CDSKR_RESTORE_REFERENCE		= 1001,
	
	// from CD Joint
	CDJ_ZERO_COORDS				= 1006,
	  
	// from CD Skin
	CDSK_BOUND					= 5000
};

struct CDTRData
{
	CDTRData(){list=NULL;}
	AtomArray *list;
};

struct CDTrnSel
{
	Bool trn;
};

struct CDMRefPoint
{
	public:
		Real		x;
		Real		y;
		Real		z;
		
		Vector GetVector(void) { return Vector(x,y,z); }
		void SetVector(const Vector v) { x = v.x; y = v.y; z = v.z; }
};


// user area classes
class CDTransferAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

class CDTROptionsUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	LONG					sx;
	
public:
	virtual Bool GetMinSize(LONG& w, LONG& h);
	virtual void Sized(LONG w, LONG h);
	
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

// GraphView call function for retrieving the port ID of a description parameter
//#define GvCall(op,fnc) (((GvOperatorData*)op)->*((OPERATORPLUGIN*)C4DOS.Bl->RetrieveTableX((NodeData*)op,1))->fnc)

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);
void RecalculateReference(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM);
void RecalculateComponents(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM);
Bool ElementsSelected(BaseDocument *doc, BaseObject *op);
Matrix GetEditModeTransferMatrix(BaseDocument *doc, BaseObject *target);
void ConvertToPointSelection(BaseSelect *bs, BaseSelect *ptS, BaseObject *op, LONG mode);

// animation track transfer functions
//void TransferAMTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk);
//void TransferUDTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk);
//void TransferVectorTracks(BaseDocument *doc, BaseObject *src, BaseObject *dst, BaseList2D *prvTrk, LONG trackID);


// sn functions
Bool RegisterCDTR(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);

#endif

