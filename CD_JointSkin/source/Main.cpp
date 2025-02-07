//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDClusterTag.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"

#include "tCDSkin.h"

// Starts the plugin registration

// forward declarations

// tag / expression plugins
Bool RegisterCDClusterTagPlugin(void);
Bool RegisterCDSkinPlugin(void);
Bool RegisterCDBindPosePlugin(void);
Bool RegisterCDSkinRefPlugin(void);

// menu command plugins
Bool RegisterAboutCDJointsAndSkin(void);
Bool RegisterCDAddSkin(void);
Bool RegisterCDJointMirrorPlugin(void);
Bool RegisterCDOrientJoints(void);
Bool RegisterCDAddJoints(void);
Bool RegisterCDDeleteJoints(void);
Bool RegisterCDMirrorSkinWeight(void);
Bool RegisterCDMirrorSkinCluster(void);
Bool RegisterCDJointXRayToggle(void);
Bool RegisterCDColorJoints(void);
Bool RegisterCDAddSkinCluster(void);
Bool RegisterCDJointProxyToggle(void);
Bool RegisterCDJointsToPolygons(void);
Bool RegisterCDRemoveSkin(void);
Bool RegisterCDJointDisplaySizePlugin(void);
Bool RegisterCDJointMirrorAssign(void);
Bool RegisterCDFreezeTransformation(void);
Bool RegisterCDLinkObjects(void);
Bool RegisterCDUnLinkObjects(void);
Bool RegisterCDNormalizeAllWeights(void);
Bool RegisterCDTogglePaintMode(void);
Bool RegisterCDMergeSkin(void);
Bool RegisterCDFreezeSkinState(void);
Bool RegisterCDJointEnvelopeToggle(void);
Bool RegisterCDRerootObjects(void);
Bool RegisterCDTransferSkinCommand(void);

Bool RegisterCDConvertToCDSkin(void);
Bool RegisterCDConvertFromCDSkin(void);
Bool RegisterCDConvertToCDJoints(void);
Bool RegisterCDConvertFromCDJoints(void);

// tool plugins
Bool RegisterCDJointToolPlugin(void);
Bool RegisterCDPaintSkinWeight(void);

// object plugins
Bool RegisterCDJoint(void);

// message plugins
Bool RegisterCDJSMessagePlugin(void);

// scene hook plugins
Bool RegisterCDJSSceneHook(void);

// Selection logger
Bool RegisterSelectionLog(void);

// library functions
Bool RegisterCDJSFunctionLib(void);

static AtomArray *cdscTagList = NULL;
static AtomArray *cdsrTagList = NULL;
static AtomArray *cdskTagList = NULL;
static AtomArray *cdJointList = NULL;
static AtomArray *cdbpTagList = NULL;

Vector colorH, colorCH;

Bool clusDialogOpen = false;
Real clusDialogRad;
Real clusDialogMax;
Real clusDialogFalloff;

static Real version = 1.562;

