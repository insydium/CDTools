//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch
	

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class CDTransUDDialog : public GeModalDialog
{
	private:
		CDTROptionsUA 			ua;
		
	public:	
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDTransUDDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDUDTRANS));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDTR_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDTR_OPTIONS_IMAGE);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(CDTR_TRANSFER),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(CDTR_MIRROR,BFH_CENTER,2,1);
					AddChild(CDTR_MIRROR, 0, GeLoadString(CDTR_SINGLE));
					AddChild(CDTR_MIRROR, 1, GeLoadString(CDTR_MULTIPLE));

			}
			GroupEnd();
		}
		GroupEnd();
			
		AddDlgGroup(DLG_OK | DLG_CANCEL);
	}

	return res;
}

Bool CDTransUDDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	BaseContainer *bc = GetWorldPluginData(ID_CDTRANSFERUSERDATA);
	if(!bc)
	{
		SetLong(CDTR_MIRROR,0);
	}
	else
	{
		SetLong(CDTR_MIRROR,bc->GetLong(CDTR_MIRROR));
	}
		
	return true;
}

Bool CDTransUDDialog::Command(LONG id,const BaseContainer &msg)
{
	LONG mir;
	
	GetLong(CDTR_MIRROR,mir);
	
	BaseContainer bc;
	bc.SetLong(CDTR_MIRROR,mir);
	SetWorldPluginData(ID_CDTRANSFERUSERDATA,bc,false);
	
	return true;
}

class CDTransferUserData : public CommandData
{
	private:
		CDTransUDDialog dlg;

	public:
		Bool DoUDTransfer(BaseDocument *doc, BaseContainer *data, Bool shift);
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDTransferUserData::DoUDTransfer(BaseDocument *doc, BaseContainer *data, Bool shift)
{
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG i, opCnt = opSelLog->GetCount();
	if(opCnt > 2 && data->GetLong(CDTR_MIRROR) == 0)
	{
		MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
		doc->SetActiveObject(NULL);
	}
	else if(opCnt < 2)
	{
		MessageDialog(GeLoadString(MD_AT_LEAST_TWO));
	}
	else
	{
		BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!op) return false;
		
		BaseObject *trg = NULL;
		
		doc->SetActiveObject(NULL);
		doc->StartUndo();
		
		for(i=1; i<opCnt; i++)
		{
			trg = static_cast<BaseObject*>(opSelLog->GetIndex(i));
			if(trg)
			{
				TransferUserData(doc,op,trg);
				doc->SetActiveObject(trg,SELECTION_ADD);
			}

		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

Bool CDTransferUserData::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)
	{
		if(!dlg.Open()) return false;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	}
	Bool shift = (state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT);

	BaseContainer *wpData = GetWorldPluginData(ID_CDTRANSFERUSERDATA);
	if(!wpData)
	{
		BaseContainer bc;
		bc.SetLong(CDTR_MIRROR,0);
		SetWorldPluginData(ID_CDTRANSFERUSERDATA,bc,false);
		wpData = GetWorldPluginData(ID_CDTRANSFERUSERDATA);
	}

	return DoUDTransfer(doc, wpData, shift);
}

class CDTransferUserDataR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDTransferUserData(void)
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
	String name=GeLoadString(IDS_CDUDTRANS); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDTRANSFERUSERDATA,name,PLUGINFLAG_HIDE,"CDTransUD.tif","CD Transfer User Data",CDDataAllocator(CDTransferUserDataR));
	else return CDRegisterCommandPlugin(ID_CDTRANSFERUSERDATA,name,0,"CDTransUD.tif","CD Transfer User Data"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDTransferUserData));
}
