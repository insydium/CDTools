//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

// c++ includes
#include "stdlib.h"

//c4d includes
#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

// CD includes
//#include "CDCompatibility.h"
#include "tCDSWConstraint.h"
#include "CDConstraint.h"
#include "CDArray.h"
//#include "CDDebug.h"

#define MAX_ADD 	10

enum
{
	//SWC_PURCHASE			= 1000,
	
	//SWC_SHOW_LINES			= 1020,
	//SWC_LINE_COLOR			= 1021,

	//SWC_ADD_SPC				= 1030,
	//SWC_SUB_SPC				= 1031,
	//SWC_LINK_GROUP			= 1032,
	
	SWC_TARGET_COUNT			= 1040,
	SWC_ACTIVE_TARGET			= 1041,
	SWC_ACTIVE					= 1042,
	SWC_G_SCALE					= 1043,
	
	//SWC_USE_START_FRAME			= 1100,
	//SWC_SET_START_FRAME			= 1101,
	//SWC_RELEASE_START_FRAME		= 1102,
	
	SWC_DISK_LEVEL				= 1200,
	SWC_KEY_COUNT				= 1201,
	
	//SWC_TARGET				= 2000,
	//SWC_SPC_MIX				= 3000,
	SWC_REF_MATRIX				= 4000,
	
	//SWC_COORD_SPACE				= 5000,
		//SWC_OBJECT				= 5001,
		//SWC_PARENT				= 5002,
		
	//SWC_ID_TARGET				= 6000,
	SWC_KEY_FRAME				= 7000,
	SWC_KEY_MATRIX				= 8000,

	SWC_START_SET				= 10000,
	SWC_START_FRAME				= 10001,
	SWC_START_POS				= 10002,
	SWC_START_SCA				= 10003,
	SWC_START_ROT				= 10004
};

class CDSpaceSwitcherPlugin : public CDTagData
{
private:
	CDArray<CDSWTrackKey>  swKey;
	
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

	LONG ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData);
	LONG GetActiveTargetIndex(BaseDocument *doc, BaseContainer *tData);
	void SetInactiveTargets(BaseDocument *doc, BaseContainer *tData, BaseObject *op);
	Matrix GetReferenceMatrix(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, LONG ind);
	
	Bool KeyFrameInList(LONG kCnt, LONG kFrm);
	LONG GetKeyListIndex(LONG kCnt, LONG kFrm);
	
	Bool AppendKeyFrames(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, DescID dscID, LONG cnt);
	Bool RemoveKeyFrames(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, DescID dscID, LONG cnt);
	
	void SetDefaultTargetParameters(BaseContainer *tData, LONG n);
	void CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst);

public:
	virtual Bool Init(GeListNode *node);
	virtual void Free(GeListNode *node);
	
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Write(GeListNode* node, HyperFile* hf);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn);
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSpaceSwitcherPlugin); }
};

Bool CDSpaceSwitcherPlugin::Init(GeListNode *node)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(SWC_SHOW_LINES,true);
	tData->SetVector(SWC_LINE_COLOR, Vector(1,0,0));

	tData->SetLong(SWC_COORD_SPACE,SWC_OBJECT);
	tData->SetLong(SWC_TARGET_COUNT,1);
	tData->SetReal(SWC_SPC_MIX,1.00);
	tData->SetMatrix(SWC_REF_MATRIX,Matrix());
	
	tData->SetBool(SWC_ACTIVE,false);
		
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	tData->SetLong(SWC_KEY_COUNT,0);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	CDCNData swd;
	PluginMessage(ID_CDSWCONSTRAINTPLUGIN,&swd);
	if(swd.list) swd.list->Append(node);
	
	return true;
}

void CDSpaceSwitcherPlugin::Free(GeListNode *node)
{
	CDCNData swd;
	PluginMessage(ID_CDSWCONSTRAINTPLUGIN,&swd);
	if(swd.list) swd.list->Remove(node);
	
	swKey.Free();
}

Bool CDSpaceSwitcherPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(SWC_DISK_LEVEL,level);
	
	LONG i, kCnt = tData->GetLong(SWC_KEY_COUNT);
	if(level > 0)
	{
		if(kCnt > 0)
		{
			if(!swKey.Alloc(kCnt)) return false;
			for(i=0; i<kCnt; i++)
			{
				CDHFReadLong(hf, &swKey[i].frm);
				CDHFReadLong(hf, &swKey[i].spc);
				
				hf->ReadMatrix(&swKey[i].m);
				hf->ReadBool(&swKey[i].set);
			}
		}
	}
	
	return true;
}

