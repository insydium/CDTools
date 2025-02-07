//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

// CD Hand containers
enum
{	
	ADD_FINGER					= 1000,

	TAG_LINK					= 3000,

	H_GRIP_VALUE				= 5000,
	H_TWIST_VALUE				= 5100,
	H_SPREAD_VALUE				= 5200,
	H_BEND_VALUE				= 5300,
	H_CURL_VALUE				= 5400,
	H_DAMPING_VALUE				= 5500,
	H_BEND_N_CURL				= 5600,

	POSE_SET					= 6200
};

// CD Thumb containers
enum
{
	T_GRIP_VALUE			= 10016,
	T_TWIST_VALUE			= 10017,
	T_SPREAD_VALUE			= 10018,
	T_BEND_VALUE			= 10019,
	T_CURL_VALUE			= 10020
};

// CD Finger containers
enum
{
	F_BEND_VALUE				= 10020,
	F_CURL_VALUE				= 10021,
	F_DAMPING_VALUE				= 10022,
	F_SPREAD_VALUE				= 10023,
	
	F_BEND_N_CURL				= 10033
};


class CDAddHandTag : public CommandData
{
	public:
		  virtual Bool Execute(BaseDocument* doc);

};

Bool CDAddHandTag::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op = doc->GetActiveObject(); if(!op) return false;
	
	AutoAlloc<AtomArray> tags; if(!tags) return false;
	doc->GetActiveTags(tags);
	LONG i, ftCnt, tCnt = tags->GetCount();
	if(tCnt == 0) return false;
	
	BaseTag *hTag = NULL, *fTag = NULL, *tTag = NULL;
	
	// Storage array for finger and thumb tags
	AutoAlloc<AtomArray> ftTags; if(!ftTags) return false;
	
	// Store CD Thumb tags first
	for(i=0; i<tCnt; i++)
	{
		tTag = static_cast<BaseTag*>(tags->GetIndex(i));
		if(tTag->IsInstanceOf(ID_CDTHUMBPLUGIN)) ftTags->Append(tTag);
	}
	// Store CD Finger tags
	for(i=0; i<tCnt; i++)
	{
		fTag = static_cast<BaseTag*>(tags->GetIndex(i));
		if(fTag->IsInstanceOf(ID_CDFINGERPLUGIN)) ftTags->Append(fTag);
	}
	ftCnt = ftTags->GetCount();
	
	doc->StartUndo();
	
	// Add the CD Hand Tag
	hTag = BaseTag::Alloc(ID_CDHANDPLUGIN);
	op->InsertTag(hTag,NULL);
	hTag->Message(MSG_MENUPREPARE);
	
	DescriptionCommand dc;
	BaseContainer *hData = hTag->GetDataInstance();
	
	for(i=0; i<ftCnt; i++)
	{
		BaseTag *tAdd = static_cast<BaseTag*>(ftTags->GetIndex(i));
		
		dc.id = DescID(ADD_FINGER);
		hTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
		
		hData->SetLink(TAG_LINK+i,tAdd);
		
		BaseContainer *tData = tAdd->GetDataInstance();
		if(tAdd->IsInstanceOf(ID_CDTHUMBPLUGIN))
		{
			hData->SetReal(H_GRIP_VALUE+i,tData->GetReal(T_GRIP_VALUE));
			hData->SetReal(H_TWIST_VALUE+i,tData->GetReal(T_TWIST_VALUE));
			hData->SetReal(H_SPREAD_VALUE+i,tData->GetReal(T_SPREAD_VALUE));
			hData->SetReal(H_BEND_VALUE+i,tData->GetReal(T_BEND_VALUE));
			hData->SetReal(H_CURL_VALUE+i,tData->GetReal(T_CURL_VALUE));
		}
		else if(tAdd->IsInstanceOf(ID_CDFINGERPLUGIN))
		{
			hData->SetReal(H_SPREAD_VALUE+i,tData->GetReal(F_SPREAD_VALUE));
			hData->SetReal(H_BEND_VALUE+i,tData->GetReal(F_BEND_VALUE));
			hData->SetBool(H_BEND_N_CURL+i,tData->GetBool(F_BEND_N_CURL));
			hData->SetReal(H_CURL_VALUE+i,tData->GetReal(F_CURL_VALUE));
			hData->SetReal(H_DAMPING_VALUE+i,tData->GetReal(F_DAMPING_VALUE));
		}
	}
	
	dc.id = DescID(POSE_SET);
	hTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	
	doc->SetActiveTag(hTag,SELECTION_NEW);

	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDAddHandTagR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDAddHandTag(void)
{
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
	String name=GeLoadString(IDS_ADDHAND); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDADDHANDTAG,name,PLUGINFLAG_HIDE,"CDAddHand.tif","CD Add Hand Tag",CDDataAllocator(CDAddHandTagR));
	else return CDRegisterCommandPlugin(ID_CDADDHANDTAG,name,0,"CDAddHand.tif","CD Add Hand Tag",CDDataAllocator(CDAddHandTag));
}
