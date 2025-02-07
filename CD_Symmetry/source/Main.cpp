//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"

#include "CDSymmetry.h"

#include "oCDDynSym.h"

// forward declarations
Bool RegisterAboutCDMorph(void);
//Bool RegisterCDMLogoBarGui();

// tag plugins
Bool RegisterCDSymmetryTag(void);
Bool RegisterCDLockCenterPoints(void);

// object plugins
Bool RegisterCDDynamicSymmetry(void);

// menu command plugins
Bool RegisterAboutCDSymmetry(void);
Bool RegisterCDAddSymmetryTag(void);
Bool RegisterCDMirrorSelection(void);
Bool RegisterCDSymmetryToTag(void);
Bool RegisterCDAddSymmetryObject(void);
Bool RegisterCDActivateNegative(void);
Bool RegisterCDActivatePositive(void);
Bool RegisterCDUpdateSymmetry(void);
Bool RegisterCDMakeSymmetrical(void);

// tool plugins
Bool RegisterCDSymmetrySelect(void);
Bool RegisterCDSymmetryAssign(void);

// message plugins
Bool RegisterCDSymmetryMessagePlugin(void);

// library funcitons
Bool RegisterCDSymFunctionLib(void);

// Selection logger
Bool RegisterSelectionLog(void);

static Real version = 1.040;

static AtomArray *clTagList = NULL;
static AtomArray *cdsyTagList = NULL;

// Starts the plugin registration
Bool PluginStart(void)
{
	String pName = "CD Symmetry Tools";
	if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDSYMMETRYTOOLS,bc,false);
	
	Bool reg = RegisterCDSY();
	
	// Register/About
	if(!RegisterAboutCDSymmetry()) return false;
	
	// message plugins
	if(!RegisterCDSymmetryMessagePlugin()) return false;
	
	clTagList = AtomArray::Alloc();
	cdsyTagList = AtomArray::Alloc();
	
	// Selection logger
	if(RegisterSelectionLog())
	{
		InitSelectionLog();
	}
	
	// tag plugins
	if(!RegisterCDLockCenterPoints()) return false;
	
	// object plugins
	if(!RegisterCDDynamicSymmetry()) return false;
	
	// menu command plugins
	if(!RegisterCDAddSymmetryTag()) return false;
	if(!RegisterCDMirrorSelection()) return false;
	if(!RegisterCDSymmetryToTag()) return false;
	if(!RegisterCDAddSymmetryObject()) return false;
	if(!RegisterCDActivateNegative()) return false;
	if(!RegisterCDActivatePositive()) return false;
	if(!RegisterCDUpdateSymmetry()) return false;
	if(!RegisterCDMakeSymmetrical()) return false;
	
	// tool plugins
	if(!RegisterCDSymmetrySelect()) return false;
	if(!RegisterCDSymmetryAssign()) return false;
	
	String license = "";
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
	
	// duplicate plugins
	if(reg)
	{
		if(!RegisterCDSymmetryTag()) return false;
	}
	
	// library funcitons
	if(!RegisterCDSymFunctionLib()) return false;
	
	return true;
}

void PluginEnd(void)
{
	if(clTagList) AtomArray::Free(clTagList);
	if(cdsyTagList) AtomArray::Free(cdsyTagList);
	
	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	CDSData *cld, *syd;
	
	//use the following lines to set a plugin priority
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if(!resource.Init())
			{
				GePrint("CD Symmetry Tools failed to load resources");
				return false; // don't start plugin without resource
			}
			return true;

		case C4DMSG_PRIORITY: 
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS+96);
			return true;
			
		case ID_CDCENTERLOCKTAG:
			cld = (CDSData *)data;
			cld->list = clTagList;
			return true;
			
		case ID_CDSYMMETRYTAG:
			syd = (CDSData *)data;
			syd->list = cdsyTagList;
			return true;
			
	}

	return false;
}

// user area
void CDSymmetryAboutUserArea::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDSymTools.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}

void CDSymOptionsUA::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	OffScreenOn();
	DrawSetPen(Vector(0.41568, 0.45098, 0.61176)); //gray blue background
	DrawRectangle(0,0,sx,24);
	
	// draw the bitmap
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDSymToolsOpt.tif"));
	DrawBitmap(bm,0,0,256,24,0,0,256,24,0);
}

