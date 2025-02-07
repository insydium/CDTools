//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"

#include "CDIKtools.h"
//#include "CDDebug.h"

// forward declarations

// tag pluigns
Bool RegisterCDLimbIKPlugin(void);
Bool RegisterCDFootIKPlugin(void);
Bool RegisterCDRotatorPlugin(void);
Bool RegisterCDDualTargetPlugin(void);
Bool RegisterCDFingerPlugin(void);
Bool RegisterCDThumbPlugin(void);
Bool RegisterCDSplineIKPlugin(void);
Bool RegisterCDSpinalPlugin(void);
Bool RegisterCDHandPlugin(void);
Bool RegisterCDQuadLegPlugin(void);
Bool RegisterCDMechIKPlugin(void);
Bool RegisterCDSmoothRotationPlugin(void);
Bool RegisterCDIKHandlePlugin(void);
Bool RegisterCDLinkageIKPlugin(void);
Bool RegisterCDPistonIKPlugin(void);

// menu command plugins
Bool RegisterAddRootNull(void);
Bool RegisterAddTipEffector(void);
Bool RegisterAddTipNull(void);
Bool RegisterAddTipGoal(void);
Bool RegisterCDIKSetupChain(void);
Bool RegisterCDAddHandTag(void);
Bool RegisterCDCopyHandPose(void);
Bool RegisterZeroGlobalRotation(void);
Bool RegisterZeroLocalRotation(void);
Bool RegisterCDSaveHandPose(void);
Bool RegisterCDLoadHandPose(void);

// tool plugins
Bool RegisterCDIKHandleTool(void);

Bool RegisterAboutCDIKTools(void);

// message plugins
Bool RegisterCDIKMessagePlugin(void);

// Selection logger
Bool RegisterSelectionLog(void);

//Bool CheckSerial(void);
static Real version = 1.549;

static AtomArray *spnlTagList = NULL;
static AtomArray *spikTagList = NULL;

// Starts the plugin registration
Bool PluginStart(void)
{
	String pName = "CD IK Tools";
	if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDIKTOOLS,bc,false);
	
	Bool reg = RegisterCDIK();

	// Register/About
	if (!RegisterAboutCDIKTools()) return false;
	
	// message plugins
	if (!RegisterCDIKMessagePlugin()) return false;
	
	// Allocate the lists
	spnlTagList = AtomArray::Alloc();
	spikTagList = AtomArray::Alloc();

	// Selection logger
	if(RegisterSelectionLog())
	{
		InitSelectionLog();
	}
	
	// tag / expression plugins
	if(!RegisterCDLimbIKPlugin()) return false;
	if(!RegisterCDFootIKPlugin()) return false;
	if(!RegisterCDRotatorPlugin()) return false;
	if(!RegisterCDDualTargetPlugin()) return false;
	if(!RegisterCDFingerPlugin()) return false;
	if(!RegisterCDThumbPlugin()) return false;
	if(!RegisterCDSplineIKPlugin()) return false;
	if(!RegisterCDSpinalPlugin()) return false;
	if(!RegisterCDHandPlugin()) return false;
	if(!RegisterCDQuadLegPlugin()) return false;
	if(!RegisterCDMechIKPlugin()) return false;
	if(!RegisterCDSmoothRotationPlugin()) return false;
	if(!RegisterCDIKHandlePlugin()) return false;
	if(!RegisterCDLinkageIKPlugin()) return false;
	if(!RegisterCDPistonIKPlugin()) return false;
	
	// menu command plugins
	if(!RegisterAddRootNull()) return false;
	if(!RegisterAddTipEffector()) return false;
	if(!RegisterAddTipNull()) return false;
	if(!RegisterCDIKSetupChain()) return false;
	if(!RegisterCDAddHandTag()) return false;
	if(!RegisterCDCopyHandPose()) return false;
	if(!RegisterZeroGlobalRotation()) return false;
	if(!RegisterZeroLocalRotation()) return false;
	if(!RegisterCDSaveHandPose()) return false;
	if(!RegisterCDLoadHandPose()) return false;
	
#if API_VERSION < 12000
	if(!RegisterAddTipGoal()) return false;
#endif
		
	// tool plugins
	if(!RegisterCDIKHandleTool()) return false;
	
	// coffee extension functions
	Coffee *cofM = GetCoffeeMaster();
	if(cofM)
	{
	#if API_VERSION > 9600
		if(AddCoffeeSymbols(cofM))
		{
			cofM->AddGlobalFunction("LayerOn",LayerOn); // enable a LayerData
			cofM->AddGlobalFunction("LayerOff",LayerOff); // disable a LayerData
		}
	#endif
		
	#if API_VERSION < 11000
		cofM->AddGlobalFunction("CallButton",CallButton);
	#endif
	}
	
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
	
	GePrint(pName+" v"+CDRealToString(version,-1,3)+license);
	return true;
}

