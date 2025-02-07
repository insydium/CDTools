//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "CDTagData.h"

#include "CDJointSkin.h"
#include "CDClusterTag.h"
#include "CDSkinRefTag.h"
#include "CDJoint.h"

#include "tCDCluster.h"

static Vector GetGlobalMatrixScale(Matrix m)
{
	Vector s;
	
	s.x = Len(m.v1);
	s.y = Len(m.v2);
	s.z = Len(m.v3);
	
	return s;
}

Bool CDClusterTagPlugin::Init(GeListNode *node)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(CLST_MIX_PREVIOUS,true);
	tData->SetBool(CLST_BOUND, false);
	tData->SetBool(CLST_DEST_SET,false);
	tData->SetBool(CLST_T_POINT_SET,false);
	tData->SetBool(CLST_SHOW_GUIDE,true);
	tData->SetVector(CLST_LINE_COLOR, Vector(0,1,1));
	tData->SetReal(CLST_GUIDE_SIZE, 10.0);
	tData->SetLong(CLST_C_POINT_COUNT,0);
	
	tData->SetBool(CLST_CHECK_REF,false);

	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	cWeight.Init();
	
	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_EXPRESSION);
			pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,250);
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	CDJSData scd;
	PluginMessage(ID_CDCLUSTERTAGPLUGIN,&scd);
	if(scd.list) scd.list->Append(node);
	
	return true;
}

void CDClusterTagPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);

	CHAR b;
	String kb, cdjnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
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
			
			tData->SetLink(CLST_DEST_LINK+T_LST,tData->GetLink(CLST_DEST_LINK,doc));
			tData->SetBool(CLST_MIX_PREVIOUS+T_LST,tData->GetBool(CLST_MIX_PREVIOUS));
			tData->SetBool(CLST_LOCAL_TRANS+T_LST,tData->GetBool(CLST_LOCAL_TRANS));
			
			tData->SetBool(CLST_BLEND_ROTATION+T_LST,tData->GetBool(CLST_BLEND_ROTATION));
			
			tData->SetBool(CLST_ATTACH_CHILD+T_LST,tData->GetBool(CLST_ATTACH_CHILD));
			tData->SetBool(CLST_ATTACH_EXTERNAL+T_LST,tData->GetBool(CLST_ATTACH_EXTERNAL));
			tData->SetLink(CLST_ATTACH_LINK+T_LST,tData->GetLink(CLST_ATTACH_LINK,doc));
			
			tData->SetVector(CLST_JOINT_OFFSET+T_LST,tData->GetVector(CLST_JOINT_OFFSET));
			tData->SetBool(CLST_ENABLE_PIVOT+T_LST,tData->GetBool(CLST_ENABLE_PIVOT));
			tData->SetVector(CLST_PIVOT_OFFSET+T_LST,tData->GetVector(CLST_PIVOT_OFFSET));
			tData->SetVector(CLST_CHILD_OFFSET+T_LST,tData->GetVector(CLST_CHILD_OFFSET));
			tData->SetBool(CLST_RESTRICT_TO_Z+T_LST,tData->GetBool(CLST_RESTRICT_TO_Z));
			tData->SetVector(CLST_STRENGTH+T_LST,tData->GetVector(CLST_STRENGTH));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDClusterTagPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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

		tData->SetLink(CLST_DEST_LINK,tData->GetLink(CLST_DEST_LINK+T_LST,doc));
		tData->SetBool(CLST_MIX_PREVIOUS,tData->GetBool(CLST_MIX_PREVIOUS+T_LST));
		tData->SetBool(CLST_LOCAL_TRANS,tData->GetBool(CLST_LOCAL_TRANS+T_LST));
		
		tData->SetBool(CLST_BLEND_ROTATION,tData->GetBool(CLST_BLEND_ROTATION+T_LST));
		
		tData->SetBool(CLST_ATTACH_CHILD,tData->GetBool(CLST_ATTACH_CHILD+T_LST));
		tData->SetBool(CLST_ATTACH_EXTERNAL,tData->GetBool(CLST_ATTACH_EXTERNAL+T_LST));
		tData->SetLink(CLST_ATTACH_LINK,tData->GetLink(CLST_ATTACH_LINK,doc+T_LST));
		
		tData->SetVector(CLST_JOINT_OFFSET,tData->GetVector(CLST_JOINT_OFFSET+T_LST));
		tData->SetBool(CLST_ENABLE_PIVOT,tData->GetBool(CLST_ENABLE_PIVOT+T_LST));
		tData->SetVector(CLST_PIVOT_OFFSET,tData->GetVector(CLST_PIVOT_OFFSET+T_LST));
		tData->SetVector(CLST_CHILD_OFFSET,tData->GetVector(CLST_CHILD_OFFSET+T_LST));
		tData->SetBool(CLST_RESTRICT_TO_Z,tData->GetBool(CLST_RESTRICT_TO_Z+T_LST));
		tData->SetVector(CLST_STRENGTH,tData->GetVector(CLST_STRENGTH+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDClusterTagPlugin::Free(GeListNode *node)
{
	cWeight.Free();

	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	BaseDocument *doc = node->GetDocument();

	BaseObject *op = tData->GetObjectLink(CLST_DEST_LINK,doc);
	if(op)
	{
		BaseTag *skRefTag = op->GetTag(ID_CDSKINREFPLUGIN);
		if(skRefTag)
		{
			BaseContainer *skrData = skRefTag->GetDataInstance();
			skrData->SetLink(SKR_TAG_LINK,tag);
			
			DescriptionCommand dc;
			dc.id = DescID(SKR_DOUBLE_CHECK);
			skRefTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
		}
	}
	
	CDJSData scd;
	PluginMessage(ID_CDCLUSTERTAGPLUGIN,&scd);
	if(scd.list) scd.list->Remove(node);
}

Bool CDClusterTagPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG cptCount = tData->GetLong(CLST_C_POINT_COUNT);
	LONG i, tCount = tData->GetLong(CLST_T_POINT_COUNT); // T_POINT_COUNT for backward compatibility
	
	if(level < 1) // previous version
	{
		if(tCount > 0)
		{
			for (i=0; i<tCount; i++)
			{
				Real temp;
				CDHFReadFloat(hf, &temp);
			}
		}
		if(cptCount > 0)
		{
			if(!cWeight.Alloc(cptCount)) return false;
			for (i=0; i<cptCount; i++)
			{
				CDHFReadLong(hf, &cWeight[i].ind);
				CDHFReadFloat(hf, &cWeight[i].w);
			}
		}
		if(cptCount > 0)
		{
			for (i=0; i<cptCount; i++)
			{
				Vector cRef;
				CDHFReadFloat(hf, &cRef.x);
				CDHFReadFloat(hf, &cRef.y);
				CDHFReadFloat(hf, &cRef.z);
			}
		}
	}
	else
	{
		if(cptCount > 0)
		{
			if(!cWeight.Alloc(cptCount)) return false;
			for (i=0; i<cptCount; i++)
			{
				CDHFReadLong(hf, &cWeight[i].ind);
				CDHFReadFloat(hf, &cWeight[i].w);
			}
		}
	}
	
	return true;
}

Bool CDClusterTagPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG i, cptCount = tData->GetLong(CLST_C_POINT_COUNT);

	//Level 1
	if(cptCount > 0)
	{
		for (i=0; i<cWeight.Size(); i++)
		{
			CDHFWriteLong(hf, cWeight[i].ind);
			CDHFWriteFloat(hf, cWeight[i].w);
		}
	}
	
	return true;
}