void CDSymOptionsUA::Sized(LONG w, LONG h) 
{
	//store the width for drawing 
	sx = w;
}

Bool CDSymOptionsUA::GetMinSize(LONG& w, LONG& h) 
{
	//the dimensions of the titlebar bitmap
	w = 256;
	h = 24;
	return true;
}

// common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 16;
	*a = 97;
	*b = -126;
	*c = 131;
}

BaseObject* GetDynSymParentObject(BaseObject *op)
{
	while(op)
	{
		if(op->IsInstanceOf(ID_CDDYNSYMMETRY)) return op;
		op = op->GetUp();
	}
	
	return NULL;
}

void CDOptimize(BaseDocument *doc, BaseObject *op, Real t, Bool pt, Bool ply, Bool un, LONG mode)
{
	if(op && doc)
	{
		ModelingCommandData mcd;
		mcd.doc = doc;
		mcd.op = op;
	#if API_VERSION < 12000
		mcd.mode = mode;
	#else
		mcd.mode = (MODELINGCOMMANDMODE)mode;
	#endif
		
		BaseContainer bc;
		bc.SetReal(MDATA_OPTIMIZE_TOLERANCE,t);
		bc.SetReal(MDATA_OPTIMIZE_POINTS,pt);
		bc.SetReal(MDATA_OPTIMIZE_POLYGONS,ply);
		bc.SetReal(MDATA_OPTIMIZE_UNUSEDPOINTS,un);
		mcd.bc = &bc;
		
		SendModelingCommand(MCOMMAND_OPTIMIZE,mcd);
	}
}

void StoreBrigeToolData(BaseDocument *doc)
{
	//GePrint("StoreBrigeToolData");
	BaseContainer *tlData = GetToolPluginData(doc,ID_MODELING_BRIDGE_TOOL);
	if(tlData)
	{
		BaseContainer bc;
		bc.SetLong(MDATA_BRIDGE_ELEMENT1,tlData->GetLong(MDATA_BRIDGE_ELEMENT1));
		bc.SetLong(MDATA_BRIDGE_ELEMENT2,tlData->GetLong(MDATA_BRIDGE_ELEMENT2));
		bc.SetLong(MDATA_BRIDGE_ELEMENT3,tlData->GetLong(MDATA_BRIDGE_ELEMENT3));
		bc.SetLong(MDATA_BRIDGE_ELEMENT4,tlData->GetLong(MDATA_BRIDGE_ELEMENT4));
		bc.SetLong(MDATA_BRIDGE_OBJINDEX1,tlData->GetLong(MDATA_BRIDGE_OBJINDEX1));
		bc.SetLong(MDATA_BRIDGE_OBJINDEX2,tlData->GetLong(MDATA_BRIDGE_OBJINDEX2));
		bc.SetLong(MDATA_BRIDGE_OBJINDEX3,tlData->GetLong(MDATA_BRIDGE_OBJINDEX3));
		bc.SetLong(MDATA_BRIDGE_OBJINDEX4,tlData->GetLong(MDATA_BRIDGE_OBJINDEX4));
		bc.SetBool(MDATA_BRIDGE_DELETE,tlData->GetBool(MDATA_BRIDGE_DELETE));
		bc.SetBool(MDATA_BRIDGE_ISO,tlData->GetBool(MDATA_BRIDGE_ISO));
		SetWorldPluginData(ID_CDSYMMETRYUPDATE,bc,false);
	}
}

