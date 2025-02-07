//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_modeling.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDLConstraint.h"
#include "CDConstraint.h"
#include "CDArray.h"

enum
{
	//LC_PURCHASE			= 1000,
	
	//LC_GLOBAL_COORDS		= 1010,

	//LC_P_LOCK_X			= 1011,
	//LC_POSITION_X			= 1012,
	//LC_P_LOCK_Y			= 1013,
	//LC_POSITION_Y			= 1014,
	//LC_P_LOCK_Z			= 1015,
	//LC_POSITION_Z			= 1016,

	LC_LOCAL_MATRIX		= 1017,
	//LC_USE_ORIENTATION		= 1018,
	LC_REF_MATRIX			= 1019,
	
	//LC_SHOW_LINES			= 1020,
	//LC_LINE_COLOR			= 1021,
	//LC_BOX_SIZE			= 1022,
		
	//LC_A_LOCK_X			= 1025,
	//LC_A_LOCK_Y			= 1026,
	//LC_A_LOCK_Z			= 1027,
	
	//LC_LOCK_ENABLE			= 1028,
	//LC_LOCK_BLEND			= 1029,
	
	//LC_USE_CUR_POS			= 1030,
	//LC_A_LOCK_ALL			= 1031,

	//LC_LOCK_POINTS			= 1040,

	LC_REF_COUNT				= 1100,
	LC_DISK_LEVEL				= 1101,
	LC_TNG_COUNT				= 1102,

	//LC_ID_LOCK				= 2000,
};

class CDLConstraintPlugin : public CDTagData
{
private:
	CDArray<Vector>		rpadr;
	CDArray<Tangent>	rtadr;
	
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Bool InitReference(BaseContainer *tData, BaseObject *op);
	void RemapReferencePoints(Vector *padr, TranslationMaps *map);
	Bool TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg);
	
	Matrix GetRotationLock(BaseTag *tag, Matrix m);
	
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
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDLConstraintPlugin); }
};

Bool CDLConstraintPlugin::Init(GeListNode *node)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(LC_SHOW_LINES,false);
	tData->SetVector(LC_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(LC_BOX_SIZE, 30.0);

	tData->SetBool(LC_LOCK_ENABLE,true);
	tData->SetReal(LC_LOCK_BLEND,1.0);

	tData->SetBool(LC_GLOBAL_COORDS,false);
	tData->SetBool(LC_USE_ORIENTATION,false);

	tData->SetBool(LC_P_LOCK_X,false);
	tData->SetBool(LC_P_LOCK_Y,false);
	tData->SetBool(LC_P_LOCK_Z,false);
		
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	tData->SetBool(LC_TNG_COUNT,false);
	tData->SetBool(LC_REF_COUNT,false);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

void CDLConstraintPlugin::Free(GeListNode *node)
{
	rpadr.Free();
	rtadr.Free();
}

Bool CDLConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(LC_DISK_LEVEL,level);
	
	LONG i, refCnt = tData->GetLong(LC_REF_COUNT), tngCnt = tData->GetLong(LC_TNG_COUNT);
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
		if(tngCnt > 0)
		{
			if(!rtadr.Alloc(tngCnt)) return false;
			for(i=0; i<tngCnt; i++)
			{
				CDHFReadFloat(hf, &rtadr[i].vl.x);
				CDHFReadFloat(hf, &rtadr[i].vl.y);
				CDHFReadFloat(hf, &rtadr[i].vl.z);
				
				CDHFReadFloat(hf, &rtadr[i].vr.x);
				CDHFReadFloat(hf, &rtadr[i].vr.y);
				CDHFReadFloat(hf, &rtadr[i].vr.z);
			}
		}
	}
	
	return true;
}

