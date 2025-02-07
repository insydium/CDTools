//	Cactus Dan's Constraints 1.2 plugin
//	Copyright 2008 by Cactus Dan Libisch

#ifndef _CDConstraint_H_
#define _CDConstraint_H_

//c4d includes
#include "c4d_tools.h"
#include "ge_matrix.h"
#include "ge_math.h"

// CD includes
#include "CDQuaternion.h"
#include "CDGeneral.h"
#include "CDSelLog.h"
#include "CDaabb.h"
#include "CDReg.h"

// compatibility includes
#include "CDCompatibility.h"
#include "CDTagData.h"
#include "CDUserArea.h"

#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

//#define EPSILON				0.00001

// CD Constraints ID
#define ID_CDCONSTRAINT				1017943

// Command ID's
#define ID_ABOUTCDCONSTRAINT 		1017944

#define ID_CDADDPCONSTRAINT			1021105
#define ID_CDADDRCONSTRAINT			1021106
#define ID_CDADDSCONSTRAINT			1021107
#define ID_CDADDACONSTRAINT			1021108
#define ID_CDADDMCONSTRAINT			1021109
#define ID_CDADDDCONSTRAINT			1021110
#define ID_CDADDPRCONSTRAINT		1021111
#define ID_CDADDCCONSTRAINT			1021112
#define ID_CDADDPSRCONSTRAINT		1021113
#define ID_CDADDLCONSTRAINT			1021114
#define ID_CDADDSFCONSTRAINT		1021115
#define ID_CDADDSPRCONSTRAINT		1021116
#define ID_CDADDPTCONSTRAINT		1021117
#define ID_CDAUTOMATICREDRAW		1021467
#define	ID_CDADDTACONSTRAINT		1021528
#define ID_CDADDNCONSTRAINT			1022243
#define ID_CDADDSPLCONSTRAINT		1022261
#define ID_CDADDSWCONSTRAINT		1023911


// Tag ID's
#define ID_CDPCONSTRAINTPLUGIN		1017945
#define ID_CDRCONSTRAINTPLUGIN		1017946
#define ID_CDSCONSTRAINTPLUGIN		1017947
#define ID_CDACONSTRAINTPLUGIN		1017948
#define ID_CDMCONSTRAINTPLUGIN		1017949
#define ID_CDNCONSTRAINTPLUGIN		1017950
#define ID_CDDCONSTRAINTPLUGIN		1017954
#define ID_CDPRCONSTRAINTPLUGIN		1017955
#define ID_CDCCONSTRAINTPLUGIN		1017979
#define ID_CDPSRCONSTRAINTPLUGIN	1018001
#define ID_CDLCONSTRAINTPLUGIN		1018014
#define ID_CDSFCONSTRAINTPLUGIN		1020598
#define ID_CDSPRCONSTRAINTPLUGIN	1020816
#define ID_CDPTCONSTRAINTPLUGIN		1020866
#define ID_CDTACONSTRAINTPLUGIN		1021527
#define ID_CDSPLCONSTRAINTPLUGIN	1022258
#define ID_CDSWCONSTRAINTPLUGIN		1023909

// Message ID's
#define ID_CDCMESSAGEPLUGIN			1021015

// External ID's
#define ID_CDJOINTOBJECT			1019254
#define ID_CDMORPHREFPLUGIN			1017273
#define ID_CDSKINPLUGIN				1019251
#define ID_CDFREEZETRANSFORMATION	1019577

#define C4D_SERIAL_SIZE				12
#define CDC_SERIAL_SIZE				27


// structures
struct CDCNData
{
	CDCNData(){list=NULL;}
	AtomArray *list;
};

struct CDContact
{
	public:
		Bool		hit;
		Vector		point;
		Vector		normal;
		Real		constant;
};

struct State
{
	// primary
	Vector			position;
	Vector			momentum;
	CDQuaternion	orientation;
	Vector			angularMomentum;
	
	// secondary
	Vector			velocity;
	CDQuaternion	spin;
	Vector			angularVelocity;
	
};

struct Derivative
{
	Vector			velocity;
	Vector			force;
	CDQuaternion	spin;
	Vector			torque;
};

struct CDSWTrackKey
{
	public:
		LONG		frm;
		LONG		spc;
		Matrix		m;
		Bool		set;
};

// user area classes
class CDCAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

class CDCOptionsUA : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	LONG					sx;
	
public:
	virtual Bool GetMinSize(LONG& w, LONG& h);
	virtual void Sized(LONG w, LONG h);
	
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

// Common Functions
int CompareFrames(const void *f1, const void *f2);
BaseTag* GetPreviousConstraintTag(BaseObject *op);
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);

// sn functions
Bool RegisterCDC(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);

#endif

