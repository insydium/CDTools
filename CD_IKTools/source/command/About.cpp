//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

class CDIKAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDIKAboutUserArea 			ua;
		
	public:
		CDIKAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDIKAboutDialog::CDIKAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDIKAboutDialog::CreateLayout(void)
{
	CHAR data[CDIK_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE))
	{
#if API_VERSION < 12000
		cdSerial.SetCString(data,CDIK_SERIAL_SIZE-1,St8bit);
#else
		cdSerial.SetCString(data,CDIK_SERIAL_SIZE-1,STRINGENCODING_8BIT);
#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDIK_ABOUT,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDIK_ABOUT_IMAGE);
		SetString(IDC_CDIK_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDIKTOOLS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
		SetString(IDC_CDIK_SERIALNUMBER,cdSerial);
	}
	
	return res;
}

Bool CDIKAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDIK_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

Bool CDIKAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDIK_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDIK_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDIK_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDIK_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDIK_SERIALNUMBER,false);
#endif
	
	return true;
}

Bool OpenAboutDialog(CHAR *sn)
{
	CDIKAboutDialog dlg(sn);
	return dlg.Open();
}

class AboutCDIKTools : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDIK_SERIAL_SIZE];
			CDIKAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDIKToolsEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDIKToolsEULADialog::CreateLayout(void)
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
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDIK_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDIK_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDIK_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDIK_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDIK_DISAGREE));
			AddButton(IDC_CDIK_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDIK_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDIKToolsEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDIK_EULA,GeLoadString(IDC_CDIK_EULA));
	
	return true;
}

Bool CDIKToolsEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDIK_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDIK_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDIKToolsRegisterDialog : public GeModalDialog
{
	private:
		CDIKAboutUserArea 			ua;
		CDIKToolsEULADialog				eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDIKToolsRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDIK_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDIK_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDIK_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDIK_SERIAL_SIZE];
					sn.GetCString(data,CDIK_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDIK_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDIKToolsRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDIK_REGISTER,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDIK_ABOUT_IMAGE);
		SetString(IDC_CDIK_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDIKTOOLS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDIKToolsRegisterDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDIK_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDIK_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDIK_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDIK_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDIK_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDIKToolsRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CDIKToolsRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDIKTools(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_ABOUTCD); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDIKTOOLS,name,0,"CDabout.tif","About CD IK Tools",CDDataAllocator(CDIKToolsRegister));
	else return CDRegisterCommandPlugin(ID_ABOUTCDIKTOOLS,name,0,"CDabout.tif","About CD IK Tools",CDDataAllocator(AboutCDIKTools));
}

