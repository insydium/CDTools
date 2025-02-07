// (c) 1989-2004 MAXON Computer GmbH, all rights reserved  //
/////////////////////////////////////////////////////////////

// example code for a metaball painting tool

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"

#include "CDIKtools.h"
#include "tCDIKHandle.h"
#include "CDCompatibility.h"
#include "CDToolData.h"

//#include "CDDebug.h"

class CDIKHandleToolDialog: public SubDialog
{
public:
	virtual Bool CreateLayout(void);
	virtual Bool InitValues(void);
	virtual Bool InitDialog(void);
	virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDIKHandleToolDialog::InitDialog(void)
{
	//GePrint("CDPSWDialog::InitDialog");
	BaseContainer *bc=GetToolData(GetActiveDocument(),ID_CDIKHANDLETOOL);
	if (!bc) return false;
	
	return true;
}

Bool CDIKHandleToolDialog::CreateLayout(void)
{
	GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
	{
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,4);
			AddStaticText(0,0,0,0,GeLoadString(IDS_TOOL_RADIUS),0);
			AddEditNumberArrows(IDS_TOOL_RADIUS,BFH_LEFT,70,0);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDS_SOLVER_TYPE),0);
			AddComboBox(IDS_SOLVER_TYPE, BFH_LEFT,100,0);
			AddChild(IDS_SOLVER_TYPE, 0, GeLoadString(IDS_IKRP));
			AddChild(IDS_SOLVER_TYPE, 1, GeLoadString(IDS_IKSC));
			AddChild(IDS_SOLVER_TYPE, 2, GeLoadString(IDS_IKHD));
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDS_POLE_AXIS),0);
			AddComboBox(IDS_POLE_AXIS, BFH_LEFT,100,0);
			AddChild(IDS_POLE_AXIS, 0, GeLoadString(IDS_POLE_X));
			AddChild(IDS_POLE_AXIS, 1, GeLoadString(IDS_POLE_Y));
			AddChild(IDS_POLE_AXIS, 2, GeLoadString(IDS_POLE_NX));
			AddChild(IDS_POLE_AXIS, 3, GeLoadString(IDS_POLE_NY));
		}
		GroupEnd();
	}
	GroupEnd();
	
	return true;
}

Bool CDIKHandleToolDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDIKHANDLETOOL);
	if(!bc)
	{
		SetReal(IDS_TOOL_RADIUS,10,0.01,CDMAXREAL,0.1,FORMAT_REAL);
		SetLong(IDS_SOLVER_TYPE,0);
		SetLong(IDS_POLE_AXIS,1);
	}
	else
	{
		SetReal(IDS_TOOL_RADIUS,bc->GetReal(IDS_TOOL_RADIUS),0.01,CDMAXREAL,0.1,FORMAT_REAL);
		SetLong(IDS_SOLVER_TYPE,bc->GetLong(IDS_SOLVER_TYPE));
		SetLong(IDS_POLE_AXIS,bc->GetLong(IDS_POLE_AXIS));
	}
	
	return InitDialog();
}

Bool CDIKHandleToolDialog::Command(LONG id,const BaseContainer &msg)
{
	Real rad;
	LONG solver, pole;
	
	GetReal(IDS_TOOL_RADIUS,rad);
	GetLong(IDS_SOLVER_TYPE,solver);
	GetLong(IDS_POLE_AXIS,pole);
	
	BaseContainer wpData;
	wpData.SetReal(IDS_TOOL_RADIUS,rad);
	wpData.SetLong(IDS_SOLVER_TYPE,solver);
	wpData.SetLong(IDS_POLE_AXIS,pole);
	SetWorldPluginData(ID_CDIKHANDLETOOL,wpData,false);
	
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *bc = GetToolData(doc,ID_CDIKHANDLETOOL);
	if(!bc) return false;
	
	bc->SetReal(IDS_TOOL_RADIUS,rad);
	bc->SetLong(IDS_SOLVER_TYPE,solver);
	bc->SetLong(IDS_POLE_AXIS,pole);
		
	return true;
}

class CDIKHandleTool : public CDToolData
{
private:
	Real			mx, my;
	BaseObject		*rootJnt, *endJnt;
	
	Bool IsJoint(BaseObject *op);
	Bool IsChild(BaseObject *pr, BaseObject *ch);
	
	Bool CreateIKHandle(BaseDocument *doc, BaseContainer &data);
	
public:
	virtual Bool InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt);
	virtual LONG GetState(BaseDocument *doc);
	virtual Bool KeyboardInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
	virtual Bool MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
	virtual Bool GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x, Real y, BaseContainer &bc);

	virtual SubDialog*	AllocSubDialog(BaseContainer* bc) { return CDDataAllocator(CDIKHandleToolDialog); }
};