Bool CDClusterTagPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag *tag  = (BaseTag*)snode; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG cptCount = tData->GetLong(CLST_C_POINT_COUNT);

	if(cWeight.Size() > 0)
	{
		cWeight.Copy(((CDClusterTagPlugin*)dest)->cWeight);
	}
	
	return true;
}

Bool CDClusterTagPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseContainer *tData = tag->GetDataInstance();
	BaseDocument	*doc = tag->GetDocument(); if(!doc) return true;
	
	if(!tData->GetBool(CLST_ATTACH_CHILD)) return true;
	if(!tData->GetBool(CLST_SHOW_GUIDE)) return true;
	if(!op->GetDown() && !tData->GetObjectLink(CLST_ATTACH_LINK,doc)) return true;
	
	bd->SetPen(tData->GetVector(CLST_LINE_COLOR));
	
	Matrix opMatrix = op->GetMg(), childMatrix;
	if(!tData->GetBool(CLST_ATTACH_EXTERNAL))
	{
		if(!op->GetDown()) return true;
		else childMatrix = op->GetDown()->GetMg();
	}
	else
	{
		if(!tData->GetObjectLink(CLST_ATTACH_LINK,doc)) return true;
		else childMatrix = tData->GetObjectLink(CLST_ATTACH_LINK,doc)->GetMg();
	}
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Matrix pivotMatrix = opMatrix; pivotMatrix.off = childMatrix.off;
	Matrix opGuideM = opMatrix, chGuideM = childMatrix, pvGuideM = pivotMatrix;
	Vector startPt, endPt, drawDirection, drawPosition;
	Real theMix, guideSize = tData->GetReal(CLST_GUIDE_SIZE);
	
	opGuideM.off = opMatrix * tData->GetVector(CLST_JOINT_OFFSET);
	chGuideM.off = childMatrix * tData->GetVector(CLST_CHILD_OFFSET);
	
	drawDirection = VNorm(opGuideM.v1);
	drawPosition = opGuideM.off + drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, opGuideM.off);
	drawPosition = opGuideM.off - drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, opGuideM.off);
	
	drawDirection = VNorm(opGuideM.v2);
	drawPosition = opGuideM.off + drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, opGuideM.off);
	drawPosition = opGuideM.off - drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, opGuideM.off);
	
	drawDirection = VNorm(opGuideM.v3);
	drawPosition = opGuideM.off + drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, opGuideM.off);
	drawPosition = opGuideM.off - drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, opGuideM.off);

	drawDirection = VNorm(chGuideM.v1);
	drawPosition = chGuideM.off + drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, chGuideM.off);
	drawPosition = chGuideM.off - drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, chGuideM.off);
	
	drawDirection = VNorm(chGuideM.v2);
	drawPosition = chGuideM.off + drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, chGuideM.off);
	drawPosition = chGuideM.off - drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, chGuideM.off);
	
	drawDirection = VNorm(chGuideM.v3);
	drawPosition = chGuideM.off + drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, chGuideM.off);
	drawPosition = chGuideM.off - drawDirection * guideSize;
	CDDrawLine(bd,drawPosition, chGuideM.off);
	
	theMix = tData->GetReal(CLST_POS_MIX);
	Vector theAxis;
	if(!tData->GetBool(CLST_ENABLE_PIVOT))
	{
		CDDrawLine(bd,opGuideM.off, chGuideM.off);
		theAxis = CDBlend(opGuideM.off,chGuideM.off,theMix);
		if(tData->GetBool(CLST_RESTRICT_TO_Z))
		{
			Real zPos = Len(theAxis - opMatrix.off);
			theAxis = opMatrix.off + VNorm(opMatrix.v3) * zPos;
		}
	}
	else
	{
		pvGuideM.off = pivotMatrix * tData->GetVector(CLST_PIVOT_OFFSET);
		
		drawDirection = VNorm(pvGuideM.v1);
		drawPosition = pvGuideM.off + drawDirection * guideSize;
		CDDrawLine(bd,drawPosition, pvGuideM.off);
		drawPosition = pvGuideM.off - drawDirection * guideSize;
		CDDrawLine(bd,drawPosition, pvGuideM.off);
		
		drawDirection = VNorm(pvGuideM.v2);
		drawPosition = pvGuideM.off + drawDirection * guideSize;
		CDDrawLine(bd,drawPosition, pvGuideM.off);
		drawPosition = pvGuideM.off - drawDirection * guideSize;
		CDDrawLine(bd,drawPosition, pvGuideM.off);
		
		drawDirection = VNorm(pvGuideM.v3);
		drawPosition = pvGuideM.off + drawDirection * guideSize;
		CDDrawLine(bd,drawPosition, pvGuideM.off);
		drawPosition = pvGuideM.off - drawDirection * guideSize;
		CDDrawLine(bd,drawPosition, pvGuideM.off);

		CDDrawLine(bd,opGuideM.off, pvGuideM.off);
		CDDrawLine(bd,pvGuideM.off, chGuideM.off);

		Real maxLen = Len(chGuideM.off - childMatrix.off) + Len(pvGuideM.off - childMatrix.off) + Len(pvGuideM.off - opGuideM.off);
		Real curLen = Len(pvGuideM.off - opGuideM.off) + Len(chGuideM.off - pvGuideM.off);
		Real mixAdd = theMix * (curLen/maxLen);
		theAxis = CDBlend(opGuideM.off,pvGuideM.off,mixAdd);
	}
	
	bd->SetPen(Vector(1,0,0));
	drawDirection = VNorm(opGuideM.v1);
	drawPosition = theAxis + drawDirection * guideSize*2;
	CDDrawLine(bd,drawPosition, theAxis);
	
	bd->SetPen(Vector(0,1,0));
	drawDirection = VNorm(opGuideM.v2);
	drawPosition = theAxis + drawDirection * guideSize*2;
	CDDrawLine(bd,drawPosition, theAxis);
	
	bd->SetPen(Vector(0,0,1));
	drawDirection = VNorm(opGuideM.v3);
	drawPosition = theAxis + drawDirection * guideSize*2;
	CDDrawLine(bd,drawPosition, theAxis);

	return true;
}

