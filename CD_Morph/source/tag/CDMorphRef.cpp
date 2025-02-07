//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "lib_modeling.h"
#include "customgui_priority.h"

#include "CDMorph.h"
#include "CDDebug.h"

#include "tCDMorphTag.h"
#include "tCDMorphRef.h"

void SendMTScalingInfo()
{
	SpecialEventAdd(CD_MSG_SCALE_CHANGE);
}

Bool CDMorphRefPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer		*tData = tag->GetDataInstance();
	
	tData->SetLong(MR_REMAP_COUNT,0);
	
	tData->SetVector(MR_SCALE,Vector(1,1,1));
	tData->SetVector(MR_REFERENCE_SCALE,Vector(1,1,1));

	tData->SetBool(MR_REFERENCE_NEW,false);
	tData->SetBool(MR_REFERENCE_SET,false);
	tData->SetBool(MR_EDITING,false);

	tData->SetBool(MR_POINTS_ADDED,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	rpadr.Init();
	mpadr.Init();
	
	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
			pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_INITIAL);
			pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,0);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	CDMData mrd;
	PluginMessage(ID_CDMORPHREFPLUGIN,&mrd);
	if(mrd.list) mrd.list->Append(node);

	return true;
}

void CDMorphRefPlugin::Free(GeListNode *node)
{
	rpadr.Free();
	mpadr.Free();
	
	CDMData mrd;
	PluginMessage(ID_CDMORPHREFPLUGIN,&mrd);
	if(mrd.list) mrd.list->Remove(node);
}

Bool CDMorphRefPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	LONG i, pCnt = tData->GetLong(MR_POINT_COUNT);
	
	tData->SetLong(MR_CDMR_LEVEL,level);
	if(level >= 0)
	{
		if(pCnt > 0)
		{
			if(!rpadr.Alloc(pCnt)) return false;
			for(i=0; i<pCnt; i++)
			{
				CDHFReadFloat(hf, &rpadr[i].x);
				CDHFReadFloat(hf, &rpadr[i].y);
				CDHFReadFloat(hf, &rpadr[i].z);
			}
		}
	}
	if(level >= 1)
	{
		if(pCnt > 0)
		{
			if(!mpadr.Alloc(pCnt)) return false;
			for(i=0; i<pCnt; i++)
			{
				CDHFReadFloat(hf, &mpadr[i].x);
				CDHFReadFloat(hf, &mpadr[i].y);
				CDHFReadFloat(hf, &mpadr[i].z);
			}
		}
	}
	
	return true;
}

Bool CDMorphRefPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	LONG pCnt = tData->GetLong(MR_POINT_COUNT);

	if(pCnt > 0 && !rpadr.IsEmpty() && !mpadr.IsEmpty())
	{
		LONG i;
		//Level 0
		for(i=0; i<rpadr.Size(); i++)
		{
			CDHFWriteFloat(hf, rpadr[i].x);
			CDHFWriteFloat(hf, rpadr[i].y);
			CDHFWriteFloat(hf, rpadr[i].z);
		}
		//Level 1
		for(i=0; i<mpadr.Size(); i++)
		{
			CDHFWriteFloat(hf, mpadr[i].x);
			CDHFWriteFloat(hf, mpadr[i].y);
			CDHFWriteFloat(hf, mpadr[i].z);
		}
	}
	
	return true;
}

Bool CDMorphRefPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag = (BaseTag*)snode;
	BaseContainer *tData = tag->GetDataInstance();

	LONG pCnt = tData->GetLong(MR_POINT_COUNT);

	if(pCnt > 0)
	{
		rpadr.Copy(((CDMorphRefPlugin*)dest)->rpadr);
		mpadr.Copy(((CDMorphRefPlugin*)dest)->mpadr);
	}
	
	return true;
}

Bool CDMorphRefPlugin::SetReference(BaseContainer *tData, BaseObject *op)
{
	if(!op) return false;
	Vector *padr = GetPointArray(op); if(!padr) return false;
		
	LONG i, pCnt = ToPoint(op)->GetPointCount();
	
	rpadr.Resize(pCnt);
	mpadr.Resize(pCnt);
	
	for(i=0; i<pCnt; i++)
	{
		rpadr[i].SetVector(padr[i]);
		mpadr[i] = rpadr[i];
	}
	tData->SetLong(MR_POINT_COUNT, pCnt);
	tData->SetBool(MR_REFERENCE_SET, true);
	
	return true;
}