Bool CDLConstraintPlugin::Write(GeListNode* node, HyperFile* hf)
{
	LONG i;
	
	if(rpadr.Size() > 0)
	{
		//Level 1
		for(i=0; i<rpadr.Size(); i++)
		{
			CDHFWriteFloat(hf, rpadr[i].x);
			CDHFWriteFloat(hf, rpadr[i].y);
			CDHFWriteFloat(hf, rpadr[i].z);
		}
	}
	
	if(rtadr.Size() > 0)
	{
		//Level 1
		for(i=0; i<rtadr.Size(); i++)
		{
			CDHFWriteFloat(hf, rtadr[i].vl.x);
			CDHFWriteFloat(hf, rtadr[i].vl.y);
			CDHFWriteFloat(hf, rtadr[i].vl.z);
			
			CDHFWriteFloat(hf, rtadr[i].vr.x);
			CDHFWriteFloat(hf, rtadr[i].vr.y);
			CDHFWriteFloat(hf, rtadr[i].vr.z);
		}
	}
	
	return true;
}

Bool CDLConstraintPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	//GePrint("CDPTConstraintPlugin::CopyTo");
	
	BaseTag	*tag  = (BaseTag*)snode;
	BaseContainer *tData = tag->GetDataInstance();
	LONG refCnt = tData->GetLong(LC_REF_COUNT), tngCnt = tData->GetLong(LC_TNG_COUNT);//size, 
	
	if(refCnt > 0)
	{
		//Level 1
		rpadr.Copy(((CDLConstraintPlugin*)dest)->rpadr);
	}
	
	if(tngCnt > 0)
	{
		//Level 1
		rtadr.Copy(((CDLConstraintPlugin*)dest)->rtadr);
	}
	
	return true;
}

Bool CDLConstraintPlugin::InitReference(BaseContainer *tData, BaseObject *op)
{
	if(!IsValidPointObject(op)) return true;
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, ptCnt = ToPoint(op)->GetPointCount();
	
	if(!rpadr.Alloc(ptCnt)) return false;
	
	for(i=0; i<ptCnt; i++)
	{
		rpadr[i] = padr[i];
	}
	tData->SetLong(LC_REF_COUNT,ptCnt);
	
	BaseContainer *oData = op->GetDataInstance();
	if(IsValidSplineObject(op) && oData->GetLong(SPLINEOBJECT_TYPE) == SPLINEOBJECT_TYPE_BEZIER)
	{
		Tangent *tngt = GetTangentArray(op);
		if(tngt)
		{
			if(rtadr.Alloc(ptCnt))
			{
				for(i=0; i<ptCnt; i++)
				{
					rtadr[i] = tngt[i];
				}
				tData->SetLong(LC_TNG_COUNT,ptCnt);
			}
		}
	}
	
	return true;
}

void CDLConstraintPlugin::RemapReferencePoints(Vector *padr, TranslationMaps *map)
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

Bool CDLConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;
	
	BaseObject *pr = NULL;

	if (!tData->GetBool(LC_SHOW_LINES)) return true;

	Matrix opM = op->GetMg(),  prM;
	Real boxSize = tData->GetReal(LC_BOX_SIZE);
	Vector drawDirection, drawPosition;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(LC_LINE_COLOR));
	prM = Matrix(Vector(0,0,0),Vector(1,0,0),Vector(0,1,0),Vector(0,0,1));
	if (!tData->GetBool(LC_GLOBAL_COORDS))
	{
		pr = op->GetUp();
		if (pr)
		{
			if(!tData->GetBool(LC_USE_ORIENTATION))
			{
				prM = pr->GetMg();
			}
			else prM = tData->GetMatrix(LC_LOCAL_MATRIX);
		}
		else
		{
			if(tData->GetBool(LC_USE_ORIENTATION))
			{
				prM = tData->GetMatrix(LC_LOCAL_MATRIX);
			}
		}
	}
	
	CDDrawLine(bd,opM.off, prM.off);
	
	// Draw a mini axis guide
	bd->SetPen(Vector(1,0,0));
	drawDirection = VNorm(prM.v1);
	drawPosition = prM.off + drawDirection * boxSize;
	CDDrawLine(bd,drawPosition, prM.off);

	bd->SetPen(Vector(0,1,0));
	drawDirection = VNorm(prM.v2);
	drawPosition = prM.off + drawDirection * boxSize;
	CDDrawLine(bd,drawPosition, prM.off);
	
	bd->SetPen(Vector(0,0,1));
	drawDirection = VNorm(prM.v3);
	drawPosition = prM.off + drawDirection * boxSize;
	CDDrawLine(bd,drawPosition, prM.off);
	
	return true;
}