Bool CDClusterTagPlugin::GetWeight(BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	ClearMem(tWeight,(sizeof(Real)*tCnt),0);
	if(!cWeight.IsEmpty())
	{
		LONG i, cInd;
		for(i=0; i<cWeight.Size(); i++)
		{
			cInd = cWeight[i].ind;
			if(cInd < tCnt)
			{
				tWeight[cInd] = cWeight[i].w;
			}
		}
	}
	
	return true;
}

Bool CDClusterTagPlugin::SetWeight(BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	LONG i, cCnt = 0;
	for(i=0; i<tCnt; i++)
	{
		if(tWeight[i] > 0.0)  cCnt++;
	}
	
	cWeight.Resize(cCnt);
	
	cCnt = 0;
	for(i=0; i<tCnt; i++)
	{
		Real wt = tWeight[i];
		if(tWeight[i] > 0.0)
		{
			cWeight[cCnt].ind = i;
			cWeight[cCnt].w = TruncatePercentFractions(wt);
			cCnt++;
		}
	}
	tData->SetLong(CLST_C_POINT_COUNT,cCnt);
	
	return true;
}

Bool CDClusterTagPlugin::RemapClusterWeights(BaseContainer *tData, TranslationMaps *map)
{
	if(map && map->m_pPointMap && map->m_nPointCount)
	{
		LONG oCnt = map->m_oPointCount, nCnt = map->m_nPointCount;
		LONG i, oInd, nInd, mCnt = map->m_mPointCount;
		
		CDArray<Real> tWeight, rmWeight;
		tWeight.Alloc(oCnt);
		
		if(GetWeight(tData,tWeight.Array(),oCnt))
		{
			tWeight.Copy(rmWeight);
			rmWeight.Resize(nCnt);
			
			for(i=0; i<mCnt; i++)
			{
				oInd = map->m_pPointMap[i].oIndex;
				nInd = map->m_pPointMap[i].nIndex;
				
				if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_DELETED) continue;
				
				if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_NEW)
				{
					rmWeight[nInd] = 0.0;
				}
				else if(nInd != oInd)
				{
					rmWeight[nInd] = tWeight[oInd];
				}
			}
			SetWeight(tData,rmWeight.Array(),nCnt);
		}
		
		rmWeight.Free();
		tWeight.Free();
	}
	
	return true;
}

