/*
 *  CDAddRope.cpp
 *  CDCloth
 *
 *  Created by Dan Libisch on 8/7/13.
 *  Copyright 2013 Libisch Graphic Design. All rights reserved.
 *
 */

#include "c4d.h"
//#include "stdlib.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDCloth.h"
#include "CDSelLog.h"


class CDAddRope : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc);
	
};

Bool CDAddRope::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified

	AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	
	LONG selCount = opSelectionList->GetCount();
	if(selCount > 0)
	{
		StatusSetSpin();
		doc->StartUndo();
		
		LONG i;
		for(i=0; i<selCount; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
			if(op && IsValidPointObject(op))
			{
				if(op->IsInstanceOf(Ospline))
				{
					BaseTag *rpTag = BaseTag::Alloc(ID_CDROPETAG);
					
					BaseTag *sknTag = op->GetTag(ID_CDSKINPLUGIN);
					BaseTag *skinRef = op->GetTag(ID_CDSKINREFPLUGIN);
					BaseTag *mrphRef = op->GetTag(ID_CDMORPHREFPLUGIN);
					
					if(sknTag) op->InsertTag(rpTag,sknTag);
					else if(mrphRef) op->InsertTag(rpTag,mrphRef);
					else if(skinRef) op->InsertTag(rpTag,skinRef);
					else op->InsertTag(rpTag,NULL);
					
					CDAddUndo(doc,CD_UNDO_NEW,rpTag);
					
					rpTag->Message(MSG_MENUPREPARE);
					doc->SetActiveTag(rpTag);
				}
			}
		}
		StatusClear();
		doc->EndUndo();

		BaseContainer *bc = GetWorldPluginData(ID_SKINWEIGHTTOOL);
		if(bc) bc->SetLong(D_WP_MODE,D_WP_MODE_SET);
		doc->SetAction(ID_SKINWEIGHTTOOL);
		
		EventAdd(EVENT_FORCEREDRAW);
	}

	return true;
}


class CDAddRopeR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		//MessageDialog(GeLoadString(MD_PLEASE_REGISTER));
		return true;
	}
};

Bool RegisterCDAddRope(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDCL_SERIAL_SIZE];
	String cdclnr, kb;
	
	if(!CDReadPluginInfo(ID_CDCLOTH,data,CDCL_SERIAL_SIZE)) reg = false;
	
	cdclnr.SetCString(data,CDCL_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdclnr)) reg = false;
	
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdclnr.FindFirst("-",&pos);
	while(h)
	{
		cdclnr.Delete(pos,1);
		h = cdclnr.FindFirst("-",&pos);
	}
	cdclnr.ToUpper();
	kb = cdclnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDADDRP); if (!name.Content()) return true;
	
	if(!reg) return CDRegisterCommandPlugin(ID_CDADDROPECOMMAND,name,PLUGINFLAG_HIDE,"CDAddRope.tif","CD Add Rope",CDDataAllocator(CDAddRopeR));
	else return CDRegisterCommandPlugin(ID_CDADDROPECOMMAND,name,0,"CDAddRope.tif","CD Add Rope",CDDataAllocator(CDAddRope));
}
