//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "cdtransfer.h"

Bool RegisterAboutCDTransfer(void);

// forward declarations
Bool RegisterCDZeroTransfromationPlugin(void);
Bool RegisterCDTransferSelectedTag(void);

Bool RegisterCDAddZeroTransformation(void);
Bool RegisterCDAddTransferSelected(void);
Bool RegisterCDCoordinatesPlugin(void);
Bool RegisterCDTransferCommand(void);
Bool RegisterCDTMirrorCommand(void);
Bool RegisterCDReplaceCommand(void);
Bool RegisterCDTSwapCommand(void);
Bool RegisterCDTAlignCommand(void);
Bool RegisterCDTransferLinks(void);
Bool RegisterSetZeroHomePosition(void);
Bool RegisterCDSelectAllSame(void);
Bool RegisterCDTransferUserData(void);
Bool RegisterCDTransferAnimation(void);
Bool RegisterCDTransAxisCommand(void);

Bool RegisterZeroGlobalRotation(void);
Bool RegisterZeroLocalRotation(void);
Bool RegisterCDLinkObjects(void);
Bool RegisterCDUnLinkObjects(void);
Bool RegisterCDFreezeTransformation(void);

Bool RegisterCDTransferMessagePlugin(void);
Bool RegisterCDTransferSelectedCommand(void);

// Selection logger
Bool RegisterSelectionLog(void);

static Real version = 1.101;

static AtomArray *trnsTagList = NULL;
static AtomArray *ztTagList = NULL;
static AtomArray *taTagList = NULL;

// Starts the plugin registration
Bool PluginStart(void)
{
	String pName = "CD Transfer Tools";
	if(!CDCheckC4DVersion(pName)) return false;
	
	BaseContainer bc;
	bc.SetReal(CD_VERSION,version);
	SetWorldPluginData(ID_CDTRANSFERTOOLS,bc,false);
	
	Bool reg = RegisterCDTR();
		
	// Register/About
	if(!RegisterAboutCDTransfer()) return false;
	
	// message plugins
	if(!RegisterCDTransferMessagePlugin()) return false;
	if(!RegisterCDTransferSelectedCommand())
	{
		GePrint("RegisterCDTransferSelectedCommand - FAILED");
		return false;
	}
	
	// Allocate the lists
	trnsTagList = AtomArray::Alloc();
	ztTagList = AtomArray::Alloc();
	taTagList = AtomArray::Alloc();
	
	// Selection logger
	if(RegisterSelectionLog())
	{
		InitSelectionLog();
	}
	
	// tag / expression plugins
	if(!RegisterCDZeroTransfromationPlugin()) return false;
	if(!RegisterCDTransferSelectedTag()) return false;
	
	// command plugins
	if(!RegisterCDAddZeroTransformation()) return false;
	if(!RegisterCDAddTransferSelected()) return false;
	if(!RegisterCDCoordinatesPlugin()) return false;
	if(!RegisterCDTransferCommand()) return false;
	if(!RegisterCDTMirrorCommand()) return false;
	if(!RegisterCDReplaceCommand()) return false;
	if(!RegisterCDTSwapCommand()) return false;
	if(!RegisterCDTAlignCommand()) return false;
	if(!RegisterCDTransferLinks()) return false;
	if(!RegisterSetZeroHomePosition()) return false;
	if(!RegisterCDSelectAllSame()) return false;
	if(!RegisterCDTransferUserData()) return false;
	if(!RegisterCDTransferAnimation()) return false;
	if(!RegisterCDTransAxisCommand()) return false;
	
	// duplicate commands
	if(reg)
	{
		if(!RegisterCDLinkObjects()) return false;
		if(!RegisterCDUnLinkObjects()) return false;
		if(!RegisterZeroGlobalRotation()) return false;
		if(!RegisterCDFreezeTransformation()) return false;
		if(!RegisterZeroLocalRotation()) return false;
	}
	
	
	String license = "";
	if(!reg) license = " - Runtime";
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO)  license = " - Demo";
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
	AtomArray::Free(trnsTagList);
	AtomArray::Free(ztTagList);
	AtomArray::Free(taTagList);
	
	FreeSelectionLog();
}

