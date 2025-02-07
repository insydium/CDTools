//	Cactus Dan's Joints & Skin plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "oCDJoint.h"
#include "CDJointSkin.h"

class CDJointDisplaySizeDialog : public GeDialog
{
	private:
		CDJSOptionsUA ua;
		
	public:
		CDJointDisplaySizeDialog(void);
		void DoEnable(void);
		void ResizeChildJoints(BaseObject *op, Real size);

		virtual Bool CoreMessage(LONG id, const BaseContainer& msg);
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

CDJointDisplaySizeDialog::CDJointDisplaySizeDialog(void)
{ 

}

BaseObject* GetFirstSelectedJoint(BaseDocument *doc)
{
	BaseObject *op = NULL;
	
	AtomArray *opSelLog = GetSelectionLog(doc);
	if(opSelLog)
	{
		LONG i, opCount = opSelLog->GetCount();
		if(opCount > 0)
		{
			for(i=0; i<opCount; i++)
			{
				op = static_cast<BaseObject*>(opSelLog->GetIndex(i));
				if(op->GetType() == ID_CDJOINTOBJECT) break;
			}
		}
	}
	
	return op;
}

Bool CDJointDisplaySizeDialog::CoreMessage(LONG id, const BaseContainer& msg)
{
	BaseDocument *doc = GetActiveDocument();
	Real jSize;
	Bool sel;
	
	GetBool(IDC_SELECTED_JOINT,sel);
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDJOINTDISPLAYSIZE);
	if(wpData)
	{
		jSize = wpData->GetReal(IDC_JOINT_SIZE);
	}
	else jSize = 1.0;

	switch (id)
	{
		case EVMSG_CHANGE:
			DoEnable();
			break;
		case EVMSG_DOCUMENTRECALCULATED:
			if(sel)
			{
				BaseObject *jnt = GetFirstSelectedJoint(doc);
				if(jnt)
				{
					BaseContainer *jData = jnt->GetDataInstance();
					if(jData)
					{
						jSize = jData->GetReal(JNT_JOINT_RADIUS) * 0.1;
					}
				}
			}
			SetPercent(IDC_JOINT_SIZE,jSize,1,1000,1);
			break;
	}
	
	return GeDialog::CoreMessage(id,msg);
}


Bool CDJointDisplaySizeDialog::CreateLayout(void)
{
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDJDISPLAY));
		
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
			GroupBorder(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
		
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(IDC_JOINT_SIZE),0);
			{
				GroupBorder(BORDER_GROUP_IN);
				GroupBorderSpace(10,5,10,10);
				
				GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
					GroupSpace(4,1);
					AddCheckbox (IDC_SELECTED_JOINT,BFH_LEFT,0,0,GeLoadString(IDC_SELECTED_JOINT));
					AddCheckbox (IDC_AUTO_SEL_CHILDREN,BFH_RIGHT,0,0,GeLoadString(IDC_AUTO_SEL_CHILDREN));
				GroupEnd();

				AddEditSlider(IDC_JOINT_SIZE,BFH_SCALEFIT);
								
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
			{
				GroupBorderNoTitle(BORDER_NONE);
				GroupBorderSpace(110,10,110,5);
				
				AddButton(IDC_J_SIZE_RESET,BFH_CENTER,0,0,GeLoadString(IDC_J_SIZE_RESET));				
			}
			GroupEnd();
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDJointDisplaySizeDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDJOINTDISPLAYSIZE);
	if(!wpData)
	{
		SetPercent(IDC_JOINT_SIZE,1.0,1,1000,1);
	}
	else
	{
		SetPercent(IDC_JOINT_SIZE,wpData->GetReal(IDC_JOINT_SIZE),1,1000,1);
	}
	
	DoEnable();
	
	return true;
}

