//	Cactus Dan's FBX Import/Export plugin
//	Copyright 2011 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDFBX.h"

class CDFBXAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDFbxAboutUserArea 	ua;
		
	public:
		CDFBXAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDFBXAboutDialog::CDFBXAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDFBXAboutDialog::CreateLayout(void)
{
	CHAR data[CDFBX_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDFBXPLUGIN,data,CDFBX_SERIAL_SIZE))
	{
	#if API_VERSION < 12000
		cdSerial.SetCString(data,CDFBX_SERIAL_SIZE-1,St8bit);
	#else
		cdSerial.SetCString(data,CDFBX_SERIAL_SIZE-1,STRINGENCODING_8BIT);
	#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDFBX_ABOUT,NULL,0);
	if(res)
	{
		String fbxsdkVersion = "";
		switch(CDGeGetCurrentOS())
		{
			case CD_MAC:
				fbxsdkVersion = "2010.2";
				break;
			case CD_WIN:
				fbxsdkVersion = "2010.0.2";
				break;
		}
		
		AttachUserArea(ua,IDC_CDFBX_ABOUT_IMAGE);
		SetString(IDC_CDFBX_SDK_INFO,"Autodesk"+GeLoadString(R_SYMBOL)+" Fbx Sdk v"+fbxsdkVersion);
		SetString(IDC_CDFBX_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDFBXPLUGIN),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2011 Cactus Dan Libisch");
		SetString(IDC_CDFBX_SERIALNUMBER,cdSerial);
	}
	
	return res;
}

Bool CDFBXAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDFBX_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

Bool CDFBXAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDFBX_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDFBX_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDFBX_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDFBX_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDFBX_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDFBXAbout : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDFBX_SERIAL_SIZE];
			CDFBXAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDFBXEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDFBXEULADialog::CreateLayout(void)
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
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDFBX_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDFBX_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDFBX_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDFBX_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDFBX_DISAGREE));
			AddButton(IDC_CDFBX_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDFBX_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDFBXEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDFBX_EULA,GeLoadString(IDC_CDFBX_EULA));
	
	return true;
}

Bool CDFBXEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDFBX_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDFBX_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDFBXRegisterDialog : public GeModalDialog
{
	private:
		CDFbxAboutUserArea 			ua;
		CDFBXEULADialog				eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDFBXRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDFBX_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDFBX_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDFBX_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDFBX_SERIAL_SIZE];
					sn.GetCString(data,CDFBX_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDFBXPLUGIN,data,CDFBX_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDFBX_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDFBXRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDFBX_REGISTER,NULL,0);
	if(res)
	{
		String fbxsdkVersion = "";
		switch(CDGeGetCurrentOS())
		{
			case CD_MAC:
				fbxsdkVersion = "2010.2";
				break;
			case CD_WIN:
				fbxsdkVersion = "2010.0.2";
				break;
		}

		AttachUserArea(ua,IDC_CDFBX_ABOUT_IMAGE);
		SetString(IDC_CDFBX_SDK_INFO,"Autodesk"+GeLoadString(R_SYMBOL)+" Fbx Sdk v"+fbxsdkVersion);
		SetString(IDC_CDFBX_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDFBXPLUGIN),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2011 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDFBXRegisterDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDFBX_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDFBX_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDFBX_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDFBX_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDFBX_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDFBXRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CDFBXRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDFBX(void)
{
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDFBX_SERIAL_SIZE];
	String cdfnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDFBXPLUGIN,data,CDFBX_SERIAL_SIZE)) reg = false;
	
	cdfnr.SetCString(data,CDFBX_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdfnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdfnr.FindFirst("-",&pos);
	while(h)
	{
		cdfnr.Delete(pos,1);
		h = cdfnr.FindFirst("-",&pos);
	}
	cdfnr.ToUpper();
	kb = cdfnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_ABOUT_CDFBX); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDFBX,name,0,"CDabout.tif","About CD FBX",CDDataAllocator(CDFBXRegister));
	else return CDRegisterCommandPlugin(ID_ABOUTCDFBX,name,0,"CDabout.tif","About CD FBX",CDDataAllocator(CDFBXAbout));
}