Bool CDSpaceSwitcherPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG kCnt = tData->GetLong(SWC_KEY_COUNT);

	if(!swKey.IsEmpty() && kCnt > 0)
	{
		//Level 1
		LONG i;
		for(i=0; i<swKey.Size(); i++)
		{
			CDHFWriteLong(hf, swKey[i].frm);
			CDHFWriteLong(hf, swKey[i].spc);
			
			hf->WriteMatrix(swKey[i].m);
			hf->WriteBool(swKey[i].set);
		}
	}
	
	return true;
}

Bool CDSpaceSwitcherPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag	*tag  = (BaseTag*)snode;
	BaseContainer *tData = tag->GetDataInstance();
	LONG kCnt = tData->GetLong(SWC_KEY_COUNT);

	if(!swKey.IsEmpty() && kCnt > 0)
	{
		//Level 1
		swKey.Copy(((CDSpaceSwitcherPlugin*)dest)->swKey);
	}
	
	return true;
}

void CDSpaceSwitcherPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdcnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
			
			
			tData->SetLong(SWC_TARGET_COUNT+T_LST,tData->GetLong(SWC_TARGET_COUNT));
			LONG i, swCnt = tData->GetLong(SWC_TARGET_COUNT);
			for(i=0; i<swCnt; i++)
			{
				tData->SetLink(SWC_TARGET+i+T_LST,tData->GetLink(SWC_TARGET+i,doc));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSpaceSwitcherPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		else tData->SetBool(T_MOV,false);
		if(tagMoved || tData->GetBool(T_MOV)) enable = false;
		
		tData->SetLong(SWC_TARGET_COUNT,tData->GetLong(SWC_TARGET_COUNT+T_LST));
		LONG i, swCnt = tData->GetLong(SWC_TARGET_COUNT);
		for(i=0; i<swCnt; i++)
		{
			tData->SetLink(SWC_TARGET+i,tData->GetLink(SWC_TARGET+i+T_LST,doc));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSpaceSwitcherPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	if(!tData->GetBool(SWC_SHOW_LINES)) return true;

	LONG ind = GetActiveTargetIndex(doc,tData);
	BaseObject *trg = tData->GetObjectLink(SWC_TARGET+ind,doc); if(!trg) return true;
	
	if(tData->GetReal(SWC_SPC_MIX+ind) > 0.001)
	{
		CDSetBaseDrawMatrix(bd, NULL, Matrix());
		
		Vector opPos = op->GetMg().off;
		Vector trgPos = trg->GetMg().off;
		
		bd->SetPen(tData->GetVector(SWC_LINE_COLOR));
		CDDrawLine(bd,opPos, trgPos);
	}
	
	return true;
}

void CDSpaceSwitcherPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(SWC_TARGET+i,NULL);
	tData->SetReal(SWC_SPC_MIX+i,1.00);
	tData->SetMatrix(SWC_REF_MATRIX+i,Matrix());
}

void CDSpaceSwitcherPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(SWC_TARGET+dst,tData->GetObjectLink(SWC_TARGET+src,doc));
	tData->SetReal(SWC_SPC_MIX+dst,tData->GetReal(SWC_SPC_MIX+src));
}

Bool CDSpaceSwitcherPlugin::KeyFrameInList(LONG kCnt, LONG kFrm)
{
	if(!swKey.IsEmpty())
	{
		LONG i;
		for(i=0; i<swKey.Size(); i++)
		{
			if(swKey[i].frm == kFrm) return true;
		}
	}
	
	return false;
}

LONG CDSpaceSwitcherPlugin::GetKeyListIndex(LONG kCnt, LONG kFrm)
{
	LONG i, ind = 0;
	if(!swKey.IsEmpty())
	{
		for(i=0; i<swKey.Size(); i++)
		{
			if(swKey[i].frm == kFrm) ind = i;
		}
	}
	
	return ind;
}

