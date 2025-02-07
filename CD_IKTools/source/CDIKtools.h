#ifndef _CDIKTools_H_
#define _CDIKTools_H_

//c4d includes
#include "c4d_tools.h"
#include "ge_matrix.h"
#include "ge_math.h"

// CD includes
#include "CDQuaternion.h"
#include "CDSelLog.h"
#include "CDMessages.h"
#include "CDGeneral.h"
#include "CDTagData.h"
#include "CDCompatibility.h"
#include "CDReg.h"
#include "CDUserArea.h"

// compatibility includes
#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

#include "CDJoint.h"

//#define EPSILON 0.00001

// CD IK Tools ID
#define ID_CDIKTOOLS				1016043

// Tag ID's
#define ID_CDIKRPSOLVER				1015854
#define ID_CDFOOTIKPLUGIN			1015856
#define ID_CDLIMBIKPLUGIN			1015855
#define ID_CDSPLINEIKPLUGIN			1015961
#define ID_CDROTATORPLUGIN			1015857
#define ID_CDDUALTARGETPLUGIN		1015908
#define ID_CDFINGERPLUGIN			1015930
#define ID_CDTHUMBPLUGIN			1016004
#define ID_CDSPINALPLUGIN			1016706
#define ID_CDHANDPLUGIN				1016815
#define ID_CDQUADLEGPLUGIN			1018184
#define ID_CDMECHLIMBPLUGIN			1023910
#define ID_CDSMOOTROTATIONPLUGIN	1025381
#define ID_CDIKHANDLEPLUGIN			1027519
#define ID_CDLINKAGEIKPLUGIN		1031895
#define ID_CDPISTONIKPLUGIN			1031896

// Command ID's
#define ID_ABOUTCDIKTOOLS			1016034
#define ID_ZEROGLOBALROTATION		1015896
#define ID_ZEROLOCALROTATION		1022172
#define ID_ADDTIPNULL				1015859
#define ID_ADDTIPEFFECTOR			1015860
#define ID_ADDTIPGOAL				1016298
#define ID_ADDROOTNULL				1015858
#define ID_CDIKSETUP				1016409
#define ID_CDADDHANDTAG				1023232
#define ID_CDCOPYHANDPOSE			1023407
#define ID_CDSAVEHANDPOSE			1031943
#define ID_CDLOADHANDPOSE			1031944

// Object ID's
#define ID_CDRIGGUIDE				1026646

// Tool ID's
#define ID_CDIKHANDLETOOL			1027521

// Message ID's
#define ID_CDIKMESSAGEPLUGIN		1022827

// External ID's
#define ID_CDJOINTOBJECT			1019254
#define ID_CDFREEZETRANSFORMATION	1019577
#define ID_CDZEROTRANORMATION		1022137

#define C4D_SERIAL_SIZE				12
#define CDIK_SERIAL_SIZE			27


#define IK_BONE_LINK		10015

enum
{
	POLE_X,
	POLE_Y,
	POLE_NX,
	POLE_NY
};

struct CDSplineJoint
{
	Matrix		jntM;
	Vector		rot;
	Vector		pos;
	Vector		sca;
	Real		length;
	Real		twist;
	Real		splPos;
	BaseObject *jnt;
};

struct CDIKData
{
	CDIKData(){list=NULL;}
	AtomArray *list;
};

// user area classes
class CDIKAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg=NULL);
};

class CDIKOptionsUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	LONG					sx;
	
public:
	virtual Bool GetMinSize(LONG& w, LONG& h);
	virtual void Sized(LONG w, LONG h);
	
	void CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg=NULL);
};

// Common Functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);
Matrix GetRPMatrix(LONG pole, Vector o, Vector g, Vector p);
Real GetBoneLength(BaseObject *op);

// coffee extension functions
enum
{
	LD_SOLO = 0,
	LD_VIEW,
	LD_RENDER,
	LD_MANAGER,
	LD_LOCKED,
	LD_GENERATORS,
	LD_DEFORMERS,
	LD_EXPRESSIONS,
	LD_ANIMATION
	
};

Bool AddCoffeeSymbols(Coffee *cof);

// enable/disable layer properties functions
void LayerOn(Coffee* cof, VALUE *&sp, LONG argc);
void LayerOff(Coffee* cof, VALUE *&sp, LONG argc);

#if API_VERSION < 11000
// CallButton() function for pre R11 versions
void CallButton(Coffee* cof, VALUE *&sp, LONG argc); 
#endif

// sn functions
Bool RegisterCDIK(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);


#endif

