//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDTransfer.h"

class CDSelectAllSame : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);
		
		Bool SelectSameObjects(BaseDocument* doc, BaseObject *op, LONG type);
		Bool SelectSameTags(BaseDocument* doc, BaseObject *op, LONG type);

};

Bool CDSelectAllSame::SelectSameObjects(BaseDocument* doc, BaseObject *op, LONG type)
{
	while(op)
	{
		if(op->GetType() == type)
		{
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			doc->SetActiveObject(op,SELECTION_ADD);
		}
		
		SelectSameObjects(doc,op->GetDown(),type);
		op = op->GetNext();
	}
	
	return true;
}

Bool CDSelectAllSame::SelectSameTags(BaseDocument* doc, BaseObject *op, LONG type)
{
	BaseTag *tag = NULL;
	
	while(op)
	{
		tag = op->GetFirstTag();
		while(tag)
		{
			if(tag->GetType() == type)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				doc->SetActiveTag(tag,SELECTION_ADD);
			}
			tag = tag->GetNext();
		}
		SelectSameTags(doc,op->GetDown(),type);
		op = op->GetNext();
	}
	
	return true;
}

Bool CDSelectAllSame::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	LONG mode = 0;
	
	BaseObject *op = NULL;
	BaseTag *tag = NULL;
	
	AutoAlloc<AtomArray> atomList; if(!atomList) return false;
	CDGetActiveObjects(doc,atomList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	LONG atomCnt = atomList->GetCount();
	
	if(atomCnt == 1)
	{
		if(!doc->GetActiveTag()) mode = 1;
	}
	else if(atomCnt == 0)
	{
		atomList->Flush();
		doc->GetActiveTags(atomList);
		atomCnt = atomList->GetCount();
		if(atomCnt == 1) mode = 2;
	}
	
	if(mode > 0)
	{
		Bool local = false;
		BaseObject *startOp = NULL;
		
		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT) local = true;
				
		doc->StartUndo();
		switch(mode)
		{
			case 1:
			{
				op = static_cast<BaseObject*>(atomList->GetIndex(0));
				LONG type = op->GetType();
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				doc->SetActiveObject(NULL,SELECTION_NEW);
				if(!local) startOp = doc->GetFirstObject();
				else
				{
					BaseObject *parent = op->GetUp();
					if(parent) startOp = parent->GetDown();
					else startOp = op;
				}
				SelectSameObjects(doc,startOp,type);
				break;
			}
			case  2:
			{
				tag = static_cast<BaseTag*>(atomList->GetIndex(0));
				LONG type = tag->GetType();
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				doc->SetActiveTag(NULL,SELECTION_NEW);
				if(!local) startOp = doc->GetFirstObject();
				else
				{
					op = tag->GetObject();
					BaseObject *parent = op->GetUp();
					if(parent) startOp = parent->GetDown();
					else startOp = op;
				}
				SelectSameTags(doc,startOp,type);
				break;
			}
		}
		doc->EndUndo();
		EventAdd(EVENT_FORCEREDRAW);
	}
	else
	{
		doc->SetActiveObject(NULL,SELECTION_NEW);
		doc->SetActiveTag(NULL,SELECTION_NEW);
		MessageDialog(GeLoadString(MD_SELECT_SINGLE));
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

class CDSelectAllSameR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDSelectAllSame(void)
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
	String name=GeLoadString(IDS_CDSELSAME); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDSELECTALLSAME,name,PLUGINFLAG_HIDE,"CDSelSame.tif","CD Select All Same",CDDataAllocator(CDSelectAllSameR));
	else return CDRegisterCommandPlugin(ID_CDSELECTALLSAME,name,0,"CDSelSame.tif","CD Select All Same",CDDataAllocator(CDSelectAllSame));
}