Bool CDSpaceSwitcherPlugin::AppendKeyFrames(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, DescID dscID, LONG cnt)
{
	LONG fps = doc->GetFps();
	LONG curFrm = doc->GetTime().GetFrame(fps);
	
	if(swKey.IsEmpty())
	{
		if(!swKey.Alloc(cnt)) return false;
		
		LONG i;
		for(i=0; i<swKey.Size(); i++)
		{
			LONG keyFrm = CDGetKeyFrame(tag,dscID,i,fps);
			swKey[i].frm = keyFrm;
			if(keyFrm == curFrm)
			{
				LONG trgInd = GetActiveTargetIndex(doc,tData);
				swKey[i].spc = trgInd;
				swKey[i].m = tData->GetMatrix(SWC_REF_MATRIX+trgInd);
				swKey[i].set = true;
			}
			else
			{
				swKey[i].spc = -1;
				swKey[i].m = Matrix();
				swKey[i].set = false;
			}
		}
	}
	else
	{
		// Create temp storage array
		LONG kCnt = tData->GetLong(SWC_KEY_COUNT);
		
		LONG i;
		for(i=0; i<cnt; i++)
		{
			LONG keyFrm = CDGetKeyFrame(tag,dscID,i,fps);
			if(!KeyFrameInList(kCnt,keyFrm))
			{
				CDSWTrackKey tmpKey;
				tmpKey.frm = keyFrm;
				if(keyFrm == curFrm)
				{
					LONG trgInd = GetActiveTargetIndex(doc,tData);
					tmpKey.spc = trgInd;
					tmpKey.m = tData->GetMatrix(SWC_REF_MATRIX+trgInd);
					tmpKey.set = true;
				}
				else
				{
					tmpKey.spc = -1;
					tmpKey.m = Matrix();
					tmpKey.set = false;
				}
				swKey.Append(tmpKey);
			}
		}
	}
	
	return true;
}

Bool CDSpaceSwitcherPlugin::RemoveKeyFrames(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, DescID dscID, LONG cnt)
{
	if(swKey.IsEmpty()) return false;
	else
	{
		// Create temp storage array
		if(cnt > 0)
		{
			CDArray<CDSWTrackKey> temp;
			if(!swKey.Copy(temp)) return false;
			swKey.Free();
			swKey.Alloc(cnt);
			
			LONG fps = doc->GetFps();
			LONG i, kCnt = tData->GetLong(SWC_KEY_COUNT);
			for(i=0; i<cnt; i++)
			{
				LONG keyFrm = CDGetKeyFrame(tag,dscID,i,fps);
				LONG kInd = GetKeyListIndex(kCnt,keyFrm);
				
				swKey[i].frm = temp[kInd].frm;
				swKey[i].spc = temp[kInd].spc;
				swKey[i].m = temp[kInd].m;
				swKey[i].set = temp[kInd].set;
			}
			temp.Free();
		}
		else swKey.Free();
	}
	
	return true;
}