Bool CDMorphRefPlugin::RestoreReference(BaseContainer *tData, BaseObject *op)
{
	if(!op) return false;
	Vector *padr = GetPointArray(op); if(!padr) return false;
	
	LONG i, pCnt = ToPoint(op)->GetPointCount();
	if(rpadr.Size() > pCnt) return false;
	
	for(i=0; i<rpadr.Size(); i++)
	{
		padr[i] = rpadr[i].GetVector();
		mpadr[i] = rpadr[i];
	}
	
	return true;
}

Bool CDMorphRefPlugin::InitReference(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op)
{
	if(!doc) return false;
	
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, pCnt = ToPoint(op)->GetPointCount();

	BaseObject *refObject = NULL;
	Vector *refadr = NULL;
	if(!tData->GetBool(MR_REFERENCE_NEW))
	{
		refObject = tData->GetObjectLink(MR_REFERENCE_LINK,doc);
		if(refObject)
		{
			if(IsValidPointObject(refObject))
			{
				refadr = GetPointArray(refObject); if(!refadr) return false;
			}
		}
		tData->SetMatrix(MR_REF_MATRIX,op->GetMg());
		tData->SetBool(MR_REFERENCE_NEW,true);
		tData->SetBool(MR_REFERENCE_SET,false);
		tData->SetLink(T_OID,op);
	}
	
	if(mpadr.IsEmpty()) mpadr.Alloc(pCnt);
	
	BaseTag *skRefTag = op->GetTag(ID_CDSKINREFPLUGIN);

	if(!tData->GetBool(MR_REFERENCE_SET))
	{
		if(rpadr.IsEmpty()) rpadr.Alloc(pCnt);
		if(refObject)
		{
			if(IsValidPointObject(refObject))
			{
				for(i=0; i<pCnt; i++)
				{
					rpadr[i].SetVector(refadr[i]);
					mpadr[i] = rpadr[i];
				}
			}
		}
		else
		{
			if(skRefTag)
			{
				CDMRefPoint *sradr = CDGetSkinReference(skRefTag);
				for(i=0; i<pCnt; i++)
				{
					if(sradr)
					{
						rpadr[i] = sradr[i];
						mpadr[i] = rpadr[i];
					}
					else
					{
						rpadr[i].SetVector(padr[i]);
						mpadr[i] = rpadr[i];
					}
					
				}
			}
			else
			{
				for(i=0; i<pCnt; i++)
				{
					rpadr[i].SetVector(padr[i]);
					mpadr[i] = rpadr[i];
				}
			}
		}
		tData->SetMatrix(MR_REF_MATRIX,op->GetMg());
		tData->SetBool(MR_REFERENCE_SET,true);
		tData->SetLong(MR_POINT_COUNT,pCnt);
	}
	
	return true;
}

void CDMorphRefPlugin::CheckTagReg(BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdmnr = tData->GetString(T_STR);
	SerialInfo si;
	
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
			tData->SetBool(T_SET,true);
		}
	}
}

void CDMorphRefPlugin::RecalculateMorphRef(BaseDocument *doc, BaseObject *op, BaseTag *tag, Matrix newM, Matrix oldM)
{
	Vector newPt, oldPt;
	BaseContainer *mrData = tag->GetDataInstance();
	LONG i, pCount = mrData->GetLong(MR_POINT_COUNT);
	 
	for(i=0; i<rpadr.Size(); i++)
	{
		oldPt = rpadr[i].GetVector();
		newPt = MInv(newM) * oldM * oldPt;
		rpadr[i].SetVector(newPt);
	}

	if(doc)
	{
		CDMData mtd;
		PluginMessage(ID_CDMORPHTAGPLUGIN,&mtd);
		LONG mtCnt;
		BaseTag *mtTag = NULL;
		BaseContainer *mtData = NULL;
		if(mtd.list)
		{
			mtCnt = mtd.list->GetCount();
			for(i=0; i<mtCnt; i++)
			{
				mtTag = static_cast<BaseTag*>(mtd.list->GetIndex(i));
				if(mtTag->GetDocument())
				{
					mtData = mtTag->GetDataInstance();
					if(mtData)
					{
						if(op == mtData->GetObjectLink(MT_DEST_LINK,doc))
						{
							CDAddUndo(doc,CD_UNDO_CHANGE,mtTag);
							CDMorphTagPlugin *mt = static_cast<CDMorphTagPlugin*>(mtTag->GetNodeData());
							mt->RecalculateMorphPoints(mtTag,mtData,newM,oldM);
						}
					}
				}
			}
		}
	}
}

