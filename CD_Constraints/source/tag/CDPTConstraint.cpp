//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_modeling.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDPTConstraint.h"
#include "CDConstraint.h"
#include "CDArray.h"

#define MAX_ADD 	100

enum
{
	//PTC_PURCHASE			= 1000,
	
	//PTC_ID_POINTS			= 1010,
	
	//PTC_SHOW_LINES			= 1021,
	//PTC_LINE_COLOR			= 1022,
	//PTC_STRENGTH				= 1023,
	
	PTC_PT_COUNT				= 1030, // Old data count
	//PTC_ADD_PT				= 1031,
	//PTC_SUB_PT				= 1032,
	
	PTC_POINTS_ADDED			= 1050,
	
	PTC_REF_COUNT				= 1100,
	PTC_DISK_LEVEL				= 1101,
	PTC_TARGET_COUNT			= 1102, // New data count
	
	//PTC_LINK_GROUP			= 2000,
	//PTC_LINE_ADD			= 3000,

	//PTC_TARGET				= 4000,
	//PTC_PT_MIX				= 5000,

	//PTC_PT_GROUP			= 6000,
	//PTC_PT_INDEX			= 7000,
	//PTC_SET_PT_SEL			= 8000,

	PTC_POS_STORED			= 10000, // Old data
	PTC_ORIG_POSITION		= 11000, // Old data
	PTC_OLD_INDEX			= 12000
};

struct CDConstrPt
{
	Bool		cnstr;
	Vector		mixpos;
	Real		mix;
	Real		totalMix;
	
	CDConstrPt()
	{
		cnstr = false;
		mixpos = Vector(0,0,0);
		mix = 0.0;
		totalMix = 0.0;
	}
};

class CDPTConstraintPlugin : public CDTagData
{
private:
	CDArray<Vector>		rpadr;
	
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Bool InitReference(BaseContainer *tData, BaseObject *op);
	void RemapReferencePoints(Vector *padr, TranslationMaps *map);

	void SetDefaultTargetParameters(BaseContainer *tData, LONG n);
	void CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst);
	LONG ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData);

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
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDPTConstraintPlugin); }
};

Bool CDPTConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(PTC_SHOW_LINES,true);
	tData->SetVector(PTC_LINE_COLOR, Vector(1,0,0));
	
	tData->SetBool(PTC_POINTS_ADDED,false);

	tData->SetLong(PTC_TARGET_COUNT,1);
	tData->SetReal(PTC_PT_MIX,1.00);
	tData->SetReal(PTC_STRENGTH,1.00);
		
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	rpadr.Init();

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

void CDPTConstraintPlugin::Free(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseObject *op = tag->GetObject();
	if(op)
	{
		Vector *padr = GetPointArray(op);
		LONG i, pCnt = ToPoint(op)->GetPointCount();
		for(i=0; i<pCnt; i++)
		{
			padr[i] = rpadr[i];
		}
	}
	rpadr.Free();
}

Bool CDPTConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(PTC_DISK_LEVEL,level);
	if(level < 1) tData->SetLong(PTC_TARGET_COUNT,tData->GetLong(PTC_PT_COUNT)+1);
	
	LONG i, refCnt = tData->GetLong(PTC_REF_COUNT);
	if(level > 0)
	{
		if(refCnt > 0)
		{
			if(!rpadr.Alloc(refCnt)) return false;
			for(i=0; i<refCnt; i++)
			{
				CDHFReadFloat(hf, &rpadr[i].x);
				CDHFReadFloat(hf, &rpadr[i].y);
				CDHFReadFloat(hf, &rpadr[i].z);
			}
		}
	}
	
	return true;
}

Bool CDPTConstraintPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if(rpadr.Size() > 0)
	{
		LONG i;
		//Level 1
		for(i=0; i<rpadr.Size(); i++)
		{
			CDHFWriteFloat(hf, rpadr[i].x);
			CDHFWriteFloat(hf, rpadr[i].y);
			CDHFWriteFloat(hf, rpadr[i].z);
		}
	}
	
	return true;
}

Bool CDPTConstraintPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag	*tag  = (BaseTag*)snode;
	BaseContainer *tData = tag->GetDataInstance();
	LONG refCnt = tData->GetLong(PTC_REF_COUNT);

	if(refCnt > 0)
	{
		//Level 1
		rpadr.Copy(((CDPTConstraintPlugin*)dest)->rpadr);
	}
	
	return true;
}

