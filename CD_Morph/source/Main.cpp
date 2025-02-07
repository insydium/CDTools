//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "CDMorph.h"

// forward declarations
Bool RegisterAboutCDMorph(void);
//Bool RegisterCDMLogoBarGui();

// tag / expression plugins
Bool RegisterCDMorphRefPlugin(void);
Bool RegisterCDMorphTagPlugin(void);
Bool RegisterCDMorphMixPlugin(void);
Bool RegisterCDMorphSliderPlugin(void);
Bool RegisterCDSymmetryTag(void);

// menu command plugins
//Bool RegisterAddMorphReference(void);
Bool RegisterSetMorphSelection(void);
Bool RegisterSetHomePosition(void);
Bool RegisterCDCreateMorphTagPlugin(void);
Bool RegisterCDMorphTagToObjectPlugin(void);
Bool RegisterCDMirrorMorphTagPlugin(void);
Bool RegisterCDMorphMixToObjectPlugin(void);
Bool RegisterCDMorphMixToTagPlugin(void);
Bool RegisterCDMorphMixSubtractPlugin(void);
Bool RegisterCDMorphConvertPlugin(void);
Bool RegisterCDMorphSplitPlugin(void);

// message plugins
Bool RegisterCDMorphMessagePlugin(void);

// scene hook plugins
Bool RegisterCDMSceneHook(void);

// Selection logger
Bool RegisterSelectionLog(void);

// library functions
Bool RegisterCDMorphFunctionLib(void);
Bool RegisterCDSymFunctionLib(void);

static AtomArray *cdmTagList = NULL;
static AtomArray *cdmrTagList = NULL;
static AtomArray *cdmxTagList = NULL;
static AtomArray *cdsyTagList = NULL;

static Real version = 1.532;

Bool mSplitDialogOpen = false;
Bool cdsymPlug = false;

// Starts the plugin registration
Bool PluginStart(void)
{
	if(CDFindPlugin(ID_ABOUTCDSYMMETRY,CD_COMMAND_PLUGIN)) cdsymPlug = true;
	
	String pName = "CD Morph";
	if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDMORPH,bc,false);
	
	Bool reg = RegisterCDM();
	
	// Register/About
	if(!RegisterAboutCDMorph()) return false;
	
	// scene hook plugins
	if(!RegisterCDMSceneHook()) return false;
	
	// message plugins
	if(!RegisterCDMorphMessagePlugin()) return false;
	
	cdmTagList = AtomArray::Alloc();
	cdmrTagList = AtomArray::Alloc();
	cdmxTagList = AtomArray::Alloc();

	if(!cdsymPlug)
	{
		cdsyTagList = AtomArray::Alloc();
	}
	
	// Selection logger
	if(RegisterSelectionLog())
	{
		InitSelectionLog();
	}
	
	// tag / expression plugins
	if(!RegisterCDMorphRefPlugin()) return false;
	if(!RegisterCDMorphTagPlugin()) return false;
	if(!RegisterCDMorphMixPlugin()) return false;
	if(!RegisterCDMorphSliderPlugin()) return false;
	
	// menu command plugins
	if(!RegisterSetMorphSelection()) return false;
	if(!RegisterSetHomePosition()) return false;
	if(!RegisterCDCreateMorphTagPlugin()) return false;
	if(!RegisterCDMorphTagToObjectPlugin()) return false;
	if(!RegisterCDMirrorMorphTagPlugin()) return false;
	if(!RegisterCDMorphMixToTagPlugin()) return false;
	if(!RegisterCDMorphMixSubtractPlugin()) return false;
	if(!RegisterCDMorphMixToObjectPlugin()) return false;
	if(!RegisterCDMorphSplitPlugin()) return false;
	
	// library functions
	if(!RegisterCDMorphFunctionLib()) return false;

	if(GetC4DVersion() > 9999)
	{
		if(!RegisterCDMorphConvertPlugin()) return false;
	}
	
	// duplicate tags
	if(!cdsymPlug)
	{
		if(!RegisterCDSymmetryTag()) return false;
		if(!RegisterCDSymFunctionLib()) return false; // cdsy library functions
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
	if(cdmTagList) AtomArray::Free(cdmTagList);
	if(cdmrTagList) AtomArray::Free(cdmrTagList);
	if(cdmxTagList) AtomArray::Free(cdmxTagList);
	
	if(!cdsymPlug)
	{
		if(cdsyTagList) AtomArray::Free(cdsyTagList);
	}

	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	//use the following lines to set a plugin priority
	CDMData *mrd, *md, *mxd;
			
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if(!resource.Init())
			{
				 // don't start plugin without resource
				GePrint("CD Morph failed to load resources");
				return false;
			}
			return true;

		case C4DMSG_PRIORITY: 
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS+95);
			return true;
			
		case ID_CDMORPHMIXPLUGIN:
			mxd = (CDMData *)data;
			mxd->list = cdmxTagList;
			return true;

		case ID_CDMORPHREFPLUGIN:
			mrd = (CDMData *)data;
			mrd->list = cdmrTagList;
			return true;

		case ID_CDMORPHTAGPLUGIN:
			md = (CDMData *)data;
			md->list = cdmTagList;
			return true;

	}
	
	if(!cdsymPlug)
	{
		CDMData *syd;
		switch (id)
		{
			case ID_CDSYMMETRYTAG:
				syd = (CDMData *)data;
				syd->list = cdsyTagList;
				return true;
		}
	}

	return false;
}