Bool CDJointDisplaySizeDialog::Command(LONG id,const BaseContainer &msg)
{
	BaseDocument *doc = GetActiveDocument();
	Bool resizeRad = false, sel, child;
	Real jSize;
	
	GetReal(IDC_JOINT_SIZE,jSize);
	DoEnable();
	
	GetBool(IDC_SELECTED_JOINT,sel);
	GetBool(IDC_AUTO_SEL_CHILDREN,child);
	
	AtomArray *opSelLog = GetSelectionLog(doc);
	if(opSelLog && sel)
	{
		BaseObject *op = NULL;
		LONG i, jCount, opCount = opSelLog->GetCount();
		if(opCount > 0)
		{
			AutoAlloc<AtomArray> jointList;
			if(jointList)
			{
				for(i=0; i<opCount; i++)
				{
					op = static_cast<BaseObject*>(opSelLog->GetIndex(i));
					if(op->GetType() == ID_CDJOINTOBJECT) jointList->Append(op);
				}
				jCount = jointList->GetCount();
				if(jCount > 0)
				{
					resizeRad = true;
					BaseContainer *data = NULL;
					switch(id)
					{
						case IDC_JOINT_SIZE:
							for(i=0; i<jCount; i++)
							{
								op = static_cast<BaseObject*>(jointList->GetIndex(i));
								data = op->GetDataInstance();
								data->SetReal(JNT_JOINT_RADIUS,jSize*10.0);
								if(child) ResizeChildJoints(op->GetDown(),jSize*10.0);
							}
							CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
							break;
						case IDC_J_SIZE_RESET:
							jSize = 1.0;
							SetPercent(IDC_JOINT_SIZE,1.0,1,1000,1);
							for(i=0; i<jCount; i++)
							{
								op = static_cast<BaseObject*>(jointList->GetIndex(i));
								data = op->GetDataInstance();
								data->SetReal(JNT_JOINT_RADIUS,jSize*10.0);
								if(child) ResizeChildJoints(op->GetDown(),jSize*10.0);
							}
							CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
							break;
					}
				}
			}
		}
	}

	
	if(!resizeRad)
	{
		BaseContainer bc;
		bc.SetReal(IDC_JOINT_SIZE,jSize);
		SetWorldPluginData(ID_CDJOINTDISPLAYSIZE,bc,false);
		
		switch(id)
		{
			case IDC_JOINT_SIZE:
				CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
				break;
			case IDC_J_SIZE_RESET:
				jSize = 1.0;
				SetPercent(IDC_JOINT_SIZE,1.0,1,1000,1);
				bc.SetReal(IDC_JOINT_SIZE,jSize);
				SetWorldPluginData(ID_CDJOINTDISPLAYSIZE,bc,false);
				CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
				break;
		}
	}
	
	return true;
}

void CDJointDisplaySizeDialog::ResizeChildJoints(BaseObject *op, Real size)
{
	BaseContainer *data = NULL;
	
	while(op)
	{
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			data = op->GetDataInstance();
			data->SetReal(JNT_JOINT_RADIUS,size);
		}
		ResizeChildJoints(op->GetDown(),size);
		
		op = op->GetNext();
	}
}

void CDJointDisplaySizeDialog::DoEnable(void)
{
	BaseDocument *doc = GetActiveDocument();
	Bool sel;
	
	GetBool(IDC_SELECTED_JOINT,sel);
	
	AtomArray *opSelLog = GetSelectionLog(doc);
	if(opSelLog)
	{
		BaseObject *op = NULL;
		LONG i, opCount = opSelLog->GetCount();
		if(opCount > 0)
		{
			Bool joint = false;
			for(i=0; i<opCount; i++)
			{
				op = static_cast<BaseObject*>(opSelLog->GetIndex(i));
				if(op->GetType() == ID_CDJOINTOBJECT) joint = true;
			}
			if(joint)
			{
				Enable(IDC_SELECTED_JOINT,true);
				if(sel) Enable(IDC_AUTO_SEL_CHILDREN,true);
				else
				{
					SetBool(IDC_AUTO_SEL_CHILDREN,false);
					Enable(IDC_AUTO_SEL_CHILDREN,false);
				}
			}
			else
			{
				SetBool(IDC_SELECTED_JOINT,false);
				SetBool(IDC_AUTO_SEL_CHILDREN,false);
				Enable(IDC_SELECTED_JOINT,false);
				Enable(IDC_AUTO_SEL_CHILDREN,false);
			}
		}
		else
		{
			SetBool(IDC_SELECTED_JOINT,false);
			SetBool(IDC_AUTO_SEL_CHILDREN,false);
			Enable(IDC_SELECTED_JOINT,false);
			Enable(IDC_AUTO_SEL_CHILDREN,false);
		}
	}
	else
	{
		SetBool(IDC_SELECTED_JOINT,false);
		SetBool(IDC_AUTO_SEL_CHILDREN,false);
		Enable(IDC_SELECTED_JOINT,false);
		Enable(IDC_AUTO_SEL_CHILDREN,false);
	}
	Enable(IDC_JOINT_SIZE,true);
}

class CDJointDisplaySizePlugin : public CommandData
{
	private:
		CDJointDisplaySizeDialog dlg;
		
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
		#if API_VERSION < 12000
			return dlg.Open(true,ID_CDJOINTDISPLAYSIZE,-1,-1);
		#else
			return dlg.Open(DLG_TYPE_ASYNC,ID_CDJOINTDISPLAYSIZE,-1,-1);
		#endif
		}
		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDJOINTDISPLAYSIZE,0,secret);
		}
};

class CDJointDisplaySizePluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDJointDisplaySizePlugin(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDJDISPLAY); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDJOINTDISPLAYSIZE,name,PLUGINFLAG_HIDE,"CDJDisplay.tif","CD Joint Display Size",CDDataAllocator(CDJointDisplaySizePluginR));
	else return CDRegisterCommandPlugin(ID_CDJOINTDISPLAYSIZE,name,0,"CDJDisplay.tif","CD Joint Display Size",CDDataAllocator(CDJointDisplaySizePlugin));
}

