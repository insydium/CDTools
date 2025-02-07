//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

class CDAddTipEffector : public CommandData
{
	public:
		  virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddTipEffector::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG  i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
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
					Real mpLen = Len(op->GetMp());
					tipPos = (opM.off + VNorm(opM.v3) * mpLen) + VNorm(opM.v3) * aLen;
				}
				
				Matrix tipM = Matrix(tipPos,opM.v1,opM.v2,opM.v3);
				
				BaseObject* tpOp = NULL;
				tpOp = BaseObject::Alloc(Onull); if(!tpOp) return false;
				tpOp->SetName(opName + GeLoadString(N_TIP_EFFECTOR));
				
				// Set the tip effector's object's parameters
				BaseContainer *tData = tpOp->GetDataInstance();
				tData->SetLong(NULLOBJECT_DISPLAY,NULLOBJECT_DISPLAY_POINT);
				tData->SetLong(NULLOBJECT_ORIENTATION,NULLOBJECT_ORIENTATION_XY);
				
				ObjectColorProperties tProp;
				tProp.color = Vector(0,1,1);
				tProp.usecolor = 2;
				tpOp->SetColorProperties(&tProp);
				
				doc->InsertObject(tpOp,op, NULL,true);
				tpOp->SetMg(tipM);

				CDAddUndo(doc,CD_UNDO_NEW,tpOp);
			}
		}
		doc->EndUndo();
	}
	
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDAddTipEffectorR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterAddTipEffector(void)
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
	String name=GeLoadString(IDS_TIPEFFECTOR); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ADDTIPEFFECTOR,name,PLUGINFLAG_HIDE,"CDteffector.tif","CD Add Tip Effector",CDDataAllocator(CDAddTipEffectorR));
	else return CDRegisterCommandPlugin(ID_ADDTIPEFFECTOR,name,0,"CDteffector.tif","CD Add Tip Effector",CDDataAllocator(CDAddTipEffector));
}