Bool CDClusterTagPlugin::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("CDClusterTagPlugin::Message");
	BaseTag	*tag = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdjnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdjnr);
			break;
		}
	}
	BaseObject *op = tag->GetObject(); if(!op) return true;
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	CheckTagReg(doc,op,tData);
	
	BaseObject *trg = tData->GetObjectLink(CLST_DEST_LINK,doc); if(!trg) return true;
	if(!IsValidPointObject(trg)) return true;
	
	if(!tData->GetBool(CLST_DEST_SET))
	{
		tData->SetLink(CLST_DEST_ID,trg);
		tData->SetBool(CLST_DEST_SET,true);
	}
	
	// Check for SkinRef tag
	DescriptionCommand skrdc;
	BaseTag *skRefTag = trg->GetTag(ID_CDSKINREFPLUGIN);
	if(!skRefTag)
	{
		if(!tData->GetBool(CLST_CHECK_REF))
		{
			tData->SetBool(CLST_CHECK_REF,true);
		}
		else
		{
			if(op == tData->GetObjectLink(T_OID,doc))
			{
				skRefTag = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
				op->InsertTag(skRefTag,NULL);
				op->Message(MSG_UPDATE);
				
				skrdc.id = DescID(SKR_SET_REFERENCE);
				skRefTag->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
				tData->SetBool(CLST_CHECK_REF,false);
			}
		}
	}
	if(!skRefTag) return true;

	switch (type)
	{
		case CD_MSG_NORMALIZE_WEIGHT:
		{
			if(!cWeight.IsEmpty())
			{
				LONG i, cCnt = tData->GetLong(CLST_C_POINT_COUNT) ;
				for(i=0; i<cCnt; i++)
				{
					Real wt = cWeight[i].w;
					cWeight[i].w = TruncatePercentFractions(wt);
				}
			}
			break;
		}
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			Vector *trnsSca = (Vector*)data;
			if(trnsSca)
			{
				Vector sca = *trnsSca;
				Matrix refM;
				
				refM = tData->GetMatrix(CLST_DEST_MATRIX);
				refM.off.x *= sca.x;
				refM.off.y *= sca.y;
				refM.off.z *= sca.z;
				tData->SetMatrix(CLST_DEST_MATRIX,refM);
				
				refM = tData->GetMatrix(CLST_B_REF_MATRIX);
				refM.off.x *= sca.x;
				refM.off.y *= sca.y;
				refM.off.z *= sca.z;
				tData->SetMatrix(CLST_B_REF_MATRIX,refM);
				
				refM = tData->GetMatrix(CLST_C_REF_MATRIX);
				refM.off.x *= sca.x;
				refM.off.y *= sca.y;
				refM.off.z *= sca.z;
				tData->SetMatrix(CLST_C_REF_MATRIX,refM);
				
				refM = tData->GetMatrix(CLST_REF_MATRIX);
				refM.off.x *= sca.x;
				refM.off.y *= sca.y;
				refM.off.z *= sca.z;
				tData->SetMatrix(CLST_REF_MATRIX,refM);
				
				refM = tData->GetMatrix(CLST_GPL_MATRIX);
				refM.off.x *= sca.x;
				refM.off.y *= sca.y;
				refM.off.z *= sca.z;
				tData->SetMatrix(CLST_GPL_MATRIX,refM);
			}
			break;
		}
		case CD_MSG_RECALC_REFERENCE:
		{
			CDTransMatrixData *tmData = (CDTransMatrixData*)data;
			if(tmData)
			{
				tData->SetMatrix(CLST_DEST_MATRIX,tmData->nM);
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==CLST_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==CLST_UNBIND_POINTS)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
				tData->SetBool(CLST_BOUND,false);
				if(skRefTag)
				{
					DescriptionCommand dc;
					dc.id = DescID(SKR_RESTORE_REFERENCE);
					skRefTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
				}
			}
			else if(dc->id[0].id==CLST_BIND_POINTS)
			{
				LONG pCount = ToPoint(trg)->GetPointCount();
				
				Matrix opMatrix = op->GetMg();
				CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,tag);
				tData->SetMatrix(CLST_REF_MATRIX,opMatrix);
				tData->SetMatrix(CLST_DEST_MATRIX,trg->GetMg());
				
				Matrix bMatrix, pMatrix = Matrix(), gM = Matrix();;
				BaseObject *pr = op->GetUp();
				if(pr)
				{
					pMatrix = pr->GetMg();
					bMatrix = MInv(pMatrix) * opMatrix;
					
					if(pr->GetUp())
					{
						gM = pr->GetUp()->GetMg();
					}
					tData->SetMatrix(CLST_GPL_MATRIX,MInv(gM) * opMatrix);
				}
				tData->SetMatrix(CLST_B_REF_MATRIX,bMatrix);
								
				
				if(tData->GetBool(CLST_ATTACH_CHILD))
				{
					if(!tData->GetBool(CLST_ATTACH_EXTERNAL) && op->GetDown())
					{
						Matrix jTransM = opMatrix, childMatrix = op->GetDown()->GetMg();
						tData->SetMatrix(CLST_C_REF_MATRIX,childMatrix);
						Matrix pivotMatrix = jTransM; pivotMatrix.off = childMatrix.off;
						Matrix opGuideM = opMatrix, chGuideM = childMatrix, pvGuideM = pivotMatrix;
						Real theMix = tData->GetReal(CLST_POS_MIX);
						
						opGuideM.off = opMatrix * tData->GetVector(CLST_JOINT_OFFSET);
						chGuideM.off = childMatrix * tData->GetVector(CLST_CHILD_OFFSET);
						if(!tData->GetBool(CLST_ENABLE_PIVOT))
						{
							jTransM.off = CDBlend(opGuideM.off,chGuideM.off,theMix);
							if(tData->GetBool(CLST_RESTRICT_TO_Z))
							{
								Real zPos = Len(jTransM.off - opMatrix.off);
								jTransM.off = opMatrix.off + VNorm(opMatrix.v3) * zPos;
							}
						}
						else
						{
							pvGuideM.off = pivotMatrix * tData->GetVector(CLST_PIVOT_OFFSET);
							Real maxLen = Len(chGuideM.off - childMatrix.off) + Len(pvGuideM.off - childMatrix.off) + Len(pvGuideM.off - opGuideM.off);
							Real curLen = Len(pvGuideM.off - opGuideM.off) + Len(chGuideM.off - pvGuideM.off);
							Real mixAdd = theMix * (curLen/maxLen);
							jTransM.off = CDBlend(opGuideM.off,pvGuideM.off,mixAdd);
						}
						tData->SetMatrix(CLST_REF_MATRIX,jTransM);
					}
					else if(tData->GetBool(CLST_ATTACH_EXTERNAL) && tData->GetObjectLink(CLST_ATTACH_LINK,doc))
					{
						Matrix jTransM = opMatrix, childMatrix = tData->GetObjectLink(CLST_ATTACH_LINK,doc)->GetMg();
						tData->SetMatrix(CLST_C_REF_MATRIX,childMatrix);
						Matrix pivotMatrix = jTransM; pivotMatrix.off = childMatrix.off;
						Matrix opGuideM = opMatrix, chGuideM = childMatrix, pvGuideM = pivotMatrix;
						Real theMix = tData->GetReal(CLST_POS_MIX);
						
						opGuideM.off = opMatrix * tData->GetVector(CLST_JOINT_OFFSET);
						chGuideM.off = childMatrix * tData->GetVector(CLST_CHILD_OFFSET);
						if(!tData->GetBool(CLST_ENABLE_PIVOT))
						{
							jTransM.off = CDBlend(opGuideM.off,chGuideM.off,theMix);
							if(tData->GetBool(CLST_RESTRICT_TO_Z))
							{
								Real zPos = Len(jTransM.off - opMatrix.off);
								jTransM.off = opMatrix.off + VNorm(opMatrix.v3) * zPos;
							}
						}
						else
						{
							pvGuideM.off = pivotMatrix * tData->GetVector(CLST_PIVOT_OFFSET);
							Real maxLen = Len(chGuideM.off - childMatrix.off) + Len(pvGuideM.off - childMatrix.off) + Len(pvGuideM.off - opGuideM.off);
							Real curLen = Len(pvGuideM.off - opGuideM.off) + Len(chGuideM.off - pvGuideM.off);
							Real mixAdd = theMix * (curLen/maxLen);
							jTransM.off = CDBlend(opGuideM.off,pvGuideM.off,mixAdd);
						}
						tData->SetMatrix(CLST_REF_MATRIX,jTransM);
					}
				}
				tData->SetLong(CLST_R_POINT_COUNT,pCount);
				tData->SetBool(CLST_BOUND,true);
			}
			break;
		}
	}

	return true;
}

