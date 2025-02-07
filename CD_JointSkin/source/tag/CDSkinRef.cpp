//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDTagData.h"

#include "CDJointSkin.h"
#include "CDSkinRefTag.h"

#include "tCDSkinRef.h"
#include "tCDCluster.h"


Bool CDSkinRefPlugin::Init(GeListNode *node)
{
	//GePrint("CDSkinRefPlugin::Init");
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	skinRef.Init();
	deformRef.Init();
	doubleCheck = false;
	
	tData->SetLong(SKR_CT_COUNT,0);
	tData->SetVector(SKR_SCALE, Vector(1,1,1));
	tData->SetVector(SKR_REFERENCE_SCALE, Vector(1,1,1));

	tData->SetBool(SKR_REF_EDITING,false);
	tData->SetBool(SKR_POINTS_ADDED,false);
	
	tData->SetBool(SKR_REF_MATRIX_FIX, false);

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	CDJSData srd;
	PluginMessage(ID_CDSKINREFPLUGIN,&srd);
	if(srd.list) srd.list->Append(node);
	
	return true;
}

void CDSkinRefPlugin::Free(GeListNode *node)
{
	skinRef.Free();
	deformRef.Free();
	
	CDJSData srd;
	PluginMessage(ID_CDSKINREFPLUGIN,&srd);
	if(srd.list) srd.list->Remove(node);
}

Bool CDSkinRefPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG i, pCnt = tData->GetLong(SKR_S_POINT_COUNT);
	
	if(level >= 0)
	{
		if(pCnt > 0)
		{
			if(!skinRef.Alloc(pCnt)) return false;
			for (i=0; i<pCnt; i++)
			{
				CDHFReadFloat(hf, &skinRef[i].x);
				CDHFReadFloat(hf, &skinRef[i].y);
				CDHFReadFloat(hf, &skinRef[i].z);
			}
		}
		if(pCnt > 0)
		{
			if(!deformRef.Alloc(pCnt)) return false;
			for (i=0; i<pCnt; i++)
			{
				CDHFReadFloat(hf, &deformRef[i].x);
				CDHFReadFloat(hf, &deformRef[i].y);
				CDHFReadFloat(hf, &deformRef[i].z);
			}
		}
		
	}
	return true;
}

Bool CDSkinRefPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG i, pCnt = tData->GetLong(SKR_S_POINT_COUNT);

	if(pCnt > 0)
	{
		//Level 0
		for (i=0; i<skinRef.Size(); i++)
		{
			CDHFWriteFloat(hf, skinRef[i].x);
			CDHFWriteFloat(hf, skinRef[i].y);
			CDHFWriteFloat(hf, skinRef[i].z);
		}
		
		for (i=0; i<deformRef.Size(); i++)
		{
			CDHFWriteFloat(hf, deformRef[i].x);
			CDHFWriteFloat(hf, deformRef[i].y);
			CDHFWriteFloat(hf, deformRef[i].z);
		}
	}
	
	return true;
}

Bool CDSkinRefPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag = (BaseTag*)snode; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG pCnt = tData->GetLong(SKR_S_POINT_COUNT);
	
	if(pCnt > 0)
	{
		skinRef.Copy(((CDSkinRefPlugin*)dest)->skinRef);
		deformRef.Copy(((CDSkinRefPlugin*)dest)->deformRef);
	}

	return true;
}

void CDSkinRefPlugin::RecalculateSkinRef(BaseTag *tag, Matrix newM, Matrix oldM)
{
	Vector newPt, oldPt;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, pCnt = tData->GetLong(SKR_S_POINT_COUNT);
	 
	for(i=0; i<pCnt; i++)
	{
		oldPt = skinRef[i].GetVector();
		newPt = MInv(newM) * oldM * oldPt;
		skinRef[i].SetVector(newPt);
	}
	
	BaseObject *op = tag->GetObject();
	if(op)
	{
		CDTransMatrixData tmData;
		tmData.nM = newM;
		tmData.oM = oldM;
		
		BaseTag *skTag = op->GetTag(ID_CDSKINPLUGIN);
		if(skTag) skTag->Message(CD_MSG_RECALC_REFERENCE,&tmData);
		
		CDJSData ctd;
		PluginMessage(ID_CDCLUSTERTAGPLUGIN,&ctd);
		LONG i, ctCnt;
		if(ctd.list)
		{
			ctCnt = ctd.list->GetCount();
			for(i=0; i<ctCnt; i++)
			{
				BaseTag *ctTag = static_cast<BaseTag*>(ctd.list->GetIndex(i));
				if(ctTag)
				{
					BaseDocument *doc = ctTag->GetDocument();
					if(doc)
					{
						BaseContainer *ctData = ctTag->GetDataInstance();
						if(ctData)
						{
							if(op == ctData->GetObjectLink(CLST_DEST_LINK,doc))
							{
								ctTag->Message(CD_MSG_RECALC_REFERENCE,&tmData);
							}
						}
					}
				}
			}
		}
	}
}

