//	Cactus Dan's IK Tools plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
//#include "customgui_InExclude.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "CDIKtools.h"

#include "tCDHand.h"
#include "tCDFinger.h"
#include "tCDThumb.h"

#define MAX_FINGER		20
#define MAX_POSE		50

enum
{	
	//HND_ADD_FINGER				= 1000,
	//HND_SUB_FINGER				= 1001,
	
	HND_FINGER_COUNT				= 1020,
	
	//HND_FINGER_GROUP				= 1021,
	
	//HND_PURCHASE					= 1030,
	//HND_LEFT_HAND					= 1031,

	
	HND_NEW							= 1050,
	//HND_USE_MIXER					= 1051,

	//HND_CONTROLLER_GROUP			= 1500,	
	
	HND_SUBGROUP					= 2000,
	HND_POSE_BUTTON_GROUP			= 2100,
	HND_POSE_MIXER_GROUP			= 2200,
	HND_LINE_ADD					= 2300,

	HND_TAG_LINK					= 3000,
	
	HND_GRIP_MIX					= 4000,
	HND_TWIST_MIX					= 4100,
	HND_SPREAD_MIX					= 4200,
	HND_BEND_MIX					= 4300,
	HND_CURL_MIX					= 4400,
	HND_DAMPING_MIX					= 4500,
	
	HND_GRIP_VALUE					= 5000,
	HND_TWIST_VALUE					= 5100,
	HND_SPREAD_VALUE				= 5200,
	HND_BEND_VALUE					= 5300,
	HND_CURL_VALUE					= 5400,
	HND_DAMPING_VALUE				= 5500,
	HND_BEND_N_CURL					= 5600,
	
	//HND_POSE_ADD					= 6000,
	//HND_POSE_SUB					= 6001,
	//HND_POSE_GROUP				= 6002,
	//HND_POSE_SUBGROUP				= 6003,
	HND_POSE_COUNT					= 6004,

	//HND_POSE_NAME					= 6050,
	HND_POSE_MIX					= 6100,
	//HND_POSE_SET					= 6200,
	//HND_POSE_EDIT					= 6300,
	HND_POSE_IS_SET					= 6400,
	//HND_POSE_RESTORE				= 6500,
	
	HND_POSE_GRIP					= 7000,
	HND_POSE_TWIST					= 7100,
	HND_POSE_SPREAD					= 7200,
	HND_POSE_BEND					= 7300,
	HND_POSE_CURL					= 7400,
	HND_POSE_DAMPING				= 7500
};

class CDHandPlugin : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

	Bool SavePose(BaseTag *tag, BaseDocument *doc, LONG count, LONG pose);
	Bool RestorePose(BaseTag *tag, BaseDocument *doc, LONG count, LONG pose);
	Bool RestoreMix(BaseTag *tag, BaseDocument *doc, LONG count);
	Bool SaveSliders(BaseTag *tag, BaseDocument *doc, LONG count);
	Bool RestoreSliders(BaseTag *tag, BaseDocument *doc, LONG count);
	void SetPoseCopy(BaseDocument *doc, BaseContainer *tData, CDPoseCopyData *pcd);

public:
	Bool initSliders[MAX_FINGER], setPose;
	
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDHandPlugin); }
};

Bool CDHandPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(HND_POSE_IS_SET,false);
	tData->SetReal(HND_USE_MIXER,0.0);
	tData->SetBool(HND_NEW,false);
	tData->SetLong(HND_POSE_COUNT,1);
	setPose = false;

	for (LONG i=0; i<MAX_FINGER; i++)
	{
		initSliders[i] = false;
	}
		
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDHandPlugin::RestorePose(BaseTag *tag, BaseDocument *doc, LONG count, LONG pose)
{
	LONG i;
	for (i=0; i<count; i++)
	{
		BaseContainer *tData = tag->GetDataInstance();
		BaseList2D *linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
		if(linkTag)
		{
			if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
			{
				tData->SetReal(HND_GRIP_VALUE+i,tData->GetReal(HND_POSE_GRIP+i+(pose*1000)));
				tData->SetReal(HND_TWIST_VALUE+i,tData->GetReal(HND_POSE_TWIST+i+(pose*1000)));
				tData->SetReal(HND_SPREAD_VALUE+i,tData->GetReal(HND_POSE_SPREAD+i+(pose*1000)));
				tData->SetReal(HND_BEND_VALUE+i,tData->GetReal(HND_POSE_BEND+i+(pose*1000)));
				tData->SetReal(HND_CURL_VALUE+i,tData->GetReal(HND_POSE_CURL+i+(pose*1000)));
			}
			else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
			{
				tData->SetReal(HND_SPREAD_VALUE+i,tData->GetReal(HND_POSE_SPREAD+i+(pose*1000)));
				tData->SetReal(HND_BEND_VALUE+i,tData->GetReal(HND_POSE_BEND+i+(pose*1000)));
				tData->SetReal(HND_CURL_VALUE+i,tData->GetReal(HND_POSE_CURL+i+(pose*1000)));
				tData->SetReal(HND_DAMPING_VALUE+i,tData->GetReal(HND_POSE_DAMPING+i+(pose*1000)));
			}
		}
	}
	return true;	
}