// user area
void CDMorphAboutUserArea::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDMorph.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}

void CDMOptionsUA::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	OffScreenOn();
	DrawSetPen(Vector(0.41568, 0.45098, 0.61176)); //gray blue background
	DrawRectangle(0,0,sx,24);
	
	// draw the bitmap
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDMorphOpt.tif"));
	DrawBitmap(bm,0,0,256,24,0,0,256,24,0);
}

void CDMOptionsUA::Sized(LONG w, LONG h) 
{
	//store the width for drawing 
	sx = w;
}

Bool CDMOptionsUA::GetMinSize(LONG& w, LONG& h) 
{
	//the dimensions of the titlebar bitmap
	w = 256;
	h = 24;
	return true;
}

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 10;
	*a = 35;
	*b = 10;
	*c = 86;
}

void ConvertToPointSelection(BaseSelect *bs, BaseSelect *ptS, BaseObject *op, LONG mode)
{
	LONG i, bsCnt, seg, a, b;
	
	if(bs && op && ptS)
	{
		CPolygon *vadr = GetPolygonArray(op);
		switch(mode)
		{
			case Medges:
			{
				bsCnt = bs->GetCount();
				if(bsCnt>0)
				{
					seg=0;
					while(CDGetRange(bs,seg++,&a,&b))
					{
						for (i=a; i<=b; ++i)
						{
							LONG a, b, side;
							side = i-((i/4)*4);
							switch (side)
							{
								case 0:
									a = vadr[i/4].a;
									b = vadr[i/4].b;
									break;
								case 1:
									a = vadr[i/4].b;
									b = vadr[i/4].c;
									break;
								case 2:
									a = vadr[i/4].c;
									b = vadr[i/4].d;
									break;
								case 3:
									a = vadr[i/4].d;
									b = vadr[i/4].a;
									break;
							}
							ptS->Select(a);
							ptS->Select(b);
						}
					}
				}
				break;
			}
			case Mpolygons:
			{
				bsCnt = bs->GetCount();
				if(bsCnt>0)
				{
					seg=0;
					while(CDGetRange(bs,seg++,&a,&b))
					{
						for (i=a; i<=b; ++i)
						{
							ptS->Select(vadr[i].a);
							ptS->Select(vadr[i].b);
							ptS->Select(vadr[i].c);
							ptS->Select(vadr[i].d);
						}
					}
				}
				break;
			}
		}
	}
}

