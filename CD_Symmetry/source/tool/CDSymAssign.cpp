//	Cactus Dan's Symmetry Tools 1.0
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_sds.h"

#include "CDSymmetry.h"
#include "CDToolData.h"
#include "CDSymmetryTag.h"

#include "tCDSymTag.h"

class CDSymAssignDialog: public SubDialog
{
public:
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool InitDialog(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDSymAssignDialog::InitDialog(void)
{
	BaseContainer *bc = GetToolData(GetActiveDocument(),ID_CDSYMMETRYASSIGN);
	if (!bc) return false;
	
	return true;
}

Bool CDSymAssignDialog::CreateLayout(void)
{
	GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
	{
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupSpace(4,1);
			AddCheckbox(D_VISIBLE_ONLY,BFH_LEFT,0,0,GeLoadString(D_VISIBLE_ONLY));
		}
		GroupEnd();
		
	}
	GroupEnd();
	
	return true;
}

Bool CDSymAssignDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDSYMMETRYASSIGN);
	if (!bc)
	{
		SetBool(D_VISIBLE_ONLY, true);
	}
	else
	{
		SetBool(D_VISIBLE_ONLY, bc->GetBool(D_VISIBLE_ONLY));
	}
	
	return InitDialog();
}

Bool CDSymAssignDialog::Command(LONG id,const BaseContainer &msg)
{
	Bool vo;
	
	GetBool(D_VISIBLE_ONLY,vo);
	
	BaseContainer wpData;
	wpData.SetBool(D_VISIBLE_ONLY,vo);
	SetWorldPluginData(ID_CDSYMMETRYASSIGN,wpData,false);
	
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *bc = GetToolData(doc,ID_CDSYMMETRYASSIGN);
	if (!bc) return false;
	
	if(bc->GetBool(D_VISIBLE_ONLY) != vo)
	{
		bc->SetBool(D_VISIBLE_ONLY,vo);
		CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	}
	
	return true;
}

class CDSymmetryAssign : public CDToolData
{
private:
	Real		mx, my, rad;
	Bool		mDrag, click1, click2;
	LONG		ptA, ptB; 
	
public:
	virtual Bool InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt);
	virtual void FreeTool(BaseDocument* doc, BaseContainer& data);
	
	virtual Bool MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
	virtual Bool KeyboardInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
	virtual LONG GetState(BaseDocument *doc);
	virtual Bool GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x, Real y, BaseContainer &bc);
	
	virtual LONG CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags);
	
	virtual SubDialog*	AllocSubDialog(BaseContainer* bc) { return CDDataAllocator(CDSymAssignDialog); }
};

Bool CDSymmetryAssign::InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt)
{
	BaseContainer *bc = GetWorldPluginData(ID_CDSYMMETRYASSIGN);
	if(!bc)
	{
		data.SetReal(D_SYM_SEL_RAD,10);
		data.SetBool(D_VISIBLE_ONLY,true);
	}
	else
	{
		data.SetReal(D_SYM_SEL_RAD,bc->GetReal(D_SYM_SEL_RAD));
		data.SetBool(D_VISIBLE_ONLY, bc->GetBool(D_VISIBLE_ONLY));
	}
	
	ptA = -1;
	ptB = -1;
	click1 = false;
	click2 = false;
	
	if(doc->GetMode() != Mpoints) doc->SetMode(Mpoints);
	
	return true;
}

void CDSymmetryAssign::FreeTool(BaseDocument* doc, BaseContainer& data)
{
}

LONG CDSymmetryAssign::GetState(BaseDocument *doc)
{
	if(doc->GetMode()==Mpaint) return 0;
	return CMD_ENABLED;
}

Bool CDSymmetryAssign::KeyboardInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg)
{
	BaseObject	*op = doc->GetActiveObject(); if(!op) return false;
	
	LONG key = msg.GetData(BFM_INPUT_CHANNEL).GetLong();
	String str = msg.GetData(BFM_INPUT_ASC).GetString();
	if(key == KEY_ESC)
	{
		if(click1)
		{
			doc->EndUndo();
		}
		
		ptA = -1;
		ptB = -1;
		click1 = false;
		click2 = false;
	}
	return false;
}

