// Cactus Dan's Springy Keys plugin
// Copyright 2010 by Cactus Dan Libisch

#include "c4d.h"
#include <string.h>

#include "CDSpringyKeys.h"

// forward declarations
Bool RegisterCDSpringyKeysTagPlugin(void);
Bool RegisterAboutCDSpringyKeys(void);

static Real version = 1.006;

Bool PluginStart(void)
{
	String pName = "CD Springy Keys";
	if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDSPRINGYKEYS,bc,false);
	
	Bool reg = RegisterCDSK();
	
	if (!RegisterCDSpringyKeysTagPlugin()) return false;
	if (!RegisterAboutCDSpringyKeys()) return false;
	
	String license = "";
	if(!reg) license = " - Runtime";
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO)  license = " - Demo";
	if(GetC4DVersion() > 10999)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) license = " - Demo";
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) license = " - Runtime";
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) license = " - Demo";
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) license = " - Runtime";
#endif

	GePrint(pName+" v"+CDRealToString(version,-1,3)+license);
	return true;
}

void PluginEnd(void)
{
}

Bool PluginMessage(LONG id, void *data)
{
	//use the following lines to set a plugin priority
	
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init())
			{
				// don't start plugin without resource
				GePrint("CD Springy Keys failed to load resources.");
				return false; 
			}
			return true;

		case C4DMSG_PRIORITY: 
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS);
			return true;

	}

	return false;
}

// user area
void CDSprKsAboutUserArea::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDSpringyKeys.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 8;
	*a = 119;
	*b = 127;
	*c = 97;
}