Bool CDSpaceSwitcherPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	BaseDocument *doc = tag->GetDocument();
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
			
			if(doc && tData->GetLong(SWC_DISK_LEVEL) == 0) CDDeleteAllAnimationTracks(doc,tag);
			
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDC_SERIAL_SIZE];
			String cdcnr;
			
			CDReadPluginInfo(ID_CDCONSTRAINT,snData,CDC_SERIAL_SIZE);
			cdcnr.SetCString(snData,CDC_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdcnr);
			
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	//ListReceivedMessages("CDSpaceSwitcherPlugin",type,data);
	
	LONG swCnt = tData->GetLong(SWC_TARGET_COUNT);
	switch (type)
	{
		case ID_CDFREEZETRANSFORMATION:
		{
			if(tData->GetBool(SWC_ACTIVE))
			{
				Vector *trnsSca = (Vector*)data;
				if(trnsSca)
				{
					LONG ind = GetActiveTargetIndex(doc,tData);
					BaseObject *trg = tData->GetObjectLink(SWC_TARGET+ind,doc);
					if(trg)
					{
						Vector gScale = GetGlobalScale(trg);
						if(!VEqual(gScale,tData->GetVector(SWC_G_SCALE),0.001))
						{
							Matrix refM = MatrixScale(tData->GetVector(SWC_G_SCALE)) * tData->GetMatrix(SWC_REF_MATRIX+ind);
							tData->SetMatrix(SWC_REF_MATRIX+ind,refM);
						}
					}
				}
			}
			break;
		}
		case CD_MSG_UPDATE:
		{
			DescID dscID = DescID(DescLevel(SWC_COORD_SPACE, DTYPE_LONG, 0));
			
			if(CDHasAnimationTrack(tag,dscID))
			{
				LONG keyCnt = CDGetTrackKeyCount(tag,dscID);
				LONG tkCnt = tData->GetLong(SWC_KEY_COUNT);
				if(keyCnt != tkCnt)
				{
					if(keyCnt > tData->GetLong(SWC_KEY_COUNT))
					{
						if(AppendKeyFrames(tag,doc,tData,dscID,keyCnt))
						{
							qsort(swKey.Array(),keyCnt,sizeof(CDSWTrackKey),CompareFrames);
							tData->SetLong(SWC_KEY_COUNT,keyCnt);
						}
					}
					else if(keyCnt < tData->GetLong(SWC_KEY_COUNT))
					{
						if(RemoveKeyFrames(tag,doc,tData,dscID,keyCnt))
						{
							qsort(swKey.Array(),keyCnt,sizeof(CDSWTrackKey),CompareFrames);
							tData->SetLong(SWC_KEY_COUNT,keyCnt);
						}
					}
				}
				else
				{
					BaseTime curTime = doc->GetTime();
					StopAllThreads();
					
					LONG fps = doc->GetFps();
					LONG i;
					for(i=0; i<keyCnt; i++)
					{
						LONG frm = CDGetKeyFrame(tag,dscID,i,fps);
						Real kVal = CDGetKeyValue(tag,dscID,i);
						LONG spc = LONG(kVal) - SWC_PARENT;
						
						if(swKey[i].frm != frm || swKey[i].spc != spc)
						{
							BaseTime tm = BaseTime(Real(frm),Real(doc->GetFps()));
							doc->SetTime(tm);
							CDAnimateDocument(doc,NULL,true,true,false);
							
							if(spc > -1)
							{
								BaseObject *trg = tData->GetObjectLink(SWC_TARGET+spc,doc);
								swKey[i].m = MInv(trg->GetMg()) * op->GetMg();
							}
							swKey[i].spc = spc;
							swKey[i].frm = frm;
							swKey[i].set = true;
						}
					}
					doc->SetTime(curTime);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == SWC_COORD_SPACE)
			{
				LONG ind = GetActiveTargetIndex(doc,tData);
				BaseObject *trg = tData->GetObjectLink(SWC_TARGET+ind,doc);
				if(trg)
				{
					Matrix opM = op->GetMg(), trgM = trg->GetMg();
					tData->SetMatrix(SWC_REF_MATRIX+ind,MInv(trgM) * opM);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SWC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==SWC_ADD_SPC)
			{
				if(tData->GetBool(T_REG))
				{
					if (swCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,swCnt);
						swCnt++;
						tData->SetLong(SWC_TARGET_COUNT,swCnt);
					}
				}
			}
			else if (dc->id[0].id==SWC_SUB_SPC)
			{
				if(tData->GetBool(T_REG))
				{
					if (swCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,swCnt);
						swCnt--;
						tData->SetLong(SWC_TARGET_COUNT,swCnt);
					}
				}
			}
			else if(dc->id[0].id==SWC_SET_START_FRAME)
			{
				tData->SetLong(SWC_START_FRAME,doc->GetTime().GetFrame(doc->GetFps()));
				tData->SetVector(SWC_START_POS,CDGetPos(op));
				tData->SetVector(SWC_START_SCA,CDGetScale(op));
				tData->SetVector(SWC_START_ROT,CDGetRot(op));
				tData->SetBool(SWC_START_SET,true);
			}
			else if(dc->id[0].id==SWC_RELEASE_START_FRAME)
			{
				tData->SetBool(SWC_START_SET,false);
				tData->SetBool(SWC_USE_START_FRAME,false);
			}
			break;
		}
	}

	return true;
}

LONG CDSpaceSwitcherPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, swCnt = tData->GetLong(SWC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<swCnt; i++)
	{
		trg = tData->GetObjectLink(SWC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < swCnt)
			{
				j++;
				trg = tData->GetObjectLink(SWC_TARGET+j,doc);
			}
			if(trg)
			{
				chkCnt++;
				CopyTargetParameters(doc,tData,j,i);
				SetDefaultTargetParameters(tData,j);
			}
		}
		else chkCnt++;
	}
	
	return chkCnt;
}