LONG CDSymmetryAssign::CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags)
{
	BaseObject	*op = doc->GetActiveObject(); if(!op) return CD_TOOLDRAW_0;
	if(!IsValidPointObject(op)) return CD_TOOLDRAW_0;
	
	BaseTag *tag = op->GetTag(ID_CDSYMMETRYTAG); if(!tag) return CD_TOOLDRAW_0;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return CD_TOOLDRAW_0;
	if(!tData->GetBool(SYM_SYMMETRY_IS_SET)) return CD_TOOLDRAW_0;
	
	CDSymmetryTag *symTag = static_cast<CDSymmetryTag*>(tag->GetNodeData()); if(!symTag) return CD_TOOLDRAW_0;
	
	Bool isoEdit = bd->GetParameterData(BASEDRAW_DATA_SDSEDIT).GetLong();
	LONG *sym = symTag->GetMirrorPoints();
	
	Matrix opM = op->GetMg();
	Vector *padr = GetPointArray(op);
	LONG i, ptCnt = ToPoint(op)->GetPointCount();
	
	// check for HyperNURBS parent
	Vector *sadr = NULL;
	BaseObject *hnPr = GetHNParentObject(op);
	if(hnPr && hnPr->GetDeformMode() && isoEdit) 
	{
		SDSObject *sdsOp = static_cast<SDSObject*>(hnPr);
		if(sdsOp)
		{
			PolygonObject *sdsMesh = sdsOp->GetSDSMesh(op);
			if(sdsMesh) sadr = GetPointArray(sdsMesh);
		}
	}
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	Bool vOnly = data.GetBool(D_VISIBLE_ONLY);
	
	AutoAlloc<GeRayCollider> rc;
	
	bd->SetPen(Vector(1,0,0));
	for(i=0; i<ptCnt; i++)
	{
		Vector pt = opM * padr[i];
		if(sym[i] == -1)
		{
			if(sadr) pt = opM * sadr[i];
			
			if(vOnly && IsValidPolygonObject(op))
			{
				if(rc && rc->Init(op, false))
				{
					Vector opt = bd->WS(pt);
					opt.z = 0.0;
					Vector rayEnd = bd->SW(opt);
					Real rayLen = Len(rayEnd - pt);
					Vector rayDir = CDTransformVector((rayEnd - pt), MInv(opM));
					Vector rayPt = padr[i] + VNorm(rayDir) * 0.1;
					if(!rc->Intersect(rayPt, VNorm(rayDir), rayLen))
						CDDrawHandle3D(bd, pt, CD_DRAWHANDLE_MIDDLE, 0);
				}
			}
			else CDDrawHandle3D(bd, pt, CD_DRAWHANDLE_MIDDLE, 0);
		}
	}
	
	if(click1 && ptA > -1)
	{
		Vector start, end;
		bd->SetPen(Vector(1,1,0));
		
		start = bd->WS(opM*padr[ptA]);
		if(sadr) start = bd->WS(opM*sadr[ptA]);
		start.z = 0.0;
		
		CDSetBaseDrawMatrix(bd, NULL, Matrix());
		
		CDDrawHandle2D(bd,start,CD_DRAWHANDLE_BIG);
		if(mx > -1 && my > -1 && (flags & CD_TOOLDRAWFLAGS_HIGHLIGHT))
		{
			end.x = mx;
			end.y = my;
			end.z = 0.0;
			CDDrawLine2D(bd,start,end);
		}
	}
	
	return CD_TOOLDRAW_HIGHLIGHTS|CD_TOOLDRAW_AXIS;
}

Bool CDSymmetryAssign::MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg)
{
	BaseObject	*op = doc->GetActiveObject(); if(!op) return false;
	if(!IsValidPointObject(op)) return false;
	
	BaseTag *tag = op->GetTag(ID_CDSYMMETRYTAG); if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(SYM_SYMMETRY_IS_SET)) return false;
	
	mx = msg.GetReal(BFM_INPUT_X);
	my = msg.GetReal(BFM_INPUT_Y);
	LONG button;
	
	switch (msg.GetLong(BFM_INPUT_CHANNEL))
	{
		case BFM_INPUT_MOUSELEFT : button=KEY_MLEFT; break;
		case BFM_INPUT_MOUSERIGHT: button=KEY_MRIGHT; break;
		default: return false;
	}
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	Bool ctrl = (state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL);
	Bool shft = (state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT);
	
	Vector *padr = GetPointArray(op);
	
	Matrix opM = op->GetMg();
	
	rad = 10.0;
	Bool vOnly = data.GetBool(D_VISIBLE_ONLY);
	
	AutoAlloc<ViewportSelect> vps; if(!vps) return false;
	
	LONG left, top, right, bottom, width, height;
	bd->GetFrame(&left, &top, &right, &bottom);
	width = right - left + 1;
	height = bottom - top + 1;
	
#if API_VERSION < 12000
	vps->Init(width,height,bd,op,Mpoints,vOnly,VIEWPORT_USE_HN|VIEWPORT_USE_DEFORMERS);
#else
	vps->Init(width,height,bd,op,Mpoints,vOnly,VIEWPORTSELECTFLAGS_USE_HN|VIEWPORTSELECTFLAGS_USE_DEFORMERS);
