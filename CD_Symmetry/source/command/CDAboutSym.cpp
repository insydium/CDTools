//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDSymmetry.h"

//#define CDSY_SERIAL_SIZE				 27
class CDSymmetryAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDSymmetryAboutUserArea 			ua;
		
	public:
		CDSymmetryAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDSymmetryAboutDialog::CDSymmetryAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDSymmetryAboutDialog::CreateLayout(void)
{
	CHAR data[CDSY_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE))
	{
#if API_VERSION < 12000
		cdSerial.SetCString(data,CDSY_SERIAL_SIZE-1,St8bit);
#else
		cdSerial.SetCString(data,CDSY_SERIAL_SIZE-1,STRINGENCODING_8BIT);
#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDSY_ABOUT,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDSY_ABOUT_IMAGE);
		SetString(IDC_CDSY_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDSYMMETRYTOOLS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
		SetString(IDC_CDSY_SERIALNUMBER,cdSerial);
	}
	
	return res;
}

Bool CDSymmetryAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDSY_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

Bool CDSymmetryAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDSY_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDSY_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDSY_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDSY_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDSY_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDSymmetryAbout : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDSY_SERIAL_SIZE];
			CDSymmetryAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDSymmetryEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDSymmetryEULADialog::CreateLayout(void)
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
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDSY_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDSY_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDSY_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDSY_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDSY_DISAGREE));
			AddButton(IDC_CDSY_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDSY_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDSymmetryEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDSY_EULA,GeLoadString(IDC_CDSY_EULA));
	
	return true;
}

Bool CDSymmetryEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDSY_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDSY_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDSymmetryRegisterDialog : public GeModalDialog
{
	private:
		CDSymmetryAboutUserArea 			ua;
		CDSymmetryEULADialog				eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDSymmetryRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDSY_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDSY_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDSY_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDSY_SERIAL_SIZE];
					sn.GetCString(data,CDSY_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDSY_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDSymmetryRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDSY_REGISTER,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDSY_ABOUT_IMAGE);
		SetString(IDC_CDSY_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDSYMMETRYTOOLS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDSymmetryRegisterDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDSY_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDSY_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDSY_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDSY_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDSY_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDSymmetryRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CDSymmetryRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDSymmetry(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) reg = false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdsnr.FindFirst("-",&pos);
	while(h)
	{
		cdsnr.Delete(pos,1);
		h = cdsnr.FindFirst("-",&pos);
	}
	cdsnr.ToUpper();
	
	kb = cdsnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_ABOUTCDSY); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDSYMMETRY,name,0,"CDabout.tif","About CD Symmetry Tools",CDDataAllocator(CDSymmetryRegister));
	else return CDRegisterCommandPlugin(ID_ABOUTCDSYMMETRY,name,0,"CDabout.tif","About CD Symmetry Tools",CDDataAllocator(CDSymmetryAbout));
}

