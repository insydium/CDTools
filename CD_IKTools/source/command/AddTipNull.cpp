//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

class CDAddTipNull : public CommandData
{
	public:
		  virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddTipNull::Execute(BaseDocument* doc)
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
				Vector tipPos;
				
				Real aLen;
				if(op->GetDown())
				{
					aLen = Len(op->GetDown()->GetMg().off - opM.off);
					tipPos = opM.off + VNorm(opM.v3) * aLen;
				}
				else
				{
					aLen = op->GetRad().z;
					tipPos = (opM.off + op->GetMp()) + VNorm(opM.v3) * aLen;
				}
				
				BaseObject* tpNull = NULL;
				tpNull = BaseObject::Alloc(Onull); if(!tpNull) return false;
				
				if(shft && opCnt == 1) doc->InsertObject(tpNull,op,NULL,true);
				else if(ctrl && opCnt == 1) doc->InsertObject(tpNull,op->GetUp(),op,true);
				else doc->InsertObject(tpNull,op->GetUp(),op->GetPred(),true);
				
				tpNull->SetName(opName + GeLoadString(N_TIP_NULL));
				Matrix nlM = opM;
				nlM.off = tipPos;
				tpNull->SetMg(nlM);
				
				BaseContainer *data = tpNull->GetDataInstance();
				data->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_HEXAGON);
				
				ObjectColorProperties prop;
				prop.color = Vector(0,1,1);
				prop.usecolor = 2;
				tpNull->SetColorProperties(&prop);
				
				if(optn && opCnt == 1)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					op->Remove();
					doc->InsertObject(op,tpNull,NULL,false);
					op->SetMg(opM);
				}
				doc->SetActiveObject(tpNull);
				CDAddUndo(doc,CD_UNDO_NEW,tpNull);
			}
		}
		doc->EndUndo();
	}
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDAddTipNullR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterAddTipNull(void)
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
	String name=GeLoadString(IDS_TIPNULL); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ADDTIPNULL,name,PLUGINFLAG_HIDE,"CDtipnull.tif","CD Add Tip Null",CDDataAllocator(CDAddTipNullR));
	else return CDRegisterCommandPlugin(ID_ADDTIPNULL,name,0,"CDtipnull.tif","CD Add Tip Null",CDDataAllocator(CDAddTipNull));
}