Bool PluginMessage(LONG id, void *data)
{
	CDTRData *trnsD, *ztD;
	
	//use the following lines to set a plugin priority
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init())
			{
				GePrint("CD Transfer Tools failed to load resources");
				return false; // don't start plugin without resource
			}
			return true;

		case C4DMSG_PRIORITY: 
			SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS+94);
			return true;
			
		case ID_CDTRANSELECTEDTAG:
			trnsD = (CDTRData *)data;
			trnsD->list = trnsTagList;
			return true;
			
		case ID_CDZEROTRANSTAG:
			ztD = (CDTRData *)data;
			ztD->list = ztTagList;
			return true;
			
	}

	return false;
}

// user area
void CDTransferAboutUserArea::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDTransTools.tif"));
	DrawBitmap(bm,0,0,320,60,0,0,320,60,0);
}

void CDTROptionsUA::CDDraw(LONG x1,LONG y1,LONG x2,LONG y2, const BaseContainer& msg)
{
	bm->Init(GeGetPluginPath() + Filename("res") + Filename("CDTransOpt.tif"));

	OffScreenOn();
	DrawSetPen(Vector(0.41568, 0.45098, 0.61176)); //gray blue background
	DrawRectangle(0,0,sx,24);
	
	// draw the bitmap
	DrawBitmap(bm,0,0,256,24,0,0,256,24,0);
}

void CDTROptionsUA::Sized(LONG w, LONG h) 
{
	//store the width for drawing 
	sx = w;
}

Bool CDTROptionsUA::GetMinSize(LONG& w, LONG& h) 
{
	//the dimensions of the titlebar bitmap
	w = 256;
	h = 24;
	return true;
}

// Common functions
void SetRValues(LONG *p, CHAR *a, CHAR *b, CHAR *c)
{
	*p = 6;
	*a = 32;
	*b = 0;
	*c = 230;
}

void RecalculateReference(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM)
{
	CDTransMatrixData tmData;
	tmData.nM = newM;
	tmData.oM = oldM;
	
	BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN);
	if(skrTag)
	{
		skrTag->Message(CD_MSG_RECALC_REFERENCE,&tmData);
		
		BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
		if(skTag)
		{
			BaseContainer *skData = skTag->GetDataInstance();
			if(!skData->GetBool(CDSK_BOUND))
			{
				DescriptionCommand dc;
				dc.id = DescID(CDSKR_RESTORE_REFERENCE);
				skrTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
			}
		}
	}
	BaseTag *mrTag = op->GetTag(ID_CDMORPHREFPLUGIN);
	if(mrTag) mrTag->Message(CD_MSG_RECALC_REFERENCE,&tmData);
}

void RecalculateComponents(BaseDocument *doc, BaseObject *op, Matrix newM, Matrix oldM)
{
	if(doc && op && IsValidPointObject(op))
	{
		AutoAlloc<BaseSelect> ptSelStored;
		AutoAlloc<BaseSelect> edgSelStored;
		AutoAlloc<BaseSelect> plySelStored;
		StoreSelections(op, ptSelStored, edgSelStored, plySelStored);
		
		AutoAlloc<BaseSelect> bs;
		if(bs)
		{
			switch(doc->GetMode())
			{
				case Mpoints:
				{
					BaseSelect *ptSel = ToPoint(op)->GetPointS();
					bs->Merge(ptSel);
					break;
				}
				case Medges:;
				{
					if(IsValidPolygonObject(op))
					{
						BaseSelect *edgeS = ToPoly(op)->GetEdgeS();
						if(edgeS) ConvertToPointSelection(edgeS,bs,op,Medges);
					}
					break;
				}
				case Mpolygons:
				{
					if(IsValidPolygonObject(op))
					{
						BaseSelect *plyS = ToPoly(op)->GetPolygonS();
						if(plyS) ConvertToPointSelection(plyS,bs,op,Mpolygons);
					}
					break;
				}
			}
			
			Matrix opM = op->GetMg();
			
			LONG bsCnt = bs->GetCount();
			if(bsCnt > 0)
			{
				Vector *padr = GetPointArray(op);
				LONG i,seg=0,a,b;
				while(CDGetRange(bs,seg++,&a,&b))
				{
					for (i=a; i<=b; ++i)
					{
						Vector pt = opM * padr[i];
						padr[i] = MInv(opM) * newM * MInv(oldM) * pt;
					}
				}
			}
		}
		
		RestoreSelections(op, ptSelStored, edgSelStored, plySelStored);
	}
}