void CDMorphRefPlugin::RemampCDMorphTags(BaseDocument *doc, BaseObject *op, TranslationMaps *tMap)
{
	LONG i, oCnt = tMap->m_oPointCount, nCnt = tMap->m_nPointCount;
	LONG oInd, nInd, mCnt = tMap->m_mPointCount; 
	
	CDArray<LONG> remap;
	remap.Alloc(oCnt);
	
	for(i=0; i<oCnt; i++)
	{
		remap[i] = i;
	}
	
	for(i=0;i<mCnt;i++)
	{
		oInd = tMap->m_pPointMap[i].oIndex;
		nInd = tMap->m_pPointMap[i].nIndex;
		
		if(nCnt < oCnt)
		{
			if(oInd > -1 && nInd != oInd) remap[oInd] = nInd;
		}
		else if(nCnt > oCnt)
		{
			if(oInd > -1 && nInd > -1 && nInd != oInd) remap[oInd] = nInd;
		}
	}
	
	CDMData mtd;
	PluginMessage(ID_CDMORPHTAGPLUGIN,&mtd);
	LONG mtCnt;
	BaseTag *mtTag = NULL;
	BaseContainer *mtData = NULL;
	if(mtd.list)
	{
		mtCnt = mtd.list->GetCount();
		for(i=0; i<mtCnt; i++)
		{
			mtTag = static_cast<BaseTag*>(mtd.list->GetIndex(i));
			if(mtTag->GetDocument())
			{
				mtData = mtTag->GetDataInstance();
				if(mtData)
				{
					if(op == mtData->GetObjectLink(MT_DEST_LINK,doc))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,mtTag);
						CDMorphTagPlugin *mt = static_cast<CDMorphTagPlugin*>(mtTag->GetNodeData());
						mt->RemapMorphPoints(mtTag,mtData,remap.Array(),oCnt);
					}
				}
			}
		}
	}
	remap.Free();
}

void CDMorphRefPlugin::RemapReferencePoints(Vector *padr, TranslationMaps *map)
{
	LONG oCnt = map->m_oPointCount, nCnt = map->m_nPointCount;
	LONG i, oInd, nInd, mCnt = map->m_mPointCount;
	
	if(!rpadr.IsEmpty() && !mpadr.IsEmpty() && map)
	{
		// Create temp storage array
		CDArray<CDMRefPoint> tempRef;
		CDArray<CDMRefPoint> tempDef;
		
		rpadr.Copy(tempRef);
		rpadr.Resize(nCnt);
		
		mpadr.Copy(tempDef);
		mpadr.Resize(nCnt);
		
		for(i=0; i<mCnt; i++)
		{
			oInd = map->m_pPointMap[i].oIndex;
			nInd = map->m_pPointMap[i].nIndex;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_DELETED) continue;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_NEW)
			{
				rpadr[nInd].SetVector(padr[nInd]);
				mpadr[nInd].SetVector(padr[nInd]);
			}
			else if(nInd != oInd)
			{
				rpadr[nInd] = tempRef[oInd];
				mpadr[nInd] = tempDef[oInd];
			}
		}
		
		tempRef.Free();
		tempDef.Free();
	}
}