Bool CDPTConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	if(!tData->GetBool(PTC_SHOW_LINES)) return true;
	if(!IsValidPointObject(op)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	LONG i, tCnt = tData->GetLong(PTC_TARGET_COUNT);
	BaseObject *goal = NULL;
	
	bd->SetPen(tData->GetVector(PTC_LINE_COLOR));
	Matrix opM = op->GetMg();
	
	Vector *padr = GetPointArray(op);
	for(i=0; i<tCnt; i++)
	{
		goal = tData->GetObjectLink(PTC_TARGET+i,doc);
		if(goal)
		{
			LONG index = tData->GetLong(PTC_PT_INDEX+i);
			Vector pt = padr[index];
			CDDrawLine(bd,opM * pt, goal->GetMg().off);
		}
	}
	
	return true;
}

void CDPTConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(PTC_TARGET_COUNT+T_LST,tData->GetLong(PTC_TARGET_COUNT));
			LONG i, tCnt = tData->GetLong(PTC_TARGET_COUNT);
			for(i=0; i<tCnt; i++)
			{
				tData->SetLink(PTC_TARGET+i+T_LST,tData->GetLink(PTC_TARGET+i,doc));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDPTConstraintPlugin::InitReference(BaseContainer *tData, BaseObject *op)
{
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, ptCnt = ToPoint(op)->GetPointCount();
	
	if(!rpadr.Alloc(ptCnt)) return false;
	
	for(i=0; i<ptCnt; i++)
	{
		rpadr[i] = padr[i];
	}
	tData->SetLong(PTC_REF_COUNT,ptCnt);

	return true;
}

void CDPTConstraintPlugin::RemapReferencePoints(Vector *padr, TranslationMaps *map)
{
	LONG oCnt = map->m_oPointCount, nCnt = map->m_nPointCount;
	LONG i, oInd, nInd, mCnt = map->m_mPointCount;
	
	if(map && !rpadr.IsEmpty())
	{
		// Create temp storage array
		CDArray<Vector> tempRef;
		rpadr.Copy(tempRef);
		rpadr.Resize(nCnt);
		
		for(i=0; i<mCnt; i++)
		{
			oInd = map->m_pPointMap[i].oIndex;
			nInd = map->m_pPointMap[i].nIndex;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_DELETED) continue;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_NEW)
			{
				rpadr[nInd] = padr[nInd];
			}
			else if(nInd != oInd)
			{
				rpadr[nInd] = tempRef[oInd];
			}
		}
		tempRef.Free();
	}
}

void CDPTConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(PTC_TARGET+i,NULL);
	tData->SetReal(PTC_PT_MIX+i,1.00);
	tData->SetLong(PTC_PT_INDEX+i,0);
}

void CDPTConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(PTC_TARGET+dst,tData->GetObjectLink(PTC_TARGET+src,doc));
	tData->SetReal(PTC_PT_MIX+dst,tData->GetReal(PTC_PT_MIX+src));
	tData->SetLong(PTC_PT_INDEX+dst,tData->GetLong(PTC_PT_INDEX+src));
}

