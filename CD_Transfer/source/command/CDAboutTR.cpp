//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class CDTransferAboutDialog : public GeModalDialog
{
	private:
		CHAR 					*str;
		CDTransferAboutUserArea 			ua;
		
	public:
		CDTransferAboutDialog(CHAR *t_str);

		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

CDTransferAboutDialog::CDTransferAboutDialog(CHAR *t_str)
{ 
	str = t_str; 
}

Bool CDTransferAboutDialog::CreateLayout(void)
{
	CHAR data[CDTR_SERIAL_SIZE];
	
	String cdSerial = "";
	if(CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE))
	{
	#if API_VERSION < 12000
		cdSerial.SetCString(data,CDTR_SERIAL_SIZE-1,St8bit);
	#else
		cdSerial.SetCString(data,CDTR_SERIAL_SIZE-1,STRINGENCODING_8BIT);
	#endif
	}
	
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDTR_ABOUT,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDTR_ABOUT_IMAGE);
		SetString(IDC_CDTR_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDTRANSFERTOOLS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
		SetString(IDC_CDTR_SERIALNUMBER,cdSerial);
	}
	
	return res;
}

Bool CDTransferAboutDialog::Command(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case IDC_CDTR_UPDATE:
			GeOpenHTML("http://www.cactus3d.com/News.html");
			break;
	}
	
	return true;
}

Bool CDTransferAboutDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDTR_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDTR_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDTR_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDTR_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDTR_SERIALNUMBER,false);
#endif
	
	return true;
}

class CDTransferAbout : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CHAR sn[CDTR_SERIAL_SIZE];
			CDTransferAboutDialog dlg(sn);
			return dlg.Open();
		}
};

class CDTransferEULADialog : public GeModalDialog
{
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
		
		String GetEULAText(void);
};

Bool CDTransferEULADialog::CreateLayout(void)
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
				if(GetC4DVersion() > 9999) AddMultiLineEditText(IDC_CDTR_EULA,BFH_SCALEFIT|BFV_SCALEFIT,720,200);
				else AddMultiLineEditText(IDC_CDTR_EULA,BFH_SCALEFIT|BFV_SCALEFIT,650,200);
			}
			else AddMultiLineEditText(IDC_CDTR_EULA,BFH_SCALEFIT|BFV_SCALEFIT,780,200);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_CENTER,3,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddButton(IDC_CDTR_DISAGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDTR_DISAGREE));
			AddButton(IDC_CDTR_AGREE,BFH_CENTER,0,0,GeLoadString(IDC_CDTR_AGREE));
		}
		GroupEnd();
	}
	
	return res;
}

Bool CDTransferEULADialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
	SetString(IDC_CDTR_EULA,GeLoadString(IDC_CDTR_EULA));
	
	return true;
}

Bool CDTransferEULADialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	
	switch(id)
	{
		case IDC_CDTR_DISAGREE:
			cl = Close(false);
			break;
		case IDC_CDTR_AGREE:
			cl = Close(true);
			break;
	}
	
	return cl;
}

class CDTransferRegisterDialog : public GeModalDialog
{
	private:
		CDTransferAboutUserArea 			ua;
		CDTransferEULADialog				eula;
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id, const BaseContainer& msg);
};

Bool CDTransferRegisterDialog::Command(LONG id, const BaseContainer& msg)
{
	Bool cl = true;
	String sn;
	
	switch(id)
	{
		case IDC_CDTR_CANCEL:
			cl = Close(false);
			break;
		case IDC_CDTR_REGISTER:
			if(eula.Open())
			{
				GetString(IDC_CDTR_SERIALNUMBER,sn);
				if(CheckKeyChecksum(sn))
				{
					CHAR data[CDTR_SERIAL_SIZE];
					sn.GetCString(data,CDTR_SERIAL_SIZE);
					CDWritePluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE);
					MessageDialog(GeLoadString(MD_THANKYOU));
					cl = Close(true);
				}
				else
				{
					MessageDialog(GeLoadString(MD_INVALID_NUMBER));
				}
			}
			break;
		case IDC_CDTR_PURCHASE:
			GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			break;
	}
	
	return cl;
}

Bool CDTransferRegisterDialog::CreateLayout(void)
{
	Bool res = GeModalDialog::CreateLayout();
	res = LoadDialogResource(DLG_CDTR_REGISTER,NULL,0);
	if(res)
	{
		AttachUserArea(ua,IDC_CDTR_ABOUT_IMAGE);
		SetString(IDC_CDTR_ABOUT_PLUGININFO,"Version "+CDRealToString(GetCDPluginVersion(ID_CDTRANSFERTOOLS),-1,3)+" "+GeLoadString(C_SYMBOL)+" 2008 Cactus Dan Libisch");
	}
	
	return res;
}

Bool CDTransferRegisterDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeModalDialog::InitValues()) return false;
	
#if API_VERSION < 12000
	if(GeGetVersionType() & VERSION_DEMO) Enable(IDC_CDTR_SERIALNUMBER,false);
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO) Enable(IDC_CDTR_SERIALNUMBER,false);
		if(GeGetVersionType() & CD_VERSION_SAVABLEDEMO_ACTIVE) Enable(IDC_CDTR_SERIALNUMBER,false);
	}
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) Enable(IDC_CDTR_SERIALNUMBER,false);
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) Enable(IDC_CDTR_SERIALNUMBER,false);
#endif
	return true;
}

class CDTransferRegister : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument *doc)
		{
			CDTransferRegisterDialog dlg;
			return dlg.Open();
		}
};

Bool RegisterAboutCDTransfer(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDTR_SERIAL_SIZE];
	String cdtnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = false;
	
	cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_ABOUTCDT); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ABOUTCDTRANSFER,name,0,"CDabout.tif","About CD Transfer Tools",CDDataAllocator(CDTransferRegister));
	else return CDRegisterCommandPlugin(ID_ABOUTCDTRANSFER,name,0,"CDabout.tif","About CD Transfer Tools",CDDataAllocator(CDTransferAbout));
}

