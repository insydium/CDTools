//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "CDCompatibility.h"
#include "CDMorph.h"
#include "tCDMorphTag.h"

class CDMorphMixToTagPlugin : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDMorphMixToTagPlugin::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  opCount = opSelLog->GetCount();
	if(opCount > 0)
	{
		BaseObject *targObject = NULL;
		BaseObject *destObject = static_cast<BaseObject*>(opSelLog->GetIndex(0));
		if(!destObject) return false;
		if(!IsValidPointObject(destObject)) return false;
		
		if(opCount > 1)
		{
			targObject = static_cast<BaseObject*>(opSelLog->GetIndex(1));
		}
		else targObject = destObject;
		
		BaseTag *refTag = destObject->GetTag(ID_CDMORPHREFPLUGIN); if (!refTag) return true;
		CDMRefPoint	*refadr = CDGetMorphReference(refTag); if(!refadr) return true;

		String opName = destObject->GetName();

		BaseSelect *bs = ToPoint(destObject)->GetPointS();
		if (bs) bs->DeselectAll();

		Vector *padr = GetPointArray(destObject);
		LONG i, pcnt = ToPoint(destObject)->GetPointCount();
		for (i=0; i<pcnt; i++)
		{
			if(!VEqual(padr[i],refadr[i].GetVector(),0.001)) bs->Select(i);
		}
		
		doc->StartUndo();
		
		BaseTag *prev = NULL;
		if(targObject == destObject) prev = refTag;

		BaseTag *cdmTag = NULL;
		cdmTag = BaseTag::Alloc(ID_CDMORPHTAGPLUGIN);
		targObject->InsertTag(cdmTag,prev);
		cdmTag->Message(MSG_MENUPREPARE);
		cdmTag->SetName(opName+".Mix");
		CDAddUndo(doc,CD_UNDO_NEW,cdmTag);

		CDAddUndo(doc,CD_UNDO_CHANGE,cdmTag);
		BaseContainer *tagData = cdmTag->GetDataInstance();
		tagData->SetLink(MT_DEST_LINK,destObject);
		cdmTag->Message(MSG_MENUPREPARE);
		
		DescriptionCommand dc;
		dc.id = DescID(MT_SET_SELECTION);
		cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
		
		dc.id = DescID(MT_SET_OFFSET);
		cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);

		EventAdd(EVENT_FORCEREDRAW);
		
		doc->EndUndo();	
	}

	return true;
}

class CDMorphMixToTagPluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMorphMixToTagPlugin(void)
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
	String name=GeLoadString(IDS_CDMMIXTAG); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMMIXTOTAGPLUGIN,name,PLUGINFLAG_HIDE,"CDMixToTag.tif","CD Morph Mix to Tag",CDDataAllocator(CDMorphMixToTagPluginR));
	else return CDRegisterCommandPlugin(ID_CDMMIXTOTAGPLUGIN,name,0,"CDMixToTag.tif","CD Morph Mix to Tag",CDDataAllocator(CDMorphMixToTagPlugin));
}
