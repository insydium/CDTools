//	Cactus Dan's Cloth
//	Copyright 2013 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCloth.h"

// Starts the plugin registration

// tag
Bool RegisterCDClothTagPlugin(void);
Bool RegisterCDRopeTagPlugin(void);
Bool RegisterCDSkinRefPlugin(void);
Bool RegisterCDClothColliderTagPlugin(void);

// command
Bool RegisterAboutCDCloth(void);
Bool RegisterCDAddCloth(void);
Bool RegisterCDAddRope(void);
Bool RegisterCDRemoveCloth(void);

// message
Bool RegisterCDClothMessagePlugin(void);

// library functions
Bool RegisterCDClothFunctionLib(void);
Bool RegisterCDJSFunctionLib(void);

static AtomArray *cdcldTagList = NULL;

static Real version = 1.000;

Bool PluginStart(void)
{
	String pName = "CD Cloth";
	if(!CDCheckC4DVersion(pName)) return false;

	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDCLOTH,bc,false);

	Bool reg = RegisterCDCL();
	
	// message
	if (!RegisterCDClothMessagePlugin()) return false;
	
	cdcldTagList = AtomArray::Alloc();

	// tag
	if (!RegisterCDClothTagPlugin()) return false;
	if (!RegisterCDRopeTagPlugin()) return false;
	if (!RegisterCDSkinRefPlugin()) return false;
	if (!RegisterCDClothColliderTagPlugin()) return false;
	
	// command
	if (!RegisterAboutCDCloth()) return false;
	if (!RegisterCDAddCloth()) return false;
	if (!RegisterCDAddRope()) return false;
	if (!RegisterCDRemoveCloth()) return false;
	
	// library functions
	if (!RegisterCDClothFunctionLib()) return false;
	if (!RegisterCDJSFunctionLib()) return false;


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
	AtomArray::Free(cdcldTagList);

	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	CDCLData *cld;
	
	//use the following lines to set a plugin priority
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return false; // don't start plugin without resource
			return true;

		case C4DMSG_PRIORITY: 
			//use the following lines to set a plugin priority
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS+93);
			return true;

		case ID_CDCLOTHCOLLIDER:
			cld = (CDCLData *)data;
			cld->list = cdcldTagList;
			return true;
			
	}

	return false;
}


// user area
void CDClothAboutUA::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDClothAbout.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}


// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 12;
	*a = 206;
	*b = 222;
	*c = 220;
}


// library functions
CDClothFuncLib cdcFLib;

Bool iCDClothGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool iCDClothSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool iCDRopeGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool iCDRopeSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt);
CDAABB* iCDColliderGetBounds(BaseTag *tag);
//Vector iCDGetNearestSurfacePoint(BaseTag *tag, BaseObject *op, Vector pt, Vector &n);
Vector iCDGetNearestSurfacePoint(CDClothCollider *cldTag, BaseObject *op, Vector pt, Vector &n);

Bool RegisterCDClothFunctionLib(void)
{
    // Clear the structure
    ClearMem(&cdcFLib, sizeof(cdcFLib));
	
    // Fill in all function pointers
    cdcFLib.CDClothGetWeight = iCDClothGetWeight;
    cdcFLib.CDClothSetWeight = iCDClothSetWeight;
    cdcFLib.CDRopeGetWeight = iCDRopeGetWeight;
    cdcFLib.CDRopeSetWeight = iCDRopeSetWeight;
    cdcFLib.CDColliderGetBounds = iCDColliderGetBounds;
    cdcFLib.CDGetNearestSurfacePoint = iCDGetNearestSurfacePoint;
 	
    // Install the library
    return InstallLibrary(ID_CDCLOTHFUNCLIB, &cdcFLib, 1, sizeof(cdcFLib));
}

CDJSFuncLib cdjsFLib;

CDMRefPoint* iCDGetSkinReference(BaseTag *tag);
CDMRefPoint* iCDGetDeformReference(BaseTag *tag);

Bool RegisterCDJSFunctionLib(void)
{
	if(CDFindPlugin(ID_CDSKINREFPLUGIN,CD_TAG_PLUGIN)) return true;// exit if CD Joints & Skin is already loaded

    // Clear the structure
    ClearMem(&cdjsFLib, sizeof(cdjsFLib));
	
    // Fill in all function pointers
    cdjsFLib.CDGetSkinReference = iCDGetSkinReference;
    cdjsFLib.CDGetDeformReference = iCDGetDeformReference;
	
	// won't need duplicates
    cdjsFLib.CDGetClusterWeight = NULL;
    cdjsFLib.CDSetClusterWeight = NULL;
    cdjsFLib.CDRemapClusterWeights = NULL;
    cdjsFLib.CDGetJointWeight = NULL;
    cdjsFLib.CDSetJointWeight = NULL;
    cdjsFLib.CDGetSkinJointColors = NULL;
    cdjsFLib.CDGetSkinWeights = NULL;
 	
    // Install the library
    return InstallLibrary(ID_CDJSFUNCLIB, &cdjsFLib, 1, sizeof(cdjsFLib));
}