Bool CDMorphRefPlugin::TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg)
{
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, pCnt =ToPoint(op)->GetPointCount();
	
	TranslationMaps tMap;
	
	LONG nCnt = vchg->new_cnt, oCnt = vchg->old_cnt, mCnt, size;
	if(vchg->map)
	{
		tMap.m_oPointCount = oCnt; // old point count
		tMap.m_nPointCount = nCnt; // new point count
			
		if(nCnt < oCnt)
		{
			// Get remap count
			size = 0;
			for(i=0; i<oCnt; i++)
			{
				if(vchg->map[i] != i) size++;
			}
			tMap.m_mPointCount = size; // map point count
			
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
			
			if(doc) RemampCDMorphTags(doc,op,&tMap);
			
			tData->SetLong(MR_POINT_COUNT,nCnt);
			pMap.Free();
		}
		else if(nCnt > oCnt)
		{
			// Get remap count
			size = 0;
			for(i=0; i<nCnt; i++)
			{
				if(i<oCnt)
				{
					if(vchg->map[i] != i) size++;
				}
				else size++;
			}
			tMap.m_mPointCount = size; // map point count
			
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
			
			if(doc) RemampCDMorphTags(doc,op,&tMap);
			
			tData->SetLong(MR_POINT_COUNT,nCnt);
			pMap.Free();
		}
	}
	else
	{
		if(nCnt < oCnt)
		{
			if(!rpadr.IsEmpty() && !mpadr.IsEmpty()  && nCnt == pCnt && pCnt < tData->GetLong(MR_POINT_COUNT))
			{
				// resize storage array
				rpadr.Resize(nCnt);
				mpadr.Resize(nCnt);
	
				tData->SetLong(MR_POINT_COUNT,nCnt);
			}
		}
		else if(nCnt > oCnt)
		{
			tMap.m_oPointCount = oCnt; // old point count
			tMap.m_nPointCount = nCnt; // new point count
			
			size = nCnt - oCnt;
			tMap.m_mPointCount = size; // map point count
			
			// Allocate the m_pPointMap array
			CDArray<TransMapData> pMap;
			
			mCnt = 0;
			for(i=oCnt; i<nCnt; i++)
			{
				if(mCnt < size)
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
			
			if(doc) RemampCDMorphTags(doc,op,&tMap);
			
			if(op->IsInstanceOf(Offd)) tData->SetLong(MR_POINT_COUNT,nCnt);
			else tData->SetBool(MR_POINTS_ADDED,true);
			
			pMap.Free();
		}
		else if(nCnt == oCnt)
		{
			if(!rpadr.IsEmpty() && !mpadr.IsEmpty()  && tData->GetBool(MR_POINTS_ADDED))
			{
				LONG oldCnt = tData->GetLong(MR_POINT_COUNT);
				
				for(i=oldCnt; i<pCnt; i++)
				{
					rpadr[i].SetVector(padr[i]);
					mpadr[i].SetVector(padr[i]);
				}
				
				tData->SetBool(MR_POINTS_ADDED,false);
				tData->SetLong(MR_POINT_COUNT,pCnt);
			}
		}
	}
	
	return true;
}

Bool CDMorphRefPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdmnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdmnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdmnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdmnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdmnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdmnr);
			break;
		}
	}
	BaseDocument *doc = node->GetDocument();
	
	if(doc) CheckTagReg(op,tData);

	if(!IsValidPointObject(op)) return true;
	
	Vector theScale, refScale, curScale;

	BaseTag *skRefTag = op->GetTag(ID_CDSKINREFPLUGIN);
	LONG oldPCnt = tData->GetLong(MR_POINT_COUNT);
	
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			if(doc)
			{
				BaseObject *dst = tData->GetObjectLink(1008,doc);
				if(dst) tData->SetLink(T_OID,dst);
				
				if(tData->GetLong(MR_CDMR_LEVEL) < 1)
				{
					if(!InitReference(tag,doc,tData,op)) return true;
				}
			}
			break;
		}
		case MSG_MENUPREPARE:
		{
			if(!InitReference(tag,doc,tData,op)) return true;
			break;
		}
		case CD_MSG_RECALC_REFERENCE:
		{
			CDTransMatrixData *tmData = (CDTransMatrixData*)data;
			if(tmData)
			{
				RecalculateMorphRef(doc,op,tag,tmData->nM,tmData->oM);
			}
			break;
		}
		case MSG_POINTS_CHANGED:
		{
			VariableChanged *vchg = (VariableChanged*)data;
			if(vchg)
			{
				if(vchg->old_cnt == oldPCnt) TransferTMaps(tag,doc,tData,op,vchg);
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
					Vector *padr = GetPointArray(op);
					if(padr)
					{
						RemapReferencePoints(padr,tMap);
						
						if(doc) RemampCDMorphTags(doc,op,tMap);
						
						tData->SetLong(MR_POINT_COUNT,nCnt);
					}
				}
			}
			break;
		}
		case  CD_MSG_SCALE_CHANGE:
		{
			if(doc)
			{
				CDMData mtd;
				PluginMessage(ID_CDMORPHTAGPLUGIN,&mtd);
				LONG i, mtCnt;
				BaseTag *mtTag = NULL;
				BaseContainer *mtData = NULL;
				if(mtd.list)
				{
					mtCnt = mtd.list->GetCount();
					for(i=0; i<mtCnt; i++)
					{
						mtTag = static_cast<BaseTag*>(mtd.list->GetIndex(i));
						if(mtTag->GetDocument())
						{
							mtData = mtTag->GetDataInstance();
							if(mtData)
							{
								if(op == mtData->GetObjectLink(MT_DEST_LINK,doc))
								{
									mtTag->Message(CD_MSG_SCALE_CHANGE);
								}
							}
						}
					}
				}
			}
			tData->SetBool(MR_M_SCALING,false);
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==MR_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==MR_EDIT_MESH)
			{
				if(doc)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					doc->SetActiveObject(op);
					doc->SetMode(Mpoints);
					
					RestoreReference(tData, op);

					tData->SetBool(MR_EDITING,true);
					tData->SetBool(MR_REFERENCE_SET,false);
					
					BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
					if(skTag)
					{
						BaseContainer *sktData = skTag->GetDataInstance();
						if(sktData) sktData->SetBool(SKN_REF_EDITING,true);
					}
					
					doc->SetActiveTag(tag);
				}
			}
			else if(dc->id[0].id==MR_RESET_REF)
			{
				if(doc)
				{
					doc->StartUndo();
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					SetReference(tData, op);
					
					if(skRefTag)
					{
						DescriptionCommand skrdc;
						skrdc.id = DescID(MR_UPDATE_REFERENCE);
						skRefTag->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
					}
					tData->SetMatrix(MR_REF_MATRIX,op->GetMg());
					doc->EndUndo();
					
					tData->SetBool(MR_EDITING,false);
					
					BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
					if(skTag)
					{
						BaseContainer *sktData = skTag->GetDataInstance();
						if(sktData) sktData->SetBool(SKN_REF_EDITING,false);
					}
					
					doc->SetMode(Mobject);
				}
			}
			else if(dc->id[0].id==MR_UPDATE_REFERENCE)
			{
				if(doc)
				{
					Vector *padr = GetPointArray(op);
					if(padr)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						
						LONG i, rpCount = tData->GetLong(MR_POINT_COUNT);
						for(i=0; i<rpCount; i++)
						{
							rpadr[i].SetVector(padr[i]);
							mpadr[i] = rpadr[i];
						}
						tData->SetMatrix(MR_REF_MATRIX,op->GetMg());
					}
				}
			}
			break;
		}
	}

	return true;
}

