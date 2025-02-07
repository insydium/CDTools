//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "CDCompatibility.h"
#include "CDMorph.h"
#include "CDMorphTag.h"

#include "tCDMorphTag.h"

class CDMorphTagToObjectPlugin : public CommandData
{
	private:
		Bool CreateObjectFromTag(BaseDocument* doc, BaseTag *tag);
		
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDMorphTagToObjectPlugin::CreateObjectFromTag(BaseDocument* doc, BaseTag *tag)
{
	BaseContainer	*mTData = tag->GetDataInstance();
	BaseObject *dest =  mTData->GetObjectLink(MT_DEST_LINK,doc); if(!dest) return false;
	if(!IsValidPointObject(dest)) return true;

	BaseTag *refTag = dest->GetTag(ID_CDMORPHREFPLUGIN); if (!refTag) return true;
	BaseContainer	*rTData = refTag->GetDataInstance();
	
	// clone the active object 
	BaseObject *mOp = (BaseObject*)CDGetClone(dest,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_ANIMATION|CD_COPYFLAGS_NO_BITS,NULL);
	if(mOp)
	{
		String opName = tag->GetName();
		
		doc->InsertObject(mOp, NULL, NULL,false);
		mOp->SetMg(dest->GetMg());
		mOp->SetName(opName);
		CDAddUndo(doc,CD_UNDO_NEW,mOp);

		CDMRefPoint  *refadr = CDGetMorphReference(refTag); if(!refadr) return true;
		
		Vector *padr = GetPointArray(mOp); 
		
		LONG i, rCnt = rTData->GetLong(MT_POINT_COUNT), pCnt = ToPoint(dest)->GetPointCount();
		if(rCnt != pCnt) return true;

		CDMPoint *mpadr = CDGetMorphPoints(tag); if(!mpadr) return false;
		
		// position all points on clone to reference
		CDAddUndo(doc,CD_UNDO_CHANGE,mOp);
		for(i=0; i>rCnt; i++)
		{
			padr[i] = refadr[i].GetVector();
		}
		
		// Find and delete all morph and skin tags on clone
		BaseTag *rTagCopy = mOp->GetTag(ID_CDMORPHREFPLUGIN), *mTagCopy=NULL, *sTagCopy=NULL, *srTagCopy=NULL;
		if (rTagCopy) BaseTag::Free(rTagCopy);
		while(mOp->GetTag(ID_CDMORPHTAGPLUGIN))
		{
			mTagCopy = mOp->GetTag(ID_CDMORPHTAGPLUGIN);
			if (mTagCopy) BaseTag::Free(mTagCopy);
		}
		while(mOp->GetTag(ID_CDSKINPLUGIN))
		{
			sTagCopy = mOp->GetTag(ID_CDSKINPLUGIN);
			if (sTagCopy) BaseTag::Free(sTagCopy);
		}
		while(mOp->GetTag(ID_CDSKINREFPLUGIN))
		{
			srTagCopy = mOp->GetTag(ID_CDSKINREFPLUGIN);
			if (srTagCopy) BaseTag::Free(srTagCopy);
		}
		
		// set the cloned object's points to the morph shape
		LONG j, ptIndex, mPtCount = mTData->GetLong(MT_BS_POINT_COUNT);
		for(j=0; j<mPtCount; j++)
		{
			ptIndex = mpadr[j].ind;
			if(ptIndex < pCnt)
			{
				padr[ptIndex] = mpadr[j].GetDelta(refadr[ptIndex].GetVector());
			}
		}
		mOp->Message(MSG_UPDATE);
		doc->SetActiveObject(mOp);
		
		return true;
	}
	else return false;
}

Bool CDMorphTagToObjectPlugin::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> tags; if (!tags) return false;
	doc->GetActiveTags(tags);
	LONG ti, tagCount = tags->GetCount();
	BaseTag *tagActive = static_cast<BaseTag*>(tags->GetIndex(0));
	if(!tagActive)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return true;
	}
	Bool cdTag = false;
	for(ti=0; ti<tagCount; ti++)
	{
		tagActive = static_cast<BaseTag*>(tags->GetIndex(ti));
		if(tagActive->GetType() == ID_CDMORPHTAGPLUGIN)
		{
			cdTag = true;
		}
	}
	if(!cdTag)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return true;
	}
	
	doc->StartUndo();
	
	for(ti=0; ti<tagCount; ti++)
	{
		StatusSetBar(LONG(Real(ti)/Real(tagCount)*100.0));
		tagActive = static_cast<BaseTag*>(tags->GetIndex(ti));
		if(tagActive->GetType() == ID_CDMORPHTAGPLUGIN)
		{
			CreateObjectFromTag(doc,tagActive);
		}
	}
	StatusClear();
	
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
	
}

class CDMorphTagToObjectPluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMorphTagToObjectPlugin(void)
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
	String name=GeLoadString(IDS_CDCREATEMOBJ); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCREATEMOBJPLUGIN,name,PLUGINFLAG_HIDE,"CDMToObj.tif","CD Morph Tag to Object",CDDataAllocator(CDMorphTagToObjectPluginR));
	else return CDRegisterCommandPlugin(ID_CDCREATEMOBJPLUGIN,name,0,"CDMToObj.tif","CD Morph Tag to Object",CDDataAllocator(CDMorphTagToObjectPlugin));
}
