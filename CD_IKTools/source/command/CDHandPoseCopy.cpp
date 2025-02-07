//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

enum
{	
	FINGER_COUNT				= 1020,
	
	TAG_LINK					= 3000,
	
	H_GRIP_VALUE				= 5000,
	H_TWIST_VALUE				= 5100,
	H_SPREAD_VALUE				= 5200,
	H_BEND_VALUE				= 5300,
	H_CURL_VALUE				= 5400,
	H_DAMPING_VALUE				= 5500,
	H_BEND_N_CURL				= 5600,

	POSE_GRIP					= 7000,
	POSE_TWIST					= 7100,
	POSE_SPREAD					= 7200,
	POSE_BEND					= 7300,
	POSE_CURL					= 7400,
	POSE_DAMPING				= 7500
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
	F_SPREAD_VALUE				= 10023
};

class CDCopyHandPose : public CommandData
{
public:
	bool CompareCDHandTags(BaseTag *src, BaseTag *dst, BaseDocument *doc);
	void SetPoseCopy(BaseTag *src, BaseTag *dst, BaseDocument *doc);
	
	virtual Bool Execute(BaseDocument* doc);

};

bool CDCopyHandPose::CompareCDHandTags(BaseTag *src, BaseTag *dst, BaseDocument *doc)
{
	BaseContainer *sData = src->GetDataInstance();
	BaseContainer *dData = dst->GetDataInstance();
	
	LONG i, fCnt = sData->GetLong(FINGER_COUNT);
	if(fCnt != dData->GetLong(FINGER_COUNT)) return false;

	for(i=0; i<fCnt; i++)
	{
		BaseList2D *srcLink = sData->GetLink(TAG_LINK+i,doc);
		BaseList2D *dstLink = dData->GetLink(TAG_LINK+i,doc);
		
		if(!srcLink || !dstLink) return false;
		if(srcLink->GetType() != dstLink->GetType()) return false;
	}
	
	return true;
}

void CDCopyHandPose::SetPoseCopy(BaseTag *src, BaseTag *dst, BaseDocument *doc)
{
	BaseContainer *sData = src->GetDataInstance();
	BaseContainer *dData = dst->GetDataInstance();
	
	LONG i, fCnt = sData->GetLong(FINGER_COUNT);

	CDAddUndo(doc,CD_UNDO_CHANGE,dst);
	for(i=0; i<fCnt; i++)
	{
		BaseList2D *srcLink = sData->GetLink(TAG_LINK+i,doc);

		BaseContainer *lnkData = srcLink->GetDataInstance();
		if(lnkData)
		{
			if(srcLink->GetType() == ID_CDTHUMBPLUGIN)
			{
				dData->SetReal(H_GRIP_VALUE+i,lnkData->GetReal(T_GRIP_VALUE));
				dData->SetReal(H_TWIST_VALUE+i,lnkData->GetReal(T_TWIST_VALUE));
				dData->SetReal(H_SPREAD_VALUE+i,lnkData->GetReal(T_SPREAD_VALUE));
				dData->SetReal(H_BEND_VALUE+i,lnkData->GetReal(T_BEND_VALUE));
				dData->SetReal(H_CURL_VALUE+i,lnkData->GetReal(T_CURL_VALUE));
			}
			else if(srcLink->GetType() == ID_CDFINGERPLUGIN)
			{
				dData->SetReal(H_SPREAD_VALUE+i,lnkData->GetReal(F_SPREAD_VALUE));
				dData->SetReal(H_BEND_VALUE+i,lnkData->GetReal(F_BEND_VALUE));
				dData->SetReal(H_CURL_VALUE+i,lnkData->GetReal(F_CURL_VALUE));
				dData->SetReal(H_DAMPING_VALUE+i,lnkData->GetReal(F_DAMPING_VALUE));
			}
		}
	}
}

Bool CDCopyHandPose::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelLog = GetSelectionLog(doc); if(!opSelLog) return false;
	
	LONG  oCnt = opSelLog->GetCount();
	if(oCnt > 1)
	{
		if(oCnt > 2)
		{
			MessageDialog(GeLoadString(MD_TOO_MANY_OBJECTS));
			doc->SetActiveObject(NULL);
			return true;
		}
		else
		{
			BaseObject *op = static_cast<BaseObject*>(opSelLog->GetIndex(0)); 
			if(!op) return false;
			BaseTag *opHTag = op->GetTag(ID_CDHANDPLUGIN); if(!opHTag) return false;
			
			BaseObject *trg = static_cast<BaseObject*>(opSelLog->GetIndex(1)); 
			if(!trg) return false;
			BaseTag *trgHTag = trg->GetTag(ID_CDHANDPLUGIN); if(!trgHTag) return false;
					
			if(!CompareCDHandTags(opHTag,trgHTag,doc))
			{
				MessageDialog(GeLoadString(MD_CDHAND_TAGS_NOT_EQUAL));
			}
			else
			{
				doc->SetActiveObject(NULL);
				doc->StartUndo();
				
				SetPoseCopy(opHTag,trgHTag,doc);
				
				doc->SetActiveObject(trg);
				doc->SetActiveTag(trgHTag);
				
				doc->EndUndo();
				
				EventAdd(EVENT_FORCEREDRAW);
			}
		}
	}

	return true;
}

class CDCopyHandPoseR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDCopyHandPose(void)
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
	String name=GeLoadString(IDS_CPYHPOSE); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCOPYHANDPOSE,name,PLUGINFLAG_HIDE,"CDHCpyPose.tif","CD Hand Pose Copy",CDDataAllocator(CDCopyHandPoseR));
	else return CDRegisterCommandPlugin(ID_CDCOPYHANDPOSE,name,0,"CDHCpyPose.tif","CD Hand Pose Copy",CDDataAllocator(CDCopyHandPose));
}
