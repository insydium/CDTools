//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "CDCompatibility.h"
#include "CDMorph.h"

class CDMorphMixToObjectPlugin : public CommandData
{
	public:
		virtual Bool Execute(BaseDocument* doc);

};

Bool CDMorphMixToObjectPlugin::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *dest =  doc->GetActiveObject(); if(!dest) return false;
	if(!IsValidPointObject(dest)) return true;

	// clone the active object
	BaseObject *mOp = (BaseObject*)CDGetClone(dest,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_ANIMATION|CD_COPYFLAGS_NO_BITS,NULL);
	if(mOp)
	{
		doc->StartUndo();	

		String opName = dest->GetName();
		
		mOp->SetName(opName + "Mix Object");
		doc->InsertObject(mOp, NULL, NULL,true);
		mOp->SetMg(dest->GetMg());
		CDAddUndo(doc,CD_UNDO_NEW,mOp);

		// Find and delete all morph and skin tags on clone
		CDAddUndo(doc,CD_UNDO_CHANGE,mOp);
		BaseTag *rTagCopy = mOp->GetTag(ID_CDMORPHREFPLUGIN), *mTagCopy=NULL;
		if (rTagCopy) BaseTag::Free(rTagCopy);
		
		BaseTag *skTagCopy = mOp->GetTag(ID_CDSKINPLUGIN);
		if (skTagCopy) BaseTag::Free(skTagCopy);
		
		BaseTag *skrTagCopy = mOp->GetTag(ID_CDSKINREFPLUGIN);
		if (skrTagCopy) BaseTag::Free(skrTagCopy);
		
		while(mOp->GetTag(ID_CDMORPHTAGPLUGIN))
		{
			mTagCopy = mOp->GetTag(ID_CDMORPHTAGPLUGIN);
			if (mTagCopy) BaseTag::Free(mTagCopy);
		}
		
		mOp->Message(MSG_UPDATE);
		doc->SetActiveObject(mOp);

		EventAdd(EVENT_FORCEREDRAW);
		
		doc->EndUndo();	
	}

	return true;
}

class CDMorphMixToObjectPluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMorphMixToObjectPlugin(void)
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
	String name=GeLoadString(IDS_CDMMIXOBJ); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMMIXTOOBJPLUGIN,name,PLUGINFLAG_HIDE,"CDMixToObj.tif","CD Morph Mix to Object",CDDataAllocator(CDMorphMixToObjectPluginR));
	else return CDRegisterCommandPlugin(ID_CDMMIXTOOBJPLUGIN,name,0,"CDMixToObj.tif","CD Morph Mix to Object",CDDataAllocator(CDMorphMixToObjectPlugin));
}