//Start the plugin
Bool PluginStart(void)
{
	String pName = "CD Joints & Skin";
	if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION, version);
	SetWorldPluginData(ID_CDJOINTSANDSKIN, bc, false);
	
	Bool reg = RegisterCDJS();
	
	// Register/About
	if(!RegisterAboutCDJointsAndSkin()) return false;
	
	// message plugins
	if(!RegisterCDJSMessagePlugin()) return false;
	
	// scene hook plugins
	if(!RegisterCDJSSceneHook()) return false;

	// Allocate the lists
	cdscTagList = AtomArray::Alloc();
	cdsrTagList = AtomArray::Alloc();
	cdskTagList = AtomArray::Alloc();
	cdJointList = AtomArray::Alloc();
	cdbpTagList = AtomArray::Alloc();
	
	// Selection logger
	if(RegisterSelectionLog())
	{
		InitSelectionLog();
	}

	// tag / expression plugins
	if(!RegisterCDClusterTagPlugin()) return false;
	if(!RegisterCDSkinPlugin()) return false;
	if(!RegisterCDBindPosePlugin()) return false;
	if(!RegisterCDSkinRefPlugin()) return false;
	
	// menu command plugins
	if(!RegisterCDAddSkin()) return false;
	if(!RegisterCDJointMirrorPlugin()) return false;
	if(!RegisterCDOrientJoints()) return false;
	if(!RegisterCDAddJoints()) return false;
	if(!RegisterCDDeleteJoints()) return false;
	if(!RegisterCDMirrorSkinWeight()) return false;
	if(!RegisterCDMirrorSkinCluster()) return false;
	if(!RegisterCDJointXRayToggle()) return false;
	if(!RegisterCDColorJoints()) return false;
	if(!RegisterCDAddSkinCluster()) return false;
	if(!RegisterCDJointProxyToggle()) return false;
	if(!RegisterCDJointsToPolygons()) return false;
	if(!RegisterCDRemoveSkin()) return false;
	if(!RegisterCDJointDisplaySizePlugin()) return false;
	if(!RegisterCDJointMirrorAssign()) return false;
	if(!RegisterCDNormalizeAllWeights()) return false;
	if(!RegisterCDTogglePaintMode()) return false;
	if(!RegisterCDMergeSkin()) return false;
	if(!RegisterCDFreezeSkinState()) return false;
	if(!RegisterCDJointEnvelopeToggle()) return false;
	if(!RegisterCDRerootObjects()) return false;
	if(!RegisterCDTransferSkinCommand()) return false;
	
	// tool plugins
	if(!RegisterCDJointToolPlugin()) return false;
	if(!RegisterCDPaintSkinWeight()) return false;
	
	// object plugins
	if(!RegisterCDJoint()) return false;
	
	// library functions
	if(!RegisterCDJSFunctionLib()) return false;
	
	String license = "";
	if(!reg) license = " - Runtime";
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) license = " - Demo";
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) license = " - Demo";
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) license = " - Runtime";
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) license = " - Demo";
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) license = " - Runtime";
#endif
	GePrint(pName+" v"+CDRealToString(version)+license);
	
	// duplicate commands
	if(reg)
	{
		if(!RegisterCDFreezeTransformation()) return false;
		if(!RegisterCDLinkObjects()) return false;
		if(!RegisterCDUnLinkObjects()) return false;
		if(!RegisterCDConvertToCDSkin()) return false;
		if(!RegisterCDConvertFromCDSkin()) return false;
		if(!RegisterCDConvertToCDJoints()) return false;
		if(!RegisterCDConvertFromCDJoints()) return false;
	}
	
	return true;
}

void PluginEnd(void)
{
	AtomArray::Free(cdscTagList);
	AtomArray::Free(cdsrTagList);
	AtomArray::Free(cdskTagList);
	AtomArray::Free(cdJointList);
	AtomArray::Free(cdbpTagList);
	
	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	CDJSData *scd, *skd, *srd, *jd, *bpd; //, *rgd
	
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if(!resource.Init())
			{
				// don't start plugin without resource
				GePrint("CD Joint & Skin failed to load resources");
				return false; 
			}
			return true;

		case C4DMSG_PRIORITY: 
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS+97);
			return true;

		case ID_CDCLUSTERTAGPLUGIN:
			scd = (CDJSData *)data;
			scd->list = cdscTagList;
			return true;

		case ID_CDSKINPLUGIN:
			skd = (CDJSData *)data;
			skd->list = cdskTagList;
			return true;

		case ID_CDSKINREFPLUGIN:
			srd = (CDJSData *)data;
			srd->list = cdsrTagList;
			return true;
			
		case ID_CDJOINTSANDSKIN:
			colorH = CDGetActiveViewColor();
			colorCH = CDGetActiveChildViewColor();
			return true;
			
		case ID_CDJOINTOBJECT:
			jd = (CDJSData *)data;
			jd->list = cdJointList;
			return true;

		case ID_CDBINDPOSETAG:
			bpd = (CDJSData *)data;
			bpd->list = cdbpTagList;
			return true;

	}

	return false;
}

// user area
void CDJSAboutUserArea::CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDJointSkin.tif"));
	DrawBitmap(bm, 0, 0, 320, 60, 0, 0, 320, 60, 0);
}