Bool CDIKHandleTool::InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt)
{
	//GePrint("CDJointToolPlugin::InitTool");
	BaseContainer *bc = GetWorldPluginData(ID_CDIKHANDLETOOL);
	if(!bc)
	{
		data.SetReal(IDS_TOOL_RADIUS,5.0);
		data.SetLong(IDS_SOLVER_TYPE,0);
		data.SetLong(IDS_POLE_AXIS,1);
	}
	else
	{
		data.SetReal(IDS_TOOL_RADIUS,bc->GetReal(IDS_TOOL_RADIUS));
		data.SetLong(IDS_SOLVER_TYPE,bc->GetLong(IDS_SOLVER_TYPE));
		data.SetLong(IDS_POLE_AXIS,bc->GetLong(IDS_POLE_AXIS));
	}
	
	doc->SetActiveObject(NULL,SELECTION_NEW);
	return true;
}

LONG CDIKHandleTool::GetState(BaseDocument *doc)
{
	//GePrint("CDJointToolPlugin::GetState");
	if (doc->GetMode()==Mpaint) return 0;
	return CMD_ENABLED;
}

Bool CDIKHandleTool::KeyboardInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg)
{
	BaseObject	*opActive = doc->GetActiveObject(); if(!opActive) return false;

	LONG key = msg.GetData(BFM_INPUT_CHANNEL).GetLong();
	String str = msg.GetData(BFM_INPUT_ASC).GetString();
	if (key == KEY_ESC)
	{
		// do what you want
		doc->SetActiveObject(NULL,SELECTION_NEW);

		// return true to signal that the key is processed!
		return true;
	}
	return false;
}

Bool CDIKHandleTool::IsChild(BaseObject *pr, BaseObject *ch)
{
	BaseObject *jnt = ch->GetUp();
	while(jnt)
	{
		if(jnt == pr) return true;
		jnt = jnt->GetUp();
	}
	
	return false;
}

Bool CDIKHandleTool::IsJoint(BaseObject *op)
{
	switch(op->GetType())
	{
	#if API_VERSION < 12000
		case Obone:
			return true;
	#endif
		case ID_CDJOINTOBJECT:
			return true;
	#if API_VERSION > 9799
		case Ojoint:
			return true;
	#endif
	}
	
	return false;
}

Bool CDIKHandleTool::CreateIKHandle(BaseDocument *doc, BaseContainer &data)
{
	if(!rootJnt || !endJnt) return false;
	
	Matrix rootM = rootJnt->GetMg(), endM = endJnt->GetMg();
	
	BaseObject *jnt = endJnt;
	
	LONG jCnt = 0;
	while(jnt)
	{
		jnt = jnt->GetUp();
		if(jnt)
		{
			jCnt++;
			if(jnt == rootJnt) break;
		}
	}
	
	doc->StartUndo();
	
	// allocate ik handle object
	BaseObject *ikHandle = BaseObject::Alloc(Onull); if(!ikHandle) return false;
	BaseContainer *hData = ikHandle->GetDataInstance(); if(!hData) return false;
	
	// allocate ik solver tag
	BaseTag *ikTag = BaseTag::Alloc(ID_CDIKHANDLEPLUGIN); if(!ikTag) return false;
	BaseContainer *tData = ikTag->GetDataInstance(); if(!tData) return false;
	
	// set the ik handle object's parameters
	ikHandle->SetName(endJnt->GetName()+"_ikHandle");
	doc->InsertObject(ikHandle, NULL, NULL,true);
	Matrix hM = ikHandle->GetMg();
	hM.off = endM.off;
	ikHandle->SetMg(hM);

	hData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_AXIS);
	hData->SetReal(NULLOBJECT_RADIUS,20.0);
	hData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
	ObjectColorProperties oProp;
	oProp.color = Vector(0,1,1);
	oProp.usecolor = 2;
	ikHandle->SetColorProperties(&oProp);
	
	rootJnt->InsertTag(ikTag,NULL);
	tData->SetLink(IKH_GOAL_LINK,ikHandle);
	tData->SetLong(IKH_BONES_IN_GROUP,jCnt);
	
	switch(data.GetLong(IDS_SOLVER_TYPE))
	{
		case 0:
			tData->SetLong(IKH_SOLVER,IKH_IKRP);
			break;
		case 1:
			tData->SetLong(IKH_SOLVER,IKH_IKSC);
			break;
		case 2:
			tData->SetLong(IKH_SOLVER,IKH_IKHD);
			break;
	}
	
	switch(data.GetLong(IDS_POLE_AXIS))
	{
		case 0:
			tData->SetLong(IKH_POLE_AXIS,IKH_POLE_X);
			break;
		case 1:
			tData->SetLong(IKH_POLE_AXIS,IKH_POLE_Y);
			break;
		case 2:
			tData->SetLong(IKH_POLE_AXIS,IKH_POLE_NX);
			break;
		case 3:
			tData->SetLong(IKH_POLE_AXIS,IKH_POLE_NY);
			break;
	}
	
	Vector hV = VNorm(hM.off - rootM.off);
	Vector rpV = VNorm(VCross(rootM.v3, hV));
	Vector pV = VNorm(VCross(hV, rpV));
	tData->SetVector(IKH_IKSC_POLE_VECTOR,pV);
	
	ikTag->Message(MSG_MENUPREPARE);
	ikTag->Message(MSG_UPDATE);
	
	CDAddUndo(doc,CD_UNDO_NEW,ikHandle);
	CDAddUndo(doc,CD_UNDO_NEW,ikTag);
	
	// set the ikhandle active
	doc->SetActiveObject(ikHandle,SELECTION_NEW);
	doc->EndUndo();
	doc->Message(MSG_UPDATE);
	
	rootJnt->Message(MSG_UPDATE);
	
	// refresh view
	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	
	// Reset the end joint's matrix & refresh view
	endJnt->SetMg(endM);
	endJnt->Message(MSG_UPDATE);

	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);

	return true;
}