LONG CDMorphRefPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
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
	if(tagMoved || tData->GetBool(T_MOV)) return false;
	
	if(mpadr.IsEmpty()) return false;
	if(!IsValidPointObject(op)) return false;

	Vector *padr = GetPointArray(op); if(!padr) return false;
	Vector refPt, opPt, newRefPt, theScale, refScale, curScale, storedScale;

	LONG i, pCnt = ToPoint(op)->GetPointCount();
	if(pCnt != tData->GetLong(MR_POINT_COUNT)) return false;
	
	if(!tData->GetBool(MR_REFERENCE_SET)) return false;
	
	BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
	BaseContainer *skData = NULL;
	if(skTag)  skData = skTag->GetDataInstance();

	Matrix refM = tData->GetMatrix(MR_REF_MATRIX), opM = op->GetMg();
	if(CDIsAxisMode(doc))
	{
		if(op->GetBit(BIT_ACTIVE))
		{
			if(opM != refM)
			{
				Vector newRefPt, refPt;
				for(i=0; i<pCnt; i++)
				{
					refPt = rpadr[i].GetVector();
					newRefPt = MInv(opM) * refM * refPt;
					rpadr[i].SetVector(newRefPt);
				}
				tData->SetMatrix(MR_REF_MATRIX,op->GetMg());
			}
		}
	}

	if(doc->IsEditMode() && (op->GetBit(BIT_ACTIVE))) return false;
	
	if(doc->GetMode() == Mmodel && doc->GetAction() == ID_MODELING_SCALE)
	{
		if(ActiveModelToolScaling(op) && !tData->GetBool(SKN_REF_EDITING))
		{
			SpecialEventAdd(ID_CDMORPHREFPLUGIN);
		}
		else
		{
			if(!mScaling)
			{
				mScaling = true;
				startScale = op->GetRad();
			}
			curScale = op->GetRad();				
			storedScale = tData->GetVector(MR_REFERENCE_SCALE);
			
			refScale.x = storedScale.x *(curScale.x / startScale.x);
			refScale.y = storedScale.x *(curScale.y / startScale.y);
			refScale.z = storedScale.x *(curScale.z / startScale.z);
			
			theScale.x = curScale.x / startScale.x;
			theScale.y = curScale.y / startScale.y;
			theScale.z = curScale.z / startScale.z;
			
			tData->SetVector(MR_SCALE,theScale);
			tData->SetVector(MR_REFERENCE_SCALE, refScale);
		}
	}
	else
	{
		if(mScaling)
		{
			storedScale = tData->GetVector(MR_SCALE);
			if(storedScale != Vector(1,1,1))
			{
				tData->SetBool(MR_M_SCALING,true);
				
				Matrix scaleM = MatrixScale(storedScale);
				for(i=0; i<pCnt; i++)
				{
					refPt = rpadr[i].GetVector();
					opPt = scaleM * refPt;
					rpadr[i].SetVector(opPt);
				}
				SendMTScalingInfo();
			}
			mScaling = false;
		}
		
		if(tData->GetBool(MR_M_SCALING)) return false;
		
		if(!skTag)
		{
			for(i=0; i<pCnt; i++)
			{
				padr[i] = rpadr[i].GetVector();
			}
		}
		else
		{
			if(!skData->GetBool(SKN_BOUND))
			{
				for(i=0; i<pCnt; i++)
				{
					padr[i] = rpadr[i].GetVector();
				}
			}
		}
	}
	
	if(!mpadr.IsEmpty())
	{
		for(i=0; i<pCnt; i++)
		{
			mpadr[i] = rpadr[i];
		}
	}
	tData->SetMatrix(MR_REF_MATRIX,op->GetMg());

	return CD_EXECUTION_RESULT_OK;
}