LONG CDClusterTagPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseDraw		*bd = doc->GetRenderBaseDraw(); if(!bd) return false;
	if(!op->GetUp()) tData->SetBool(CLST_BLEND_ROTATION,false);
	if(tData->GetBool(CLST_BLEND_ROTATION)) tData->SetBool(CLST_ATTACH_CHILD,false);

	BaseObject *trg = tData->GetObjectLink(CLST_DEST_LINK,doc); if(!trg) return false;
	if(!IsValidPointObject(trg))
	{
		return false;
	}
	if(trg == op)  return false;
	
	if(trg != tData->GetObjectLink(CLST_DEST_ID,doc))
	{
		tData->SetLink(CLST_DEST_ID, trg);
		tData->SetBool(CLST_BOUND, false);
		tData->SetBool(CLST_DEST_SET,false);
	}
	
	BaseTag *skRefTag = trg->GetTag(ID_CDSKINREFPLUGIN);
	if(!skRefTag) return false;
	
	CDMRefPoint *skinRef = CDGetSkinReference(skRefTag); if(!skinRef) return false;

	LONG i, ptCount = ToPoint(trg)->GetPointCount();
	if(!tData->GetBool(CLST_DEST_SET)) return false;
	if(!tData->GetBool(CLST_BOUND)) return false;

	BaseTag *refTag = trg->GetTag(ID_CDMORPHREFPLUGIN);
	if(refTag)
	{
		BaseContainer *refTData = refTag->GetDataInstance();
		if(refTData->GetBool(1003))
		{
			skinRef = CDGetMorphCache(refTag);
		}
	}

	if(doc->GetMode() == Mmodel && doc->GetAction() == ID_MODELING_SCALE)
	{
		if(ActiveModelToolScaling(trg)) return false;
	}
	if(doc->IsEditMode() && (trg->GetBit(BIT_ACTIVE))) return false;
	
	Vector  refPt, *padr = GetPointArray(trg); if(!padr) return false;
	Matrix  jMatrix, mMatrix, scaleMg, jTransM = op->GetMg(), meshMatrix = trg->GetMg();
	
	// Get the Bind Pose reference matrices
	jMatrix = tData->GetMatrix(CLST_REF_MATRIX);
	mMatrix = tData->GetMatrix(CLST_DEST_MATRIX);
	
	BaseObject *pr = op->GetUp();
	if(!pr) tData->SetBool(CLST_LOCAL_TRANS, false);
	
	if(tData->GetBool(CLST_LOCAL_TRANS))
	{
		Matrix pMatrix = pr->GetMg();
		if(pr->GetUp())
		{
			Matrix gplM = tData->GetMatrix(CLST_GPL_MATRIX);
			jTransM = pr->GetUp()->GetMg() * gplM * op->GetMl();
		}
		else jTransM = jMatrix * op->GetMl();
	}
	if(tData->GetBool(CLST_BLEND_ROTATION))
	{
		Matrix bMatrix, pMatrix = Matrix();
		if(pr)
		{
			pMatrix = pr->GetMg();
		}
		CDQuaternion opQuat, bQuat, newQuat;
		opQuat.SetMatrix(jTransM);
		bMatrix = pMatrix * tData->GetMatrix(CLST_B_REF_MATRIX);
		bQuat.SetMatrix(bMatrix);
		
		Real theMix = tData->GetReal(CLST_ROT_MIX);
		newQuat = CDQSlerp(opQuat, bQuat, theMix);
		
		scaleMg = MatrixScale(GetGlobalMatrixScale(jTransM));
		Matrix qMatrix = newQuat.GetMatrix() * scaleMg;
		qMatrix.off = jTransM.off;
		jTransM = qMatrix;
	}
	if(tData->GetBool(CLST_ATTACH_CHILD))
	{
		if(!tData->GetBool(CLST_ATTACH_EXTERNAL) && op->GetDown())
		{
			Matrix opMatrix = jTransM, childMatrix = op->GetDown()->GetMg();
			Matrix pivotMatrix = jTransM; pivotMatrix.off = childMatrix.off;
			Matrix opGuideM = jTransM, chGuideM = childMatrix, pvGuideM = pivotMatrix;
			Matrix sMatrix, skRefMatrix = tData->GetMatrix(CLST_C_REF_MATRIX);
			Real theMix = tData->GetReal(CLST_POS_MIX), refLen, curLen;
			Vector theScale, cRefPos, jRefPos, sStrength = tData->GetVector(CLST_STRENGTH);
			
			opGuideM.off = opMatrix * tData->GetVector(CLST_JOINT_OFFSET);
			chGuideM.off = childMatrix * tData->GetVector(CLST_CHILD_OFFSET);
			if(!tData->GetBool(CLST_ENABLE_PIVOT))
			{
				jTransM.off = CDBlend(opGuideM.off,chGuideM.off,theMix);
				if(tData->GetBool(CLST_RESTRICT_TO_Z))
				{
					Real zPos = Len(jTransM.off - opMatrix.off);
					jTransM.off = opMatrix.off + VNorm(opMatrix.v3) * zPos;
				}
				cRefPos = skRefMatrix * tData->GetVector(CLST_CHILD_OFFSET);
				jRefPos = jMatrix * tData->GetVector(CLST_JOINT_OFFSET);
				refLen = Len(cRefPos - jRefPos);
				curLen = Len(chGuideM.off - opGuideM.off);
			}
			else
			{
				pvGuideM.off = pivotMatrix * tData->GetVector(CLST_PIVOT_OFFSET);
				refLen = Len(chGuideM.off - childMatrix.off) + Len(pvGuideM.off - childMatrix.off) + Len(pvGuideM.off - opGuideM.off);
				curLen = Len(pvGuideM.off - opGuideM.off) + Len(chGuideM.off - pvGuideM.off);
				Real mixAdd = theMix * (curLen/refLen);
				jTransM.off = CDBlend(opGuideM.off,pvGuideM.off,mixAdd);
			}
			theScale.x = CDBlend(1.0,Real(refLen/curLen),sStrength.x);
			theScale.y = CDBlend(1.0,Real(refLen/curLen),sStrength.y);
			theScale.z = CDBlend(1.0,Real(curLen/refLen),sStrength.z);
			
			sMatrix = jTransM;
			Matrix scaleM = MatrixScale(theScale);
			jTransM = sMatrix * scaleM;
		}
		else if(tData->GetBool(CLST_ATTACH_EXTERNAL) && tData->GetObjectLink(CLST_ATTACH_LINK,doc))
		{
			Matrix opMatrix = jTransM, childMatrix = tData->GetObjectLink(CLST_ATTACH_LINK,doc)->GetMg();
			Matrix pivotMatrix = jTransM; pivotMatrix.off = childMatrix.off;
			Matrix opGuideM = jTransM, chGuideM = childMatrix, pvGuideM = pivotMatrix;
			Matrix sMatrix, skRefMatrix = tData->GetMatrix(CLST_C_REF_MATRIX);
			Real theMix = tData->GetReal(CLST_POS_MIX), refLen, curLen;
			Vector theScale, cRefPos, jRefPos, sStrength = tData->GetVector(CLST_STRENGTH);
			
			opGuideM.off = opMatrix * tData->GetVector(CLST_JOINT_OFFSET);
			chGuideM.off = childMatrix * tData->GetVector(CLST_CHILD_OFFSET);
			if(!tData->GetBool(CLST_ENABLE_PIVOT))
			{
				jTransM.off = CDBlend(opGuideM.off,chGuideM.off,theMix);
				if(tData->GetBool(CLST_RESTRICT_TO_Z))
				{
					Real zPos = Len(jTransM.off - opMatrix.off);
					jTransM.off = opMatrix.off + VNorm(opMatrix.v3) * zPos;
				}
				cRefPos = skRefMatrix * tData->GetVector(CLST_CHILD_OFFSET);
				jRefPos = jMatrix * tData->GetVector(CLST_JOINT_OFFSET);
				refLen = Len(cRefPos - jRefPos);
				curLen = Len(chGuideM.off - opGuideM.off);
			}
			else
			{
				pvGuideM.off = pivotMatrix * tData->GetVector(CLST_PIVOT_OFFSET);
				refLen = Len(chGuideM.off - childMatrix.off) + Len(pvGuideM.off - childMatrix.off) + Len(pvGuideM.off - opGuideM.off);
				curLen = Len(pvGuideM.off - opGuideM.off) + Len(chGuideM.off - pvGuideM.off);
				Real mixAdd = theMix * (curLen/refLen);
				jTransM.off = CDBlend(opGuideM.off,pvGuideM.off,mixAdd);
			}
			theScale.x = CDBlend(1.0,Real(refLen/curLen),sStrength.x);
			theScale.y = CDBlend(1.0,Real(refLen/curLen),sStrength.y);
			theScale.z = CDBlend(1.0,Real(curLen/refLen),sStrength.z);
			
			sMatrix = jTransM;
			Matrix scaleM = MatrixScale(theScale);
			jTransM = sMatrix * scaleM;
		}
	}
	
	// Calculate the translation matrix
	CDJTransM jM;
	jM.m  = jMatrix; 
	jM.base = false;
	jM.useSpline = false;
	jM.stretch = 1.0;
	jM.squash = 1.0;
	jM.volume = 1.0;
	if(op->GetType() == ID_CDJOINTOBJECT)
	{
		BaseContainer *opData = op->GetDataInstance();

		jM.base = opData->GetBool(JNT_J_BASE);
		jM.stretch = opData->GetReal(JNT_STRETCH_VALUE);
		jM.squash = opData->GetReal(JNT_SQUASH_VALUE);
		jM.volume = opData->GetReal(JNT_SKIN_VOLUME);
		
		jM.useSpline = opData->GetBool(JNT_USE_SPLINE);
		if(jM.useSpline)
		{
			
			jM.splStart = opData->GetReal(JNT_SPL_START);
			jM.splEnd = opData->GetReal(JNT_SPL_END);
			jM.length = opData->GetReal(JNT_J_LENGTH);
			jM.spline = (SplineData*)opData->GetCustomDataType(JNT_S_AND_S_SPLINE,CUSTOMDATATYPE_SPLINE);
		}
	}
	
	LONG index, cptCount = tData->GetLong(CLST_C_POINT_COUNT);
	for (i=0; i<cptCount; i++)
	{
		index = cWeight[i].ind;
		
		if(index < ptCount)
		{
			refPt = skinRef[index].GetVector();
			
			Matrix J, jTM = jTransM;
			
			// if point is not behind the joint do stretch
			Vector htY, ptV = ((mMatrix * refPt) - jM.m.off);
			Real bias = 1.0, adj, dotProd = VDot(VNorm(ptV), VNorm(jM.m.v3));
			if(dotProd > 0 || !(jM.base))
			{
				jTM.v3 *= jM.stretch;
				bias = jM.squash;
				
				if(jM.useSpline && jM.spline)
				{
					adj = dotProd * Len(ptV);
					Real splMix, sqMix, bMix;
					
					splMix = CDBlend(jM.splStart,jM.splEnd,adj/jM.length);
					htY = jM.spline->GetPoint(splMix);
					sqMix = CDBlend(1.0,jM.squash,htY.y);
					
					if(jM.squash > 1.0)
					{
						bMix = Len(CDGetPos(op->GetDown()))/jM.length;
						bias = CDBlend(sqMix,1.0,bMix);
					}
					else if(jM.squash < 1.0)
					{
						bMix = jM.length/Len(CDGetPos(op->GetDown()));
						Real vol = jM.volume;
						if(vol > 1.0) bMix = CDBlend(0.0,bMix,1/vol);
						bias = CDBlend(sqMix,1.0,bMix);
					}
				}
			}
			jTM.v1 *= bias;
			jTM.v2 *= bias;
			J = MInv(meshMatrix) * jTM * MInv(jM.m) * mMatrix;
				
			if(!tData->GetBool(CLST_MIX_PREVIOUS))
			{
				padr[index] = (J * refPt * cWeight[i].w)+(refPt * (1.0-cWeight[i].w));
			}
			else
			{
				padr[index] = (J * refPt * cWeight[i].w)+(padr[index] * (1.0-cWeight[i].w));
			}
		}
	}
	trg->Message(MSG_UPDATE);

	return CD_EXECUTION_RESULT_OK;
}