void PluginEnd(void)
{
	AtomArray::Free(spnlTagList);
	AtomArray::Free(spikTagList);

	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	CDIKData *spnl, *spik;
	
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if(!resource.Init())
			{
				// don't start plugin without resource
				GePrint("CD IK Tools failed to load resources");
				return false; 
			}
			return true;

		case C4DMSG_PRIORITY: 
			//use the following lines to set a plugin priority
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS+99);
			return true;
			
		case ID_CDSPINALPLUGIN:
			spnl = (CDIKData *)data;
			spnl->list = spnlTagList;
			return true;
			
		case ID_CDSPLINEIKPLUGIN:
			spik = (CDIKData *)data;
			spik->list = spikTagList;
			return true;

	}

	return false;
}

// user area
void CDIKAboutUserArea::CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDIKTools.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}

void CDIKOptionsUA::CDDraw(LONG x1, LONG y1, LONG x2, LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDIKOptions.tif"));

	OffScreenOn();
	DrawSetPen(Vector(0.41568, 0.45098, 0.61176)); //gray blue background
	DrawRectangle(0,0,sx,24);
	
	// draw the bitmap
	DrawBitmap(bm,0,0,256,24,0,0,256,24,0);
}

void CDIKOptionsUA::Sized(LONG w, LONG h) 
{
	//store the width for drawing 
	sx = w;
}

Bool CDIKOptionsUA::GetMinSize(LONG& w, LONG& h) 
{
	//the dimensions of the titlebar bitmap
	w = 256;
	h = 24;
	return true;
}


// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 12;
	*a = 3;
	*b = 10;
	*c = 191;
}