Bool CDPTConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	if(!IsValidPointObject(op)) return true;
	Vector *padr = GetPointArray(op);
	
	BaseDocument *doc = node->GetDocument();
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
			
			if(tData->GetLong(PTC_DISK_LEVEL) < 1)
			{
				LONG i, ptCnt = tData->GetLong(PTC_PT_COUNT);
				for(i=0; i<ptCnt; i++)
				{
					LONG index = tData->GetLong(PTC_PT_INDEX+i);
					padr[index] = tData->GetVector(PTC_ORIG_POSITION+i);
				}
				if(!InitReference(tData,op)) return true;
			}
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
			if(!InitReference(tData,op)) return true;
			break;
		}
	}
	if(doc) CheckTagReg(doc,op,tData);
	
	LONG i, tCnt = tData->GetLong(PTC_TARGET_COUNT);
	LONG oldPCnt = tData->GetLong(PTC_REF_COUNT);
	
	switch (type)
	{
		case MSG_POINTS_CHANGED:
		{
			VariableChanged *vchg = (VariableChanged*)data;
			if(vchg)
			{
				LONG nCnt = vchg->new_cnt, oCnt = vchg->old_cnt;
				if(oCnt == oldPCnt)
				{
					if(vchg->map)
					{
						TranslationMaps tMap;
						
						tMap.m_oPointCount = oCnt; // old point count
						tMap.m_nPointCount = nCnt; // new point count
							
						if(nCnt < oCnt)
						{
							// Get remap count
							LONG mCnt = 0;
							for(i=0; i<oCnt; i++)
							{
								if(vchg->map[i] != i) mCnt++;
							}
							tMap.m_mPointCount = mCnt; // map point count
							
							// Allocate the m_pPointMap array
							CDArray<TransMapData> pMap;
							
							// build the TranslationMaps data
							mCnt = 0;
							for(i=0; i<oCnt; i++)
							{
								if(vchg->map[i] != i)
								{
									TransMapData tmd = TransMapData(i,vchg->map[i]);
									tmd.mIndex = mCnt;
									if(vchg->map[i] < 0) tmd.lFlags |= TRANSMAP_FLAG_DELETED;
									pMap.Append(tmd);
									mCnt++;
								}
							}
							tMap.m_pPointMap = pMap.Array();
							
							if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
							RemapReferencePoints(padr,&tMap);
							
							tData->SetLong(PTC_REF_COUNT,nCnt);
							pMap.Free();
						}
						else if(nCnt > oCnt)
						{
							// Get remap count
							LONG mCnt = 0;
							for(i=0; i<nCnt; i++)
							{
								if(i<oCnt)
								{
									if(vchg->map[i] != i) mCnt++;
								}
								else mCnt++;
							}
							tMap.m_mPointCount = mCnt; // map point count
							
							// Allocate the m_pPointMap array
							CDArray<TransMapData> pMap;
							
							// build the TranslationMaps data
							mCnt = 0;
							for(i=0; i<nCnt; i++)
							{
								if(i<oCnt)
								{
									if(vchg->map[i] != i)
									{
										TransMapData tmd = TransMapData(i,vchg->map[i]);
										tmd.mIndex = mCnt;
										if(vchg->map[i] < 0) tmd.lFlags |= TRANSMAP_FLAG_DELETED;
										pMap.Append(tmd);
										mCnt++;
									}
								}
								else
								{
									TransMapData tmd = TransMapData(-1,i);
									tmd.mIndex = mCnt;
									tmd.lFlags |= TRANSMAP_FLAG_NEW;
									pMap.Append(tmd);
									mCnt++;
								}
							}
							tMap.m_pPointMap = pMap.Array();
							
							if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
							RemapReferencePoints(padr,&tMap);
							
							tData->SetLong(PTC_REF_COUNT,nCnt);
							pMap.Free();
						}
					}
					else
					{
						if(nCnt < oCnt)
						{
							if(!rpadr.IsEmpty() && nCnt == tCnt)
							{
								// resize storage array
								rpadr.Resize(nCnt);
								tData->SetLong(PTC_REF_COUNT,nCnt);
							}
						}
						else if(nCnt > oCnt)
						{
							if(!rpadr.IsEmpty() && nCnt == tCnt)
							{
								// resize storage array
								rpadr.Resize(nCnt);
								for(i=oCnt; i<nCnt; i++)
								{
									rpadr[i] = padr[i];
								}
								tData->SetBool(PTC_POINTS_ADDED,true);
							}
						}
						else if(nCnt == oCnt)
						{
							if(!rpadr.IsEmpty() && tData->GetBool(PTC_POINTS_ADDED))
							{
								LONG oldCnt = tData->GetLong(PTC_REF_COUNT);
								
								for(i=oldCnt; i<tCnt; i++)
								{
									rpadr[i] = padr[i];
								}
								
								tData->SetBool(PTC_POINTS_ADDED,false);
								tData->SetLong(PTC_REF_COUNT,tCnt);
							}
						}
					}
				}
			}
			break;
		}
		case  MSG_TRANSLATE_POINTS:
		{
			TranslationMaps *tMap = (TranslationMaps*)data;
			if(tMap) 
			{
				LONG oCnt = tMap->m_oPointCount, nCnt = tMap->m_nPointCount;
				if(oCnt == oldPCnt && oCnt != nCnt)
				{
					if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					RemapReferencePoints(padr,tMap);
					
					tData->SetLong(PTC_REF_COUNT,nCnt);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_INITUNDO:
		{
			if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,op);
			break;
		}
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			
			for(i=0; i<tCnt; i++)
			{
				LONG oldIndex = tData->GetLong(PTC_OLD_INDEX+i);
				if(descID[0].id  == PTC_PT_INDEX+i)
				{
					padr[oldIndex] = rpadr[oldIndex];
					
					tData->SetLong(PTC_OLD_INDEX+i,tData->GetLong(PTC_PT_INDEX+i));
				}
				else if(descID[0].id  == PTC_TARGET+i)
				{
					if(!tData->GetObjectLink(PTC_TARGET+i,doc))
					{
						padr[oldIndex] = rpadr[oldIndex];
					}
				}
			}
			break;
		}
		case ID_CDFREEZETRANSFORMATION:
		{
			Vector *trnsSca = (Vector*)data;
			if(trnsSca)
			{
				Vector sca = *trnsSca;
				LONG refCnt = tData->GetLong(PTC_REF_COUNT);
				for(i=0; i<refCnt; i++)
				{
					rpadr[i].x *= sca.x;
					rpadr[i].y *= sca.y;
					rpadr[i].z *= sca.z;
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==PTC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==PTC_ADD_PT)
			{
				if(tData->GetBool(T_REG))
				{
					if(tCnt < MAX_ADD)
					{
						if(doc) CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,tCnt);
						tCnt++;
						tData->SetLong(PTC_TARGET_COUNT,tCnt);
					}
				}
			}
			else if(dc->id[0].id==PTC_SUB_PT)
			{
				if(tData->GetBool(T_REG))
				{
					if(tCnt > 1)
					{
						if(doc) CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						
						tCnt--;
						tData->SetLong(PTC_TARGET_COUNT,tCnt);
						
						//Restore point
						LONG index = tData->GetLong(PTC_PT_INDEX+tCnt);
						padr[index] = rpadr[index];
						
						//Clear stored values
						SetDefaultTargetParameters(tData,tCnt);
					}
				}
			}
			for(i=0; i<tCnt; i++)
			{
				if(tData->GetBool(T_REG))
				{
					if(dc->id[0].id==PTC_SET_PT_SEL+i)
					{
						if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,op);
						
						BaseSelect *bs = NULL;
						bs = ToPoint(op)->GetPointS();
						if(bs)
						{
							LONG bsPtCount = bs->GetCount();
							if(bsPtCount > 0)
							{
								if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
								
								LONG a,b;
								CDGetRange(bs,0,&a,&b);
								tData->SetLong(PTC_PT_INDEX+i,a);
								LONG index = tData->GetLong(PTC_OLD_INDEX+i);
								if(a != index)
								{
									//Restore old point index's data
									padr[index] = rpadr[index];
								}
								tData->SetLong(PTC_OLD_INDEX+i,a);
							}
						}
					}
				}
			}
			break;
		}
	}

	return true;
}

