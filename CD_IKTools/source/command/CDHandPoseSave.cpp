//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDIKtools.h"
#include "CDCompatibility.h"
#include "CDArray.h"

#include "tCDHand.h"
#include "tCDFinger.h"
#include "tCDThumb.h"

// CD Hand containers
enum
{	
	H_FINGER_COUNT				= 1020,
	
	H_TAG_LINK					= 3000,
	
	H_GRIP_VALUE				= 5000,
	H_TWIST_VALUE				= 5100,
	H_SPREAD_VALUE				= 5200,
	H_BEND_VALUE				= 5300,
	H_CURL_VALUE				= 5400,
	H_DAMPING_VALUE				= 5500,
	H_BEND_N_CURL				= 5600,
	
	H_POSE_COUNT				= 6004,

	H_POSE_IS_SET				= 6400,

	H_POSE_GRIP					= 7000,
	H_POSE_TWIST				= 7100,
	H_POSE_SPREAD				= 7200,
	H_POSE_BEND					= 7300,
	H_POSE_CURL					= 7400,
	H_POSE_DAMPING				= 7500
};

class CDSaveHandPose : public CommandData
{
private:
	Bool SaveHandPoses(BaseDocument *doc, Filename &dir, BaseTag *tag);
	
public:
	virtual Bool Execute(BaseDocument* doc);
	
};

Bool CDSaveHandPose::SaveHandPoses(BaseDocument *doc, Filename &dir, BaseTag *tag)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG i, p, fCnt = tData->GetLong(H_FINGER_COUNT), pCnt = tData->GetLong(H_POSE_COUNT);
	
	CDArray<CDLong> ftype;
	if(!ftype.Alloc(fCnt)) return false;
	
	for(i=0; i<fCnt; i++)
	{
		BaseList2D *linkTag = tData->GetLink(H_TAG_LINK+i, doc);
		if(linkTag)
		{
			ftype[i] = linkTag->GetType();
		}
	}
	
	for(p=1; p<pCnt; p++)
	{
		String poseName = tData->GetString(HND_POSE_NAME+p);
		Filename fName = Filename(poseName);
		fName.SetSuffix("hpf");
		fName.SetDirectory(dir);
		
		AutoAlloc<BaseFile> pFile;
		if(pFile->Open(fName))
		{
			if(!QuestionDialog(poseName+GeLoadString(MD_OVERWRITE_FILE))) continue;
		}
		
	#if API_VERSION < 12000
		pFile->Open(fName, GE_WRITE);
	#else
		pFile->Open(fName, FILEOPEN_WRITE);
	#endif
		
		CDBFWriteLong(pFile, fCnt);
		for(i=0; i<fCnt; i++)
		{
			CDBFWriteLong(pFile,ftype[i]);
		}
		
		for(i=0; i<fCnt; i++)
		{
			if(ftype[i] == ID_CDTHUMBPLUGIN)
			{
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_GRIP+i+(p*1000)));
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_TWIST+i+(p*1000)));
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_SPREAD+i+(p*1000)));
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_BEND+i+(p*1000)));
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_CURL+i+(p*1000)));
			}
			else if(ftype[i] == ID_CDFINGERPLUGIN)
			{
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_SPREAD+i+(p*1000)));
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_BEND+i+(p*1000)));
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_CURL+i+(p*1000)));
				CDBFWriteDouble(pFile, tData->GetReal(H_POSE_DAMPING+i+(p*1000)));
			}
		}
	}
	ftype.Free();
	
	return true;
}

Bool CDSaveHandPose::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseTag *tag = doc->GetActiveTag();
	if(!tag || tag->GetType() != ID_CDHANDPLUGIN)
	{
		BaseObject *op = doc->GetActiveObject();
		if(op)
		{
			tag = op->GetTag(ID_CDHANDPLUGIN);
			if(!tag)
			{
				MessageDialog(GeLoadString(MD_SELECT_CDHAND_TAG));
				doc->SetActiveObject(NULL);
				return true;
			}
		}
		else
		{
			MessageDialog(GeLoadString(MD_SELECT_CDHAND_TAG));
			doc->SetActiveObject(NULL);
			return true;
		}
	}

	Filename dir;
#if API_VERSION < 12000
	String title = GeLoadString(IDS_SAVEHPOSE);
	if(!dir.FileSelect(FSTYPE_ANYTHING,GE_DIRECTORY,&title)) return false;
#else
	if(!dir.FileSelect(FILESELECTTYPE_ANYTHING,FILESELECT_DIRECTORY,GeLoadString(IDS_SAVEHPOSE))) return false;
#endif
	
	Bool ret = SaveHandPoses(doc, dir, tag);
	
	EventAdd(EVENT_FORCEREDRAW);
	return ret;
}

class CDSaveHandPoseR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		return true;
	}
};

Bool RegisterCDSaveHandPose(void)
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
	String name=GeLoadString(IDS_SAVEHPOSE); if(!name.Content()) return true;
	
	if(!reg) return CDRegisterCommandPlugin(ID_CDSAVEHANDPOSE,name,PLUGINFLAG_HIDE,"CDHSvPose.tif","CD Hand Pose Save",CDDataAllocator(CDSaveHandPoseR));
	else return CDRegisterCommandPlugin(ID_CDSAVEHANDPOSE,name,0,"CDHSvPose.tif","CD Hand Pose Save",CDDataAllocator(CDSaveHandPose));
}


