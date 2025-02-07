// CD Springy Keys plugin
// Copyright 2010 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDSpringyKeys.h"

class CDSprKsAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDSprKsAboutUserArea 			ua;
		
	public:
		CDSprKsAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDSprKsAboutDialog::CDSprKsAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDSprKsAboutDialog::CreateLayout(void)
{
	CHAR data[CDSK_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDSPRINGYKEYS,data,CDSK_SERIAL_SIZE))
	{
	#if API_VERSION < 12000
		cdSerial.SetCString(data,CDSK_SERIAL_SIZE-1,St8bit);
	#else
		cdSerial.SetCString(data,CDSK_SERIAL_SIZE-1,STRINGENCODING_8BIT);
	#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDSK_ABOUT,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDSK_ABOUT_IMAGE);
		SetString(IDC_CDSK_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDSPRINGYKEYS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
		SetString(IDC_CDSK_SERIALNUMBER,cdSerial);
	}
	
	return res;
}

Bool CDSprKsAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDSK_SERIALNUMBER,false);
#else
#endif
	return true;
}

Bool CDSprKsAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDSK_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

class CDSprKsAbout : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDSK_SERIAL_SIZE];
			CDSprKsAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDSprKsEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDSprKsEULADialog::CreateLayout(void)
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
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDSK_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDSK_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDSK_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDSK_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDSK_DISAGREE));
			AddButton(IDC_CDSK_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDSK_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDSprKsEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDSK_EULA,GeLoadString(IDC_CDSK_EULA));
	
	return true;
}

Bool CDSprKsEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDSK_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDSK_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDSprKsRegisterDialog : public GeModalDialog
{
	private:
		CDSprKsAboutUserArea 			ua;
		CDSprKsEULADialog			eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDSprKsRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDSK_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDSK_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDSK_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDSK_SERIAL_SIZE];
					sn.GetCString(data,CDSK_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDSPRINGYKEYS,data,CDSK_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDSK_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDSprKsRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDSK_REGISTER,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDSK_ABOUT_IMAGE);
		SetString(IDC_CDSK_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDSPRINGYKEYS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2010 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDSprKsRegisterDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	Bool demo = false;
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) demo = true;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) demo = true;
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) demo = true;
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) demo = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) demo = true;
#endif
	
	if(demo) Enable(IDC_CDSK_SERIALNUMBER,false);
	
	return true;
}

class CDSprKsRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CDSprKsRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDSpringyKeys(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSK_SERIAL_SIZE];
	String cdsknr, kb;
	
	if(!CDReadPluginInfo(ID_CDSPRINGYKEYS,data,CDSK_SERIAL_SIZE)) reg = false;
	
	cdsknr.SetCString(data,CDSK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsknr)) reg = false;
	
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	kb = GetKeyByte(cdsknr,pK,2);
	b = GetKeyByte(seed,aK,bK,cK);
	if(kb != LongToHexString(b,2)) reg = false;
	
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
	String name=GeLoadString(IDS_ABOUT_CDSPRK); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDSPRINGYKEYS,name,0,"CDabout.tif","About CD Springy Keys",CDDataAllocator(CDSprKsRegister));
	else return CDRegisterCommandPlugin(ID_ABOUTCDSPRINGYKEYS,name,0,"CDabout.tif","About CD Springy Keys",CDDataAllocator(CDSprKsAbout));
}