void CDJSOptionsUA::CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDJSOptions.tif"));

	OffScreenOn();
	DrawSetPen(Vector(0.41568, 0.45098, 0.61176)); //gray blue background
	DrawRectangle(0, 0, sx, 24);
	
	// draw the bitmap
	DrawBitmap(bm, 0, 0, 256, 24, 0, 0, 256, 24, 0);
}

void CDJSOptionsUA::Sized(LONG w, LONG h) 
{
	//store the width for drawing 
	sx = w;
}

Bool CDJSOptionsUA::GetMinSize(LONG &w, LONG &h) 
{
	//the dimensions of the titlebar bitmap
	w = 256;
	h = 24;
	return true;
}

//common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 8;
	*a = 10;
	*b = 5;
	*c = 38;
}

int CompareWeights(const void *w1, const void *w2)
{
  	CDClusterWeight	*tw1, *tw2;
  	
  	tw1 = (CDClusterWeight *)w1;
  	tw2 = (CDClusterWeight *)w2;
  	if(tw1->w > tw2->w) return -1;
	else if(tw1->w < tw2->w) return 1;
	else return 0;
}

void RecalculateReference(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM)
{
	CDTransMatrixData tmData;
	tmData.nM = newM;
	tmData.oM = oldM;
	
	BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN);
	if(skrTag)
	{
		skrTag->Message(CD_MSG_RECALC_REFERENCE, &tmData);
		
		BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
		if(skTag)
		{
			BaseContainer *skData = skTag->GetDataInstance();
			if(!skData->GetBool(SKN_BOUND))
			{
				DescriptionCommand dc;
				dc.id = DescID(SKR_RESTORE_REFERENCE);
				skrTag->Message(MSG_DESCRIPTION_COMMAND, &dc);
			}
		}
	}
	BaseTag *mrTag = op->GetTag(ID_CDMORPHREFPLUGIN);
	if(mrTag) mrTag->Message(CD_MSG_RECALC_REFERENCE, &tmData);
}

LONG GetUserDataCount(BaseObject *op)
{
	LONG udNum = 0;
	
	if(op)
	{
		DynamicDescription* dd = op->GetDynamicDescription();
		if(dd)
		{
			DescID id; const BaseContainer* bc;
			void* handle = dd->BrowseInit();
			while (dd->BrowseGetNext(handle, &id, &bc)) { udNum++; }
			dd->BrowseFree(handle);
		}
	}
	
	return udNum;
}

// library functions
CDJSFuncLib cdjsFLib;

CDMRefPoint* iCDGetSkinReference(BaseTag *tag);
CDMRefPoint* iCDGetDeformReference(BaseTag *tag);
Bool iCDGetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool iCDSetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt);
Bool iCDRemapClusterWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map);
Bool iCDGetJointWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG pCnt);
Bool iCDSetJointWeight(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt);
Bool iCDGetSkinJointColors(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt);
CDSkinVertex* iCDGetSkinWeights(BaseTag *tag);

Bool RegisterCDJSFunctionLib(void)
{
    // Clear the structure
    ClearMem(&cdjsFLib, sizeof(cdjsFLib));
	
    // Fill in all function pointers
    cdjsFLib.CDGetSkinReference = iCDGetSkinReference;
    cdjsFLib.CDGetDeformReference = iCDGetDeformReference;
    cdjsFLib.CDGetClusterWeight = iCDGetClusterWeight;
    cdjsFLib.CDSetClusterWeight = iCDSetClusterWeight;
    cdjsFLib.CDRemapClusterWeights = iCDRemapClusterWeights;
    cdjsFLib.CDGetJointWeight = iCDGetJointWeight;
    cdjsFLib.CDSetJointWeight = iCDSetJointWeight;
    cdjsFLib.CDGetSkinJointColors = iCDGetSkinJointColors;
    cdjsFLib.CDGetSkinWeights = iCDGetSkinWeights;
 	
    // Install the library
    return InstallLibrary(ID_CDJSFUNCLIB, &cdjsFLib, 1, sizeof(cdjsFLib));
}
