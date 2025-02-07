//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"

#include "oCDJoint.h"

class CDLinkObjects : public CommandData
{
	public:		
		virtual Bool Execute(BaseDocument* doc);
};

Bool CDLinkObjects::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	opSelectionList->FilterObjectChildren();
	
	LONG  selCount = opSelectionList->GetCount();
	if(selCount > 1)
	{
		BaseObject *pr = static_cast<BaseObject*>(opSelectionList->GetIndex(selCount-1)); 
		if(!pr) return false;
		
		LONG i;
		Bool connect = false;
		BaseObject *ch = NULL;
		
		doc->StartUndo();		
		for(i=0; i<selCount-1; i++)
		{
			ch = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
			if(ch)
			{
				BaseObject *pred = pr->GetDownLast();
				
				BaseContainer state;
				GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
				if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL) pred = NULL;
				if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT) connect = true;
				
				Matrix chM = ch->GetMg();
				
				CDAddUndo(doc,CD_UNDO_CHANGE,ch);
				ch->Remove();
				doc->InsertObject(ch,pr,pred,false);
				ch->SetMg(chM);
				
				if(ch->GetType() == ID_CDJOINTOBJECT && pr->GetType() == ID_CDJOINTOBJECT)
				{
					if(connect)
					{
						BaseContainer *jcData = ch->GetDataInstance();
						if(jcData) jcData->SetBool(JNT_CONNECTED, true);
					}
				}
			}
		}
		
		doc->SetActiveObject(pr);
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDLinkObjectsR : public CommandData
{
	public:		
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDLinkObjects(void)
{
	if(CDFindPlugin(ID_CDLINKOBJECTS,CD_COMMAND_PLUGIN)) return true;
	
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
	String name=GeLoadString(IDS_CDLINK); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDLINKOBJECTS,name,PLUGINFLAG_HIDE,"CDLinkObjects.TIF","CD Link Objects",CDDataAllocator(CDLinkObjectsR));
	else return CDRegisterCommandPlugin(ID_CDLINKOBJECTS,name,0,"CDLinkObjects.TIF","CD Link Objects",CDDataAllocator(CDLinkObjects));
}