#endif
	
	LONG x = mx, y = my;
	ViewportPixel *vp = vps->GetNearestPoint(op,x,y,rad);
	if(vp)
	{
		if(!click1 && !click2)
		{
			ptA = vp->i;
			click1 = true;
			doc->StartUndo();
		}
		else if(click1 && !click2)
		{
			ptB = vp->i;
			click2 = true;
		}
	}
	
	CDSymmetryTag *symTag = static_cast<CDSymmetryTag*>(tag->GetNodeData());
	if(symTag)
	{
		LONG *sym = symTag->GetMirrorPoints();
		if(sym)
		{
			if(shft && ptA > -1)
			{
				if(sym[ptA] == -1)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					sym[ptA] = -2;
					
					if(!tData->GetBool(SYM_RESTRICT_SYM))
					{
						switch(tData->GetLong(SYM_SYMMETRY_AXIS))
						{
							case SYM_MX:
								padr[ptA].x = 0.0;
								break;
							case SYM_MY:
								padr[ptA].y = 0.0;
								break;
							case SYM_MZ:
								padr[ptA].z = 0.0;
								break;
						}
					}
				}
				click2 = true;
			}
			else if(ctrl && ptA > -1)
			{
				if(sym[ptA] != -1)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					if(sym[ptA] > -1)
					{
						LONG sInd = sym[ptA];
						sym[sInd] = -1;
					}
					sym[ptA] = -1;
				}
				click2 = true;
			}
			else
			{
				if(ptA > -1 && ptB > -1 && ptA != ptB)
				{
					if(sym[ptA] == -1 && sym[ptB] == -1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						
						sym[ptA] = ptB;
						sym[ptB] = ptA;
						
						if(!tData->GetBool(SYM_RESTRICT_SYM))
						{
							padr[ptA] = padr[ptB];
							switch(tData->GetLong(SYM_SYMMETRY_AXIS))
							{
								case SYM_MX:
									padr[ptA].x = -padr[ptB].x;
									break;
								case SYM_MY:
									padr[ptA].y = -padr[ptB].y;
									break;
								case SYM_MZ:
									padr[ptA].z = -padr[ptB].z;
									break;
							}
							
						}
					}
				}
			}
			tag->Message(MSG_UPDATE);
			op->Message(MSG_UPDATE);
		}
	}
	
	if(click1 && click2)
	{
		doc->EndUndo();
		ptA = -1;
		ptB = -1;
		click1 = false;
		click2 = false;
	}
	
	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	
	return true;
}

Bool CDSymmetryAssign::GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x,Real y, BaseContainer &bc)
{
	bc.SetString(RESULT_BUBBLEHELP,GeLoadString(IDS_CDSYMSELECT));
	
	BaseObject	*op = doc->GetActiveObject(); if(!op) return true;
	if(!IsValidPointObject(op)) return true;
	
	BaseTag *tag = op->GetTag(ID_CDSYMMETRYTAG); if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	if(!tData->GetBool(SYM_SYMMETRY_IS_SET)) return true;
	
	mx = x; my = y;
	if(click1 && !click2) SpecialEventAdd(EVMSG_UPDATEHIGHLIGHT);
	
	return true;
}

class CDSymmetryAssignR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		return true;
	}
};

Bool RegisterCDSymmetryAssign(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) reg = false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdsnr.FindFirst("-",&pos);
	while(h)
	{
		cdsnr.Delete(pos,1);
		h = cdsnr.FindFirst("-",&pos);
	}
	cdsnr.ToUpper();
	
	kb = cdsnr.SubStr(pK,2);
	
	CHAR chr;
	
	aK = Mod(aK,25);
	bK = Mod(bK,3);
	if(Mod(aK,2) == 0) chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) | cK);
	else chr = ((seed >> aK) & 0x000000FF) ^ ((seed >> bK) & cK);
	b = chr;
	
	String s, hStr, oldS;
	CHAR ch[2];
	LONG i, rem, n = Abs(b);
	
	ch[1] = 0;
	for(i=0; i<2; i++)
	{
		rem = Mod(n,16);
		ch[0] = GetHexCharacter(rem);
		oldS.SetCString(ch,1);
		hStr += oldS;
		n /= 16;
	}
	s = hStr;
	if(kb != s) reg = false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) reg = true;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) reg = true;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) reg = false;
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name ifthe plugin shall be registered - just for user convenience   PLUGINFLAG_TOOL_HIGHLIGHT|
	String name=GeLoadString(IDS_CDSYMASSIGN); if(!name.Content()) return true;
	
	if(!reg) return CDRegisterCommandPlugin(ID_CDSYMMETRYASSIGN,name,PLUGINFLAG_HIDE,"CDSymAssign.tif","CD Symmetry Assign",CDDataAllocator(CDSymmetryAssignR));
	else return CDRegisterToolPlugin(ID_CDSYMMETRYASSIGN,name,PLUGINFLAG_TOOL_EDITSTATES|PLUGINFLAG_TOOL_TWEAK,"CDSymAssign.tif","CD Symmetry Assign",CDDataAllocator(CDSymmetryAssign));
}
