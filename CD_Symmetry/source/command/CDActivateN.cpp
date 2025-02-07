//	Cactus Dan's Symmetry Tools 1.0
//	Copyright 2009 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"

#include "CDSymmetry.h"
#include "CDSymmetryTag.h"

#include "tCDSymTag.h"
#include "oCDDynSym.h"

class CDActivateNegative : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDActivateNegative::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG  i, opCount = objects->GetCount();
	
	if(opCount > 0)
	{
		doc->StartUndo();
		for(i=0; i<opCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i)); 
			if(op)
			{
				if(op->GetTag(ID_CDSYMMETRYTAG))
				{
					BaseTag *tag = op->GetTag(ID_CDSYMMETRYTAG);
					BaseContainer *tData = tag->GetDataInstance();
					if(tData)
					{
						if(tData->GetBool(SYM_RESTRICT_SYM))
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
							tData->SetLong(SYM_RESTRICT_AXIS,SYM_NEGATIVE);
						}
					}
				}
				else
				{
					BaseObject *dynSym = GetDynSymParentObject(op);
					if(dynSym)
					{
						BaseContainer *oData = dynSym->GetDataInstance();
						if(oData)
						{
							CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,dynSym);
							oData->SetLong(DS_ACTIVE_SYMMETRY,DS_NEGATIVE);
						}
					}
				}
			}
		}
		
		doc->EndUndo();
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}

class CDActivateNegativeR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDActivateNegative(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) reg = false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdsnr.FindFirst("-",&pos);
	while(h)
	{
		cdsnr.Delete(pos,1);
		h = cdsnr.FindFirst("-",&pos);
	}
	cdsnr.ToUpper();
	
	kb = cdsnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDNACTIVE); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDACTIVATENEGATIVE,name,PLUGINFLAG_HIDE,"CDActivateN.tif","CD Activate Negative",CDDataAllocator(CDActivateNegativeR));
	else return CDRegisterCommandPlugin(ID_CDACTIVATENEGATIVE,name,0,"CDActivateN.tif","CD Activate Negative",CDDataAllocator(CDActivateNegative));
}
