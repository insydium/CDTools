//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"

#include "oCDJoint.h"

class CDRerootObjects : public CommandData
{
	private:
		BaseObject* GetTopParent(BaseObject *op, LONG type);
		void RerootObjects(BaseDocument *doc, BaseObject *top, BaseObject *pr, BaseObject *ch);
		
	public:		
		virtual Bool Execute(BaseDocument* doc);
};

BaseObject* CDRerootObjects::GetTopParent(BaseObject *op, LONG type)
{
	BaseObject *res = op;
	
	BaseObject *pr = op->GetUp();
	while(pr)
	{
		if(pr->GetType() != type) break;
		else
		{
			res = pr;
			if(res->GetType() == ID_CDJOINTOBJECT) 
			{
				BaseContainer *resData = res->GetDataInstance();
				if(!resData) break;
				else
				{
					if(!resData->GetBool(JNT_CONNECTED)) break;
				}
			}
		}
		pr = pr->GetUp();
	}
	
	return res;
}

void CDRerootObjects::RerootObjects(BaseDocument *doc, BaseObject *top, BaseObject *pr, BaseObject *ch)
{	
	while(ch != top)
	{
		BaseObject *nxt = ch->GetUp();
		Matrix chM = ch->GetMg();
		
		CDAddUndo(doc,CD_UNDO_CHANGE,ch);
		ch->Remove();
		doc->InsertObject(ch,pr,NULL,false);
		ch->SetMg(chM);
		
		if(ch->GetType() == ID_CDJOINTOBJECT && nxt->GetType() == ID_CDJOINTOBJECT)
		{
			BaseContainer *chData = ch->GetDataInstance();
			BaseContainer *nxtData = nxt->GetDataInstance();
			if(chData && nxtData) chData->SetBool(JNT_CONNECTED,nxtData->GetBool(JNT_CONNECTED));
		}
					
		pr = ch;
		ch = nxt;
	}
	
	Matrix topM = top->GetMg();
	
	CDAddUndo(doc,CD_UNDO_CHANGE,top);
	top->Remove();
	doc->InsertObject(top,pr,NULL,false);
	top->SetMg(topM);
}

Bool CDRerootObjects::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	opSelectionList->FilterObjectChildren();
	
	LONG  opCnt = opSelectionList->GetCount();
	if(opCnt > 0)
	{
		doc->StartUndo();
				
		LONG i;
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
			if(op)
			{
				BaseObject *top = GetTopParent(op,op->GetType());
				if(top != op)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					CDAddUndo(doc,CD_UNDO_CHANGE,top);
					
					BaseContainer *opData = op->GetDataInstance();
					BaseContainer *topData = top->GetDataInstance();
					if(opData && topData)
					{
						if(op->GetType() == ID_CDJOINTOBJECT && top->GetType() == ID_CDJOINTOBJECT)
						{
							opData->SetBool(JNT_CONNECTED,topData->GetBool(JNT_CONNECTED));
							BaseObject *jnt = top->GetDown();
							if(jnt)
							{
								if(jnt->GetType() == ID_CDJOINTOBJECT)
								{
									BaseContainer *jData = jnt->GetDataInstance();
									if(jData) topData->SetBool(JNT_CONNECTED,jData->GetBool(JNT_CONNECTED));
								}
							}
						}
					}
					
					BaseObject *ch = op->GetUp();
					Matrix opM = op->GetMg();
					
					op->Remove();
					doc->InsertObject(op,top->GetUp(),top->GetPred(),false);
					op->SetMg(opM);
					
					RerootObjects(doc,top,op,ch);
					
					doc->SetActiveObject(op);
					CallCommand(ID_CDORIENTJOINTS);
				}
			}
		}
		
		doc->EndUndo();
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDRerootObjectsR : public CommandData
{
	public:		
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDRerootObjects(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDREROOT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDREROOTJOINTS,name,PLUGINFLAG_HIDE,"CDReroot.TIF","CD Reroot Objects",CDDataAllocator(CDRerootObjectsR));
	else return CDRegisterCommandPlugin(ID_CDREROOTJOINTS,name,0,"CDReroot.TIF","CD Reroot Objects",CDDataAllocator(CDRerootObjects));
}
