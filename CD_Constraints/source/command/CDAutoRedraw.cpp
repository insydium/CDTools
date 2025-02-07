//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDConstraint.h"

enum
{
	CD_AUTOREDRAW_ON		= 1001,
	CD_AUTO_MLSCNDS			= 1002	
};

class CDAutoRedrawDialog : public GeDialog
{
	public:
		Bool autoRedrawOn;
		Real autoMlScnds;

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
		virtual void Timer(const BaseContainer& msg);
};

Bool CDAutoRedrawDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	SetTitle(GeLoadString(IDS_CDAUTOREDRAW));

	GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
	{
		GroupBorder(BORDER_NONE);
		GroupBorderSpace(10,0,10,10);
		
		GroupBegin(0,BFH_SCALEFIT,4,0,"",0);
			AddCheckbox(CD_AUTOREDRAW_ON,BFH_LEFT,0,0,GeLoadString(IDS_CDREDRAW));
			AddEditNumberArrows(CD_AUTO_MLSCNDS,BFH_LEFT);
			AddStaticText(0,BFH_LEFT,0,0,GeLoadString(IDS_CDMILLISECONDS),0);
			AddButton(IDS_SET_TO_FPS,BFH_CENTER,0,0,GeLoadString(IDS_SET_TO_FPS));
		GroupEnd();
	}
	GroupEnd();
	
	return res;
}

Bool CDAutoRedrawDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;

	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	BaseContainer *dData = doc->GetDataInstance(); if(!dData) return false;
	
	BaseContainer *cnData = dData->GetContainerInstance(ID_CDAUTOMATICREDRAW);
	if(!cnData)
	{
		SetBool(CD_AUTOREDRAW_ON,false);
		SetReal(CD_AUTO_MLSCNDS,50.0,0.0,CDMAXREAL,1.0,FORMAT_REAL);
	}
	else
	{
		SetBool(CD_AUTOREDRAW_ON,cnData->GetBool(CD_AUTOREDRAW_ON));
		SetReal(CD_AUTO_MLSCNDS,cnData->GetReal(CD_AUTO_MLSCNDS),1.0,100.0,0.1,FORMAT_REAL);
	}
	
	GetBool(CD_AUTOREDRAW_ON,autoRedrawOn);
	GetReal(CD_AUTO_MLSCNDS,autoMlScnds);
	
	if(autoRedrawOn)
	{
		SetTimer(LONG(autoMlScnds));
	}
	else SetTimer(0);

	return true;
}

Bool CDAutoRedrawDialog::Command(LONG id,const BaseContainer &msg)
{
	GetBool(CD_AUTOREDRAW_ON,autoRedrawOn);
	GetReal(CD_AUTO_MLSCNDS,autoMlScnds);
	
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *docData = doc->GetDataInstance();
	BaseContainer dbc;
	dbc.SetBool(CD_AUTOREDRAW_ON,autoRedrawOn);
	dbc.SetReal(CD_AUTO_MLSCNDS,autoMlScnds);
	docData->SetContainer(ID_CDAUTOMATICREDRAW,dbc);
	
	switch(id)
	{
		case IDS_SET_TO_FPS:
		{
			autoMlScnds = 1000/GetActiveDocument()->GetFps();
			SetReal(CD_AUTO_MLSCNDS,autoMlScnds,0.0,CDMAXREAL,1.0,FORMAT_REAL);
		}
	}
	
	if(autoRedrawOn)
	{
		SetTimer(LONG(autoMlScnds));
	}
	else SetTimer(0);
	
	
	return true;
}

void CDAutoRedrawDialog::Timer(const BaseContainer& msg)
{
	if(!autoRedrawOn) return;
	
	Bool redraw = true;
	
	if(CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING)) redraw = false;
	if(CheckIsRunning(CHECKISRUNNING_VIEWDRAWING)) redraw = false;
	if(CheckIsRunning(CHECKISRUNNING_EDITORRENDERING)) redraw = false;
	if(CheckIsRunning(CHECKISRUNNING_EXTERNALRENDERING)) redraw = false;
	if(CheckIsRunning(CHECKISRUNNING_PAINTERUPDATING)) redraw = false;
	if(CheckIsRunning(CHECKISRUNNING_MATERIALPREVIEWRUNNING)) redraw = false;
	
	if(redraw) CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_ANIMATION|CD_DRAWFLAGS_INDRAG); 
}

class CDAutoRedraw : public CommandData
{
private:
	CDAutoRedrawDialog dlg;

public:
	virtual Bool Execute(BaseDocument *doc)
	{
	#if API_VERSION < 12000
		return dlg.Open(true,ID_CDAUTOMATICREDRAW,-1,-1);
	#else
		return dlg.Open(DLG_TYPE_ASYNC,ID_CDAUTOMATICREDRAW,-1,-1);
	#endif
	}
	virtual Bool RestoreLayout(void *secret)
	{
		return dlg.RestoreLayout(ID_CDAUTOMATICREDRAW,0,secret);
	}
};

class CDAutoRedrawDialogR : public GeDialog
{
	public:
		virtual Bool CreateLayout(void)
		{
			Bool res = GeDialog::CreateLayout();
			if(res)
			{
				SetTitle(GeLoadString(IDS_CDAUTOREDRAW));
				
				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(10,5,10,5);
					
					AddStaticText(0,BFH_LEFT,0,0,"CD Constraints v"+CDRealToString(GetCDPluginVersion(ID_CDCONSTRAINT),-1,3),0);
				}
				GroupEnd();

			}
			return res;
		}
};

class CDAutoRedrawR : public CommandData
{
	private:
		CDAutoRedrawDialogR dlg;
		
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDAUTOMATICREDRAW,0,secret);
		}
};

Bool RegisterCDAutoRedraw(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDAUTOREDRAW); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDAUTOMATICREDRAW,name,PLUGINFLAG_HIDE,"CDAutoRedraw.tif","CD Auto Redraw",CDDataAllocator(CDAutoRedrawR));
	else return CDRegisterCommandPlugin(ID_CDAUTOMATICREDRAW,name,0,"CDAutoRedraw.tif","CD Auto Redraw",CDDataAllocator(CDAutoRedraw));
}
