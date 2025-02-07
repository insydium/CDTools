//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "CDTransfer.h"

class ZeroGlobalRotation : public CommandData
{
	public:
		  virtual Bool Execute(BaseDocument* doc);

};

Bool ZeroGlobalRotation::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	if(objects->GetCount() == 0) return false;

	Matrix transM = Matrix();
	Vector theScale;
	LONG i, opCount = objects->GetCount();
	
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	Bool shift = (state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT);

	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op)
		{
			Matrix opM = op->GetMg();
			transM.off = opM.off;
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			
			if(shift) RepositionChildren(doc,op->GetDown(),transM,opM,true);
			
			theScale = CDGetScale(op);		
			op->SetMg(transM);
			CDSetScale(op,theScale);	
		}
	}
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class ZeroGlobalRotationR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterZeroGlobalRotation(void)
{
	if(CDFindPlugin(ID_ABOUTCDIKTOOLS,CD_COMMAND_PLUGIN) && CDFindPlugin(ID_ZEROGLOBALROTATION,CD_COMMAND_PLUGIN)) return true;
	else
	{
		Bool reg = true;
		
		LONG pK;
		CHAR aK, bK, cK;
		SetRValues(&pK,&aK,&bK,&cK);
		
		CHAR b, data[CDTR_SERIAL_SIZE];
		String cdtnr, kb;
		SerialInfo si;
		
		if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = false;
		
		cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
		if(!CheckKeyChecksum(cdtnr)) reg = false;
		
		CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
		LONG seed = GetSeed(si.nr);
		
		LONG pos;
		Bool h = cdtnr.FindFirst("-",&pos);
		while(h)
		{
			cdtnr.Delete(pos,1);
			h = cdtnr.FindFirst("-",&pos);
		}
		cdtnr.ToUpper();
		
		kb = cdtnr.SubStr(pK,2);
		
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
		String name=GeLoadString(IDS_ZEROGROT); if(!name.Content()) return true;

		if(!reg) return CDRegisterCommandPlugin(ID_ZEROGLOBALROTATION,name,PLUGINFLAG_HIDE,"CDZeroGRot.tif","CD Zero Global Rotation",CDDataAllocator(ZeroGlobalRotationR));
		else return CDRegisterCommandPlugin(ID_ZEROGLOBALROTATION,name,0,"CDZeroGRot.tif","CD Zero Global Rotation",CDDataAllocator(ZeroGlobalRotation));
	}
}