Matrix CDLConstraintPlugin::GetRotationLock(BaseTag *tag, Matrix m)
{
	BaseContainer *tData = tag->GetDataInstance();

	Real xdot, ydot, zdot;
	Matrix r = m;
	
	if (tData->GetBool(LC_A_LOCK_X))
	{
		r.v1 = Vector(1,0,0);
		ydot = Abs(VDot(VNorm(r.v1), VNorm(r.v2)));
		zdot = Abs(VDot(VNorm(r.v1), VNorm(r.v3)));
		if(ydot < zdot)
		{
			r.v3 = VNorm(VCross(r.v1, r.v2));
			r.v2 = VNorm(VCross(r.v3, r.v1));
		}
		else
		{
			r.v2 = VNorm(VCross(r.v3, r.v1));
			r.v3 = VNorm(VCross(r.v1, r.v2));
		}
	}
	else if (tData->GetBool(LC_A_LOCK_Y))
	{
		r.v2 = Vector(0,1,0);
		xdot = Abs(VDot(VNorm(r.v2),VNorm(r.v1)));
		zdot = Abs(VDot(VNorm(r.v2),VNorm(r.v3)));
		if(xdot < zdot)
		{
			r.v3 = VNorm(VCross(r.v1, r.v2));
			r.v1 = VNorm(VCross(r.v2, r.v3));
		}
		else
		{
			r.v1 = VNorm(VCross(r.v2, r.v3));
			r.v3 = VNorm(VCross(r.v1, r.v2));
		}
	}
	else if (tData->GetBool(LC_A_LOCK_Z))
	{
		r.v3 = Vector(0,0,1);
		xdot = Abs(VDot(VNorm(r.v3),VNorm(r.v1)));
		ydot = Abs(VDot(VNorm(r.v3),VNorm(r.v2)));
		if(xdot < ydot)
		{
			r.v2 = VNorm(VCross(r.v3, r.v1));
			r.v1 = VNorm(VCross(r.v2, r.v3));
		}
		else
		{
			r.v1 = VNorm(VCross(r.v2, r.v3));
			r.v2 = VNorm(VCross(r.v3, r.v1));
		}
	}
	else if (tData->GetBool(LC_A_LOCK_ALL))
	{
		r.v1 = Vector(1,0,0);
		r.v2 = Vector(0,1,0);
		r.v3 = Vector(0,0,1);
	}
	
	return r;
}

void CDLConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetBool(LC_LOCK_ENABLE+T_LST,tData->GetBool(LC_LOCK_ENABLE));

			tData->SetBool(LC_GLOBAL_COORDS+T_LST,tData->GetBool(LC_GLOBAL_COORDS));
			tData->SetBool(LC_USE_ORIENTATION+T_LST,tData->GetBool(LC_USE_ORIENTATION));

			tData->SetBool(LC_USE_CUR_POS+T_LST,tData->GetBool(LC_USE_CUR_POS));
			tData->SetBool(LC_P_LOCK_X+T_LST,tData->GetBool(LC_P_LOCK_X));
			tData->SetBool(LC_P_LOCK_Y+T_LST,tData->GetBool(LC_P_LOCK_Y));
			tData->SetBool(LC_P_LOCK_Z+T_LST,tData->GetBool(LC_P_LOCK_Z));
			tData->SetReal(LC_POSITION_X+T_LST,tData->GetReal(LC_POSITION_X));
			tData->SetReal(LC_POSITION_Y+T_LST,tData->GetReal(LC_POSITION_Y));
			tData->SetReal(LC_POSITION_Z+T_LST,tData->GetReal(LC_POSITION_Z));

			tData->SetBool(LC_A_LOCK_ALL+T_LST,tData->GetBool(LC_A_LOCK_ALL));
			tData->SetBool(LC_A_LOCK_X+T_LST,tData->GetBool(LC_A_LOCK_X));
			tData->SetBool(LC_A_LOCK_Y+T_LST,tData->GetBool(LC_A_LOCK_Y));
			tData->SetBool(LC_A_LOCK_Z+T_LST,tData->GetBool(LC_A_LOCK_Z));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDLConstraintPlugin::TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg)
{
	if(!IsValidPointObject(op)) return false;
	
	Vector *padr = GetPointArray(op);
	LONG i, oldPCnt = tData->GetLong(LC_REF_COUNT);
	
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
				
				tData->SetLong(LC_REF_COUNT,nCnt);
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
				
				tData->SetLong(LC_REF_COUNT,nCnt);
				pMap.Free();
			}
		}
		else
		{
			if(nCnt < oCnt)
			{
				if(!rpadr.IsEmpty())
				{
					// resize storage array
					rpadr.Resize(nCnt);
					tData->SetLong(LC_REF_COUNT,nCnt);
				}
			}
			else if(nCnt > oCnt)
			{
				if(!rpadr.IsEmpty())
				{
					// resize storage array
					rpadr.Resize(nCnt);
					
					for(i=oCnt; i<nCnt; i++)
					{
						rpadr[i] = padr[i];
					}
					tData->SetLong(LC_REF_COUNT,nCnt);
				}
			}
		}
	}
	
	return true;
}