Matrix GetRPMatrix(LONG pole, Vector o, Vector g, Vector p)
{
	Matrix m = Matrix();

	m.off = o;
	switch(pole)
	{
		case POLE_X:	
			m.v2 = VNorm(VCross(g, p));
			m.v1 = VNorm(VCross(m.v2, g));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case POLE_Y:
			m.v1 = VNorm(VCross(p, g));
			m.v2 = VNorm(VCross(g, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case POLE_NX:	
			m.v2 = VNorm(VCross(p, g));
			m.v1 = VNorm(VCross(m.v2, g));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
		case POLE_NY:
			m.v1 = VNorm(VCross(g, p));
			m.v2 = VNorm(VCross(g, m.v1));
			m.v3 = VNorm(VCross(m.v1, m.v2));
			break;
	}

	return m;	
}

Real GetBoneLength(BaseObject *op)
{
#if API_VERSION < 12000
	if(!op) return 0.0;
	return op->GetDataInstance()->GetReal(BONEOBJECT_LENGTH);
#else
	return 0.0;
#endif
}

#if API_VERSION > 9600
// coffee extension functions
Bool AddCoffeeSymbols(Coffee *cof)
{
	VALUE LAYERDATA_SOLO;
	LAYERDATA_SOLO.SetLong(LD_SOLO);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_SOLO",&LAYERDATA_SOLO)) return false;
	
	VALUE LAYERDATA_VIEW;
	LAYERDATA_VIEW.SetLong(LD_VIEW);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_VIEW",&LAYERDATA_VIEW)) return false;
	
	VALUE LAYERDATA_RENDER;
	LAYERDATA_RENDER.SetLong(LD_RENDER);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_RENDER",&LAYERDATA_RENDER)) return false;
	
	VALUE LAYERDATA_MANAGER;
	LAYERDATA_MANAGER.SetLong(LD_MANAGER);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_MANAGER",&LAYERDATA_MANAGER)) return false;
	
	VALUE LAYERDATA_LOCKED;
	LAYERDATA_LOCKED.SetLong(LD_LOCKED);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_LOCKED",&LAYERDATA_LOCKED)) return false;
	
	VALUE LAYERDATA_GENERATORS;
	LAYERDATA_GENERATORS.SetLong(LD_GENERATORS);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_GENERATORS",&LAYERDATA_GENERATORS)) return false;
	
	VALUE LAYERDATA_DEFORMERS;
	LAYERDATA_DEFORMERS.SetLong(LD_DEFORMERS);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_DEFORMERS",&LAYERDATA_DEFORMERS)) return false;
	
	VALUE LAYERDATA_EXPRESSIONS;
	LAYERDATA_EXPRESSIONS.SetLong(LD_EXPRESSIONS);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_EXPRESSIONS",&LAYERDATA_EXPRESSIONS)) return false;
	
	VALUE LAYERDATA_ANIMATION;
	LAYERDATA_ANIMATION.SetLong(LD_ANIMATION);
	if(!CDAddGlobalSymbol(cof,"LAYERDATA_ANIMATION",&LAYERDATA_ANIMATION)) return false;
	
	return true;
}

void LayerOn(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	if(argc == 2 && sp[argc-1].IsType(GetCoffeeType(CD_COFFEE_OBJECT)) && sp[argc-2].IsType(GetCoffeeType(CD_COFFEE_LONG)))
	{
		GeData gData;
		GeCoffeeValue2GeData(cof,&sp[argc-1],&gData);
		BaseLink* bl = gData.GetBaseLink();
		
		BaseList2D *lst = bl->GetLink(GetActiveDocument());
		if(lst)
		{
			const LayerData *lstLD = lst->GetLayerData(GetActiveDocument());
			if(lstLD)
			{
				Bool chngd = false;
				LayerData ldata = *lstLD;
				LONG element = sp[argc-2].GetLong();
				switch(element)
				{
					case LD_SOLO:
						if(!lstLD->solo) { ldata.solo = true; chngd = true; }
						break;
					case LD_VIEW:
						if(!lstLD->view) { ldata.view = true; chngd = true; }
						break;
					case LD_RENDER:
						if(!lstLD->render) { ldata.render = true; chngd = true; }
						break;
					case LD_MANAGER:
						if(!lstLD->manager) { ldata.manager = true; chngd = true; }
						break;
					case LD_LOCKED:
						if(!lstLD->locked) { ldata.locked = true; chngd = true; }
						break;
					case LD_GENERATORS:
						if(!lstLD->generators) { ldata.generators = true; chngd = true; }
						break;
					case LD_DEFORMERS:
						if(!lstLD->deformers) { ldata.deformers = true; chngd = true; }
						break;
					case LD_EXPRESSIONS:
						if(!lstLD->expressions) { ldata.expressions = true; chngd = true; }
						break;
					case LD_ANIMATION:
						if(!lstLD->animation) { ldata.animation = true; chngd = true; }
						break;
				}
				if(chngd)
				{
					lst->SetLayerData(GetActiveDocument(), ldata);
					EventAdd(EVENT_FORCEREDRAW);
				}
			}
		}
	}
	
	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}

void LayerOff(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	if(argc == 2 && sp[argc-1].IsType(GetCoffeeType(CD_COFFEE_OBJECT)) && sp[argc-2].IsType(GetCoffeeType(CD_COFFEE_LONG)))
	{
		GeData gData;
		GeCoffeeValue2GeData(cof,&sp[argc-1],&gData);
		BaseLink* bl = gData.GetBaseLink();
		
		BaseList2D *lst = bl->GetLink(GetActiveDocument());
		if(lst)
		{
			const LayerData *lstLD = lst->GetLayerData(GetActiveDocument());
			if(lstLD)
			{
				Bool chngd = false;
				LayerData ldata = *lstLD;
				LONG element = sp[argc-2].GetLong();
				switch(element)
				{
					case LD_SOLO:
						if(lstLD->solo) { ldata.solo = false; chngd = true; }
						break;
					case LD_VIEW:
						if(lstLD->view) { ldata.view = false; chngd = true; }
						break;
					case LD_RENDER:
						if(lstLD->render) { ldata.render = false; chngd = true; }
						break;
					case LD_MANAGER:
						if(lstLD->manager) { ldata.manager = false; chngd = true; }
						break;
					case LD_LOCKED:
						if(lstLD->locked) { ldata.locked = false; chngd = true; }
						break;
					case LD_GENERATORS:
						if(lstLD->generators) { ldata.generators = false; chngd = true; }
						break;
					case LD_DEFORMERS:
						if(lstLD->deformers) { ldata.deformers = false; chngd = true; }
						break;
					case LD_EXPRESSIONS:
						if(lstLD->expressions) { ldata.expressions = false; chngd = true; }
						break;
					case LD_ANIMATION:
						if(lstLD->animation) { ldata.animation = false; chngd = true; }
						break;
				}
				if(chngd)
				{
					lst->SetLayerData(GetActiveDocument(), ldata);
					EventAdd(EVENT_FORCEREDRAW);
				}
			}
		}
	}
	
	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}
#endif

#if API_VERSION < 11000
void CallButton(Coffee* cof, VALUE *&sp, LONG argc)
{
	Bool res = false;
	
	if(argc == 2 && sp[argc-1].IsType(DT_OBJECT) && sp[argc-2].IsType(DT_LONG))
	{
		GeData dest = NULL;
		GeCoffeeValue2GeData(cof,&sp[argc-1],&dest);
		BaseLink* bl = dest.GetBaseLink();
		
		BaseList2D *lst = bl->GetLink(GetActiveDocument());
		if(lst)
		{
			LONG buttonID = sp[argc-2].GetLong();
			DescriptionCommand dc;
			dc.id = DescID(buttonID);
			lst->Message(MSG_DESCRIPTION_COMMAND,&dc);
		}
	}
	
	GeCoffeeGeData2Value(cof, res, &sp[argc]);
	sp += argc;
}
#endif


