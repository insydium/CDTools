//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDJoint.h"

#include "oCDJoint.h"

class CDJointMirrorAssign : public CommandData
{
	public:
		void ChildJointMirrorAssign(BaseDocument *doc, BaseObject *op, BaseObject *target);
		
		virtual Bool Execute(BaseDocument* doc);

};

void CDJointMirrorAssign::ChildJointMirrorAssign(BaseDocument *doc, BaseObject *op, BaseObject *target)
{
	BaseContainer *opData = NULL, *trgData = NULL;
	
	while(op && target)
	{
		if(op->IsInstanceOf(ID_CDJOINTOBJECT) && target->IsInstanceOf(ID_CDJOINTOBJECT))
		{
			opData = op->GetDataInstance();
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			opData->SetLink(JNT_JOINT_M,target);
			
			trgData = target->GetDataInstance();
			CDAddUndo(doc,CD_UNDO_CHANGE,target);
			trgData->SetLink(JNT_JOINT_M,op);
		}
		
		ChildJointMirrorAssign(doc,op->GetDown(),target->GetDown());
		
		op = op->GetNext();
		target = target->GetNext();
	}
}

Bool CDJointMirrorAssign::Execute(BaseDocument* doc)
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
	
	if(opCount == 2)
	{
		BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0));
		BaseObject *target = static_cast<BaseObject*>(opSelLog->GetIndex(1));
		if(!op || !target) return true;

		BaseContainer *opData = NULL, *trgData = NULL;
		if(op->IsInstanceOf(ID_CDJOINTOBJECT) && target->IsInstanceOf(ID_CDJOINTOBJECT))
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();
			
			opData = op->GetDataInstance();
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			opData->SetLink(JNT_JOINT_M,target);
			
			trgData = target->GetDataInstance();
			CDAddUndo(doc,CD_UNDO_CHANGE,target);
			trgData->SetLink(JNT_JOINT_M,op);

			if(op->GetDown() && target->GetDown())
			{
				Bool includeChild = true;
				if(!QuestionDialog(GeLoadString(D_AD_CHILDREN))) includeChild = false;
				if(includeChild)
				{
					ChildJointMirrorAssign(doc,op->GetDown(),target->GetDown());
				}
			}
			doc->EndUndo();
			
			EventAdd(EVENT_FORCEREDRAW);
		}
	}

	return true;
}

class CDJointMirrorAssignR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDJointMirrorAssign(void)
{
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDJMASSIGN); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDJOINTMIRRORASSIGN,name,PLUGINFLAG_HIDE,"CDJMAssign.tif","CD Joint Mirror Assign",CDDataAllocator(CDJointMirrorAssignR));
	else return CDRegisterCommandPlugin(ID_CDJOINTMIRRORASSIGN,name,0,"CDJMAssign.tif","CD Joint Mirror Assign",CDDataAllocator(CDJointMirrorAssign));
}
