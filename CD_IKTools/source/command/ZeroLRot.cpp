//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

enum // from CD Zero Tranformaion tag
{
	Z_TRANS_ROT			= 1003,
	
	Z_LOCAL_MATRIX		= 2001
};

enum // from CD Joint
{
	J_TRANS_ROT			= 1009,
	
	J_LOCAL_MATRIX		= 2001
};

class ZeroLocalRotation : public CommandData
{
	public:
		  virtual Bool Execute(BaseDocument* doc);

};

Bool ZeroLocalRotation::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	if(objects->GetCount() == 0) return false;

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
			Matrix opM = op->GetMg(), prM = Matrix(), transM;
			BaseObject *pr = op->GetUp();
			if(pr) prM = pr->GetMg();
			
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			if(op->GetType() == ID_CDJOINTOBJECT)
			{
				BaseContainer *opData = op->GetDataInstance();
				if(opData)
				{
					if(shift)
					{
						transM = prM * opData->GetMatrix(J_LOCAL_MATRIX);
						RepositionChildren(doc,op->GetDown(),transM,opM,true);
					}
					opData->SetVector(J_TRANS_ROT,Vector(0,0,0));
				}
			}
			else
			{
				if(op->GetTag(ID_CDZEROTRANORMATION))
				{
					BaseContainer *tData = op->GetTag(ID_CDZEROTRANORMATION)->GetDataInstance();
					if(tData)
					{
						if(shift)
						{
							transM = prM * tData->GetMatrix(Z_LOCAL_MATRIX);
							RepositionChildren(doc,op->GetDown(),transM,opM,true);
						}
						tData->SetVector(Z_TRANS_ROT,Vector(0,0,0));
					}
				}
				else
				{
					if(shift)
					{
						transM = prM;
						transM.off = opM.off;
						RepositionChildren(doc,op->GetDown(),transM,opM,true);
					}
					CDSetRot(op,Vector(0,0,0));
				}
			}
		}
	}
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class ZeroLocalRotationR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterZeroLocalRotation(void)
{
	if(CDFindPlugin(ID_ZEROLOCALROTATION,CD_COMMAND_PLUGIN)) return true;

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
	String name=GeLoadString(IDS_ZEROLROT); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_ZEROLOCALROTATION,name,PLUGINFLAG_HIDE,"CDZeroLRot.tif","CD Zero Local Rotation",CDDataAllocator(ZeroLocalRotationR));
	else return CDRegisterCommandPlugin(ID_ZEROLOCALROTATION,name,0,"CDZeroLRot.tif","CD Zero Local Rotation",CDDataAllocator(ZeroLocalRotation));
}
