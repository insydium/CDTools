//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDConstraint.h"

#include "tCDPRConstraint.h"

class CDAddPRConstraint : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddPRConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 1)
	{
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
		if(!destObject) return false;
		
		BaseObject *target = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
		if(!target) return false;
		
		if(opCount > 2)
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return true;
		}
		if(destObject->GetTag(ID_CDPRCONSTRAINTPLUGIN)) return true;

		doc->SetActiveObject(NULL);
		doc->StartUndo();

		BaseTag *prTag = BaseTag::Alloc(ID_CDPRCONSTRAINTPLUGIN);
		
		BaseTag *prev = GetPreviousConstraintTag(destObject);
		destObject->InsertTag(prTag,prev);
		
		CDAddUndo(doc,CD_UNDO_NEW,prTag);
		prTag->Message(MSG_MENUPREPARE);
		
		CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,prTag);
		BaseContainer *tData = prTag->GetDataInstance();
		tData->SetLink(PRC_TARGET,target);
		
		doc->SetActiveObject(target);
		doc->SetActiveTag(prTag);
		prTag->Message(MSG_UPDATE);
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);

	}
	return true;
}

class CDAddPRConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddPRConstraint(void)
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
	String name=GeLoadString(IDS_CDADDPR); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDPRCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddPRConst.tif","CD Add Parent Constraint",CDDataAllocator(CDAddPRConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDPRCONSTRAINT,name,0,"CDAddPRConst.tif","CD Add Parent Constraint",CDDataAllocator(CDAddPRConstraint));
}