LONG CDSpaceSwitcherPlugin::GetActiveTargetIndex(BaseDocument *doc, BaseContainer *tData)
{
	Bool active = false;
	LONG ind = tData->GetLong(SWC_COORD_SPACE) - SWC_PARENT;
	if(ind > -1)
	{
		BaseObject *trg = tData->GetObjectLink(SWC_TARGET+ind,doc);
		if(trg)
		{
			tData->SetLong(SWC_ACTIVE_TARGET,SWC_TARGET+ind);
			active = true;
		}
	}
	tData->SetBool(SWC_ACTIVE,active);
	
	return ind;
}

void CDSpaceSwitcherPlugin::SetInactiveTargets(BaseDocument *doc, BaseContainer *tData, BaseObject *op)
{
	BaseObject *trg = NULL;
	
	LONG i, cnt = ConfirmTargetCount(doc,tData);
	Matrix opM = op->GetMg(), trgM;
	
	LONG ind = tData->GetLong(SWC_COORD_SPACE) - SWC_PARENT;
	for(i=0; i<cnt; i++)
	{
		trg = tData->GetObjectLink(SWC_TARGET+i,doc);
		if(trg && i != ind)
		{
			trgM = trg->GetMg();
			tData->SetMatrix(SWC_REF_MATRIX+i,MInv(trgM) * opM);
		}
	}
}

Matrix CDSpaceSwitcherPlugin::GetReferenceMatrix(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, LONG ind)
{
	Matrix m = Matrix();
	BaseObject *op = tag->GetObject(), *trg = tData->GetObjectLink(SWC_TARGET+ind,doc);
	
	if(op && trg)
	{
		Matrix opM = op->GetMg(), trgM = trg->GetMg();
		Matrix refM =  MInv(trgM) * opM;
		
		DescID dscID = DescID(DescLevel(SWC_COORD_SPACE, DTYPE_LONG, 0));
		if(!swKey.IsEmpty() && CDHasAnimationTrack(tag,dscID))
		{
			LONG fps = doc->GetFps();
			LONG curFrm = doc->GetTime().GetFrame(fps);
			LONG nKey = CDGetNearestKey(tag,dscID,curFrm,fps);
			
			LONG keyFrm = CDGetKeyFrame(tag,dscID,nKey,fps);
			
			if(CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING))
			{
				if(keyFrm == curFrm)
				{
					if(!MatrixEqual(swKey[nKey].m,refM,0.001))
					{
						swKey[nKey].m = refM;
						swKey[nKey].spc = ind;
						swKey[nKey].set = true;
					}
				}
			}
			
			if(KeyFrameInList(tData->GetLong(SWC_KEY_COUNT),keyFrm))
			{
				LONG kCnt = tData->GetLong(SWC_KEY_COUNT);
				if(swKey[nKey].spc < 0 && kCnt < 2)
				{
					m = tData->GetMatrix(SWC_REF_MATRIX+ind);
				}
				else
				{
					if(swKey[nKey].set) m = swKey[nKey].m;
					else
					{
						if(keyFrm == curFrm)
						{
							swKey[nKey].m = MInv(trgM) * opM;
							swKey[nKey].spc = ind;
							swKey[nKey].set = true;
							m = swKey[nKey].m;
						}
						else m = tData->GetMatrix(SWC_REF_MATRIX+ind);
					}
				}
			}
			else m = tData->GetMatrix(SWC_REF_MATRIX+ind);
		}
		else m = tData->GetMatrix(SWC_REF_MATRIX+ind);
	}
	
	return m;
}

