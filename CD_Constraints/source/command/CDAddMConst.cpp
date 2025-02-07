//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDConstraint.h"

#include "tCDMConstraint.h"

class CDAddMConstraint : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddMConstraint::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		if(opCount > 2)
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return true;
		}
		else
		{
			BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
			if(!destObject) return false;
			
			if(destObject->GetTag(ID_CDMCONSTRAINTPLUGIN)) return true;

			doc->SetActiveObject(NULL);
			doc->StartUndo();

			BaseTag *mTag = BaseTag::Alloc(ID_CDMCONSTRAINTPLUGIN);
			
			BaseTag *prev = GetPreviousConstraintTag(destObject);
			destObject->InsertTag(mTag,prev);
			
			CDAddUndo(doc,CD_UNDO_NEW,mTag);
			mTag->Message(MSG_MENUPREPARE);
			
			if(opCount > 1)
			{
				BaseObject *target = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
				if(!target) return false;
				
				BaseObject *dParent = destObject->GetUp();
				BaseObject *tParent = target->GetUp();
				
				Matrix pM = Matrix();
				Vector dir = destObject->GetMg().off - target->GetMg().off;
				Real dot,ydot,zdot;
				LONG axis;
				
				Bool local = false;
				if(dParent && tParent && dParent == tParent)
				{
					local = true;
					pM = dParent->GetMg();
				}
				dot = VDot(VNorm(dir), VNorm(pM.v1));
				axis = MC_X_AXIS;
				
				ydot = VDot(VNorm(dir), VNorm(pM.v2));
				if(Abs(ydot) > Abs(dot))
				{
					dot = ydot;
					axis = MC_Y_AXIS;
				}
				
				zdot = VDot(VNorm(dir), VNorm(pM.v3));
				if(Abs(zdot) > Abs(dot))
				{
					axis = MC_Z_AXIS;
				}
				
				CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,mTag);
				BaseContainer *tData = mTag->GetDataInstance();
				tData->SetLong(MC_MIRROR_AXIS,axis);
				tData->SetLink(MC_TARGET,target);
				
				doc->SetActiveObject(target);
			}
			doc->SetActiveTag(mTag);
			mTag->Message(MSG_UPDATE);
			
			doc->EndUndo();
			
			EventAdd(EVENT_FORCEREDRAW);
		}
	}

	return true;
}

class CDAddMConstraintR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddMConstraint(void)
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
	String name=GeLoadString(IDS_CDADDM); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDMCONSTRAINT,name,PLUGINFLAG_HIDE,"CDAddMConst.tif","CD Add Mirror Constraint",CDDataAllocator(CDAddMConstraintR));
	else return CDRegisterCommandPlugin(ID_CDADDMCONSTRAINT,name,0,"CDAddMConst.tif","CD Add Mirror Constraint",CDDataAllocator(CDAddMConstraint));
}