Bool CDIKHandleTool::MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg)
{
	BaseObject	*opActive = doc->GetActiveObject();

	LONG button;
	switch (msg.GetLong(BFM_INPUT_CHANNEL))
	{
		case BFM_INPUT_MOUSELEFT : button=KEY_MLEFT; break;
		case BFM_INPUT_MOUSERIGHT: button=KEY_MRIGHT; break;
		default: return true;
	}
	
	AutoAlloc<C4DObjectList> list;
	if (!list) return false;
	
#if API_VERSION < 9800
	if(SelectionListCreate(doc, NULL, bd, mx, my, NULL, list))
#else
	Real rad = data.GetReal(IDS_TOOL_RADIUS);

	#if API_VERSION < 13000
		if(ViewportSelect::PickObject(bd, doc, mx, my, rad, true, NULL, list))
	#else
		if(ViewportSelect::PickObject(bd, doc, mx, my, rad, VIEWPORT_PICK_FLAGS_ALLOW_OGL, NULL, list))
	#endif
#endif
	{
		Real maxRad = CDMAXREAL;
		LONG i, oCnt = list->GetCount();
		
		if(!opActive)
		{
			for(i=0; i<oCnt; i++)
			{
				BaseObject *jnt = list->GetObject(i);
				if(IsJoint(jnt))
				{
					Vector mPt = Vector(mx,my,0.0);
					Vector scrPt = bd->WS(jnt->GetMg().off);
					scrPt.z = 0.0;
					Real opRad = Len(scrPt - mPt);
					
					if(opRad < maxRad)
					{
						rootJnt = jnt;
						maxRad = opRad;
					}
				}
			}
			if(rootJnt) rootJnt->SetBit(BIT_ACTIVE);
		}
		else
		{
			if(opActive == rootJnt)
			{
				for(i=0; i<oCnt; i++)
				{
					BaseObject *jnt = list->GetObject(i);
					{
						if(jnt->GetType() == rootJnt->GetType() && IsChild(rootJnt,jnt))
						{
							Vector mPt = Vector(mx,my,0.0);
							Vector scrPt = bd->WS(jnt->GetMg().off);
							scrPt.z = 0.0;
							Real opRad = Len(scrPt - mPt);
							
							if(opRad < maxRad)
							{
								endJnt = jnt;
								maxRad = opRad;
							}
						}
					}
				}
			}
		}
	}
	if(rootJnt && endJnt)
	{
		if(CreateIKHandle(doc, data)) SpecialEventAdd(ID_CDIKHANDLETOOL);
		rootJnt = NULL;
		endJnt = NULL;
	}
	
	CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	
	return true;
}

Bool CDIKHandleTool::GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x,Real y, BaseContainer &bc)
{
	bc.SetString(RESULT_BUBBLEHELP,GeLoadString(IDS_CDADDHNDL));
	bc.SetLong(RESULT_CURSOR,MOUSE_CROSS);
	
	mx = x;
	my = y;
	
	return true;
}

class CDIKHandleToolR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		return true;
	}
};


Bool RegisterCDIKHandleTool(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDIK_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE)) reg = false;
	
	cdknr.SetCString(data,CDIK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
	
	if(GeGetVersionType() & VERSION_NET) reg = true;
	if(GeGetVersionType() & VERSION_SERVER) reg = true;
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = true;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDADDHNDL); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDIKHANDLETOOL,name,PLUGINFLAG_HIDE,"CDIKHandle.tif","CD IK Handle Tool",CDDataAllocator(CDIKHandleToolR));
	else return CDRegisterToolPlugin(ID_CDIKHANDLETOOL,name,0,"CDIKHandle.tif","CD IK Handle Tool",CDDataAllocator(CDIKHandleTool));
}
