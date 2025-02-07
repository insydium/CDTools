//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

class CDAddRootNull : public CommandData
{
	public:
		  virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddRootNull::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	
	LONG  i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		Bool shft = false, ctrl = false, optn = false;
		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT) shft = true;
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL) ctrl = true;
		if(state.GetLong(BFM_INPUT_QUALIFIER) & QALT) optn = true;

		doc->StartUndo();
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				Matrix opM = op->GetMg();
				String opName = op->GetName();
				
				BaseObject* rtNull = NULL;
				rtNull = BaseObject::Alloc(Onull); if(!rtNull) return false;
				
				if(shft && opCnt == 1) doc->InsertObject(rtNull,op,NULL,true);
				else if(ctrl && opCnt == 1) doc->InsertObject(rtNull,op->GetUp(),op,true);
				else doc->InsertObject(rtNull,op->GetUp(),op->GetPred(),true);
				
				rtNull->SetName(opName + GeLoadString(N_ROOT_NULL));
				rtNull->SetMg(opM);
				
				BaseContainer *data = rtNull->GetDataInstance();
				data->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
				
				ObjectColorProperties prop;
				prop.color = Vector(0,1,1);
				prop.usecolor = 2;
				rtNull->SetColorProperties(&prop);
				
				if(optn && opCnt == 1)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					op->Remove();
					doc->InsertObject(op,rtNull,NULL,false);
					op->SetMg(opM);
					CDSetRot(op,Vector(0.0,0.0,0.0));
				}
				doc->SetActiveObject(rtNull);
				CDAddUndo(doc,CD_UNDO_NEW,rtNull);
			}
		}
		doc->EndUndo();
	}
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDAddRootNullR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterAddRootNull(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDIK_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDIKTOOLS,data,CDIK_SERIAL_SIZE)) reg = false;
	
	cdknr.SetCString(data,CDIK_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_ROOTNULL); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ADDROOTNULL,name,PLUGINFLAG_HIDE,"CDrootnull.tif","CD Add Root Null",CDDataAllocator(CDAddRootNullR));
	else return CDRegisterCommandPlugin(ID_ADDROOTNULL,name,0,"CDrootnull.tif","CD Add Root Null",CDDataAllocator(CDAddRootNull));
}