Bool ElementsSelected(BaseDocument *doc, BaseObject *op)
{
	if(!IsValidPointObject(op)) return false;
	
	BaseSelect *bs = NULL;
	
	switch (doc->GetMode())
	{
		case Mpoints:
		{
			bs = ToPoint(op)->GetPointS();
			if(bs)
			{
				if(bs->GetCount() > 0) return true;
			}
			break;
		}
		case Medges:;
		{
			if(IsValidPolygonObject(op))
			{
				bs = ToPoly(op)->GetEdgeS();
				if(bs)
				{
					if(bs->GetCount() > 0) return true;
				}
			}
			break;
		}
		case Mpolygons:
		{
			if(IsValidPolygonObject(op))
			{
				bs = ToPoly(op)->GetPolygonS();
				if(bs)
				{
					if(bs->GetCount() > 0) return true;
				}
			}
			break;
		}
	}
	
	return false;
}

Matrix GetEditModeTransferMatrix(BaseDocument *doc, BaseObject *target)
{
	Matrix targM;
	Vector *padr = GetPointArray(target);
	BaseSelect *edgeS = NULL, *plyS = NULL, *ptSel = ToPoint(target)->GetPointS();
	LONG i;
	
	Bool singlePly = false;
	AutoAlloc<BaseSelect> bs;
	if(bs)
	{
		switch (doc->GetMode())
		{
			case Mpoints:
			{
				bs->Merge(ptSel);
				break;
			}
			case Medges:;
			{
				if(IsValidPolygonObject(target))
				{
					edgeS = ToPoly(target)->GetEdgeS();
					if(edgeS) ConvertToPointSelection(edgeS,bs,target,Medges);
				}
				break;
			}
			case Mpolygons:
			{
				if(IsValidPolygonObject(target))
				{
					plyS = ToPoly(target)->GetPolygonS();
					if(plyS)
					{
						LONG plyCnt = plyS->GetCount();
						if(plyCnt == 1) singlePly = true;
						ConvertToPointSelection(plyS,bs,target,Mpolygons);
					}
				}
				break;
			}
		}
		Vector norm = Vector(0,0,0);
		if(singlePly)
		{
			LONG seg=0,a,b;
			CDGetRange(plyS,seg++,&a,&b);
			CPolygon *vadr = GetPolygonArray(target);
			CPolygon ply = vadr[a];
			
			norm = CalcFaceNormal(padr,ply);
			targM = target->GetMg() * CDHPBToMatrix(VectorToHPB(VNorm(norm)));
			targM.off = target->GetMg() * CalcPolygonCenter(padr,ply);
		}
		else
		{
			LONG bsCnt = bs->GetCount();
			if(bsCnt > 0)
			{
				CDAABB ptBounds;
				ptBounds.Empty();
				
				LONG seg=0,a,b;
				
				while(CDGetRange(bs,seg++,&a,&b))
				{
					for (i=a; i<=b; ++i)
					{
						Vector pt = padr[i];
						norm += CalcPointNormal(target, padr, i) * 1.0/(Real)bsCnt;
						ptBounds.AddPoint(pt);
					}
				}
				targM = target->GetMg() * CDHPBToMatrix(VectorToHPB(VNorm(norm)));
				targM.off = target->GetMg() * ptBounds.GetCenterPoint();
			}
		}
	}
	else
	{
		targM = target->GetMg();
	}
	target->Message(MSG_UPDATE);
	
	return targM;
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

