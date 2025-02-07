//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "CDMorph.h"
#include "tCDMorphTag.h"

class CDCreateMorphTagPlugin : public CommandData
{
	public:
		Bool CreateTagFromObject(BaseDocument* doc, BaseObject *op, BaseObject *mop, BaseTag *rTag);

		virtual Bool Execute(BaseDocument* doc);

};

Bool CDCreateMorphTagPlugin::CreateTagFromObject(BaseDocument* doc, BaseObject *op, BaseObject *mop, BaseTag *rTag)
{
	Vector		*tpadr = GetPointArray(mop);
	Vector		*padr = GetPointArray(op);
	
	String mopName = mop->GetName();
	
	BaseSelect *bs = ToPoint(op)->GetPointS();
	if (bs) bs->DeselectAll();

	LONG i, pcnt=LMin(ToPoint(mop)->GetPointCount(),ToPoint(op)->GetPointCount());
	for (i=0; i<pcnt; i++)
	{
		if(!VEqual(padr[i],tpadr[i],0.001)) bs->Select(i);
	}
	
	BaseTag *cdmTag = NULL;
	cdmTag = BaseTag::Alloc(ID_CDMORPHTAGPLUGIN);
	op->InsertTag(cdmTag,rTag);
	cdmTag->SetName(mopName);
	CDAddUndo(doc,CD_UNDO_NEW,cdmTag);
	
	CDAddUndo(doc,CD_UNDO_CHANGE,cdmTag);
	BaseContainer *tagData = cdmTag->GetDataInstance();
	tagData->SetLink(MT_DEST_LINK,op);
	cdmTag->Message(MSG_MENUPREPARE);
	
	DescriptionCommand dc;
	dc.id = DescID(MT_SET_SELECTION);
	cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	LONG bsPtCount = bs->GetCount();
	if (bsPtCount > 0)
	{
		LONG seg=0,a,b;
		while(CDGetRange(bs,seg++,&a,&b))
		{
			for (i=a; i<=b; ++i)
			{
				padr[i] = tpadr[i];
			}
		}
	}
	dc.id = DescID(MT_SET_OFFSET);
	cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	return true;
}

Bool CDCreateMorphTagPlugin::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *tagOp = NULL;
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0));
		if(!destObject) return false;
		if (!IsValidPointObject(destObject)) return false;
		
		LONG i;
		if(opCount > 1)
		{
			doc->SetActiveObject(NULL);
			doc->StartUndo();
			
			BaseTag *refTag = destObject->GetTag(ID_CDMORPHREFPLUGIN);
			if (!refTag)
			{
				refTag = BaseTag::Alloc(ID_CDMORPHREFPLUGIN);
				destObject->InsertTag(refTag,NULL);
				refTag->Message(MSG_MENUPREPARE);
				destObject->Message(MSG_UPDATE);
				CDAddUndo(doc,CD_UNDO_NEW,refTag);
			}

			for(i=1; i<opCount; i++)
			{
				StatusSetBar(LONG(Real(i)/Real(opCount)*100.0));
				tagOp = static_cast<BaseObject*>(opSelLog->GetIndex(i));
				if(IsValidPointObject(tagOp))
				{
					CreateTagFromObject(doc,destObject,tagOp,refTag);
				}
			}
			StatusClear();
			
			doc->SetActiveObject(destObject);
			
			doc->EndUndo();
			
			EventAdd(EVENT_FORCEREDRAW);
		}
	}

	return true;
}

class CDCreateMorphTagPluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDCreateMorphTagPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDM_SERIAL_SIZE];
	String cdmnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDMORPH,data,CDM_SERIAL_SIZE)) reg = false;
	
	cdmnr.SetCString(data,CDM_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdmnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdmnr.FindFirst("-",&pos);
	while(h)
	{
		cdmnr.Delete(pos,1);
		h = cdmnr.FindFirst("-",&pos);
	}
	cdmnr.ToUpper();
	kb = cdmnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDCREATEMTAG); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCREATEMTAGPLUGIN,name,PLUGINFLAG_HIDE,"CDObjToM.tif","CD Object to Morph Tag",CDDataAllocator(CDCreateMorphTagPluginR));
	else return CDRegisterCommandPlugin(ID_CDCREATEMTAGPLUGIN,name,0,"CDObjToM.tif","CD Object to Morph Tag",CDDataAllocator(CDCreateMorphTagPlugin));
}