Bool CDClusterTagPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(CLST_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDClusterTagPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case CLST_DEST_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case CLST_BIND_POINTS:	
			if(!tData->GetBool(CLST_BOUND)) return true;
			else return false;
		case CLST_MIX_PREVIOUS:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case CLST_UNBIND_POINTS:	
			if(tData->GetBool(CLST_BOUND))return true;
			else return false;
		case CLST_LOCAL_TRANS:	
			if(!tData->GetBool(T_REG)) return false;
			else if(op->GetUp() && !tData->GetBool(CLST_BOUND)) return true;
			else return false;
		case CLST_BLEND_ROTATION:	
			if(!tData->GetBool(T_REG)) return false;
			else if(op->GetUp() && !tData->GetBool(CLST_ATTACH_CHILD) && !tData->GetBool(CLST_LOCAL_TRANS)) return true;
			else return false;
		case CLST_ROT_MIX:	
			if(tData->GetBool(CLST_BLEND_ROTATION) && !tData->GetBool(CLST_LOCAL_TRANS)) return true;
			else return false;
		case CLST_ATTACH_CHILD:	
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(CLST_BLEND_ROTATION) && !tData->GetBool(CLST_LOCAL_TRANS)) return true;
			else return false;
		case CLST_SHOW_GUIDE:	
			if(tData->GetBool(CLST_ATTACH_CHILD)) return true;
			else return false;
		case CLST_GUIDE_SIZE:	
			if(tData->GetBool(CLST_ATTACH_CHILD)) return true;
			else return false;
		case CLST_LINE_COLOR:	
			if(tData->GetBool(CLST_ATTACH_CHILD)) return true;
			else return false;
		case CLST_JOINT_OFFSET:	
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(CLST_ATTACH_CHILD)) return true;
			else return false;
		case CLST_ENABLE_PIVOT:	
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(CLST_ATTACH_CHILD) && !tData->GetBool(CLST_RESTRICT_TO_Z)) return true;
			else return false;
		case CLST_PIVOT_OFFSET:	
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(CLST_ATTACH_CHILD) && tData->GetBool(CLST_ENABLE_PIVOT)) return true;
			else return false;
		case CLST_CHILD_OFFSET:	
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(CLST_ATTACH_CHILD)) return true;
			else return false;
		case CLST_RESTRICT_TO_Z:	
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(CLST_ATTACH_CHILD) && !tData->GetBool(CLST_ENABLE_PIVOT)) return true;
			else return false;
		case CLST_POS_MIX:	
			if(tData->GetBool(CLST_ATTACH_CHILD)) return true;
			else return false;
		case CLST_STRENGTH:	
			if(!tData->GetBool(T_REG)) return false;
			else if(tData->GetBool(CLST_ATTACH_CHILD)) return true;
			else return false;
		case CLST_ATTACH_EXTERNAL:	
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(CLST_LOCAL_TRANS)) return true;
			else return false;
		case CLST_ATTACH_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(CLST_ATTACH_EXTERNAL);
	}
	return true;
}

