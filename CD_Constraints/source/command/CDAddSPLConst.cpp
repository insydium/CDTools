//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "tCDSPLConstraint.h"
#include "CDConstraint.h"


class CDAddSPLConstraint : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddSPLConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();

	if(opCount > 3)
	{
		MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
		doc->SetActiveObject(NULL);
		return true;
	}
	
	if(opCount > 0)
	{
		BaseTag *splTag = NULL;
		BaseObject *op = NULL;
		BaseObject *target = static_cast<BaseObject*>(opSelLog->GetIndex(1));
		if(target)
		{
			if(!target->GetRealSpline())
			{
				MessageDialog(GeLoadString(MD_NOT_SPLINE_OBJECT));
				doc->SetActiveObject(NULL);
				return true;
			}
		}
		
		op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(op)
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();
			if(!op->GetTag(ID_CDSPLCONSTRAINTPLUGIN))
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				
				BaseTag *splTag = BaseTag::Alloc(ID_CDSPLCONSTRAINTPLUGIN);
				
				BaseTag *prev = GetPreviousConstraintTag(op);
				op->InsertTag(splTag,prev);
				
				CDAddUndo(doc,CD_UNDO_NEW,splTag);
				splTag->Message(MSG_MENUPREPARE);
				
				CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,splTag);
				BaseContainer *tData = splTag->GetDataInstance();
				if(target) tData->SetLink(SPLC_TARGET,target);
				if(opCount == 3)
				{
					BaseObject *up = static_cast<BaseObject*>(opSelLog->GetIndex(2));
					if(up)
					{
						tData->SetLink(SPLC_UP_VECTOR,up);
						tData->SetBool(SPLC_TANGENT,true);
					}
				}
				splTag->Message(MSG_UPDATE);
			}
			
			doc->SetActiveObject(op);
			doc->SetActiveTag(splTag);
			
			doc->EndUndo();
			
			EventAdd(EVENT_FORCEREDRAW);
		}
	}

	return true;
}

class CDAddSPLConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddSPLConstraint(void)
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
	String name=GeLoadString(IDS_CDADDSPL); if (!name.Content()) return true;
	if(!reg) return CDRegisterCommandPlugin(ID_CDADDSPLCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddSPLConst.tif","CD Add Spline Constraint",CDDataAllocator(CDAddSPLConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDSPLCONSTRAINT,name,0,"CDAddSPLConst.tif","CD Add Spline Constraint",CDDataAllocator(CDAddSPLConstraint));
}
