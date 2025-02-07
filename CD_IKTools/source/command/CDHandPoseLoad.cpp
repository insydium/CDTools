//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDCompatibility.h"
#include "CDIKtools.h"

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

class CDLoadHandPose : public CommandData
{
private:
	Bool LoadHandPose(BaseDocument *doc, Filename &fName, BaseTag *tag);
	
public:
	virtual Bool Execute(BaseDocument* doc);
	
};

Bool CDLoadHandPose::LoadHandPose(BaseDocument *doc, Filename &fName, BaseTag *tag)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	Filename poseName = fName.GetFile();
	poseName.ClearSuffix();
	
	LONG i, fCnt = tData->GetLong(H_FINGER_COUNT), p = tData->GetLong(H_POSE_COUNT);

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

	AutoAlloc<BaseFile> pFile;
	pFile->Open(fName);
	
	CDLong f;
	CDBFReadLong(pFile, &f);
	if(f != fCnt)
	{
		MessageDialog(GeLoadString(MD_FILE_DOES_NOT_MATCH_CDHTAG));
		return false;
	}
	
	for(i=0; i<f; i++)
	{
		CDLong type;
		CDBFReadLong(pFile, &type);
		if(type != ftype[i])
		{
			MessageDialog(GeLoadString(MD_FILE_DOES_NOT_MATCH_CDHTAG));
			return false;
		}
	}
	
	CDDouble val;
	for(i=0; i<fCnt; i++)
	{
		if(ftype[i] == ID_CDTHUMBPLUGIN)
		{
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_GRIP+i+(p*1000), val);
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_TWIST+i+(p*1000), val);
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_SPREAD+i+(p*1000), val);
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_BEND+i+(p*1000), val);
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_CURL+i+(p*1000), val);
		}
		else if(ftype[i] == ID_CDFINGERPLUGIN)
		{
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_SPREAD+i+(p*1000), val);
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_BEND+i+(p*1000), val);
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_CURL+i+(p*1000), val);
			CDBFReadDouble(pFile, &val);
			tData->SetReal(H_POSE_DAMPING+i+(p*1000), val);
		}
	}
	tData->SetString(HND_POSE_NAME+p, poseName.GetString());
	tData->SetBool(H_POSE_IS_SET+p, true);
	p++;
	tData->SetLong(H_POSE_COUNT, p);
	
	ftype.Free();
	
	return true;
}

Bool CDLoadHandPose::Execute(BaseDocument* doc)
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

	Filename fName;
#if API_VERSION < 12000
	String title = GeLoadString(IDS_SAVEHPOSE);
	if(!fName.FileSelect(FSTYPE_ANYTHING,0,&title)) return false;
#else
	if(!fName.FileSelect(FILESELECTTYPE_ANYTHING,FILESELECT_LOAD,GeLoadString(IDS_LOADHPOSE))) return false;
#endif
	
	Bool ret = LoadHandPose(doc, fName, tag);
	
	EventAdd(EVENT_FORCEREDRAW);
	return ret;
}

class CDLoadHandPoseR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		return true;
	}
};

Bool RegisterCDLoadHandPose(void)
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
	String name=GeLoadString(IDS_LOADHPOSE); if(!name.Content()) return true;
	
	if(!reg) return CDRegisterCommandPlugin(ID_CDLOADHANDPOSE,name,PLUGINFLAG_HIDE,"CDHLdPose.tif","CD Hand Pose Load",CDDataAllocator(CDLoadHandPoseR));
	else return CDRegisterCommandPlugin(ID_CDLOADHANDPOSE,name,0,"CDHLdPose.tif","CD Hand Pose Load",CDDataAllocator(CDLoadHandPose));
}