LONG CDSpaceSwitcherPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	if(!op) return false;
	
	if(tData->GetBool(SWC_USE_START_FRAME) && !tData->GetBool(SWC_START_SET)) return true;
	
	if(tData->GetBool(SWC_START_SET))
	{
		if(doc->GetTime().GetFrame(doc->GetFps()) == tData->GetLong(SWC_START_FRAME))
		{
			CDSetPos(op,tData->GetVector(SWC_START_POS));
			CDSetScale(op,tData->GetVector(SWC_START_SCA));
			CDSetRot(op,tData->GetVector(SWC_START_ROT));
			return true;
		}
	}
	
	LONG ind = GetActiveTargetIndex(doc,tData);
	Matrix refM = GetReferenceMatrix(tag,doc,tData,ind);
	
	if(tData->GetBool(SWC_ACTIVE))
	{
		BaseObject *trg = tData->GetObjectLink(SWC_TARGET+ind,doc);
		if(trg)
		{
			tData->SetVector(SWC_G_SCALE,GetGlobalScale(trg));
			Matrix opM = op->GetMg(), trgM = trg->GetMg();
			
			Matrix transM = trgM * refM;
			Vector trgPos = transM.off;
			
			Real mix = tData->GetReal(SWC_SPC_MIX+ind);
			
			CDQuaternion opQ, refQ, mixQ;
			opQ.SetMatrix(opM);
			refQ.SetMatrix(trgM * refM);
			
			mixQ = CDQSlerp(opQ,refQ,mix);
			transM = mixQ.GetMatrix();
			transM.off = CDBlend(opM.off,trgPos,mix);
			
			Vector sca = CDGetScale(op);
			op->SetMg(transM);
			CDSetScale(op,sca);
		}
	}
	SetInactiveTargets(doc,tData,op);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSpaceSwitcherPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = tag->GetDocument(); if(!doc) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	
	LONG i, swCnt = tData->GetLong(SWC_TARGET_COUNT);
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SWC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
		
	bc = description->GetParameterI(DescLevel(SWC_COORD_SPACE), ar);
	
	BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_LONG);
	bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
	if(bc) bc1.SetString(DESC_NAME,bc->GetString(DESC_NAME));
	else bc1.SetString(DESC_NAME,"Space");
	
	BaseContainer cycle;
	cycle.SetString(SWC_OBJECT,op->GetName());
	
	for (i=0; i<swCnt; i++)
	{
		BaseObject *pr = tData->GetObjectLink(SWC_TARGET+i,doc);
		if(pr) cycle.SetString(SWC_PARENT+i,pr->GetName());

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(SWC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(SWC_ID_TARGET))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_OFF);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(SWC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(SWC_LINK_GROUP+i))) return true;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,0.0);
		if (!description->SetParameter(DescLevel(SWC_SPC_MIX+i, DTYPE_REAL, 0), bc3, DescLevel(SWC_LINK_GROUP+i))) return true;
	}
	
	bc1.SetContainer(DESC_CYCLE, cycle);
	if(!description->SetParameter(DescLevel(SWC_COORD_SPACE, DTYPE_LONG, 0), bc1, DescLevel(ID_TAGPROPERTIES))) return true;

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSpaceSwitcherPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	LONG i, swCnt = tData->GetLong(SWC_TARGET_COUNT);
	switch (id[0].id)
	{
		case SWC_ADD_SPC:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SWC_SUB_SPC:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SWC_USE_START_FRAME:
			if(!tData->GetBool(SWC_START_SET)) return true;
			else return false;
		case SWC_SET_START_FRAME:
			if(!tData->GetBool(SWC_START_SET) && tData->GetBool(SWC_USE_START_FRAME)) return true;
			else return false;
		case SWC_RELEASE_START_FRAME:
			return tData->GetBool(SWC_START_SET);
	}
	for(i=0; i<swCnt; i++)
	{
		if(id[0].id == SWC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == SWC_SPC_MIX+i)
		{
			BaseObject *trg = tData->GetObjectLink(SWC_TARGET+i,doc);
			if(!trg) return false;
			else return true;
		}
	}
	return true;
}

Bool RegisterCDSpaceSwitcherPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDC_SERIAL_SIZE];
	String cdcnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDCONSTRAINT,data,CDC_SERIAL_SIZE)) reg = false;
	
	cdcnr.SetCString(data,CDC_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdcnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdcnr.FindFirst("-",&pos);
	while(h)
	{
		cdcnr.Delete(pos,1);
		h = cdcnr.FindFirst("-",&pos);
	}
	cdcnr.ToUpper();
	
	kb = cdcnr.SubStr(pK,2);
	
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
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDSPCSWITCH); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSWCONSTRAINTPLUGIN,name,info,CDSpaceSwitcherPlugin::Alloc,"tCDSWConstraint","CDSWConstraint.tif",1);
}
