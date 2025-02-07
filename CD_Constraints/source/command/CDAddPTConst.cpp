//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDConstraint.h"
#include "CDArray.h"

#define MAX_ADD 	100

enum
{
	PTC_ADD_PT				= 1031,

	PTC_TARGET_COUNT			= 1102,
	
	PTC_TARGET				= 4000,
	
	PTC_PT_INDEX			= 7000
};

class CDAddPTConstraint : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddPTConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCnt = opSelLog->GetCount();
	if(opCnt > 0)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		if(!IsValidPointObject(destObject))
		{
			MessageDialog(GeLoadString(MD_NOT_POINT_OBJECT));
			doc->SetActiveObject(destObject);
			return true;
		}
		
		doc->SetActiveObject(NULL);
		doc->StartUndo();

		BaseTag *ptTag = NULL;
		if(!destObject->GetTag(ID_CDPTCONSTRAINTPLUGIN))
		{
			ptTag = BaseTag::Alloc(ID_CDPTCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(ptTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,ptTag);
			ptTag->Message(MSG_MENUPREPARE);
		}
		else ptTag = destObject->GetTag(ID_CDPTCONSTRAINTPLUGIN);
		
		BaseContainer *tData = ptTag->GetDataInstance();
		
		BaseSelect *bs = ToPoint(destObject)->GetPointS();
		LONG bsCount = bs->GetCount();
		
		CDArray<LONG> ptInd;
		ptInd.Alloc(bsCount);
		
		LONG i, ptCnt=0, seg=0, a, b;
		while(CDGetRange(bs,seg++,&a,&b))
		{
			for (i=a; i<=b; ++i)
			{
				ptInd[ptCnt] = i;
				ptCnt++;
			}
		}
		
		BaseObject *target = NULL;
		LONG tCnt = tData->GetLong(PTC_TARGET_COUNT);
		if(bs && bsCount > 0)
		{
			if(opCnt > 1)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,destObject);
				
				LONG cnt = 0;
				for(i=1; i<opCnt; i++)
				{
					if(tCnt < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(tCnt == 1 && tData->GetObjectLink(PTC_TARGET,doc) == NULL)
						{
							tData->SetLong(PTC_PT_INDEX,ptInd[cnt]);
							tData->SetLink(PTC_TARGET,target);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(PTC_ADD_PT);
							ptTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,ptTag);
							tData->SetLong(PTC_PT_INDEX+tCnt,ptInd[cnt]);
							tData->SetLink(PTC_TARGET+tCnt,target);
							
							tCnt = tData->GetLong(PTC_TARGET_COUNT);
						}
						if(i < ptCnt) cnt++;
					}
				}
			}
			else
			{
				Vector *padr = GetPointArray(destObject);
				for(i=0; i<ptCnt; i++)
				{
					if(i < MAX_ADD)
					{
						target = BaseObject::Alloc(Onull); if (!target) return false;
						target->SetName(destObject->GetName() + GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
						doc->InsertObject(target, NULL, NULL, false);
						CDAddUndo(doc,CD_UNDO_NEW,target);
						
						Matrix m = destObject->GetMg();
						Vector ptPos = m * padr[ptInd[i]];
						m.off = ptPos;
						target->SetMg(m);
						
						BaseContainer *data = target->GetDataInstance();
						data->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_CUBE);
						data->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
						ObjectColorProperties prop;
						prop.color = Vector(0,1,1);
						prop.usecolor = 2;
						target->SetColorProperties(&prop);
						
						if(tCnt == 1 && tData->GetObjectLink(PTC_TARGET,doc) == NULL)
						{
							tData->SetLong(PTC_PT_INDEX,ptInd[i]);
							tData->SetLink(PTC_TARGET,target);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(PTC_ADD_PT);
							ptTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,ptTag);
							tData->SetLong(PTC_PT_INDEX+tCnt,ptInd[i]);
							tData->SetLink(PTC_TARGET+tCnt,target);
							
							tCnt = tData->GetLong(PTC_TARGET_COUNT);
						}
					}
				}
			}
		}
		else
		{
			if(opCnt > 1)
			{
				for(i=1; i<opCnt; i++)
				{
					if(tCnt < MAX_ADD)
					{			
						target = static_cast<BaseObject*>(opSelLog->GetIndex(i));
						if(tCnt == 1 && tData->GetObjectLink(PTC_TARGET,doc) == NULL)
						{
							tData->SetLink(PTC_TARGET,target);
						}
						else
						{
							DescriptionCommand dc;
							dc.id = DescID(PTC_ADD_PT);
							ptTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,ptTag);
							tData->SetLink(PTC_TARGET+tCnt,target);
							
							tCnt = tData->GetLong(PTC_TARGET_COUNT);
						}
					}
				}
			}
		}
		

		doc->SetActiveObject(target);
		doc->SetActiveTag(ptTag);
		ptTag->Message(MSG_UPDATE);
		
		doc->EndUndo();

		ptInd.Free();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddPTConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddPTConstraint(void)
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
	String name=GeLoadString(IDS_CDADDPT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDPTCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddPTConst.tif","CD Add Points Constraint",CDDataAllocator(CDAddPTConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDPTCONSTRAINT,name,0,"CDAddPTConst.tif","CD Add Points Constraint",CDDataAllocator(CDAddPTConstraint));
}