Bool CDMorphRefPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(MR_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDMorphRefPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	Bool bound = false;
	BaseObject *op = tag->GetObject();
	if(op)
	{
		BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
		if(skTag)
		{
			BaseContainer *skData = skTag->GetDataInstance();
			if(skData) bound = skData->GetBool(SKN_BOUND);
		}
	}
	
	switch (id[0].id)
	{
		case MR_EDIT_MESH:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(MR_EDITING) && !bound) return true;
			else return false;
		case MR_RESET_REF:
			if(!tData->GetBool(T_REG)) return false;	
			return tData->GetBool(MR_EDITING);
	}
	return true;
}

Bool RegisterCDMorphRefPlugin(void)
{
	if(CDFindPlugin(ID_CDMORPHREFPLUGIN,CD_TAG_PLUGIN)) return true;
	
	// decide by name ifthe plugin shall be registered - just foruser convenience     
	String name=GeLoadString(IDS_CDMORPHREF); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDMORPHREFPLUGIN,name,PLUGINFLAG_HIDE|TAG_EXPRESSION|TAG_VISIBLE,CDMorphRefPlugin::Alloc,"tCDMorphRef","CDMorphRef.tif",1);
}

// library functions
CDMRefPoint* iCDGetMorphReference(BaseTag *tag)
{
	CDMorphRefPlugin *mrTag = static_cast<CDMorphRefPlugin*>(tag->GetNodeData());
	return mrTag ? mrTag->GetReferenceArray() : NULL;
}

CDMRefPoint* iCDGetMorphCache(BaseTag *tag)
{
	CDMorphRefPlugin *mrTag = static_cast<CDMorphRefPlugin*>(tag->GetNodeData());
	return mrTag ? mrTag->GetMorphArray() : NULL;
}