Bool CDLConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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

			if(tData->GetLong(LC_DISK_LEVEL) < 1)
			{
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
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	
	switch (type)
	{
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			if(tData->GetBool(LC_LOCK_POINTS))
			{
				LONG i, pCnt = tData->GetLong(LC_REF_COUNT), tCnt = tData->GetLong(LC_TNG_COUNT);
				if(pCnt > 0)
				{
					Vector *trnsSca = (Vector*)data;
					if(trnsSca)
					{
						Vector sca = *trnsSca;
						Matrix scaM = MatrixScale(sca);
						
						for(i=0; i<pCnt; i++)
						{
							rpadr[i] = scaM * rpadr[i];
						}
						if(!rtadr.IsEmpty())
						{
							for(i=0; i<tCnt; i++)
							{
								rtadr[i].vl = scaM * rtadr[i].vl;
								rtadr[i].vr = scaM * rtadr[i].vr;
							}
						}
					}
				}
			}
			break;
		}
		case MSG_POINTS_CHANGED:
		{
			VariableChanged *vchg = (VariableChanged*)data;
			if(vchg)
			{
				TransferTMaps(tag,doc,tData,op,vchg);
			}
			break;
		}
		case  MSG_TRANSLATE_POINTS:
		{
			if(!IsValidPointObject(op)) return true;
			
			Vector *padr = GetPointArray(op);
			LONG oldPCnt = tData->GetLong(LC_REF_COUNT);
			
			TranslationMaps *tMap = (TranslationMaps*)data;
			if(tMap) 
			{
				LONG oCnt = tMap->m_oPointCount, nCnt = tMap->m_nPointCount;
				if(oCnt == oldPCnt && oCnt != nCnt)
				{
					if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					RemapReferencePoints(padr,tMap);
					
					tData->SetLong(LC_REF_COUNT,nCnt);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==LC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}

	return true;
}

Bool CDLConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetBool(LC_LOCK_ENABLE,tData->GetBool(LC_LOCK_ENABLE+T_LST));

		tData->SetBool(LC_GLOBAL_COORDS,tData->GetBool(LC_GLOBAL_COORDS+T_LST));
		tData->SetBool(LC_USE_ORIENTATION,tData->GetBool(LC_USE_ORIENTATION+T_LST));

		tData->SetBool(LC_USE_CUR_POS,tData->GetBool(LC_USE_CUR_POS+T_LST));
		tData->SetBool(LC_P_LOCK_X,tData->GetBool(LC_P_LOCK_X+T_LST));
		tData->SetBool(LC_P_LOCK_Y,tData->GetBool(LC_P_LOCK_Y+T_LST));
		tData->SetBool(LC_P_LOCK_Z,tData->GetBool(LC_P_LOCK_Z+T_LST));
		tData->SetReal(LC_POSITION_X,tData->GetReal(LC_POSITION_X+T_LST));
		tData->SetReal(LC_POSITION_Y,tData->GetReal(LC_POSITION_Y+T_LST));
		tData->SetReal(LC_POSITION_Z,tData->GetReal(LC_POSITION_Z+T_LST));

		tData->SetBool(LC_A_LOCK_ALL,tData->GetBool(LC_A_LOCK_ALL+T_LST));
		tData->SetBool(LC_A_LOCK_X,tData->GetBool(LC_A_LOCK_X+T_LST));
		tData->SetBool(LC_A_LOCK_Y,tData->GetBool(LC_A_LOCK_Y+T_LST));
		tData->SetBool(LC_A_LOCK_Z,tData->GetBool(LC_A_LOCK_Z+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDLConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	if(tData->GetBool(LC_USE_CUR_POS))
	{
		tData->SetReal(LC_POSITION_X,0.0);
		tData->SetReal(LC_POSITION_Y,0.0);
		tData->SetReal(LC_POSITION_Z,0.0);
	}

	BaseObject *pr = op->GetUp();
	
	Matrix opM = op->GetMg(), transM, trgM, rotLockM;
	Vector theScale = CDGetScale(op);
	Vector oldRot, newRot, rotSet;
	CDQuaternion qA, qB, qC;
	
	Matrix prM = Matrix();
	Bool axisLock = false;
	if(tData->GetBool(LC_A_LOCK_X) || tData->GetBool(LC_A_LOCK_Y) || tData->GetBool(LC_A_LOCK_Z) || tData->GetBool(LC_A_LOCK_ALL))  axisLock = true;
	
	// Check for op rotating
	LONG rotateTool;
	if(GetC4DVersion() < 9000) rotateTool = 12238;
	else rotateTool = 200000090;
	if(doc->GetAction()==rotateTool)
	{
		if(!tData->GetBool(LC_GLOBAL_COORDS))
		{
			if(pr)
			{
				if(!tData->GetBool(LC_USE_ORIENTATION))
				{
					prM = pr->GetMg();
				}
				else
				{
					prM = pr->GetMg() * tData->GetMatrix(LC_REF_MATRIX);
				}
			}
			else
			{
				if(tData->GetBool(LC_USE_ORIENTATION))
				{
					prM = tData->GetMatrix(LC_LOCAL_MATRIX);
				}
			}
		}
		
		if(tData->GetBool(LC_USE_CUR_POS))
		{
			prM = tData->GetMatrix(LC_LOCAL_MATRIX);
		}
		
		transM = MInv(prM) * opM;
		if(tData->GetBool(LC_LOCK_ENABLE))
		{
			rotLockM = GetRotationLock(tag, transM);
			
			qA.SetMatrix(transM);
			qB.SetMatrix(rotLockM);
			
			Real theMix = tData->GetReal(LC_LOCK_BLEND);
			qC = CDQSlerp(qA,qB,theMix);
			
			Vector posTM = transM.off;
			transM = qC.GetMatrix();
			transM.off = posTM;
		}
		if (tData->GetBool(LC_P_LOCK_X)) transM.off.x = tData->GetReal(LC_POSITION_X);
		if (tData->GetBool(LC_P_LOCK_Y)) transM.off.y = tData->GetReal(LC_POSITION_Y);
		if (tData->GetBool(LC_P_LOCK_Z)) transM.off.z = tData->GetReal(LC_POSITION_Z);
		
		trgM = prM * transM;
		if(tData->GetBool(LC_LOCK_ENABLE))
		{
			Real theMix = tData->GetReal(LC_LOCK_BLEND);
			Vector targetPosition = CDBlend(opM.off,trgM.off,theMix);
			trgM.off = targetPosition;
			oldRot = CDGetRot(op);
			op->SetMg(trgM);
			newRot = CDGetRot(op);
			rotSet = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,rotSet);
			CDSetScale(op,theScale);
		}
		
		if(tData->GetBool(LC_USE_ORIENTATION))
		{
			if(!axisLock)
			{
				tData->SetMatrix(LC_LOCAL_MATRIX, opM);
				if (pr)
				{
					tData->SetMatrix(LC_REF_MATRIX, MInv(pr->GetMg()) * opM);
				}
			}
		}
		opM = op->GetMg();
		if((!tData->GetBool(LC_P_LOCK_X) && !tData->GetBool(LC_P_LOCK_Y) && !tData->GetBool(LC_P_LOCK_Z)) || !tData->GetBool(LC_USE_ORIENTATION) || !tData->GetBool(LC_LOCK_ENABLE) || tData->GetReal(LC_LOCK_BLEND) == 0.0 || !tData->GetBool(LC_USE_CUR_POS))
		{
			if(!axisLock && !tData->GetBool(LC_USE_CUR_POS))
			{
				tData->SetMatrix(LC_LOCAL_MATRIX, opM);
				if (pr)
				{
					tData->SetMatrix(LC_REF_MATRIX, MInv(pr->GetMg()) * opM);
				}
			}
			else
			{
				if(tData->GetBool(LC_USE_CUR_POS))
				{
					tData->SetMatrix(LC_LOCAL_MATRIX, opM);
					if (pr)
					{
						tData->SetMatrix(LC_REF_MATRIX, MInv(pr->GetMg()) * opM);
					}
				}
			}
		}
	}
	else
	{
		if (!tData->GetBool(LC_GLOBAL_COORDS))
		{
			if(pr)
			{
				if(!tData->GetBool(LC_USE_ORIENTATION))
				{
					prM = pr->GetMg();
				}
				else
				{
					Bool sameRot = true;
					prM = pr->GetMg() * tData->GetMatrix(LC_REF_MATRIX);
					
					if(!VEqual(prM.v1,opM.v1,0.001)) sameRot = false;
					if(!VEqual(prM.v2,opM.v2,0.001)) sameRot = false;
					if(!VEqual(prM.v3,opM.v3,0.001)) sameRot = false;
					if(!sameRot)
					{
						prM.v1 = opM.v1;
						prM.v2 = opM.v2;
						prM.v3 = opM.v3;
						tData->SetMatrix(LC_LOCAL_MATRIX, prM);
					}
				}
			}
			else
			{
				if(tData->GetBool(LC_USE_ORIENTATION))
				{
					Bool sameRot = true;
					prM = tData->GetMatrix(LC_LOCAL_MATRIX);
					if(!VEqual(prM.v1,opM.v1,0.001)) sameRot = false;
					if(!VEqual(prM.v2,opM.v2,0.001)) sameRot = false;
					if(!VEqual(prM.v3,opM.v3,0.001)) sameRot = false;
					if(!sameRot)
					{
						prM = opM;
						tData->SetMatrix(LC_LOCAL_MATRIX, prM);
					}
				}
			}
		}
		if(tData->GetBool(LC_USE_ORIENTATION))
		{
			Bool sameRot = true;
			prM = tData->GetMatrix(LC_LOCAL_MATRIX);
			if(!VEqual(prM.v1,opM.v1,0.001)) sameRot = false;
			if(!VEqual(prM.v2,opM.v2,0.001)) sameRot = false;
			if(!VEqual(prM.v3,opM.v3,0.001)) sameRot = false;
			if(!sameRot)
			{
				prM = opM;
				tData->SetMatrix(LC_LOCAL_MATRIX, prM);
			}
		}
		if(tData->GetBool(LC_USE_CUR_POS))
		{
			prM = tData->GetMatrix(LC_LOCAL_MATRIX);
		}
		
		transM = MInv(prM) * opM;
		if(tData->GetBool(LC_LOCK_ENABLE))
		{
			rotLockM = GetRotationLock(tag, transM);
			
			qA.SetMatrix(transM);
			qB.SetMatrix(rotLockM);
			
			Real theMix = tData->GetReal(LC_LOCK_BLEND);
			qC = CDQSlerp(qA,qB,theMix);
			
			Vector posTM = transM.off;
			transM = qC.GetMatrix();
			transM.off = posTM;
		}
		if (tData->GetBool(LC_P_LOCK_X)) transM.off.x = tData->GetReal(LC_POSITION_X);
		if (tData->GetBool(LC_P_LOCK_Y)) transM.off.y = tData->GetReal(LC_POSITION_Y);
		if (tData->GetBool(LC_P_LOCK_Z)) transM.off.z = tData->GetReal(LC_POSITION_Z);
		
		trgM = prM * transM;
		if(tData->GetBool(LC_LOCK_ENABLE))
		{
			Real theMix = tData->GetReal(LC_LOCK_BLEND);
			Vector targetPosition = CDBlend(opM.off,trgM.off,theMix);
			trgM.off = targetPosition;
			oldRot = CDGetRot(op);
			op->SetMg(trgM);
			newRot = CDGetRot(op);
			rotSet = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,rotSet);
			CDSetScale(op,theScale);
		}
		
		opM = op->GetMg();
		if((!tData->GetBool(LC_P_LOCK_X) && !tData->GetBool(LC_P_LOCK_Y) && !tData->GetBool(LC_P_LOCK_Z)) || !tData->GetBool(LC_USE_ORIENTATION) || !tData->GetBool(LC_LOCK_ENABLE) || tData->GetReal(LC_LOCK_BLEND) == 0.0 || !tData->GetBool(LC_USE_CUR_POS))
		{
			tData->SetMatrix(LC_LOCAL_MATRIX, opM);
			if (pr)
			{
				tData->SetMatrix(LC_REF_MATRIX, MInv(pr->GetMg()) * opM);
			}
		}
	}
	
	if(IsValidPointObject(op))
	{
		Vector *padr = GetPointArray(op);
		if(padr && !rpadr.IsEmpty())
		{
			LONG i, pCnt = tData->GetLong(LC_REF_COUNT), tCnt = tData->GetLong(LC_TNG_COUNT);
			
			if(!tData->GetBool(LC_LOCK_POINTS))
			{
				for(i=0; i<pCnt; i++)
				{
					rpadr[i] = padr[i];
				}
				if(!rtadr.IsEmpty())
				{
					Tangent *tngt = GetTangentArray(op);
					for(i=0; i<tCnt; i++)
					{
						rtadr[i] = tngt[i];
					}
				}
			}
			else
			{
				for(i=0; i<pCnt; i++)
				{
					padr[i] = rpadr[i];
				}
				if(!rtadr.IsEmpty())
				{
					Tangent *tngt = GetTangentArray(op);
					for(i=0; i<tCnt; i++)
					{
						tngt[i] = rtadr[i];
					}
				}
			}
		}
	}
	

	return CD_EXECUTION_RESULT_OK;
}

Bool CDLConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(LC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDLConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case LC_LOCK_ENABLE:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LC_GLOBAL_COORDS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LC_USE_ORIENTATION:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LC_USE_CUR_POS:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LC_P_LOCK_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LC_P_LOCK_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LC_P_LOCK_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case LC_POSITION_X:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LC_USE_CUR_POS)) return false;
				else return tData->GetBool(LC_P_LOCK_X);
			}
		case LC_POSITION_Y:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LC_USE_CUR_POS)) return false;
				else return tData->GetBool(LC_P_LOCK_Y);
			}
		case LC_POSITION_Z:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(LC_USE_CUR_POS)) return false;
				else return tData->GetBool(LC_P_LOCK_Z);
			}
		case LC_A_LOCK_ALL:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(LC_A_LOCK_X) && !tData->GetBool(LC_A_LOCK_Y) && !tData->GetBool(LC_A_LOCK_Z)) return true;
			else return false;
		case LC_A_LOCK_X:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(LC_A_LOCK_Y) && !tData->GetBool(LC_A_LOCK_Z) && !tData->GetBool(LC_A_LOCK_ALL)) return true;
			else return false;
		case LC_A_LOCK_Y:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(LC_A_LOCK_X) && !tData->GetBool(LC_A_LOCK_Z) && !tData->GetBool(LC_A_LOCK_ALL)) return true;
			else return false;
		case LC_A_LOCK_Z:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(LC_A_LOCK_X) && !tData->GetBool(LC_A_LOCK_Y) && !tData->GetBool(LC_A_LOCK_ALL)) return true;
			else return false;
		case LC_LOCK_POINTS:
			if(!tData->GetBool(T_REG)) return false;
			else if(IsValidPointObject(op)) return true;
			else return false;
	}
	return true;
}

Bool RegisterCDLConstraintPlugin(void)
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
	String name=GeLoadString(IDS_CDLCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDLCONSTRAINTPLUGIN,name,info,CDLConstraintPlugin::Alloc,"tCDLConstraint","CDLConstraint.tif",1);
}
