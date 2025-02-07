//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"

class CDJSAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDJSAboutUserArea 	ua;
		
	public:
		CDJSAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDJSAboutDialog::CDJSAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDJSAboutDialog::CreateLayout(void)
{
	CHAR data[CDJS_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE))
	{
	#if API_VERSION < 12000
		cdSerial.SetCString(data,CDJS_SERIAL_SIZE-1,St8bit);
	#else
		cdSerial.SetCString(data,CDJS_SERIAL_SIZE-1,STRINGENCODING_8BIT);
	#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDJS_ABOUT,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDJS_ABOUT_IMAGE);
		SetString(IDC_CDJS_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDJOINTSANDSKIN),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
		SetString(IDC_CDJS_SERIALNUMBER,cdSerial);
	}
	
	return res;
}

Bool CDJSAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDJS_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

Bool CDJSAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDJS_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDJS_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDJS_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDJS_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDJS_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDJSAbout : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDJS_SERIAL_SIZE];
			CDJSAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDJSEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDJSEULADialog::CreateLayout(void)
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
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDJS_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDJS_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDJS_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDJS_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDJS_DISAGREE));
			AddButton(IDC_CDJS_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDJS_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDJSEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDJS_EULA,GeLoadString(IDC_CDJS_EULA));
	
	return true;
}

Bool CDJSEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDJS_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDJS_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDJSRegisterDialog : public GeModalDialog
{
	private:
		CDJSAboutUserArea 			ua;
		CDJSEULADialog				eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDJSRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDJS_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDJS_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDJS_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDJS_SERIAL_SIZE];
					sn.GetCString(data,CDJS_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDJS_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDJSRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDJS_REGISTER,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDJS_ABOUT_IMAGE);
		SetString(IDC_CDJS_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDJOINTSANDSKIN),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDJSRegisterDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDJS_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDJS_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDJS_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDJS_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDJS_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDJSRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CDJSRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDJointsAndSkin(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_ABOUT_CDJS); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDJOINTSKIN,name,0,"CDabout.tif","About CD Joints & Skin",CDDataAllocator(CDJSRegister));
	else return CDRegisterCommandPlugin(ID_ABOUTCDJOINTSKIN,name,0,"CDabout.tif","About CD Joints & Skin",CDDataAllocator(CDJSAbout));
}