void RestoreBrigeToolData(BaseDocument *doc)
{
	//GePrint("RestoreBrigeToolData");
	BaseContainer *tlData = GetToolPluginData(doc,ID_MODELING_BRIDGE_TOOL);
	BaseContainer *wpData = GetWorldPluginData(ID_CDSYMMETRYUPDATE);
	if(tlData && wpData)
	{
		tlData->SetLong(MDATA_BRIDGE_ELEMENT1,wpData->GetLong(MDATA_BRIDGE_ELEMENT1));
		tlData->SetLong(MDATA_BRIDGE_ELEMENT2,wpData->GetLong(MDATA_BRIDGE_ELEMENT2));
		tlData->SetLong(MDATA_BRIDGE_ELEMENT3,wpData->GetLong(MDATA_BRIDGE_ELEMENT3));
		tlData->SetLong(MDATA_BRIDGE_ELEMENT4,wpData->GetLong(MDATA_BRIDGE_ELEMENT4));
		tlData->SetLong(MDATA_BRIDGE_OBJINDEX1,wpData->GetLong(MDATA_BRIDGE_OBJINDEX1));
		tlData->SetLong(MDATA_BRIDGE_OBJINDEX2,wpData->GetLong(MDATA_BRIDGE_OBJINDEX2));
		tlData->SetLong(MDATA_BRIDGE_OBJINDEX3,wpData->GetLong(MDATA_BRIDGE_OBJINDEX3));
		tlData->SetLong(MDATA_BRIDGE_OBJINDEX4,wpData->GetLong(MDATA_BRIDGE_OBJINDEX4));
		tlData->SetBool(MDATA_BRIDGE_DELETE,wpData->GetBool(MDATA_BRIDGE_DELETE));
		tlData->SetBool(MDATA_BRIDGE_ISO,wpData->GetBool(MDATA_BRIDGE_ISO));
	}
}

void CacheToMesh(BaseDocument *doc, BaseObject *pr, BaseObject *op)
{
	if(op && pr)
	{
		BaseObject *cln = (BaseObject*)CDGetClone(pr->GetCache(),CD_COPYFLAGS_0,NULL);
		if(cln)
		{
			cln->SetName(op->GetName());
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDTransferGoals(op,cln);
														
			doc->InsertObject(cln,pr,NULL,false);
			if(op->GetBit(BIT_ACTIVE)) doc->SetActiveObject(cln,SELECTION_ADD);
			
			BaseTag *clTag = cln->GetTag(ID_CDCENTERLOCKTAG);
			if(!clTag)
			{
				clTag = BaseTag::Alloc(ID_CDCENTERLOCKTAG);
				cln->InsertTag(clTag,NULL);
			}
			
			BaseContainer *tData = clTag->GetDataInstance();
			if(tData)
			{
				BaseContainer *oData = pr->GetDataInstance();
				if(oData)
				{
					switch(oData->GetLong(DS_SYMMETRY_AXIS))
					{
						case DS_MX:
							tData->SetLong(CL_SYMMETRY_AXIS,0);
							break;
						case DS_MY:
							tData->SetLong(CL_SYMMETRY_AXIS,1);
							break;
						case DS_MZ:
							tData->SetLong(CL_SYMMETRY_AXIS,2);
							break;
					}
					tData->SetReal(CL_TOLERANCE,oData->GetReal(DS_TOLERANCE));
					tData->SetBool(CL_LOCK_ON,oData->GetBool(DS_LOCK_CENTER));
					clTag->Message(CD_MSG_UPDATE);
				}
			}
			CDAddUndo(doc,CD_UNDO_NEW,cln);
			
			CDAddUndo(doc,CD_UNDO_DELETE,op);
			op->Remove();
			BaseObject::Free(op);
			
		}
	}
}

LONG GetEdgeSymmetry(BaseObject *op, LONG *sym, LONG ind)
{
	LONG symInd = ind;
	
	CPolygon *vadr = GetPolygonArray(op);
	
	LONG a, b, side;
	side = ind-((ind/4)*4);
	switch (side)
	{
		case 0:
			a = vadr[ind/4].a;
			b = vadr[ind/4].b;
			break;
		case 1:
			a = vadr[ind/4].b;
			b = vadr[ind/4].c;
			break;
		case 2:
			a = vadr[ind/4].c;
			b = vadr[ind/4].d;
			break;
		case 3:
			a = vadr[ind/4].d;
			b = vadr[ind/4].a;
			break;
	}
	
	if(sym[a] != -1 && sym[b] != -1)
	{
		LONG vCnt = ToPoly(op)->GetPolygonCount(), pCnt = ToPoint(op)->GetPointCount();;
		LONG *dadr = NULL, dcnt = 0, p, side;
		LONG eptA, eptB;
		
		if(sym[a] > -1) eptA = sym[a]; else eptA = a;
		if(sym[b] > -1) eptB = sym[b]; else eptB = b;
		
		Neighbor n;
		if(n.Init(pCnt,vadr,vCnt,NULL))
		{
			n.GetPointPolys(eptA, &dadr, &dcnt);
			
			for (p=0; p<dcnt; p++)
			{
				CPolygon ply = vadr[dadr[p]];
				PolyInfo *pli = n.GetPolyInfo(dadr[p]);
				
				for(side=0; side<4; side++)
				{
					if(pli->mark[side] || side==2 && ply.c==ply.d) continue;
					
					switch (side)
					{
						case 0:
						{
							if(ply.a == eptA && ply.b == eptB || ply.b == eptA && ply.a == eptB) symInd = 4*dadr[p]+side;
							break;
						}
						case 1:
						{
							if(ply.b == eptA && ply.c == eptB || ply.c == eptA && ply.b == eptB)  symInd = 4*dadr[p]+side;
							break;
						}
						case 2:
						{
							if(ply.c == eptA && ply.d == eptB || ply.d == eptA && ply.c == eptB)  symInd = 4*dadr[p]+side;
							break;
						}
						case 3:
						{
							if(ply.d == eptA && ply.a == eptB || ply.a == eptA && ply.d == eptB)  symInd = 4*dadr[p]+side;
							break;
						}
					}
				}
			}
		}
	}
	
	return symInd;
}

