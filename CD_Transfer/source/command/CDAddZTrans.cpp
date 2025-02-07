//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDTransfer.h"

#include "tCDZeroTrans.h"

class CDAddZeroTransformation : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddZeroTransformation::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	BaseTag *tag = NULL;
	
	AutoAlloc<AtomArray> opActiveList; if(!opActiveList) return false;
	CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	LONG i, activeCnt = opActiveList->GetCount();
	if(activeCnt > 0)
	{
		Bool addZTrans = false;
		for(i=0; i<activeCnt; i++)
		{
			op = static_cast<BaseObject*>(opActiveList->GetIndex(i));
			if(op)
			{
				if(!op->GetTag(ID_CDZEROTRANSTAG)) addZTrans = true;
			}
		}
		if(addZTrans)
		{
			doc->StartUndo();
			for(i=0; i<activeCnt; i++)
			{
				op = static_cast<BaseObject*>(opActiveList->GetIndex(i));
				if(op)
				{
					if(!op->GetTag(ID_CDZEROTRANSTAG))
					{
						tag = BaseTag::Alloc(ID_CDZEROTRANSTAG);
						op->InsertTag(tag,NULL);
						tag->Message(MSG_MENUPREPARE);
						CDAddUndo(doc,CD_UNDO_NEW,tag);
						
						tag->Message(MSG_MENUPREPARE);
						
						DescriptionCommand dc;
						dc.id = DescID(ZT_ZERO_COORDS);
						tag->Message(MSG_DESCRIPTION_COMMAND,&dc);

					}
				}
			}
			doc->SetActiveObject(op);
			doc->SetActiveTag(tag);
			
			doc->EndUndo();
			EventAdd(EVENT_FORCEREDRAW);
		}
	}
	
	return true;
}

class CDAddZeroTransformationR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddZeroTransformation(void)
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
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDADDZT); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDZEROTRANS,name,PLUGINFLAG_HIDE,"CDAddZTrans.tif","CD Add Zero Transformation",CDDataAllocator(CDAddZeroTransformationR));
	else return CDRegisterCommandPlugin(ID_CDADDZEROTRANS,name,0,"CDAddZTrans.tif","CD Add Zero Transformation",CDDataAllocator(CDAddZeroTransformation));
}
