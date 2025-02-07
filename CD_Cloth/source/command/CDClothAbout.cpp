// CD Springy Keys plugin
// Copyright 2010 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCloth.h"

class CDClothAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDClothAboutUA 			ua;
		
	public:
		CDClothAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDClothAboutDialog::CDClothAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDClothAboutDialog::CreateLayout(void)
{
	CHAR data[CDCL_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDCLOTH,data,CDCL_SERIAL_SIZE))
	{
	#if API_VERSION < 12000
		cdSerial.SetCString(data,CDCL_SERIAL_SIZE-1,St8bit);
	#else
		cdSerial.SetCString(data,CDCL_SERIAL_SIZE-1,STRINGENCODING_8BIT);
	#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDCL_ABOUT,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDCL_ABOUT_IMAGE);
		SetString(IDC_CDCL_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDCLOTH),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2013 Cactus Dan Libisch");
		SetString(IDC_CDCL_SERIALNUMBER,cdSerial);
	}
	//else GePrint("couldn't load resource");
	
	return res;
}

Bool CDClothAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDCL_SERIALNUMBER,false);
#else
#endif
	return true;
}

Bool CDClothAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDCL_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

class CDClothAbout : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDCL_SERIAL_SIZE];
			CDClothAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDClothEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDClothEULADialog::CreateLayout(void)
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
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDCL_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDCL_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDCL_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDCL_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDCL_DISAGREE));
			AddButton(IDC_CDCL_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDCL_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDClothEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDCL_EULA,GeLoadString(IDC_CDCL_EULA));
	
	return true;
}

Bool CDClothEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDCL_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDCL_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDClothRegisterDialog : public GeModalDialog
{
	private:
		CDClothAboutUA 			ua;
		CDClothEULADialog			eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDClothRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDCL_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDCL_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDCL_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDCL_SERIAL_SIZE];
					sn.GetCString(data,CDCL_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDCLOTH,data,CDCL_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					//GePrint(sn);
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDCL_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDClothRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDCL_REGISTER,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDCL_ABOUT_IMAGE);
		SetString(IDC_CDCL_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDCLOTH),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2013 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDClothRegisterDialog::InitValues(void)
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
	
	if(demo) Enable(IDC_CDCL_SERIALNUMBER,false);
	
	return true;
}

class CDClothRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			//CHAR sn[SERIAL_SIZE];
			CDClothRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDCloth(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDCL_SERIAL_SIZE];
	String cdclnr, kb;
	
	if(!CDReadPluginInfo(ID_CDCLOTH,data,CDCL_SERIAL_SIZE)) reg = false;
	
	cdclnr.SetCString(data,CDCL_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdclnr)) reg = false;
	
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	kb = GetKeyByte(cdclnr,pK,2);
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
	String name=GeLoadString(IDS_ABOUT_CDCL); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDCLOTH,name,0,"CDabout.tif","About CD Cloth",CDDataAllocator(CDClothRegister) );
	else return CDRegisterCommandPlugin(ID_ABOUTCDCLOTH,name,0,"CDabout.tif","About CD Cloth",CDDataAllocator(CDClothAbout));
}