Bool CDHandPlugin::SaveSliders(BaseTag *tag, BaseDocument *doc, LONG count)
{
	LONG i;
	for (i=0; i<count; i++)
	{
		BaseContainer *tData = tag->GetDataInstance();
		BaseList2D *linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
		if(linkTag)
		{
			if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
			{
				tData->SetReal(HND_GRIP_VALUE+i+50,tData->GetReal(HND_GRIP_VALUE+i));
				tData->SetReal(HND_TWIST_VALUE+i+50,tData->GetReal(HND_TWIST_VALUE+i));
				tData->SetReal(HND_SPREAD_VALUE+i+50,tData->GetReal(HND_SPREAD_VALUE+i));
				tData->SetReal(HND_BEND_VALUE+i+50,tData->GetReal(HND_BEND_VALUE+i));
				tData->SetReal(HND_CURL_VALUE+i+50,tData->GetReal(HND_CURL_VALUE+i));
			}
			else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
			{
				tData->SetReal(HND_SPREAD_VALUE+i+50,tData->GetReal(HND_SPREAD_VALUE+i));
				tData->SetReal(HND_BEND_VALUE+i+50,tData->GetReal(HND_BEND_VALUE+i));
				tData->SetReal(HND_CURL_VALUE+i+50,tData->GetReal(HND_CURL_VALUE+i));
				tData->SetReal(HND_DAMPING_VALUE+i+50,tData->GetReal(HND_DAMPING_VALUE+i));
			}
		}
	}
	return true;	
}

Bool CDHandPlugin::RestoreSliders(BaseTag *tag, BaseDocument *doc, LONG count)
{
	LONG i;
	for (i=0; i<count; i++)
	{
		BaseContainer *tData = tag->GetDataInstance();
		BaseList2D *linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
		if(linkTag)
		{
			if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
			{
				tData->SetReal(HND_GRIP_VALUE+i,tData->GetReal(HND_GRIP_VALUE+i+50));
				tData->SetReal(HND_TWIST_VALUE+i,tData->GetReal(HND_TWIST_VALUE+i+50));
				tData->SetReal(HND_SPREAD_VALUE+i,tData->GetReal(HND_SPREAD_VALUE+i+50));
				tData->SetReal(HND_BEND_VALUE+i,tData->GetReal(HND_BEND_VALUE+i+50));
				tData->SetReal(HND_CURL_VALUE+i,tData->GetReal(HND_CURL_VALUE+i+50));
			}
			else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
			{
				tData->SetReal(HND_SPREAD_VALUE+i,tData->GetReal(HND_SPREAD_VALUE+i+50));
				tData->SetReal(HND_BEND_VALUE+i,tData->GetReal(HND_BEND_VALUE+i+50));
				tData->SetReal(HND_CURL_VALUE+i,tData->GetReal(HND_CURL_VALUE+i+50));
				tData->SetReal(HND_DAMPING_VALUE+i,tData->GetReal(HND_DAMPING_VALUE+i+50));
			}
		}
	}
	return true;	
}

Bool CDHandPlugin::RestoreMix(BaseTag *tag, BaseDocument *doc, LONG count)
{
	LONG i;
	for (i=0; i<count; i++)
	{
		BaseContainer *tData = tag->GetDataInstance();
		BaseList2D *linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
		BaseContainer *lnkData = linkTag->GetDataInstance();
		if(linkTag)
		{
			if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
			{
				tData->SetReal(HND_GRIP_VALUE+i,lnkData->GetReal(TMB_GRIP_VALUE));
				tData->SetReal(HND_TWIST_VALUE+i,lnkData->GetReal(TMB_TWIST_VALUE));
				tData->SetReal(HND_SPREAD_VALUE+i,lnkData->GetReal(TMB_SPREAD_VALUE));
				tData->SetReal(HND_BEND_VALUE+i,lnkData->GetReal(TMB_BEND_VALUE));
				tData->SetReal(HND_CURL_VALUE+i,lnkData->GetReal(TMB_CURL_VALUE));
			}
			else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
			{
				tData->SetReal(HND_SPREAD_VALUE+i,lnkData->GetReal(FNGR_SPREAD_VALUE));
				tData->SetReal(HND_BEND_VALUE+i,lnkData->GetReal(FNGR_BEND_VALUE));
				tData->SetReal(HND_CURL_VALUE+i,lnkData->GetReal(FNGR_CURL_VALUE));
				tData->SetReal(HND_DAMPING_VALUE+i,lnkData->GetReal(FNGR_DAMPING_VALUE));
			}
		}
	}
	return true;	
}

Bool CDHandPlugin::SavePose(BaseTag *tag, BaseDocument *doc, LONG count, LONG pose)
{
	LONG i;
	for (i=0; i<count; i++)
	{
		BaseContainer *tData = tag->GetDataInstance();
		BaseList2D *linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
		BaseContainer *lnkData = linkTag->GetDataInstance();
		if(linkTag)
		{
			if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
			{
				tData->SetReal(HND_POSE_GRIP+i+(pose*1000),lnkData->GetReal(TMB_GRIP_VALUE));
				tData->SetReal(HND_POSE_TWIST+i+(pose*1000),lnkData->GetReal(TMB_TWIST_VALUE));
				tData->SetReal(HND_POSE_SPREAD+i+(pose*1000),lnkData->GetReal(TMB_SPREAD_VALUE));
				tData->SetReal(HND_POSE_BEND+i+(pose*1000),lnkData->GetReal(TMB_BEND_VALUE));
				tData->SetReal(HND_POSE_CURL+i+(pose*1000),lnkData->GetReal(TMB_CURL_VALUE));
			}
			else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
			{
				tData->SetReal(HND_POSE_SPREAD+i+(pose*1000),lnkData->GetReal(FNGR_SPREAD_VALUE));
				tData->SetReal(HND_POSE_BEND+i+(pose*1000),lnkData->GetReal(FNGR_BEND_VALUE));
				tData->SetReal(HND_POSE_CURL+i+(pose*1000),lnkData->GetReal(FNGR_CURL_VALUE));
				tData->SetReal(HND_POSE_DAMPING+i+(pose*1000),lnkData->GetReal(FNGR_DAMPING_VALUE));
			}
		}
	}
	return true;	
}

void CDHandPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdknr = tData->GetString(T_STR);
	SerialInfo si;
	
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
	
	if(GeGetVersionType() & VERSION_NET) reg = true;
	if(GeGetVersionType() & VERSION_SERVER) reg = true;
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = true;
#endif
	
	tData->SetBool(T_REG,reg);
	if(!reg)
	{
		if(!tData->GetBool(T_SET))
		{
			tData->SetLink(T_OID,op);
			tData->SetLink(T_PID,op->GetUp());
			
			LONG i, fCnt = tData->GetLong(HND_FINGER_COUNT);
			tData->SetLong(HND_FINGER_COUNT+T_LST,fCnt);
			
			for (i=0; i<fCnt; i++)
			{
				tData->SetLink(HND_TAG_LINK+i+T_LST,tData->GetLink(HND_TAG_LINK+i,doc));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

void CDHandPlugin::SetPoseCopy(BaseDocument *doc, BaseContainer *tData, CDPoseCopyData *pcd)
{
	BaseContainer *hData = pcd->hTag->GetDataInstance();
	LONG pose = pcd->pInd;
	
	LONG mpCnt = hData->GetLong(HND_POSE_COUNT);
	if(pose < mpCnt)
	{
		LONG i, fCnt = hData->GetLong(HND_FINGER_COUNT);
		if(fCnt == tData->GetLong(HND_FINGER_COUNT))
		{
			for(i=0; i<fCnt; i++)
			{
				BaseList2D *linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
				if(linkTag)
				{
					if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
					{
						tData->SetReal(HND_GRIP_VALUE+i,hData->GetReal(HND_POSE_GRIP+i+(pose*1000)));
						tData->SetReal(HND_TWIST_VALUE+i,hData->GetReal(HND_POSE_TWIST+i+(pose*1000)));
						tData->SetReal(HND_SPREAD_VALUE+i,hData->GetReal(HND_POSE_SPREAD+i+(pose*1000)));
						tData->SetReal(HND_BEND_VALUE+i,hData->GetReal(HND_POSE_BEND+i+(pose*1000)));
						tData->SetReal(HND_CURL_VALUE+i,hData->GetReal(HND_POSE_CURL+i+(pose*1000)));
					}
					else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
					{
						tData->SetReal(HND_SPREAD_VALUE+i,hData->GetReal(HND_POSE_SPREAD+i+(pose*1000)));
						tData->SetReal(HND_BEND_VALUE+i,hData->GetReal(HND_POSE_BEND+i+(pose*1000)));
						tData->SetReal(HND_CURL_VALUE+i,hData->GetReal(HND_POSE_CURL+i+(pose*1000)));
						tData->SetReal(HND_DAMPING_VALUE+i,hData->GetReal(HND_POSE_DAMPING+i+(pose*1000)));
					}
				}
			}
		}
	}
}

Bool CDHandPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	BaseDocument *doc = node->GetDocument();
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDIK_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDIKTOOLS,snData,CDIK_SERIAL_SIZE);
			cdknr.SetCString(snData,CDIK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDIK_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDIKTOOLS,snData,CDIK_SERIAL_SIZE);
			cdknr.SetCString(snData,CDIK_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	LONG fCnt = tData->GetLong(HND_FINGER_COUNT);
	LONG pCnt = tData->GetLong(HND_POSE_COUNT);

	if(!tData->GetBool(HND_NEW))
	{
		SaveSliders(tag, doc, fCnt);
		if(pCnt > 1)
		{
			LONG i;
			for(i=0; i<pCnt; i++)
			{
				tData->SetBool(HND_POSE_IS_SET+i,true);
			}
		}
		else
		{
			pCnt = 1;
		}
		tData->SetBool(HND_NEW,true);
	}
	
	LONG f;
	for(f=0; f<fCnt; f++)
	{
		if(!tData->GetLink(HND_TAG_LINK+f,doc)) initSliders[f] = true;
	}
	
	switch (type)
	{
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == HND_LEFT_HAND)
			{
				for(f=0; f<fCnt; f++)
				{
					BaseList2D *link = tData->GetLink(HND_TAG_LINK+f,doc);
					if(link)
					{
						switch(link->GetType())
						{
							case ID_CDFINGERPLUGIN:
								tData->SetReal(HND_SPREAD_VALUE+f,tData->GetReal(HND_SPREAD_VALUE+f) * -1);
								break;
							case ID_CDTHUMBPLUGIN:
								tData->SetReal(HND_TWIST_VALUE+f,tData->GetReal(HND_TWIST_VALUE+f) * -1);
								tData->SetReal(HND_SPREAD_VALUE+f,tData->GetReal(HND_SPREAD_VALUE+f) * -1);
								break;
						}
					}
				}
			}
			break;
		}
		case CD_MSG_POSE_COPY:
		{
			CDPoseCopyData *pcd = (CDPoseCopyData*) data;
			if(pcd) SetPoseCopy(doc,tData,pcd);
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==HND_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==HND_ADD_FINGER)
			{
				if(tData->GetBool(T_REG))
				{
					fCnt++;
					if(fCnt > MAX_FINGER)
					{
						fCnt = MAX_FINGER;
					}
					else
					{
						initSliders[fCnt-1] = true;
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						tData->SetLong(HND_FINGER_COUNT,fCnt);
					}
				}
			}
			else if(dc->id[0].id==HND_SUB_FINGER)
			{
				if(tData->GetBool(T_REG))
				{
					fCnt--;
					if(fCnt < 0)
					{
						fCnt = 0;
					}
					else
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						tData->SetLong(HND_FINGER_COUNT,fCnt);
						tData->SetLink(HND_TAG_LINK+fCnt,NULL);
					}
				}
			}
			else if(dc->id[0].id==HND_POSE_ADD)
			{
				if(tData->GetBool(T_REG))
				{
					pCnt++;
					if(pCnt > MAX_POSE)
					{
						pCnt = MAX_POSE;
					}
					else
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						tData->SetLong(HND_POSE_COUNT,pCnt);
					}
					SaveSliders(tag, doc, fCnt);
					RestoreMix(tag, doc, fCnt);
				}
			}
			else if(dc->id[0].id==HND_POSE_SUB)
			{
				if(tData->GetBool(T_REG))
				{
					pCnt--;
					if(pCnt < 1)
					{
						pCnt = 1;
					}
					else
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						
						// remove any animation tracks
						DescID dscID = DescID(DescLevel(HND_POSE_MIX+pCnt,DTYPE_REAL,0));
						CDDeleteAnimationTrack(doc,tag,dscID);
						
						tData->SetLong(HND_POSE_COUNT,pCnt);
						tData->SetString(HND_POSE_NAME+pCnt,"");
						tData->SetBool(HND_POSE_IS_SET+pCnt,false);
					}
				}
			}
			
			for(LONG i=0; i<pCnt;i++)
			{
				if(dc->id[0].id==HND_POSE_SET+i)
				{
					if(tData->GetBool(T_REG))
					{
						if(i == 0 && !tData->GetBool(HND_POSE_IS_SET) && pCnt == 1)
						{
							SaveSliders(tag, doc, fCnt);
						}
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SavePose(tag, doc, fCnt, i);
						RestoreSliders(tag, doc, fCnt);
						tData->SetBool(HND_POSE_IS_SET+i,true);
					}
				}
				else if(dc->id[0].id==HND_POSE_EDIT+i)
				{
					if(tData->GetBool(T_REG))
					{
						for(LONG p=0; p<pCnt; p++)
						{
							tData->SetReal(HND_POSE_MIX+p,0.0);
						}
						SaveSliders(tag, doc, fCnt);
						RestorePose(tag, doc, fCnt, i);
						tData->SetBool(HND_POSE_IS_SET+i,false);
					}
				}
				else if(dc->id[0].id==HND_POSE_RESTORE+i)
				{
					for(LONG p=1; p<pCnt; p++)
					{
						if(p!=i) tData->SetReal(HND_POSE_MIX+p,0.0);
					}
					RestorePose(tag, doc, fCnt, i);
				}
			}
			break;
		}
	}

	return true;
}

