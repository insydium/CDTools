//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "oCDJoint.h"
#include "CDJointSkin.h"

class CDColorJointsDialog : public GeModalDialog
{
	private:
		CDJSOptionsUA ua;
		
	public:	
		LONG	cType;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDColorJointsDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDCOLORJOINT));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDJS_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDJS_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDC_COLOR_TYPE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_COLOR_TYPE,BFH_CENTER,2,1);
					AddChild(IDC_COLOR_TYPE, 0, GeLoadString(IDS_COLOR_SUBCHAINS));
					AddChild(IDC_COLOR_TYPE, 1, GeLoadString(IDS_COLOR_INDIVIDUAL));

			}
			GroupEnd();
		}
		GroupEnd();
		
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDColorJointsDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDCOLORIZEJOINTS);
	if(!bc)
	{
		SetLong(IDC_COLOR_TYPE,0);
	}
	else
	{
		SetLong(IDC_COLOR_TYPE,bc->GetLong(IDC_COLOR_TYPE),1,15,1);
	}

	return true;
}

Bool CDColorJointsDialog::Command(LONG id,const BaseContainer &msg)
{
	GetLong(IDC_COLOR_TYPE,cType);
	
	BaseContainer bc;
	bc.SetLong(IDC_COLOR_TYPE,cType);
	SetWorldPluginData(ID_CDCOLORIZEJOINTS,bc,false);
	
	return true;
}

class CDColorJoints : public CommandData
{
	private:
		CDColorJointsDialog dlg;

	public:
		LONG colorType;
		
		Bool ColorChildJoints(BaseDocument* doc, BaseObject *op);
		LONG GetRandomNumber(LONG limit);
	
		virtual Bool Execute(BaseDocument* doc);

};

LONG CDColorJoints::GetRandomNumber(LONG limit)
{
	int rNum = rand()%limit + 1;
	
	return LONG(rNum);
}

Bool CDColorJoints::ColorChildJoints(BaseDocument* doc, BaseObject *op)
{
	while(op)
	{
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			LONG cRandom = GetRandomNumber(1000);
			ObjectColorProperties prop;
			op->GetColorProperties(&prop);
			
			if(colorType == 0)
			{
				BaseContainer *opData = op->GetDataInstance();
				if(opData && opData->GetBool(JNT_CONNECTED))
				{
					ObjectColorProperties propPR;
					op->GetUp()->GetColorProperties(&propPR);
					
					prop.usecolor = propPR.usecolor;
					prop.color = propPR.color;
				}
				else
				{
					prop.usecolor = 2;
					prop.color = HSVToRGB(Vector(cRandom*0.001,1,1));
				}
			}
			else
			{
				prop.usecolor = 2;
				prop.color = HSVToRGB(Vector(cRandom*0.001,1,1));
			}
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			op->SetColorProperties(&prop);
		}
		ColorChildJoints(doc,op->GetDown());
		op = op->GetNext();
	}
	return true;
}

Bool CDColorJoints::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
	}
	
	BaseContainer *bc = GetWorldPluginData(ID_CDCOLORIZEJOINTS);
	if(bc) colorType = bc->GetLong(IDC_COLOR_TYPE);
	else colorType = 0;
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG i, opCount = objects->GetCount();
	
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && op->GetType() == ID_CDJOINTOBJECT)
		{
			LONG cRandom = GetRandomNumber(1000);
			
			ObjectColorProperties prop;
			op->GetColorProperties(&prop);
			prop.usecolor = 2;
			prop.color = HSVToRGB(Vector(cRandom * 0.001,1,1));
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			op->SetColorProperties(&prop);

			if(op->GetDown())
			{
				if(!ColorChildJoints(doc,op->GetDown())) return true;
			}
		}
	}
	
	if(doc->GetAction() != ID_SKINWEIGHTTOOL) doc->SetActiveObject(NULL);
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDColorJointsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDColorJoints(void)
{
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
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
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDCOLORJOINT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCOLORIZEJOINTS,name,PLUGINFLAG_HIDE,"CDColorJoints.TIF","CD Colorize Joints",CDDataAllocator(CDColorJointsR));
	else return CDRegisterCommandPlugin(ID_CDCOLORIZEJOINTS,name,0,"CDColorJoints.TIF","CD Colorize Joints"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDColorJoints));
}