Bool CDPTConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(PTC_TARGET_COUNT,tData->GetLong(PTC_TARGET_COUNT+T_LST));
		LONG i, tCnt = tData->GetLong(PTC_TARGET_COUNT);
		for(i=0; i<tCnt; i++)
		{
			tData->SetLink(PTC_TARGET+i,tData->GetLink(PTC_TARGET+i+T_LST,doc));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDPTConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, tCnt = tData->GetLong(PTC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<tCnt; i++)
	{
		trg = tData->GetObjectLink(PTC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < tCnt)
			{
				j++;
				trg = tData->GetObjectLink(PTC_TARGET+j,doc);
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

LONG CDPTConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{	
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, index, tCnt = ConfirmTargetCount(doc,tData);//oldIndex, 
	BaseObject *goal = NULL;

	if(!IsValidPointObject(op)) return false;
	Matrix opM = op->GetMg();
	
	Vector pt, *padr = GetPointArray(op);
	LONG ptCnt = ToPoint(op)->GetPointCount();
	Real strength = tData->GetReal(PTC_STRENGTH);
	
	CDArray<CDConstrPt> cnstrPt;
	cnstrPt.Alloc(ptCnt);
	
	CDConstrPt c;
	cnstrPt.Fill(c);

	for(i=0; i<tCnt; i++)
	{
		goal = tData->GetObjectLink(PTC_TARGET+i,doc);
		if(goal)
		{
			LONG ind = tData->GetLong(PTC_PT_INDEX+i);
			if(ind >= ptCnt)
			{
				ind = ptCnt-1;
				tData->SetLong(PTC_PT_INDEX+i,ind);
			}
			cnstrPt[ind].totalMix += tData->GetReal(PTC_PT_MIX+i);
		}
	}
	
	for(i=0; i<tCnt; i++)
	{
		index = tData->GetLong(PTC_PT_INDEX+i);
		
		if(tData->GetLong(PTC_PT_INDEX+i) > ptCnt-1) tData->SetLong(PTC_PT_INDEX+i, ptCnt-1);
		
		goal = tData->GetObjectLink(PTC_TARGET+i,doc);
		if(!goal)
		{
			//Restore point index's data
			padr[index] = rpadr[index];
		}
		else
		{
			if(cnstrPt[index].totalMix == 0.0) cnstrPt[index].mixpos = rpadr[i];
			else cnstrPt[index].mixpos += (MInv(opM) * goal->GetMg().off) * (tData->GetReal(PTC_PT_MIX+i)/cnstrPt[index].totalMix);
			
			cnstrPt[index].cnstr = true;
			cnstrPt[index].mix += tData->GetReal(PTC_PT_MIX+i);
			if(cnstrPt[index].mix > 1.0) cnstrPt[index].mix = 1.0;
		}
	}
	
	for(i=0; i<ptCnt; i++)
	{
		if(cnstrPt[i].cnstr)
		{
			Vector mixPt = CDBlend(rpadr[i],cnstrPt[i].mixpos,cnstrPt[i].mix);
			padr[i] = CDBlend(padr[i],mixPt,strength);
		}
	}
#if API_VERSION < 12000
	if(op->GetInfo() & OBJECT_ISSPLINE || op->GetInfo() & OBJECT_MODIFIER) op->Message(MSG_UPDATE);
#else
	op->Message(MSG_UPDATE);
#endif
	cnstrPt.Free();

	return CD_EXECUTION_RESULT_OK;
}

Bool CDPTConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, tCnt = tData->GetLong(PTC_TARGET_COUNT);
	BaseObject *op = tag->GetObject(); if(!op) return false;
	
	if(!IsValidPointObject(op)) return false;
	LONG maxInd = ToPoint(op)->GetPointCount() - 1;
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(PTC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	for (i=0; i<tCnt; i++)
	{
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if (!description->SetParameter(DescLevel(PTC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(PTC_ID_POINTS))) return false;

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(PTC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(PTC_ID_POINTS))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(PTC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(PTC_LINK_GROUP+i))) return false;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_STRENGTH)+"."+CDLongToString(i));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(PTC_PT_MIX+i, DTYPE_REAL, 0), bc3, DescLevel(PTC_LINK_GROUP+i))) return true;

		BaseContainer subgroup2 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup2.SetLong(DESC_COLUMNS, 4);
		if(!description->SetParameter(DescLevel(PTC_PT_GROUP+i, DTYPE_GROUP, 0), subgroup2, DescLevel(PTC_ID_POINTS))) return true;
		
		BaseContainer bc7 = GetCustomDataTypeDefault(DTYPE_BUTTON);
		bc7.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BUTTON);
		bc7.SetString(DESC_NAME, GeLoadString(IDS_SET_P_SEL));
		if (!description->SetParameter(DescLevel(PTC_SET_PT_SEL+i, DTYPE_BUTTON, 0), bc7, DescLevel(PTC_PT_GROUP+i))) return true;

		BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_LONG);
		bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
		bc6.SetString(DESC_NAME, GeLoadString(IDS_INDEX)+"."+CDLongToString(i));
		bc6.SetLong(DESC_UNIT, DESC_UNIT_LONG);
		bc6.SetReal(DESC_MIN,0);
		bc6.SetReal(DESC_DEFAULT,0);
		bc6.SetReal(DESC_MAX,maxInd);
		if (!description->SetParameter(DescLevel(PTC_PT_INDEX+i, DTYPE_LONG, 0), bc6, DescLevel(PTC_PT_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDPTConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	BaseObject *op = tag->GetObject(); if(!op) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	LONG i, tCnt = tData->GetLong(PTC_TARGET_COUNT);
	
	switch (id[0].id)
	{
		case PTC_ADD_PT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PTC_SUB_PT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	for(i=0; i<tCnt;i++)
	{
		BaseObject *goal = tData->GetObjectLink(PTC_TARGET+i,doc);
		if(goal)
		{
			if(id[0].id == PTC_TARGET+i)
			{
				if(!tData->GetBool(T_REG)) return false;
				else return true;
			}
			else if(id[0].id == PTC_PT_INDEX+i)
			{
				if(!tData->GetBool(T_REG)) return false;
				else if(IsValidPointObject(op)) return true;
				else return false;
			}
			else if(id[0].id == PTC_PT_MIX+i)
			{
				if(IsValidPointObject(op)) return true;
				else return false;
			}
			else if(id[0].id == PTC_SET_PT_SEL+i)
			{
				if(!tData->GetBool(T_REG)) return false;
				else if(IsValidPointObject(op)) return true;
				else return false;
			}
		}
		else
		{
			if(id[0].id == PTC_PT_INDEX+i) return false;
			else if(id[0].id == PTC_PT_MIX+i) return false;
			else if(id[0].id == PTC_SET_PT_SEL+i) return false;
		}
	}
	
	return true;
}

Bool RegisterCDPTConstraintPlugin(void)
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
	String name=GeLoadString(IDS_CDPTCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDPTCONSTRAINTPLUGIN,name,info,CDPTConstraintPlugin::Alloc,"tCDPTConstraint","CDPTConstraint.tif",1);
}