Bool CDHandPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool enable = true;
	
	if(!tData->GetBool(T_REG))
	{
		if(!tData->GetBool(T_SET)) enable = false;
		
		Bool tagMoved = false;
		if(op != tData->GetObjectLink(T_OID,doc))
		{
			BaseObject *tOp = tData->GetObjectLink(T_OID,doc);
			if(tOp)
			{
				if(tOp->GetDocument())
				{
					tagMoved = true;
					tData->SetBool(T_MOV,true);
				}
			}
			if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_OID,op);
		}
		else
		{
			if(op->GetUp() != tData->GetObjectLink(T_PID,doc))
			{
				BaseObject *pOp = tData->GetObjectLink(T_PID,doc);
				if(pOp)
				{
					if(pOp->GetDocument())
					{
						tagMoved = true;
						tData->SetBool(T_MOV,true);
					}
				}
				if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_PID,op->GetUp());
			}
			else tData->SetBool(T_MOV,false);
		}
		if(tagMoved || tData->GetBool(T_MOV)) enable = false;
		
		LONG i, fCnt = tData->GetLong(HND_FINGER_COUNT+T_LST);
		tData->SetLong(HND_FINGER_COUNT,fCnt);
		
		for (i=0; i<fCnt; i++)
		{
			tData->SetLink(HND_TAG_LINK+i,tData->GetLink(HND_TAG_LINK+i+T_LST,doc));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDHandPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	LONG fCnt = tData->GetLong(HND_FINGER_COUNT);
	LONG pCnt = tData->GetLong(HND_POSE_COUNT);
	
	setPose = false;
	LONG i;
	for(i=0; i<pCnt;i++)
	{
		if(!tData->GetBool(HND_POSE_IS_SET+i))  setPose = true;
	}
	Real g=0.0, t=0.0, s=0.0, b=0.0, c=0.0, d=0.0;
	Real blg=0.0, blt=0.0, bls=0.0, blb=0.0, blc=0.0, bld=0.0;
	Bool bnc = false;
	
	Real blend = tData->GetReal(HND_USE_MIXER);
	BaseList2D *linkTag = NULL;
	if(!setPose)
	{
		LONG p, f;
		for(f=0; f<fCnt; f++)
		{
			blg = tData->GetReal(HND_GRIP_VALUE+f);
			blt = tData->GetReal(HND_TWIST_VALUE+f);
			bls = tData->GetReal(HND_SPREAD_VALUE+f);
			blb = tData->GetReal(HND_BEND_VALUE+f);
			blc = tData->GetReal(HND_CURL_VALUE+f);
			bld = tData->GetReal(HND_DAMPING_VALUE+f);
			bnc = tData->GetBool(HND_BEND_N_CURL+f);

			g = tData->GetReal(HND_POSE_GRIP+f);
			t = tData->GetReal(HND_POSE_TWIST+f);
			s = tData->GetReal(HND_POSE_SPREAD+f);
			b = tData->GetReal(HND_POSE_BEND+f);
			c = tData->GetReal(HND_POSE_CURL+f);
			d = tData->GetReal(HND_POSE_DAMPING+f);
			
			for(p=1; p<pCnt; p++)
			{
				Real mix = tData->GetReal(HND_POSE_MIX+p);
				linkTag = tData->GetLink(HND_TAG_LINK+f,doc);
				if(linkTag)
				{
					if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
					{
						
						g += (tData->GetReal(HND_POSE_GRIP+f+(p*1000)) - tData->GetReal(HND_POSE_GRIP+f)) * mix;
						t += (tData->GetReal(HND_POSE_TWIST+f+(p*1000)) - tData->GetReal(HND_POSE_TWIST+f)) * mix;
						s += (tData->GetReal(HND_POSE_SPREAD+f+(p*1000)) - tData->GetReal(HND_POSE_SPREAD+f)) * mix;
						b += (tData->GetReal(HND_POSE_BEND+f+(p*1000)) - tData->GetReal(HND_POSE_BEND+f)) * mix;
						c += (tData->GetReal(HND_POSE_CURL+f+(p*1000)) - tData->GetReal(HND_POSE_CURL+f)) * mix;
					}
					else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
					{
						s += (tData->GetReal(HND_POSE_SPREAD+f+(p*1000)) - tData->GetReal(HND_POSE_SPREAD+f)) * mix;
						b += (tData->GetReal(HND_POSE_BEND+f+(p*1000)) - tData->GetReal(HND_POSE_BEND+f)) * mix;
						c += (tData->GetReal(HND_POSE_CURL+f+(p*1000)) - tData->GetReal(HND_POSE_CURL+f)) * mix;
						d += (tData->GetReal(HND_POSE_DAMPING+f+(p*1000)) - tData->GetReal(HND_POSE_DAMPING+f)) * mix;
					}
				}
			}
			tData->SetReal(HND_GRIP_MIX+f, CDBlend(blg,g,blend));
			tData->SetReal(HND_TWIST_MIX+f, CDBlend(blt,t,blend));
			tData->SetReal(HND_SPREAD_MIX+f, CDBlend(bls,s,blend));
			tData->SetReal(HND_BEND_MIX+f, CDBlend(blb,b,blend));
			if(!bnc)
			{
				tData->SetReal(HND_CURL_MIX+f, CDBlend(blc,c,blend));
				tData->SetReal(HND_DAMPING_MIX+f, CDBlend(bld,d,blend));
			}
		}
		for(i=0; i<fCnt; i++)
		{
			linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
			if(linkTag)
			{
				BaseContainer *lData = linkTag->GetDataInstance();
				if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
				{
					g = tData->GetReal(HND_GRIP_MIX+i);
					t = tData->GetReal(HND_TWIST_MIX+i);
					s = tData->GetReal(HND_SPREAD_MIX+i);
					b = tData->GetReal(HND_BEND_MIX+i);
					c = tData->GetReal(HND_CURL_MIX+i);
					
					lData->SetBool(TMB_LEFT_HAND,tData->GetBool(HND_LEFT_HAND));
					lData->SetReal(TMB_GRIP_VALUE,g);
					lData->SetReal(TMB_TWIST_VALUE,t);
					lData->SetReal(TMB_SPREAD_VALUE,s);
					lData->SetReal(TMB_BEND_VALUE,b);
					lData->SetBool(TMB_BEND_N_CURL,tData->GetBool(HND_BEND_N_CURL+i));
					lData->SetReal(TMB_CURL_VALUE,c);
				}
				else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
				{
					s = tData->GetReal(HND_SPREAD_MIX+i);
					b = tData->GetReal(HND_BEND_MIX+i);
					c = tData->GetReal(HND_CURL_MIX+i);
					d = tData->GetReal(HND_DAMPING_MIX+i);
					
					lData->SetBool(FNGR_LEFT_HAND,tData->GetBool(HND_LEFT_HAND));
					lData->SetReal(FNGR_SPREAD_VALUE,s);
					lData->SetReal(FNGR_BEND_VALUE,b);
					lData->SetBool(FNGR_BEND_N_CURL,tData->GetBool(HND_BEND_N_CURL+i));
					lData->SetReal(FNGR_CURL_VALUE,c);
					lData->SetReal(FNGR_DAMPING_VALUE,d);
				}
			}
		}
	}
	else
	{
		for(i=0; i<fCnt; i++)
		{
			linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
			if(linkTag)
			{
				BaseContainer *lData = linkTag->GetDataInstance();
				if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
				{
					lData->SetBool(TMB_LEFT_HAND,tData->GetBool(HND_LEFT_HAND));
					lData->SetReal(TMB_GRIP_VALUE,tData->GetReal(HND_GRIP_VALUE+i));
					lData->SetReal(TMB_TWIST_VALUE,tData->GetReal(HND_TWIST_VALUE+i));
					lData->SetReal(TMB_SPREAD_VALUE,tData->GetReal(HND_SPREAD_VALUE+i));
					lData->SetReal(TMB_BEND_VALUE,tData->GetReal(HND_BEND_VALUE+i));
					lData->SetReal(TMB_CURL_VALUE,tData->GetReal(HND_CURL_VALUE+i));
				}
				else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
				{
					lData->SetBool(FNGR_LEFT_HAND,tData->GetBool(HND_LEFT_HAND));
					lData->SetReal(FNGR_SPREAD_VALUE,tData->GetReal(HND_SPREAD_VALUE+i));
					lData->SetReal(FNGR_BEND_VALUE,tData->GetReal(HND_BEND_VALUE+i));
					lData->SetBool(FNGR_BEND_N_CURL,tData->GetBool(HND_BEND_N_CURL+i));
					lData->SetReal(FNGR_CURL_VALUE,tData->GetReal(HND_CURL_VALUE+i));
					lData->SetReal(FNGR_DAMPING_VALUE,tData->GetReal(HND_DAMPING_VALUE+i));
				}
			}
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDHandPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseDocument 	*doc = node->GetDocument();
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance(), *lData = NULL;

	if(!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(HND_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	LONG idOffset = 0;
	if(!tData->GetBool(T_REG)) idOffset = 510000;
	
	LONG fCnt = tData->GetLong(HND_FINGER_COUNT);
	LONG pCnt = tData->GetLong(HND_POSE_COUNT);
	LONG i;
	for (i=0; i<fCnt; i++)
	{
		String linkName;
		BaseList2D *linkTag = tData->GetLink(HND_TAG_LINK+i,doc);
		if(!linkTag)
		{
			linkName = GeLoadString(H_FINGER)+"."+CDLongToString(i);
		}
		else
		{
			lData = linkTag->GetDataInstance();
			linkName = linkTag->GetName();
			if(initSliders[i])
			{
				if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
				{
					tData->SetReal(HND_GRIP_VALUE+i,lData->GetReal(TMB_GRIP_VALUE+idOffset));
					tData->SetReal(HND_TWIST_VALUE+i,lData->GetReal(TMB_TWIST_VALUE+idOffset));
					tData->SetReal(HND_SPREAD_VALUE+i,lData->GetReal(TMB_SPREAD_VALUE+idOffset));
					tData->SetReal(HND_BEND_VALUE+i,lData->GetReal(TMB_BEND_VALUE+idOffset));
					tData->SetReal(HND_CURL_VALUE+i,lData->GetReal(TMB_CURL_VALUE+idOffset));
				}
				else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
				{
					tData->SetReal(HND_SPREAD_VALUE+i,lData->GetReal(FNGR_SPREAD_VALUE+idOffset));
					tData->SetReal(HND_BEND_VALUE+i,lData->GetReal(FNGR_BEND_VALUE+idOffset));
					tData->SetReal(HND_CURL_VALUE+i,lData->GetReal(FNGR_CURL_VALUE+idOffset));
					tData->SetReal(HND_DAMPING_VALUE+i,lData->GetReal(FNGR_DAMPING_VALUE+idOffset));
					tData->SetBool(HND_BEND_N_CURL+i,lData->GetReal(FNGR_BEND_N_CURL+idOffset));
				}
				initSliders[i] = false;
			}
		}

		BaseContainer bcLink = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bcLink.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bcLink.SetString(DESC_NAME,linkName);
		bcLink.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bcLink.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(HND_TAG_LINK+i,DTYPE_BASELISTLINK,0),bcLink,DescLevel(HND_FINGER_GROUP))) return false;

		BaseContainer subgroup = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup.SetString(DESC_NAME,linkName);
		subgroup.SetLong(DESC_COLUMNS, 1);
		if(!description->SetParameter(DescLevel(HND_SUBGROUP+i, DTYPE_GROUP, 0), subgroup, DescLevel(HND_CONTROLLER_GROUP))) return true;
		
		if(linkTag)
		{
			if(linkTag->GetType() == ID_CDTHUMBPLUGIN)
			{
				Real gripMin = lData->GetReal(TMB_GRIP_MIN+idOffset);	
				Real gripMax = lData->GetReal(TMB_GRIP_MAX+idOffset);	
				Real twistMin = lData->GetReal(TMB_TWIST_MIN+idOffset);	
				Real twistMax = lData->GetReal(TMB_TWIST_MAX+idOffset);	
				Real spreadMin = lData->GetReal(TMB_SPREAD_MIN+idOffset);	
				Real spreadMax = lData->GetReal(TMB_SPREAD_MAX+idOffset);	
				Real bendMin = lData->GetReal(TMB_BEND_MIN+idOffset);	
				Real bendMax = lData->GetReal(TMB_BEND_MAX+idOffset);	
				Real curlMin = lData->GetReal(TMB_CURL_MIN+idOffset);	
				Real curlMax = lData->GetReal(TMB_CURL_MAX+idOffset);	

				BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc1.SetString(DESC_NAME, GeLoadString(H_GRIP));
				bc1.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc1.SetReal(DESC_MINSLIDER, gripMin);
				bc1.SetReal(DESC_MAXSLIDER, gripMax);
				bc1.SetReal(DESC_STEP, Rad(0.1));
				bc1.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc1.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_GRIP_VALUE+i, DTYPE_REAL, 0), bc1, DescLevel(HND_SUBGROUP+i))) return true;

				BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc2.SetString(DESC_NAME, GeLoadString(H_TWIST));
				bc2.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc2.SetReal(DESC_MINSLIDER, twistMin);
				bc2.SetReal(DESC_MAXSLIDER, twistMax);
				bc2.SetReal(DESC_STEP, Rad(0.1));
				bc2.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc2.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_TWIST_VALUE+i, DTYPE_REAL, 0), bc2, DescLevel(HND_SUBGROUP+i))) return true;

				BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc3.SetString(DESC_NAME, GeLoadString(H_SPREAD));
				bc3.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc3.SetReal(DESC_MINSLIDER, spreadMin);
				bc3.SetReal(DESC_MAXSLIDER, spreadMax);
				bc3.SetReal(DESC_STEP, Rad(0.1));
				bc3.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc3.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_SPREAD_VALUE+i, DTYPE_REAL, 0), bc3, DescLevel(HND_SUBGROUP+i))) return true;

				BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc4.SetString(DESC_NAME, GeLoadString(H_BEND));
				bc4.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc4.SetReal(DESC_MINSLIDER, bendMin);
				bc4.SetReal(DESC_MAXSLIDER, bendMax);
				bc4.SetReal(DESC_STEP, Rad(0.1));
				bc4.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc4.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_BEND_VALUE+i, DTYPE_REAL, 0), bc4, DescLevel(HND_SUBGROUP+i))) return true;

				BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc5.SetString(DESC_NAME, GeLoadString(H_CURL));
				bc5.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc5.SetReal(DESC_MINSLIDER, curlMin);
				bc5.SetReal(DESC_MAXSLIDER, curlMax);
				bc5.SetReal(DESC_STEP, Rad(0.1));
				bc5.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc5.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_CURL_VALUE+i, DTYPE_REAL, 0), bc5, DescLevel(HND_SUBGROUP+i))) return true;
				
				BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_BOOL);
				bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
				bc6.SetString(DESC_NAME, GeLoadString(H_BND_N_CRL));
				bc6.SetBool(DESC_DEFAULT, false);
				if(!description->SetParameter(DescLevel(HND_BEND_N_CURL+i, DTYPE_BOOL, 0), bc6, DescLevel(HND_SUBGROUP+i))) return true;
			}
			else if(linkTag->GetType() == ID_CDFINGERPLUGIN)
			{
				Real spreadMin = lData->GetReal(FNGR_SPREAD_MIN+idOffset);	
				Real spreadMax = lData->GetReal(FNGR_SPREAD_MAX+idOffset);	
				Real bendMin = lData->GetReal(FNGR_BEND_MIN+idOffset);	
				Real bendMax = lData->GetReal(FNGR_BEND_MAX+idOffset);	
				Real curlMin = lData->GetReal(FNGR_CURL_MIN+idOffset);	
				Real curlMax = lData->GetReal(FNGR_CURL_MAX+idOffset);	

				BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc1.SetString(DESC_NAME, GeLoadString(H_SPREAD));
				bc1.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc1.SetReal(DESC_MINSLIDER, spreadMin);
				bc1.SetReal(DESC_MAXSLIDER, spreadMax);
				bc1.SetReal(DESC_STEP, Rad(0.1));
				bc1.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc1.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_SPREAD_VALUE+i, DTYPE_REAL, 0), bc1, DescLevel(HND_SUBGROUP+i))) return true;

				BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc2.SetString(DESC_NAME, GeLoadString(H_BEND));
				bc2.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc2.SetReal(DESC_MINSLIDER, bendMin);
				bc2.SetReal(DESC_MAXSLIDER, bendMax);
				bc2.SetReal(DESC_STEP, Rad(0.1));
				bc2.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc2.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_BEND_VALUE+i, DTYPE_REAL, 0), bc2, DescLevel(HND_SUBGROUP+i))) return true;

				BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc3.SetString(DESC_NAME, GeLoadString(H_CURL));
				bc3.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
				bc3.SetReal(DESC_MINSLIDER, curlMin);
				bc3.SetReal(DESC_MAXSLIDER, curlMax);
				bc3.SetReal(DESC_STEP, Rad(0.1));
				bc3.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc3.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_CURL_VALUE+i, DTYPE_REAL, 0), bc3, DescLevel(HND_SUBGROUP+i))) return true;

				BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
				bc4.SetString(DESC_NAME, GeLoadString(H_DAMPING));
				bc4.SetReal(DESC_MINSLIDER,0.0);
				bc4.SetReal(DESC_MAXSLIDER,1.0);
				bc4.SetReal(DESC_STEP,0.01);
				bc4.SetLong(DESC_UNIT,DESC_UNIT_PERCENT);
				bc4.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
				bc4.SetBool(DESC_REMOVEABLE,false);
				if(!description->SetParameter(DescLevel(HND_DAMPING_VALUE+i,DTYPE_REAL,0),bc4,DescLevel(HND_SUBGROUP+i))) return false;
				
				BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_BOOL);
				bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
				bc5.SetString(DESC_NAME, GeLoadString(H_BND_N_CRL));
				bc5.SetBool(DESC_DEFAULT, false);
				if(!description->SetParameter(DescLevel(HND_BEND_N_CURL+i, DTYPE_BOOL, 0), bc5, DescLevel(HND_SUBGROUP+i))) return true;
			}		
		}
	}

	for (i=1; i<pCnt; i++)
	{
		BaseContainer bcLn = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bcLn.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bcLn.SetBool(DESC_SEPARATORLINE,true);
		if(!description->SetParameter(DescLevel(HND_LINE_ADD+i, DTYPE_SEPARATOR, 0),bcLn,DescLevel(HND_POSE_SUBGROUP))) return false;

		BaseContainer subgroup = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup.SetLong(DESC_COLUMNS, 4);
		if(!description->SetParameter(DescLevel(HND_POSE_BUTTON_GROUP+i, DTYPE_GROUP, 0), subgroup, DescLevel(HND_POSE_SUBGROUP))) return true;

		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_STRING);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_STRING);
		bc1.SetString(DESC_NAME,GeLoadString(H_POSE)+"."+CDLongToString(i));
		bc1.SetLong(DESC_ANIMATE, DESC_ANIMATE_OFF);
		if(!description->SetParameter(DescLevel(HND_POSE_NAME+i, DTYPE_STRING, 0), bc1, DescLevel(HND_POSE_BUTTON_GROUP+i))) return true;

		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc2.SetString(DESC_NAME,GeLoadString(H_POSE_SET));
		if(!description->SetParameter(DescLevel(HND_POSE_SET+i,DTYPE_BUTTON,0),bc2,DescLevel(HND_POSE_BUTTON_GROUP+i))) return false;

		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc3.SetString(DESC_NAME,GeLoadString(H_POSE_EDIT));
		if(!description->SetParameter(DescLevel(HND_POSE_EDIT+i,DTYPE_BUTTON,0),bc3,DescLevel(HND_POSE_BUTTON_GROUP+i))) return false;

		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc4.SetString(DESC_NAME,GeLoadString(H_POSE_RESTORE));
		if(!description->SetParameter(DescLevel(HND_POSE_RESTORE+i,DTYPE_BUTTON,0),bc4,DescLevel(HND_POSE_BUTTON_GROUP+i))) return false;

		subgroup.SetLong(DESC_COLUMNS, 1);
		if(!description->SetParameter(DescLevel(HND_POSE_MIXER_GROUP+i, DTYPE_GROUP, 0), subgroup, DescLevel(HND_POSE_SUBGROUP))) return true;

		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REALSLIDER);
		bc5.SetString(DESC_NAME, tData->GetString(HND_POSE_NAME+i)+"."+GeLoadString(H_POSE_MIX));
		bc5.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc5.SetReal(DESC_MINSLIDER,0.0);
		bc5.SetReal(DESC_MAXSLIDER,1.0);
		bc5.SetReal(DESC_STEP,0.01);
		bc5.SetLong(DESC_ANIMATE, DESC_ANIMATE_ON);
		bc5.SetBool(DESC_REMOVEABLE,false);
		if(!description->SetParameter(DescLevel(HND_POSE_MIX+i, DTYPE_REAL, 0), bc5, DescLevel(HND_POSE_MIXER_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDHandPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = tag->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	LONG i, pCnt = tData->GetLong(HND_POSE_COUNT), fCnt = tData->GetLong(HND_FINGER_COUNT);
	
	switch (id[0].id)
	{
		case HND_ADD_FINGER:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case HND_SUB_FINGER:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case HND_POSE_ADD:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(HND_POSE_IS_SET);
		case HND_POSE_SUB:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(HND_POSE_IS_SET);
		case HND_POSE_EDIT:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(HND_POSE_IS_SET);
		case HND_POSE_RESTORE:	
			return tData->GetBool(HND_POSE_IS_SET);
		case HND_POSE_SET:	
			if(!tData->GetBool(T_REG)) return false;
			else 
			{
				if(!tData->GetBool(HND_POSE_IS_SET)) return true;
				else return false;
			}
	}
	for (i=0; i<fCnt; i++)
	{
		if(id[0].id == HND_TAG_LINK+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
			
		}
		BaseList2D *link = tData->GetLink(HND_TAG_LINK+i,doc);
		if(link)
		{
			if(link->GetType() == ID_CDFINGERPLUGIN)
			{
				if(id[0].id == HND_CURL_VALUE+i)
				{
					if(tData->GetBool(HND_BEND_N_CURL+i)) return false;
					else return true;
				}
				else if(id[0].id == HND_DAMPING_VALUE+i)
				{
					if(tData->GetBool(HND_BEND_N_CURL+i)) return false;
					else return true;
				}
			}
		}
	}
	for(i=1; i<pCnt;i++)
	{
		if(id[0].id == HND_POSE_MIX+i)
		{
			return tData->GetBool(HND_POSE_IS_SET+i);
		}
		else if(id[0].id == HND_POSE_EDIT+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(HND_POSE_IS_SET+i);
		}
		else if(id[0].id == HND_POSE_RESTORE+i)
		{
			return tData->GetBool(HND_POSE_IS_SET+i);
		}
		else if(id[0].id == HND_POSE_SET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else 
			{
				if(!tData->GetBool(HND_POSE_IS_SET+i)) return true;
				else return false;
			}
		}
	}
	return true;
}

Bool RegisterCDHandPlugin(void)
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
	
	if(GeGetVersionType() & VERSION_NET) reg = true;
	if(GeGetVersionType() & VERSION_SERVER) reg = true;
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
	
	if(GeGetVersionType() == VERSIONTYPE_NET_CLIENT) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_3) reg = true;
	if(GeGetVersionType() == VERSIONTYPE_NET_SERVER_UNLIMITED) reg = true;
#endif
	
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDHAND); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDHANDPLUGIN,name,info,CDHandPlugin::Alloc,"tCDHand","CDHand.tif",0);
}
