#ifndef _CDSpringyKeys_H_
#define _CDSpringyKeys_H_

// CD includes
#include "CDQuaternion.h"
#include "CDaabb.h"
#include "CDGeneral.h"
#include "CDReg.h"
//#include "CDDebug.h"


// compatibility includes
#include "CDCompatibility.h"
#include "CDUserArea.h"

#if API_VERSION < 9800
	#include "R9Animation.h"
#else
	#include "R10Animation.h"
#endif

#define ID_CDSPRINGYKEYS			1025262

#define ID_ABOUTCDSPRINGYKEYS 		1025389
#define ID_CDSPRINGYKEYSTAG			1025390

#define C4D_SERIAL_SIZE				12
#define CDSK_SERIAL_SIZE			27

// user area classes
class CDSprKsAboutUserArea : public CDUserArea
{
private:
	AutoAlloc<BaseBitmap>	bm;
	
public:
	void CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg=NULL);
};

// Common Functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c);

// sn functions
Bool RegisterCDSK(void);
LONG GetSeed(String sn);
CHAR GetHexCharacter(LONG n);
String GetChecksum(const String s);
Bool CheckKeyChecksum(const String key);
String GetKeyByte(const String key, LONG start, LONG cnt);
CHAR GetKeyByte(const LONG seed, CHAR a, CHAR b, CHAR c);
String LongToHexString(LONG value, const LONG size);

#endif