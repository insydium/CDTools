//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDTransfer.h"

#include "tCDTranSel.h"

class CDAddTransferSelected : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddTransferSelected::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 1)
	{
		BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!op) return false;
		
		BaseObject *dest = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
		if(!dest) return false;
		
		if(opCount > 2)
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return false;
		}
		if(op->GetTag(ID_CDTRANSELECTEDTAG)) return false;
		
		doc->StartUndo();
		doc->SetActiveObject(NULL);

		BaseTag *tslTag = BaseTag::Alloc(ID_CDTRANSELECTEDTAG);
		op->InsertTag(tslTag,NULL);
		tslTag->Message(MSG_MENUPREPARE);
		CDAddUndo(doc,CD_UNDO_NEW,tslTag);
		
		CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tslTag);
		BaseContainer *tData = tslTag->GetDataInstance();
		tData->SetLink(TRNSEL_TARGET,dest);
		tData->SetBool(TRNSEL_TRANSFER_ON,true);
		
		CDAddUndo(doc,CD_UNDO_CHANGE,dest);
		doc->SetActiveObject(dest,SELECTION_ADD);
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}
	else if(opCount == 1)
	{
		BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!op) return false;
		
		BaseTag *dstTag = doc->GetActiveTag();
		
		doc->StartUndo();
		doc->SetActiveObject(NULL);
		doc->SetActiveTag(NULL);

		BaseTag *tslTag = BaseTag::Alloc(ID_CDTRANSELECTEDTAG);
		op->InsertTag(tslTag,NULL);
		tslTag->Message(MSG_MENUPREPARE);
		CDAddUndo(doc,CD_UNDO_NEW,tslTag);
			
		if(dstTag)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tslTag);
			BaseContainer *tData = tslTag->GetDataInstance();
			tData->SetLink(TRNSEL_TARGET,dstTag);
			tData->SetBool(TRNSEL_TRANSFER_ON,true);
		}
			
		CDAddUndo(doc,CD_UNDO_CHANGE,dstTag);
		doc->SetActiveTag(dstTag,SELECTION_ADD);
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddTransferSelectedR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddTransferSelected(void)
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
	String name=GeLoadString(IDS_CDADDTSL); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDTRANSELECTED,name,PLUGINFLAG_HIDE,"CDAddTrSel.tif","CD Add Transfer Selected",CDDataAllocator(CDAddTransferSelectedR));
	else return CDRegisterCommandPlugin(ID_CDADDTRANSELECTED,name,0,"CDAddTrSel.tif","CD Add Transfer Selected",CDDataAllocator(CDAddTransferSelected));
}