void CDSkinRefPlugin::RemampCDSkinClusterTags(BaseDocument *doc, BaseObject *op, TranslationMaps *tMap)
{
	CDJSData ctd;
	PluginMessage(ID_CDCLUSTERTAGPLUGIN,&ctd);
	LONG i, ctCnt;
	BaseTag *ctTag = NULL;
	BaseContainer *ctData = NULL;
	if(ctd.list)
	{
		ctCnt = ctd.list->GetCount();
		for(i=0; i<ctCnt; i++)
		{
			ctTag = static_cast<BaseTag*>(ctd.list->GetIndex(i));
			if(ctTag->GetDocument())
			{
				ctData = ctTag->GetDataInstance();
				if(ctData)
				{
					if(op == ctData->GetObjectLink(CLST_DEST_LINK,doc))
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,ctTag);
						CDRemapClusterWeights(ctTag, ctData, tMap);
					}
				}
			}
		}
	}
}

void CDSkinRefPlugin::RemapReferencePoints(Vector *padr, TranslationMaps *map)
{
	LONG oCnt = map->m_oPointCount, nCnt = map->m_nPointCount;
	LONG i, oInd, nInd, mCnt = map->m_mPointCount;
	
	if(!skinRef.IsEmpty() && !deformRef.IsEmpty() && map)
	{
		CDArray<CDMRefPoint> tempRef, tempDef;
		skinRef.Copy(tempRef);
		skinRef.Resize(nCnt);
		deformRef.Copy(tempDef);
		deformRef.Resize(nCnt);
		
		// Remap the changes
		for(i=0; i<mCnt; i++)
		{
			oInd = map->m_pPointMap[i].oIndex;
			nInd = map->m_pPointMap[i].nIndex;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_DELETED) continue;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_NEW)
			{
				skinRef[nInd].SetVector(padr[nInd]);
				deformRef[nInd].SetVector(padr[nInd]);
			}
			else if(nInd != oInd)
			{
				skinRef[nInd] = tempRef[oInd];
				deformRef[nInd] = tempDef[oInd];
			}
		}
		
		// Free old arrays
		tempRef.Free();
		tempDef.Free();
	}
}

Bool CDSkinRefPlugin::TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg)
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
					if(mCnt < size)
					{
						TransMapData tmd = TransMapData(i,vchg->map[i]);
						tmd.mIndex = mCnt;
						if(vchg->map[i] < 0) tmd.lFlags |= TRANSMAP_FLAG_DELETED;
						pMap.Append(tmd);
						mCnt++;
					}
				}
			}
			tMap.m_pPointMap = pMap.Array();
			
			if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			RemapReferencePoints(padr,&tMap);
			
			if(doc) RemampCDSkinClusterTags(doc,op,&tMap);
			
			tData->SetLong(SKR_S_POINT_COUNT,nCnt);
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
						if(mCnt < size)
						{
							TransMapData tmd = TransMapData(i,vchg->map[i]);
							tmd.mIndex = mCnt;
							if(vchg->map[i] < 0) tmd.lFlags |= TRANSMAP_FLAG_DELETED;
							pMap.Append(tmd);
							mCnt++;
						}
					}
				}
				else
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
			}
			tMap.m_pPointMap = pMap.Array();
			
			if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			RemapReferencePoints(padr,&tMap);
			
			if(doc) RemampCDSkinClusterTags(doc,op,&tMap);
			
			tData->SetLong(SKR_S_POINT_COUNT,nCnt);
			pMap.Free();
		}
	}
	else
	{
		if(nCnt > oCnt)
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
			
			if(doc) RemampCDSkinClusterTags(doc,op,&tMap);
			
			tData->SetBool(SKR_POINTS_ADDED,true);
			pMap.Free();
		}
		else if(nCnt == oCnt)
		{
			if(!skinRef.IsEmpty() && !deformRef.IsEmpty()  && tData->GetBool(SKR_POINTS_ADDED))
			{
				LONG oldCnt = tData->GetLong(SKR_S_POINT_COUNT);
				
				for(i=oldCnt; i<pCnt; i++)
				{
					skinRef[i].SetVector(padr[i]);
					deformRef[i].SetVector(padr[i]);
				}
				
				tData->SetBool(SKR_POINTS_ADDED,false);
				tData->SetLong(SKR_S_POINT_COUNT,pCnt);
			}
		}
	}
	
	return true;
}