Bool RegisterCDClusterTagPlugin(void)
{
	if(CDFindPlugin(ID_CDCLUSTERTAGPLUGIN,CD_TAG_PLUGIN)) return true;
	
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdjnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdjnr.FindFirst("-",&pos);
	while(h)
	{
		cdjnr.Delete(pos,1);
		h = cdjnr.FindFirst("-",&pos);
	}
	cdjnr.ToUpper();
	kb = cdjnr.SubStr(pK,2);
	
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
	if(!reg) info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE;
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDCLUSTERTAG); if(!name.Content()) return true;

	return CDRegisterTagPlugin(ID_CDCLUSTERTAGPLUGIN,name,info,CDClusterTagPlugin::Alloc,"tCDCluster","CDCluster.tif",1);
}

// library functions
Bool iCDGetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClusterTagPlugin *clTag = static_cast<CDClusterTagPlugin*>(tag->GetNodeData());
	return clTag ? clTag->GetWeight(tData, tWeight, tCnt) : false;
}

Bool iCDSetClusterWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClusterTagPlugin *clTag = static_cast<CDClusterTagPlugin*>(tag->GetNodeData());
	return clTag ? clTag->SetWeight(tData, tWeight, tCnt) : false;
}

Bool iCDRemapClusterWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map)
{
	CDClusterTagPlugin *clTag = static_cast<CDClusterTagPlugin*>(tag->GetNodeData());
	return clTag ? clTag->RemapClusterWeights(tData, map) : false;
}


