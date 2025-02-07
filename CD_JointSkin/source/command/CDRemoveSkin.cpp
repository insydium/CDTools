//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinRefTag.h"
#include "tCDCluster.h"

class CDRemoveSkin : public CommandData
{
	public:		
		virtual Bool Execute(BaseDocument* doc);
		
		void RemoveCDSkinClusters(BaseDocument* doc, BaseObject *op);
		
};

void CDRemoveSkin::RemoveCDSkinClusters(BaseDocument* doc, BaseObject *op)
{
	CDJSData scd;
	PluginMessage(ID_CDCLUSTERTAGPLUGIN,&scd);
	if(scd.list)
	{
		LONG i, scCnt = scd.list->GetCount();
		BaseContainer *scData = NULL;
		BaseTag *scTag = NULL;
		for(i=scCnt; i>0; i--)
		{
			scTag = static_cast<BaseTag*>(scd.list->GetIndex(i-1));
			if(scTag->GetDocument() == doc)
			{
				scData = scTag->GetDataInstance();
				if(scData)
				{
					if(op == scData->GetObjectLink(CLST_DEST_LINK,doc))
					{
						CDAddUndo(doc,CD_UNDO_DELETE,scTag);
						BaseTag::Free(scTag);
					}
				}
			}
		}
	}
}

Bool CDRemoveSkin::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	LONG i, opCount = objects->GetCount();
	
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && op->GetTag(ID_CDSKINREFPLUGIN))
		{
			BaseTag *skin = op->GetTag(ID_CDSKINPLUGIN);
			if(skin)
			{
				CDAddUndo(doc,CD_UNDO_DELETE,skin);
				BaseTag::Free(skin);
			}
			RemoveCDSkinClusters(doc,op);
			
			BaseTag *skinRef = op->GetTag(ID_CDSKINREFPLUGIN);
			if(skinRef && !op->GetTag(ID_CDCLOTHTAG))
			{
				DescriptionCommand skrdc;
				skrdc.id = DescID(SKR_RESTORE_REFERENCE);
				skinRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);

				CDAddUndo(doc,CD_UNDO_DELETE,skinRef);
				BaseTag::Free(skinRef);
			}
		}
	}
	
	doc->SetActiveObject(op);
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDRemoveSkinR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDRemoveSkin(void)
{
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
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDUNSKIN); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDREMOVESKIN,name,PLUGINFLAG_HIDE,"CDDeleteSkin.TIF","CD Remove Skin",CDDataAllocator(CDRemoveSkinR));
	else return CDRegisterCommandPlugin(ID_CDREMOVESKIN,name,0,"CDDeleteSkin.TIF","CD Remove Skin",CDDataAllocator(CDRemoveSkin));
}
