//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "customgui_priority.h"

#include "CDCompatibility.h"
#include "tCDNConstraint.h"
#include "CDConstraint.h"

class CDAddNConstraint : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddNConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  i, pVal = 0, opCount = opSelLog->GetCount();
	DescriptionCommand dc;
	
	if(opCount > 1)
	{
		BaseTag *nTag = NULL;
		BaseObject *op = NULL;
		BaseObject *trg = static_cast<BaseObject*>(opSelLog->GetIndex(opCount-1)); 
		if(!trg) return false;
		if(!GetPolygonalObject(trg))
		{
			MessageDialog(GeLoadString(MD_NOT_POLYGON_SURFACE));
			doc->SetActiveObject(trg);
			return true;
		}
		
		LONG xPrty = CYCLE_EXPRESSION;
		if(trg->GetTag(ID_CDMORPHREFPLUGIN)) pVal = 101;
		if(trg->GetTag(ID_CDSKINPLUGIN)) pVal = 201;
		if(trg->GetInfo() & OBJECT_GENERATOR)
		{
			xPrty = CYCLE_GENERATORS;
			pVal = 1;
		}
		
		doc->SetActiveObject(NULL);
		doc->StartUndo();
		for(i=0; i<opCount-1; i++)
		{
			op = static_cast<BaseObject*>(opSelLog->GetIndex(i)); 
			if(op)
			{
				if(!op->GetTag(ID_CDNCONSTRAINTPLUGIN))
				{
					nTag = BaseTag::Alloc(ID_CDNCONSTRAINTPLUGIN);
					
					BaseTag *prev = GetPreviousConstraintTag(op);
					op->InsertTag(nTag,prev);
					
					CDAddUndo(doc,CD_UNDO_NEW,nTag);
					nTag->Message(MSG_MENUPREPARE);
					
					CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,nTag);
					GeData d;
					if(CDGetParameter(nTag,DescLevel(EXPRESSION_PRIORITY),d))
					{
						PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
						if(pd)
						{
							pd->SetPriorityValue(PRIORITYVALUE_MODE,xPrty);
							pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,pVal);
							pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
						}
						CDSetParameter(nTag,DescLevel(EXPRESSION_PRIORITY),d);
					}
					BaseContainer *tData = nTag->GetDataInstance();
					tData->SetLink(NC_TARGET,trg);
					dc.id = DescID(NC_N_SET);
					nTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
					
					nTag->Message(MSG_UPDATE);
				}
			}
		}
		
		doc->SetActiveObject(op);
		doc->SetActiveTag(nTag);
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDAddNConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddNConstraint(void)
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
	String name=GeLoadString(IDS_CDADDN); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDNCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddNConst.tif","CD Add Nail Constraint",CDDataAllocator(CDAddNConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDNCONSTRAINT,name,0,"CDAddNConst.tif","CD Add Nail Constraint",CDDataAllocator(CDAddNConstraint));
}