static void RestoreReference(BaseObject *op, BaseContainer *tData, CDMRefPoint *sRef, CDMRefPoint *dRef)
{
	if(op && tData && sRef && dRef)
	{
		Vector *padr = GetPointArray(op);
		if(padr)
		{
			LONG pCnt = ToPoint(op)->GetPointCount();
			LONG i, skCnt = tData->GetLong(SKR_S_POINT_COUNT);
			
			for(i=0; i<skCnt; i++)
			{
				if(i< pCnt)
				{
					padr[i] = sRef[i].GetVector();
					dRef[i] = sRef[i];
				}
			}
			op->Message(MSG_UPDATE);
		}
	}
}

Bool CDSkinRefPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
	if(!tData->GetBool(SKR_REF_MATRIX_FIX))
	{
		tData->SetMatrix(SKR_REF_MATRIX,op->GetMln());
		tData->SetBool(SKR_REF_MATRIX_FIX,true);
	}
	if(deformRef.IsEmpty())
	{
		LONG pCnt =ToPoint(op)->GetPointCount();
		deformRef.Alloc(pCnt);
	}
	
	LONG oldPCnt = tData->GetLong(SKR_S_POINT_COUNT);
	
	switch (type)
	{
		case CD_MSG_RECALC_REFERENCE:
		{
			CDTransMatrixData *tmData = (CDTransMatrixData*)data;
			if(tmData)
			{
				RecalculateSkinRef(tag,tmData->nM,tmData->oM);
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
				Vector *padr = GetPointArray(op);
				if(padr)
				{
					LONG oCnt = tMap->m_oPointCount, nCnt = tMap->m_nPointCount;
					
					if(oCnt == oldPCnt && oCnt != nCnt)
					{
						if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						RemapReferencePoints(padr,tMap);
						
						if(doc) RemampCDSkinClusterTags(doc,op,tMap);
						
						tData->SetLong(SKR_S_POINT_COUNT,nCnt);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SKR_SET_REFERENCE)
			{
				Vector *padr = GetPointArray(op);
				if(padr)
				{
					LONG i, pCnt =ToPoint(op)->GetPointCount();
					tData->SetLong(SKR_S_POINT_COUNT,pCnt);
					
					skinRef.Resize(pCnt);
					deformRef.Resize(pCnt);
					
					Vector opPt;
					for(i=0; i<pCnt; i++)
					{
						skinRef[i].SetVector(padr[i]);
						deformRef[i] = skinRef[i];
					}
					tData->SetMatrix(SKR_REF_MATRIX,op->GetMln());
				}
			}
			else if(dc->id[0].id==SKR_RESTORE_REFERENCE)
			{
				RestoreReference(op,tData,skinRef.Array(),deformRef.Array());
			}
			else if(dc->id[0].id==SKR_UPDATE_REFERENCE)
			{
				Vector *padr = GetPointArray(op);
				if(padr)
				{
					if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					Vector opPt;
					LONG i, pCnt =ToPoint(op)->GetPointCount();
					for(i=0; i<pCnt; i++)
					{
						skinRef[i].SetVector(padr[i]);
						deformRef[i] = skinRef[i];
					}
					op->Message(MSG_UPDATE);
					tData->SetMatrix(SKR_REF_MATRIX,op->GetMln());
				}
			}
			else if(dc->id[0].id==SKR_DOUBLE_CHECK)
			{
				if(doc)
				{
					BaseList2D *theTag = tData->GetLink(SKR_TAG_LINK,doc);
					if(theTag)
					{
						doubleCheck = true;
					}
				}
			}
			break;
		}
	}

	return true;
}

LONG CDSkinRefPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return false;

	if(doubleCheck)
	{
		BaseList2D *theTag = tData->GetLink(SKR_TAG_LINK,doc);
		if(!theTag)
		{
			RestoreReference(op,tData,skinRef.Array(),deformRef.Array());
			doubleCheck = false;
		}
	}
	
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, pCnt =ToPoint(op)->GetPointCount(), skCnt = tData->GetLong(SKR_S_POINT_COUNT);
	if(pCnt != skCnt) return false;
	
	Vector refPt, opPt, newRefPt, theScale, refScale, curScale, storedScale;
	if(doc->GetMode() == Mmodel && doc->GetAction() == ID_MODELING_SCALE)
	{
		if(ActiveModelToolScaling(op) && !tData->GetBool(SKR_REF_EDITING))
		{
			SpecialEventAdd(ID_CDSKINREFPLUGIN);
		}
		else
		{
			if(!mScaling)
			{
				mScaling = true;
				startScale = op->GetRad();
			}
			curScale = op->GetRad();
			storedScale = tData->GetVector(SKR_REFERENCE_SCALE);
			
			refScale.x = storedScale.x *(curScale.x / startScale.x);
			refScale.y = storedScale.x *(curScale.y / startScale.y);
			refScale.z = storedScale.x *(curScale.z / startScale.z);
			
			theScale.x = curScale.x / startScale.x;
			theScale.y = curScale.y / startScale.y;
			theScale.z = curScale.z / startScale.z;
			
			tData->SetVector(SKR_SCALE,theScale);
			tData->SetVector(SKR_REFERENCE_SCALE, refScale);
		}
	}
	else
	{
		if(mScaling)
		{
			storedScale = tData->GetVector(SKR_SCALE);
			if(storedScale != Vector(1,1,1))
			{
				Matrix scaleM = MatrixScale(storedScale);
				for (i=0; i<pCnt; i++)
				{
					refPt = skinRef[i].GetVector();
					newRefPt = scaleM * refPt;
					skinRef[i].SetVector(newRefPt);
				}
			}
			mScaling = false;
		}
	}

	if(CDIsAxisMode(doc))
	{
		if(op->GetBit(BIT_ACTIVE))
		{
			Matrix refM = tData->GetMatrix(SKR_REF_MATRIX), opM = op->GetMln();
			if(opM != refM)
			{
				Vector newRefPt, refPt; 
				for(i=0; i<pCnt; i++)
				{
					refPt = skinRef[i].GetVector();
					newRefPt = MInv(opM) * refM * refPt;
					skinRef[i].SetVector(newRefPt);
				}
				tData->SetMatrix(SKR_REF_MATRIX,op->GetMl());
			}
		}
	}
	
	if(!deformRef.IsEmpty())
	{
		Vector opPt;
		for(i=0; i<pCnt; i++)
		{
			deformRef[i].SetVector(padr[i]);
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool RegisterCDSkinRefPlugin(void)
{
	if(CDFindPlugin(ID_CDSKINREFPLUGIN,CD_TAG_PLUGIN)) return true;
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDSKINREF); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSKINREFPLUGIN,name,TAG_EXPRESSION|PLUGINFLAG_HIDE,CDSkinRefPlugin::Alloc,"tCDSkinRef","CDSkin.tif",0);
}

// library function
CDMRefPoint* iCDGetSkinReference(BaseTag *tag)
{
	CDSkinRefPlugin *srTag = static_cast<CDSkinRefPlugin*>(tag->GetNodeData());
	return srTag ? srTag->GetSkinReference() : NULL;
}

CDMRefPoint* iCDGetDeformReference(BaseTag *tag)
{
	CDSkinRefPlugin *srTag = static_cast<CDSkinRefPlugin*>(tag->GetNodeData());
	return srTag ? srTag->GetDeformReference() : NULL;
}

