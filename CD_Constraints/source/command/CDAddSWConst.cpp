//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tCDSWConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	9

enum
{
	SWC_TARGET_COUNT			= 1040
};

class CDAddSWConstraint : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddSWConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCnt = opSelLog->GetCount();
	if(opCnt > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		
		BaseTag *sTag = NULL;
		if(!destObject->GetTag(ID_CDSWCONSTRAINTPLUGIN))
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();
			sTag = BaseTag::Alloc(ID_CDSWCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(sTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,sTag);
			sTag->Message(MSG_MENUPREPARE);
		}
		else
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();

			sTag = destObject->GetTag(ID_CDSWCONSTRAINTPLUGIN);
		}
		
		BaseContainer *tData = sTag->GetDataInstance();
		
		BaseObject *target = NULL;
		LONG i, sCnt = tData->GetLong(SWC_TARGET_COUNT);
		if(opCnt > 1)
		{
			for(i=1; i<opCnt; i++)
			{
				if(sCnt < MAX_ADD)
				{			
					target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
					if(sCnt == 1 && tData->GetObjectLink(SWC_TARGET,doc) == NULL)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,sTag);
						tData->SetLink(SWC_TARGET,target);
					}
					else
					{
						DescriptionCommand dc;
						dc.id = DescID(SWC_ADD_SPC);
						sTag->Message(MSG_DESCRIPTION_COMMAND,&dc);

						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,sTag);
						tData->SetLink(SWC_TARGET+sCnt,target);
						
						sCnt = tData->GetLong(SWC_TARGET_COUNT);
					}
				}
			}
		}

		doc->SetActiveObject(target);
		doc->SetActiveTag(sTag);
		sTag->Message(MSG_UPDATE);
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddSWConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};
Bool RegisterCDAddSWConstraint(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDADDSW); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDSWCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddSWConst.tif","CD Add Space Switch Constraint",CDDataAllocator(CDAddSWConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDSWCONSTRAINT,name,0,"CDAddSWConst.tif","CD Add Space Switch Constraint"+GeLoadString(IDS_HLP_CNTRL_CLICK),CDDataAllocator(CDAddSWConstraint));
}
