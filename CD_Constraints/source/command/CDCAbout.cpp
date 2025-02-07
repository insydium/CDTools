//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDConstraint.h"

class CDCAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDCAboutUserArea 			ua;
		
	public:
		CDCAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDCAboutDialog::CDCAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDCAboutDialog::CreateLayout(void)
{
	CHAR data[CDC_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE))
	{
	#if API_VERSION < 12000
		cdSerial.SetCString(data,CDC_SERIAL_SIZE-1,St8bit);
	#else
		cdSerial.SetCString(data,CDC_SERIAL_SIZE-1,STRINGENCODING_8BIT);
	#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDC_ABOUT,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDC_ABOUT_IMAGE);
		SetString(IDC_CDC_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDCONSTRAINT),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
		SetString(IDC_CDC_SERIALNUMBER,cdSerial);
	}
	
	return res;
}

Bool CDCAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDC_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

Bool CDCAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDC_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDC_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDC_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDC_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDC_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDConstraintsAbout : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDC_SERIAL_SIZE];
			CDCAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDConstraintsEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDConstraintsEULADialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	if(res)
	{
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,10,10,10);
			
			if(CDGeGetCurrentOS() == CD_MAC)
			{
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDCN_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDCN_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDCN_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDCN_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDCN_DISAGREE));
			AddButton(IDC_CDCN_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDCN_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDConstraintsEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDCN_EULA,GeLoadString(IDC_CDCN_EULA));
	
	return true;
}

Bool CDConstraintsEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDCN_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDCN_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDCRegisterDialog : public GeModalDialog
{
	private:
		CDCAboutUserArea 			ua;
		CDConstraintsEULADialog		eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDCRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDC_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDC_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDC_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDC_SERIAL_SIZE];
					sn.GetCString(data,CDC_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDC_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDCRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDC_REGISTER,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDC_ABOUT_IMAGE);
		SetString(IDC_CDC_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDCONSTRAINT),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDCRegisterDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDC_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDC_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDC_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDC_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDC_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDConstraintsRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CDCRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDConstraints(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_ABOUT_CDC); if(!name.Content()) return true;
	
	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDCONSTRAINT,name,0,"CDabout.tif","About CD Constraints",CDDataAllocator(CDConstraintsRegister));
	else return CDRegisterCommandPlugin(ID_ABOUTCDCONSTRAINT,name,0,"CDabout.tif","About CD Constraints",CDDataAllocator(CDConstraintsAbout));
}