LONG GetPolySymmetry(BaseObject *op, LONG *sym, LONG ind)
{
	LONG symInd = ind;
	
	LONG a, b, c, d, i, pt[4];
	CPolygon *vadr = GetPolygonArray(op);
	a = vadr[ind].a;
	b = vadr[ind].b;
	c = vadr[ind].c;
	d = vadr[ind].d;
	
	if(sym[a] != -1 && sym[b] != -1 && sym[c] != -1 && sym[d] != -1)
	{
		if(sym[a] > -1) pt[0] = sym[a]; else pt[0] = a;
		if(sym[b] > -1) pt[1] = sym[b]; else pt[1] = b;
		if(sym[c] > -1) pt[2] = sym[c]; else pt[2] = c;
		if(sym[d] > -1) pt[3] = sym[d]; else pt[3] = d;
		
		CPolygon *vadr = GetPolygonArray(op);
		LONG vCnt = ToPoly(op)->GetPolygonCount(), pCnt = ToPoint(op)->GetPointCount();;
		LONG *dadr = NULL, dcnt = 0, p;
		
		Neighbor n;
		if(n.Init(pCnt,vadr,vCnt,NULL))
		{
			n.GetPointPolys(pt[0], &dadr, &dcnt);
			for (p=0; p<dcnt; p++)
			{						
				CPolygon ply = vadr[dadr[p]];
				Bool plyPt[4];
				
				for(i=0; i<4; i++)
				{
					plyPt[i] = false;
				}
				
				for(i=0; i<4; i++)
				{
					if(pt[i] == ply.a || pt[i] == ply.b || pt[i] == ply.c || pt[i] == ply.d) plyPt[i] = true;
				}
				
				if(plyPt[0] && plyPt[1] && plyPt[2] && plyPt[3]) symInd = dadr[p];
			}
		}
	}
	
	return symInd;
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
							LONG aPt, bPt, side;
							side = i-((i/4)*4);
							switch (side)
							{
								case 0:
									aPt = vadr[i/4].a;
									bPt = vadr[i/4].b;
									break;
								case 1:
									aPt = vadr[i/4].b;
									bPt = vadr[i/4].c;
									break;
								case 2:
									aPt = vadr[i/4].c;
									bPt = vadr[i/4].d;
									break;
								case 3:
									aPt = vadr[i/4].d;
									bPt = vadr[i/4].a;
									break;
							}
							ptS->Select(aPt);
							ptS->Select(bPt);
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


// library funcitons
CDSymFuncLib cdsyFLib;

LONG* iCDSymGetMirrorPoints(BaseTag *tag);

Bool RegisterCDSymFunctionLib(void)
{
	if(CDFindPlugin(ID_CDSYMMETRYTAG,CD_TAG_PLUGIN)) return true;

    // Clear the structure
    ClearMem(&cdsyFLib, sizeof(cdsyFLib));
	
    // Fill in all function pointers
    cdsyFLib.CDSymGetMirrorPoints = iCDSymGetMirrorPoints;
 	
    // Install the library
    return InstallLibrary(ID_CDSYMFUNCLIB, &cdsyFLib, 1, sizeof(cdsyFLib));
}