Vector CDMCalcFaceNormal(CDMRefPoint *refadr, const CPolygon &ply)
{
	Vector a,b,c,d;
	
	a = refadr[ply.a].GetVector();
	b = refadr[ply.b].GetVector();
	c = refadr[ply.c].GetVector();
	d = refadr[ply.d].GetVector();
	
	if(ply.c == ply.d)
	{
		return VNorm(VCross((b - a), (c - a)));
	}
	else
	{
		return VNorm(VCross((b - d), (c - a)));
	}
}

Vector CDMCalcPolygonCenter(CDMRefPoint *refadr, const CPolygon &ply)
{
	Vector a,b,c,d;
	
	a = refadr[ply.a].GetVector();
	b = refadr[ply.b].GetVector();
	c = refadr[ply.c].GetVector();
	d = refadr[ply.d].GetVector();
	
	if(ply.c == ply.d)
	{
		return (a+b+c)/3;
	}
	else
	{
		return (a+b+c+d)/4;
	}
}

Vector CDMCalcPointNormal(BaseObject *op, CDMRefPoint *refadr, LONG ind)
{
	Vector pt, norm, ptNorm = Vector(0,0,0);
	
	CPolygon *vadr = GetPolygonArray(op);
	if(vadr)
	{
		LONG pCnt = ToPoint(op)->GetPointCount(), vCnt = ToPoly(op)->GetPolygonCount();
		LONG *dadr = NULL, dcnt = 0, p;
		CPolygon ply;
		
		Neighbor n;
		n.Init(pCnt,vadr,vCnt,NULL);
		n.GetPointPolys(ind, &dadr, &dcnt);

		pt = refadr[ind].GetVector();
		Real totalLen = 0.0;
		for(p=0; p<dcnt; p++)
		{
			ply = vadr[dadr[p]];
			totalLen += Len(pt - CDMCalcPolygonCenter(refadr,ply));
		}
		for(p=0; p<dcnt; p++)
		{
			ply = vadr[dadr[p]];
			norm = CDMCalcFaceNormal(refadr,ply);
			ptNorm += norm * Len(pt - CDMCalcPolygonCenter(refadr,ply))/totalLen;
		}
	}
	else
	{
		ptNorm = refadr[ind].GetVector();
	}
	
	return VNorm(ptNorm);
}


// library functions
CDMorphFuncLib cdmFLib;

CDMRefPoint* iCDGetMorphReference(BaseTag *tag);
CDMRefPoint* iCDGetMorphCache(BaseTag *tag);
CDMPoint* iCDGetMorphPoints(BaseTag *tag);
CDMSpPoint* iCDGetMorphSphericalPoints(BaseTag *tag);

Bool RegisterCDMorphFunctionLib(void)
{
    // Clear the structure
    ClearMem(&cdmFLib, sizeof(cdmFLib));
	
    // Fill in all function pointers
    cdmFLib.CDGetMorphReference = iCDGetMorphReference;
    cdmFLib.CDGetMorphCache = iCDGetMorphCache;
    cdmFLib.CDGetMorphPoints = iCDGetMorphPoints;
    cdmFLib.CDGetMorphSphericalPoints = iCDGetMorphSphericalPoints;
 	
    // Install the library
    return InstallLibrary(ID_CDMORPHFUNCLIB, &cdmFLib, 1, sizeof(cdmFLib));
}

CDSymFuncLib cdsyFLib;

LONG* iCDSymGetMirrorPoints(BaseTag *tag);

Bool RegisterCDSymFunctionLib(void)
{
    // Clear the structure
    ClearMem(&cdsyFLib, sizeof(cdsyFLib));
	
    // Fill in all function pointers
    cdsyFLib.CDSymGetMirrorPoints = iCDSymGetMirrorPoints;
 	
    // Install the library
    return InstallLibrary(ID_CDSYMFUNCLIB, &cdsyFLib, 1, sizeof(cdsyFLib));
}
