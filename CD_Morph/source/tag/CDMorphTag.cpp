//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "CDMorph.h"

#include "oCDJoint.h"
#include "tCDMorphTag.h"
#include "CDDebug.h"

//from CD Zero Tranformation
enum
{
	ZT_TRANS_ROT			= 1003
};

Bool CDMorphTagPlugin::Init(GeListNode *node)
{	
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(MT_SELECTION_EXISTS,false);
	tData->SetBool(MT_SELECTION_EDITING,false);
	tData->SetBool(MT_OFFSET_EXISTS,false);
	tData->SetBool(MT_OFFSET_EDITING,true);
	tData->SetBool(MT_MIDPOINT_EXISTS,false);
	tData->SetBool(MT_MIDPOINT_EDITING,false);
	tData->SetLong(MT_ROTATION_AXIS,MT_ROTATION_H);
	tData->SetVector(MT_GUIDE_COLOR, Vector(1,0,0));
	
	tData->SetBool(MT_USE_SLIDERS,false);
	tData->SetReal(MT_MID_OFFSET,0.0);
	tData->SetReal(MT_MID_WIDTH,0.0);
	
	tData->SetBool(MT_REFERENCE_NEW,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
			pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,100);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	// Add this instance to the list
	CDMData md;
	PluginMessage(ID_CDMORPHTAGPLUGIN,&md);
	if(md.list) md.list->Append(node);
	
	return true;
}

void CDMorphTagPlugin::Free(GeListNode *node)
{	
	msadr.Free();
	sphPt.Free();
	
	// Remove this instance from the list
	CDMData md;
	PluginMessage(ID_CDMORPHTAGPLUGIN,&md);
	if(md.list) md.list->Remove(node);
}

Bool CDMorphTagPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag		*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
	
	tData->SetLong(MT_LEVEL,level);
	if(bsPtCount > 0)
	{
		if(level < 3)
		{
			//Level 0
			if(level >= 0)
			{
				if(!msadr.Alloc(bsPtCount)) return false;
				LONG i;
				for (i=0; i<bsPtCount; i++)
				{
					CDHFReadLong(hf, &msadr[i].ind);
					CDHFReadFloat(hf, &msadr[i].x);
					CDHFReadFloat(hf, &msadr[i].y);
					CDHFReadFloat(hf, &msadr[i].z);
				}
			}
			
			//Level 1
			CDArray<CDMPoint> mRefPt;
			if(level >= 1)
			{
				if(!mRefPt.Alloc(bsPtCount)) return false;
				LONG i;
				for (i=0; i<bsPtCount; i++)
				{
					CDHFReadLong(hf, &mRefPt[i].ind);
					CDHFReadFloat(hf, &mRefPt[i].x);
					CDHFReadFloat(hf, &mRefPt[i].y);
					CDHFReadFloat(hf, &mRefPt[i].z);
					
					Vector m = msadr[i].GetVector();
					Vector r = mRefPt[i].GetVector();
					msadr[i].SetDelta(m,r);
				}
			}
			//Level 2
			if(level >= 2)
			{
				if(tData->GetBool(MT_MIDPOINT_EXISTS))
				{
					if(!sphPt.Alloc(bsPtCount)) return false;
					LONG i;
					for (i=0; i<bsPtCount; i++)
					{
						CDHFReadFloat(hf, &sphPt[i].normal.x);
						CDHFReadFloat(hf, &sphPt[i].normal.y);
						CDHFReadFloat(hf, &sphPt[i].normal.z);
						CDHFReadFloat(hf, &sphPt[i].center.x);
						CDHFReadFloat(hf, &sphPt[i].center.y);
						CDHFReadFloat(hf, &sphPt[i].center.z);
						
						Vector c = sphPt[i].center.GetVector();
						Vector n = sphPt[i].normal.GetVector();
						Vector r = mRefPt[i].GetVector();
						
						sphPt[i].center.SetDelta(c,r);
						sphPt[i].normal.SetVector(VNorm(n-c));
					}
				}
			}
			mRefPt.Free();
		}
		else
		{
			if(!msadr.Alloc(bsPtCount)) return false;
			LONG i;
			for (i=0; i<bsPtCount; i++)
			{
				CDHFReadLong(hf, &msadr[i].ind);
				CDHFReadFloat(hf, &msadr[i].x);
				CDHFReadFloat(hf, &msadr[i].y);
				CDHFReadFloat(hf, &msadr[i].z);
			}
			
			if(tData->GetBool(MT_MIDPOINT_EXISTS))
			{
				if(!sphPt.Alloc(bsPtCount)) return false;
				LONG i;
				for (i=0; i<bsPtCount; i++)
				{
					CDHFReadFloat(hf, &sphPt[i].normal.x);
					CDHFReadFloat(hf, &sphPt[i].normal.y);
					CDHFReadFloat(hf, &sphPt[i].normal.z);
					CDHFReadFloat(hf, &sphPt[i].center.x);
					CDHFReadFloat(hf, &sphPt[i].center.y);
					CDHFReadFloat(hf, &sphPt[i].center.z);
				}
			}
		}
	}
	
	
	return true;
}

Bool CDMorphTagPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag		*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
	
	if(bsPtCount > 0)
	{
		LONG i;
		for (i=0; i<bsPtCount; i++)
		{
			CDHFWriteLong(hf, msadr[i].ind);
			CDHFWriteFloat(hf, msadr[i].x);
			CDHFWriteFloat(hf, msadr[i].y);
			CDHFWriteFloat(hf, msadr[i].z);
		}
		
		if(tData->GetBool(MT_MIDPOINT_EXISTS))
		{
			for (i=0; i<bsPtCount; i++)
			{
				CDHFWriteFloat(hf, sphPt[i].normal.x);
				CDHFWriteFloat(hf, sphPt[i].normal.y);
				CDHFWriteFloat(hf, sphPt[i].normal.z);
				CDHFWriteFloat(hf, sphPt[i].center.x);
				CDHFWriteFloat(hf, sphPt[i].center.y);
				CDHFWriteFloat(hf, sphPt[i].center.z);
			}
		}
	}
	
	return true;
}

Bool CDMorphTagPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag		*tag  = (BaseTag*)snode;
	BaseContainer *tData = tag->GetDataInstance();
	
	LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
	
	if(bsPtCount > 0)
	{
		msadr.Copy(((CDMorphTagPlugin*)dest)->msadr);
		
		if(tData->GetBool(MT_MIDPOINT_EXISTS))
		{
			if(!sphPt.IsEmpty())
			{
				sphPt.Copy(((CDMorphTagPlugin*)dest)->sphPt);
			}
		}
	}
	
	return true;
}

Bool CDMorphTagPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = tag->GetDocument(); if(!doc) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	BaseObject *mesh = tData->GetObjectLink(MT_DEST_LINK,doc); if(!mesh) return true;
	
	CDMRefPoint	*refadr = GetReference(mesh);
	if(!refadr) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	if(tData->GetBool(MT_USE_MIDPOINT))
	{
		if(tData->GetBool(MT_MIDPOINT_EDITING) || tData->GetBool(MT_SHOW_GUIDE))
		{
			if(!sphPt.IsEmpty())
			{
				Matrix opM = mesh->GetMg();
				Vector *padr = GetPointArray(mesh);
				if(padr)
				{
					BaseSelect *bs = NULL;
					Bool editing = false, selOnly = tData->GetBool(MT_SELECTED_ONLY);
					if(tData->GetBool(MT_MIDPOINT_EDITING) && doc->IsEditMode())
					{
						bs = ToPoint(mesh)->GetPointS();
						editing = true;
					}
					
					if(editing) bd->SetPen(Vector(1,1,0));
					else bd->SetPen(tData->GetVector(MT_GUIDE_COLOR));
					
					LONG i, p, cnt = tData->GetLong(MT_BS_POINT_COUNT), ptCount = ToPoint(mesh)->GetPointCount();
					Vector refPt, mpoint, startPt, endPt, midPt, centPt;
					CDMSpPoint sPt;
					
					BaseTag *skTag = mesh->GetTag(ID_CDSKINPLUGIN);
					for(i=0; i<cnt; i++)
					{
						LONG mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							Bool gDraw = true;
							if(editing && bs && selOnly)
							{
								if(!bs->IsSelected(mInd)) gDraw = false;
							}
							if(gDraw)
							{
								Vector r,m,c,n;
								r = refadr[mInd].GetVector();
								m = msadr[i].GetDelta(r);
								c = sphPt[i].center.GetDelta(r);
								n = c + sphPt[i].normal.GetVector() * Len(r-c);
								if(skTag)
								{
									refPt = GetSkinnedPoint(doc,mesh,r,skTag,mInd);
									mpoint = GetSkinnedPoint(doc,mesh,m,skTag,mInd);
									if(tData->GetBool(MT_MIDPOINT_EDITING)) midPt = padr[mInd];
									else midPt = GetSkinnedPoint(doc,mesh,n,skTag,mInd);
								}
								else
								{
									refPt = r;
									mpoint = m;
									if(tData->GetBool(MT_MIDPOINT_EDITING)) midPt = padr[mInd];
									else midPt = n;
								}
								centPt = CalcCenter(refPt,midPt,mpoint);
								
								sPt.center.SetDelta(centPt,refPt);
								sPt.normal.SetVector(VNorm(midPt-centPt));
								
								startPt = opM * refPt;
								if(!VEqual(centPt,midPt,0.001))
								{
									for(p=1; p<20; p++)
									{
										endPt = opM * (MorphSlerp(refPt,mpoint,sPt,p*0.05) + refPt);
										CDDrawLine(bd,startPt,endPt);
										startPt = endPt;
									}
								}
								else
								{
									endPt = opM * midPt;
									CDDrawLine(bd,startPt,endPt);
									startPt = endPt;
								}
								endPt = opM * mpoint;
								CDDrawLine(bd,startPt,endPt);
							}
						}
					}
				}
			}
		}
	}
	
	return true;
}

void CDMorphTagPlugin::RecalculateMorphPoints(BaseTag *tag, BaseContainer *tData, Matrix newM, Matrix oldM)
{
	Vector newPt, oldPt, oldCPt, newCPt;
	LONG i, bsCnt = tData->GetLong(MT_BS_POINT_COUNT);
	newM.off = Vector(0,0,0);
	oldM.off = Vector(0,0,0);
	
	for(i=0; i<bsCnt; i++)
	{
		oldPt = msadr[i].GetVector();
		newPt = MInv(newM) * oldM * oldPt;
		msadr[i].SetVector(newPt);
		
		if(tData->GetBool(MT_MIDPOINT_EXISTS))
		{
			oldCPt = sphPt[i].center.GetVector();
			newCPt = MInv(newM) * oldM * oldCPt;
			sphPt[i].center.SetVector(newCPt);
		}
	}
}

void CDMorphTagPlugin::RemapMorphPoints(BaseTag *tag, BaseContainer *tData, LONG *remap, LONG cnt)
{
	LONG i, index, nCnt = 0, bsCnt = tData->GetLong(MT_BS_POINT_COUNT);
	
	if(remap)
	{
		// allocate morph selection translation map
		CDArray<LONG> mRemap;
		mRemap.Alloc(bsCnt);
		mRemap.Fill(-1);
		
		
		// set morph selection translation map values
		for(i=0; i<bsCnt; i++)
		{
			index = msadr[i].ind;
			if(index < cnt)
			{
				if(remap[index] > -1)
				{
					mRemap[nCnt] = i;
					msadr[i].ind = remap[index];
					nCnt++;
				}
			}
		}
		
		// Remap morph selection
		CDArray<CDMPoint> tempPoints;
		msadr.Copy(tempPoints);
		msadr.Resize(nCnt);
		
		for(i=0; i<nCnt; i++)
		{
			msadr[i] = tempPoints[mRemap[i]];
		}
		
		tempPoints.Free();
		
		// check for and remap spherical morph 
		if(tData->GetBool(MT_USE_MIDPOINT) && tData->GetBool(MT_MIDPOINT_EXISTS))
		{
			CDArray<CDMSpPoint> tempSphPt;
			sphPt.Copy(tempSphPt);
			sphPt.Resize(nCnt);
			
			for(i=0; i<nCnt; i++)
			{
				sphPt[i] = tempSphPt[mRemap[i]];
			}
			tempSphPt.Free();
		}
		
		mRemap.Free();
		
		tData->SetLong(MT_BS_POINT_COUNT,nCnt);
	}
}

BaseObject* CDMorphTagPlugin::GetMesh(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	BaseObject *mesh = tData->GetObjectLink(MT_DEST_LINK,doc);
	if(!mesh)
	{
		if(!tData->GetBool(MT_OP_DEST))
		{
			tData->SetLong(MT_BS_POINT_COUNT,0);
		}
		else
		{
			tData->SetLink(MT_DEST_LINK,op);
			mesh = op;
		}
	}
	else
	{
		if(!tData->GetBool(MT_OP_DEST))
		{
			if(mesh == op) tData->SetBool(MT_OP_DEST, true);
		}
		else
		{
			if(mesh != op) tData->SetBool(MT_OP_DEST, false);
		}
	}
	
	return mesh;
}

CDMRefPoint* CDMorphTagPlugin::GetReference(BaseObject *op)
{
	CDMRefPoint	*ref = NULL;
	
	BaseTag *skRefTag = op->GetTag(ID_CDSKINREFPLUGIN);
	if(skRefTag)
	{
		ref = CDGetSkinReference(skRefTag);
		
		if(!ref)
		{
			BaseTag *mRefTag = GetReferenceTag(op,false);
			if(mRefTag) ref = CDGetMorphReference(mRefTag);
		}
	}
	else
	{
		BaseTag *mRefTag = GetReferenceTag(op,false);
		if(mRefTag) ref = CDGetMorphReference(mRefTag);
	}
	
	return ref;
}

BaseTag* CDMorphTagPlugin::GetReferenceTag(BaseObject *op, Bool update)
{
	BaseTag *rTag = op->GetTag(ID_CDMORPHREFPLUGIN);
	if(!rTag)
	{
		rTag = BaseTag::Alloc(ID_CDMORPHREFPLUGIN);
		op->InsertTag(rTag,NULL);
		rTag->Message(MSG_MENUPREPARE);
		if(update) EventAdd(EVENT_FORCEREDRAW);
	}
	
	return rTag;
}

Vector CDMorphTagPlugin::GetSkinnedPoint(BaseDocument *doc, BaseObject *op, Vector pt, BaseTag *skTag, LONG mInd)
{
	CDSkinVertex *skinWeight = CDGetSkinWeights(skTag);
	if(!skinWeight) return pt;
	
	BaseContainer *sData = skTag->GetDataInstance();
	
	Matrix jM, jntM, jTrM, opM = op->GetMg(), skM = sData->GetMatrix(SKN_SKIN_MATRIX);
	Vector skinVert = Vector();
	
	LONG j, jIndex = SKN_J_LINK, mIndex = SKN_J_MATRIX, jCount = skinWeight[mInd].jn;
	if(jCount > 0)
	{
		for(j=0; j<jCount; j++)
		{
			LONG indPlus = skinWeight[mInd].jPlus[j];
			BaseObject *joint = sData->GetObjectLink(jIndex+indPlus,doc);
			if(joint)
			{
				jntM = sData->GetMatrix(mIndex+indPlus);
				jTrM = joint->GetMg();
				jM  = MInv(opM) * jTrM * MInv(jntM) * skM;
				
				skinVert += jM * pt * (skinWeight[mInd].jw[indPlus] / skinWeight[mInd].taw);
			}
		}
		return skinVert;
	}
	else return pt;
	
}

Vector CDMorphTagPlugin::SetSkinnedPoint(BaseDocument *doc, BaseObject *op, Vector pt, BaseTag *skTag, LONG mInd)
{
	CDSkinVertex *skinWeight = CDGetSkinWeights(skTag);
	if(!skinWeight) return pt;
	
	BaseContainer *stData = skTag->GetDataInstance();
	
	Matrix  jM, jntM, jTrM, opM = op->GetMg();
	Matrix invM, sumM, sM, skM = stData->GetMatrix(SKN_SKIN_MATRIX);
	Vector skinVert = Vector();
	
	LONG j, jIndex = SKN_J_LINK, mIndex = SKN_J_MATRIX, jCount = skinWeight[mInd].jn;
	if(jCount > 0)
	{
		for(j=0; j<jCount; j++)
		{
			LONG indPlus = skinWeight[mInd].jPlus[j];
			BaseObject *joint = stData->GetObjectLink(jIndex+indPlus,doc);
			if(joint)
			{
				jntM = stData->GetMatrix(mIndex+indPlus);
				jTrM = joint->GetMg();
				jM  = MInv(opM) * jTrM * MInv(jntM) * skM;
				
				sM = operator *((skinWeight[mInd].jw[indPlus] / skinWeight[mInd].taw),jM);
				if(j>0)
				{
					invM = operator +(sumM,sM);
				}
				else
				{
					invM = sM;
				}
				sumM = invM;
			}
		}
		skinVert = MInv(invM) * pt;
		return skinVert;
	}
	else
	{
		return pt;
	}	
}

void CDMorphTagPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLink(MT_DEST_LINK+T_LST,tData->GetLink(MT_DEST_LINK,doc));
			tData->SetBool(MT_USE_MIDPOINT+T_LST,tData->GetBool(MT_USE_MIDPOINT));
			
			tData->SetBool(MT_USE_BONE_ROTATION+T_LST,tData->GetBool(MT_USE_BONE_ROTATION));
			tData->SetLong(MT_ROTATION_AXIS+T_LST,tData->GetLong(MT_ROTATION_AXIS));
			tData->SetReal(MT_MIN_VALUE+T_LST,tData->GetReal(MT_MIN_VALUE));
			tData->SetReal(MT_MAX_VALUE+T_LST,tData->GetReal(MT_MAX_VALUE));
			tData->SetBool(MT_CLAMP_MAX+T_LST,tData->GetBool(MT_CLAMP_MAX));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDMorphTagPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	switch (type)
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
	BaseObject *op = tag->GetObject(); if(!op) return true;
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	CheckTagReg(doc,op,tData);
	
	BaseObject *mesh = GetMesh(doc,op,tData); if(!mesh) return true;
	if(!IsValidPointObject(mesh)) return true;
	
	BaseTag *refTag = GetReferenceTag(mesh,true); if(!refTag) return true;
	
	CDMRefPoint	*refadr = GetReference(mesh);
	if(!refadr)
	{
		refTag->Message(MSG_MENUPREPARE);
		refadr = GetReference(mesh);
		if(!refadr) return true;
	}
	
	LONG ptCount = ToPoint(mesh)->GetPointCount();
	Vector *padr = GetPointArray(mesh); if(!padr) return true;
	
	BaseContainer *skData = NULL;
	BaseTag *skTag = mesh->GetTag(ID_CDSKINPLUGIN);
	if(skTag) skData = skTag->GetDataInstance();
	
	ModelingCommandData		mcd;
	mcd.doc = doc;
#if API_VERSION < 12000
	mcd.mode = MODIFY_POINTSELECTION;
#else
	mcd.mode = MODELINGCOMMANDMODE_POINTSELECTION;
#endif
	mcd.op = mesh;
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			if(tData->GetLong(MT_LEVEL) < 3)
			{
				if(!tData->GetBool(MT_REFERENCE_NEW))
				{
					tData->SetLong(MT_POINT_COUNT,ptCount);
					tData->SetMatrix(MT_REF_MATRIX,mesh->GetMg());
					LONG i, bsCnt = tData->GetLong(MT_BS_POINT_COUNT);
					for(i=0; i<bsCnt; i++)
					{
						Vector m = msadr[i].GetVector();
						Vector r = refadr[msadr[i].ind].GetVector();
						msadr[i].SetDelta(m,r);
					}
					tData->SetBool(MT_REFERENCE_NEW,true);
				}
			}
			break;
		}
		case MSG_MENUPREPARE:
		{
			if(!tData->GetBool(MT_REFERENCE_NEW))
			{
				tData->SetLong(MT_POINT_COUNT,ptCount);
				tData->SetMatrix(MT_REF_MATRIX,mesh->GetMg());
				tData->SetBool(MT_REFERENCE_NEW,true);
			}
			break;
		}
		case  CD_MSG_SCALE_CHANGE:
		{
			if(refTag)
			{
				BaseContainer *rData = refTag->GetDataInstance();
				if(rData)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					Vector oldPt, newPt, oldMdPt, newMdPt, oldCPt, newCPt;
					Matrix scaleM = MatrixScale(rData->GetVector(MR_SCALE));
					LONG i, mInd, bsCnt = tData->GetLong(MT_BS_POINT_COUNT);
					for(i=0; i<bsCnt; i++)
					{
						mInd = msadr[i].ind;
						oldPt = msadr[i].GetVector();
						newPt = scaleM * oldPt;
						msadr[i].SetVector(newPt);
						
						if(tData->GetBool(MT_MIDPOINT_EXISTS))
						{
							oldCPt = sphPt[i].center.GetVector();
							newCPt = scaleM * oldCPt;
							sphPt[i].center.SetVector(newCPt);
						}
					}
					tData->SetBool(MT_M_SCALING,false);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==MT_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==MT_SET_SELECTION)
			{
				tData->SetLong(MT_POINT_COUNT,ptCount);
				
				if(tData->GetBool(MT_SELECTION_EDITING))
				{
					BaseSelect *bs = ToPoint(mesh)->GetPointS(); if(!bs) return true;
					
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					LONG morphCount = tData->GetLong(MT_BS_POINT_COUNT);
					
					CDArray<CDMPoint> tempPoints;
					CDArray<CDMSpPoint> tempSphPts;
					msadr.Copy(tempPoints);
					if(tData->GetBool(MT_MIDPOINT_EXISTS))
					{
						sphPt.Copy(tempSphPts);
					}
					
					LONG i,j,mInd;
					LONG bsPtCount = bs->GetCount();
					if(bsPtCount > 0)
					{
						msadr.Resize(bsPtCount);
						
						if(tData->GetBool(MT_MIDPOINT_EXISTS))
						{
							sphPt.Resize(bsPtCount);
						}
						
						tData->SetLong(MT_BS_POINT_COUNT,bsPtCount);
						
						// resize arrays
						LONG cnt=0,seg=0,a,b;
						while(CDGetRange(bs,seg++,&a,&b))
						{
							for (i=a; i<=b; ++i)
							{
								msadr[cnt].ind = i;
								msadr[cnt].SetVector(Vector(0,0,0));
								cnt++;
							}
						}
						
						// move the saved morphed points into resized array
						for (i=0; i<bsPtCount; i++)
						{
							if(tData->GetBool(MT_MIDPOINT_EXISTS))
							{
								mInd = msadr[i].ind;
								Vector mr,mp,md;
								mr = refadr[mInd].GetVector();
								mp = msadr[i].GetDelta(mr);
								
								md = CDBlend(mr,mp,0.5);
								sphPt[i].center.SetDelta(md,mr);
								sphPt[i].normal.SetVector(Vector(0,1,0));
							}
							for (j=0; j<morphCount; j++)
							{
								if(tempPoints[j].ind == msadr[i].ind)
								{
									msadr[i].x = tempPoints[j].x;
									msadr[i].y = tempPoints[j].y;
									msadr[i].z = tempPoints[j].z;
									if(tData->GetBool(MT_MIDPOINT_EXISTS))
									{
										sphPt[i].normal = tempSphPts[j].normal;
										sphPt[i].center = tempSphPts[j].center;
									}
								}
							}
						}
						tData->SetBool(MT_SELECTION_EXISTS,true);
						tData->SetBool(MT_SELECTION_EDITING,false);
						tData->SetMatrix(MT_REF_MATRIX,mesh->GetMg());
					}
					else
					{
						MessageDialog(GeLoadString(NO_POINTS));
					}
					tempPoints.Free();
					tempSphPts.Free();
				}
				else
				{
					BaseSelect *bs = ToPoint(mesh)->GetPointS(); if(!bs) return true;
					
					LONG bsPtCount = bs->GetCount();
					if(bsPtCount > 0)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						
						msadr.Alloc(bsPtCount);
						
						LONG cnt=0,seg=0,a,b,i;
						while(CDGetRange(bs,seg++,&a,&b))
						{
							for (i=a; i<=b; ++i)
							{
								msadr[cnt].ind = i;
								msadr[cnt].SetVector(Vector(0,0,0));
								cnt++;
							}
						}
						tData->SetBool(MT_SELECTION_EXISTS,true);
						tData->SetBool(MT_OFFSET_EXISTS,false);
						tData->SetLong(MT_BS_POINT_COUNT,bsPtCount);
						tData->SetMatrix(MT_REF_MATRIX,mesh->GetMg());
					}
					else
					{
						MessageDialog(GeLoadString(NO_POINTS));
					}
				}
			}
			else if(dc->id[0].id==MT_EDIT_SELECTION)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				BaseSelect *bs = ToPoint(mesh)->GetPointS();
				SendModelingCommand(MCOMMAND_UNHIDE, mcd);
				if(bs) bs->DeselectAll();
				
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					LONG i, mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							bs->Select(mInd);
						}
					}
					tData->SetBool(MT_SELECTION_EDITING,true);
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_RESTORE_SELECTION)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				BaseSelect *bs = ToPoint(mesh)->GetPointS();
				SendModelingCommand(MCOMMAND_UNHIDE, mcd);
				if(bs) bs->DeselectAll();
				
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					LONG i, mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							bs->Select(mInd);
						}
					}
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_SELECT_POINTS)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				BaseSelect *bs = ToPoint(mesh)->GetPointS();				
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					LONG i, mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							bs->Select(mInd);
						}
					}
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_SET_OFFSET)
			{
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					LONG i, mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							Vector pt;
							if(skTag && skData->GetBool(SKN_BOUND)) pt = SetSkinnedPoint(doc,mesh,padr[mInd],skTag,mInd);
							else pt = padr[mInd];
							msadr[i].SetDelta(pt,refadr[mInd].GetVector());
						}
					}
					
					if(tData->GetBool(MT_MIDPOINT_EXISTS) && !sphPt.IsEmpty())
					{
						for (i=0; i<bsPtCount; i++)
						{
							mInd = msadr[i].ind;
							if(mInd < ptCount)
							{
								Vector b, pt;
								Vector refPt = refadr[mInd].GetVector();
								Vector mpoint = msadr[i].GetDelta(refPt);
								if(!VEqual(refPt,mpoint,0.001))
								{
									Vector cPt = sphPt[i].center.GetDelta(refPt);
									pt = cPt + sphPt[i].normal.GetVector() * Len(refPt-cPt);
								}
								else pt = refPt;
								if(skTag && skData->GetBool(SKN_BOUND)) padr[mInd] = GetSkinnedPoint(doc,mesh,pt,skTag,mInd);
								else padr[mInd] = pt;
								
								if(skTag && skData->GetBool(SKN_BOUND)) b = SetSkinnedPoint(doc,mesh,padr[mInd],skTag,mInd);
								else b = padr[mInd];
								
								Vector center = CalcCenter(refPt,b,mpoint);
								sphPt[i].normal.SetVector(VNorm(b-center));
								sphPt[i].center.SetDelta(center,refPt);
							}
						}
					}
					
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							padr[mInd] = refadr[mInd].GetVector();
						}
					}
					tData->SetBool(MT_OFFSET_EDITING,false);
					tData->SetBool(MT_OFFSET_EXISTS,true);
					doc->SetMode(Mobject);
					SendModelingCommand(MCOMMAND_UNHIDE, mcd);
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_EDIT_OFFSET)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					// store current point selection
					BaseSelect *bs = ToPoint(mesh)->GetPointS();
					LONG bsCount = bs->GetCount();
					CDArray<LONG> ptSelected;
					ptSelected.Alloc(bsCount);
					
					LONG cnt=0,seg=0,a,b,i;
					while(CDGetRange(bs,seg++,&a,&b))
					{
						for (i=a; i<=b; ++i)
						{
							ptSelected[cnt] = i;
							cnt++;
						}
					}
					SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
					
					// hide points not in the morph selection
					LONG mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							bs->Select(mInd);
						}
					}
					SendModelingCommand(MCOMMAND_HIDEUNSELECTED, mcd);
					SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
					
					// set point positions to the offset
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							Vector pt = msadr[i].GetDelta(refadr[mInd].GetVector());
							
							if(skTag && skData->GetBool(SKN_BOUND)) padr[mInd] = GetSkinnedPoint(doc,mesh,pt,skTag,mInd);
							else padr[mInd] = pt;
						}
					}
					
					// restore the stored selection
					for (i=0; i<bsCount; i++)
					{
						mInd = ptSelected[i];
						bs->Select(mInd);
					}
					ptSelected.Free();
					
					tData->SetBool(MT_OFFSET_EDITING,true);
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_ERASE_OFFSET)
			{
				doc->SetActiveObject(mesh);
				doc->SetActiveTag(tag);
				
				LONG i, bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					LONG mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							msadr[i].SetVector(Vector(0,0,0));
						}
					}
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							Vector pt = msadr[i].GetDelta(refadr[mInd].GetVector());
							if(skTag && skData->GetBool(SKN_BOUND)) padr[mInd] = GetSkinnedPoint(doc,mesh,pt,skTag,mInd);
							else padr[mInd] = pt;
						}
					}
					tData->SetBool(MT_OFFSET_EXISTS,false);
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_HIDE_UNSELECTED)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					BaseSelect *bs = ToPoint(mesh)->GetPointS();
					LONG bsCount = bs->GetCount();
					CDArray<LONG> ptSelected;
					ptSelected.Alloc(bsCount);
					
					LONG cnt=0,seg=0,a,b,i;
					while(CDGetRange(bs,seg++,&a,&b))
					{
						for (i=a; i<=b; ++i)
						{
							ptSelected[cnt] = i;
							cnt++;
						}
					}
					SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
					
					LONG mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							bs->Select(mInd);
						}
					}
					SendModelingCommand(MCOMMAND_HIDEUNSELECTED, mcd);
					SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
					
					for (i=0; i<bsCount; i++)
					{
						mInd = ptSelected[i];
						bs->Select(mInd);
					}
					ptSelected.Free();
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_UNHIDE_ALL)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				if(!SendModelingCommand(MCOMMAND_UNHIDE, mcd)) return false;
			}
			else if(dc->id[0].id==MT_SET_MIDPOINT)
			{
				
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					LONG mpCnt = tData->GetLong(MT_MIDPOINT_COUNT);
					
					if(!sphPt.IsEmpty())
					{
						sphPt.Resize(bsPtCount);
					}
					else sphPt.Alloc(bsPtCount);
					
					Vector a,b,c;
					LONG i, mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							a = refadr[mInd].GetVector();
							c = msadr[i].GetDelta(a);
							if(skTag && skData->GetBool(SKN_BOUND)) b = SetSkinnedPoint(doc,mesh,padr[mInd],skTag,mInd);
							else b = padr[mInd];
							
							Vector center = CalcCenter(a,b,c);
							sphPt[i].normal.SetVector(VNorm(b-center));
							sphPt[i].center.SetDelta(center,a);
						}
					}
					
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							padr[mInd] = refadr[mInd].GetVector();
						}
					}
					
					if(!tData->GetBool(MT_MIDPOINT_EXISTS)) tData->SetBool(MT_MIDPOINT_EXISTS,true);
					tData->SetBool(MT_MIDPOINT_EDITING,false);
					tData->SetLong(MT_MIDPOINT_COUNT, bsPtCount);
					tData->SetBool(MT_USE_SLIDERS,false);
					doc->SetMode(Mobject);
					SendModelingCommand(MCOMMAND_UNHIDE, mcd);
				}
				else
				{
					MessageDialog(GeLoadString(NO_SELECTION));
				}
			}
			else if(dc->id[0].id==MT_EDIT_MIDPOINT)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				LONG bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
				if(bsPtCount > 0)
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					BaseSelect *bs = ToPoint(mesh)->GetPointS();
					LONG bsCount = bs->GetCount();
					CDArray<LONG> ptSelected;
					ptSelected.Alloc(bsCount);
					
					LONG cnt=0,seg=0,a,b,i;
					while(CDGetRange(bs,seg++,&a,&b))
					{
						for (i=a; i<=b; ++i)
						{
							ptSelected[cnt] = i;
							cnt++;
						}
					}
					SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
					
					LONG mInd;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							bs->Select(mInd);
						}
					}
					SendModelingCommand(MCOMMAND_HIDEUNSELECTED, mcd);
					SendModelingCommand(MCOMMAND_DESELECTALL, mcd);
					
					if(sphPt.IsEmpty()) sphPt.Alloc(bsPtCount);
					Vector pt, mpoint, refPt, cPt;
					for (i=0; i<bsPtCount; i++)
					{
						mInd = msadr[i].ind;
						if(mInd < ptCount)
						{
							refPt = refadr[mInd].GetVector();
							mpoint = msadr[i].GetDelta(refPt);
							if(tData->GetBool(MT_MIDPOINT_EXISTS))
							{
								if(!VEqual(refPt,mpoint,0.001))
								{
									cPt = sphPt[i].center.GetDelta(refPt);
									pt = cPt + sphPt[i].normal.GetVector() * Len(refPt-cPt);
								}
								else pt = refPt;
							}
							else
							{
								if(!VEqual(refPt,mpoint,0.001)) pt = refPt + (mpoint-refPt)*0.5;
								else pt = refPt;
							}
							if(skTag && skData->GetBool(SKN_BOUND)) padr[mInd] = GetSkinnedPoint(doc,mesh,pt,skTag,mInd);
							else padr[mInd] = pt;
						}
					}
					tData->SetBool(MT_MIDPOINT_EDITING,true);
					
					for (i=0; i<bsCount; i++)
					{
						mInd = ptSelected[i];
						bs->Select(mInd);
					}
					ptSelected.Free();
				}
			}
			else if(dc->id[0].id==MT_ERASE_MIDPOINT)
			{
				doc->SetActiveObject(mesh);
				doc->SetMode(Mpoints);
				doc->SetActiveTag(tag);
				
				tData->SetBool(MT_MIDPOINT_EXISTS,false);
				sphPt.Free();
			}
			break;
		}
	}
	
	return true;
}

void CDMorphTagPlugin::InitMidpointNormals(BaseObject *op, CDMRefPoint *refadr, BaseContainer *tData)
{
	Vector sphCen, cenPt, norm, refPt, mpoint, midPt;
	Vector *padr = GetPointArray(op), commonNorm = Vector(0,0,0);
	LONG i, mInd, bsPtCount = tData->GetLong(MT_BS_POINT_COUNT), ptCount = ToPoint(op)->GetPointCount();;
	
	if(bsPtCount > 0)
	{
		CDArray<Vector> ptNorm;
		ptNorm.Alloc(bsPtCount);
		
		// Calculate point normals
		for (i=0; i<bsPtCount; i++)
		{
			mInd = msadr[i].ind;
			if(mInd < ptCount)
			{
				ptNorm[i] = CDMCalcPointNormal(op,refadr,mInd);
			}
		}
		// Set points halfway and calculate the midpoint normals
		for (i=0; i<bsPtCount; i++)
		{
			mInd = msadr[i].ind;
			if(mInd < ptCount)
			{
				refPt = refadr[mInd].GetVector();
				mpoint = msadr[i].GetDelta(refPt);
				
				Vector perp = VCross(VNorm(ptNorm[i]), VNorm(mpoint-refPt));
				norm = VNorm(VCross(VNorm(mpoint-refPt), perp));
				sphPt[i].normal.SetVector(norm);
			}
		}
		// Calculate the common normal
		for (i=0; i<bsPtCount; i++)
		{
			mInd = msadr[i].ind;
			if(mInd < ptCount)
			{
				norm = sphPt[i].normal.GetVector();
				commonNorm += norm * 1.0/(Real)bsPtCount;
			}
		}
		tData->SetVector(MT_COMMON_NORM, VNorm(commonNorm));
		
		// Set a parent Matrix
		Matrix obbM = CDHPBToMatrix(VectorToHPB(VNorm(commonNorm)), op);
		CDAABB midPtBounds, refBounds, mrpBounds;
		midPtBounds.Empty();
		refBounds.Empty();
		mrpBounds.Empty();
		for (i=0; i<bsPtCount; i++)
		{
			mInd = msadr[i].ind;
			if(mInd < ptCount)
			{
				refPt = refadr[mInd].GetVector();
				mpoint = msadr[i].GetDelta(refPt);
				refBounds.AddPoint(MInv(obbM) * refPt);
				mrpBounds.AddPoint(MInv(obbM) * mpoint);
				
				midPt = refPt + (mpoint-refPt)*0.5;
				midPtBounds.AddPoint(MInv(obbM) * midPt);
			}
		}
		
		Vector rCen = refBounds.GetCenterPoint();
		rCen.z = refBounds.min.z;
		Vector mCen = mrpBounds.GetCenterPoint();
		mCen.z = mrpBounds.min.z;
		cenPt = rCen + (mCen-rCen)*0.5;
		tData->SetReal(MT_COMMON_WIDTH,midPtBounds.max.z - midPtBounds.min.z);
		Vector offPos = obbM * cenPt;
		obbM.off = offPos;
		tData->SetMatrix(MT_COMMON_PARENT,obbM);
		
		// Calculate centers
		for (i=0; i<bsPtCount; i++)
		{
			mInd = msadr[i].ind;
			if(mInd < ptCount)
			{
				refPt = refadr[mInd].GetVector();
				mpoint = msadr[i].GetDelta(refPt);
				midPt = refPt + (mpoint-refPt)*0.5;
				
				Vector midLocal = MInv(obbM) * midPt;
				midLocal.y = 0.0;
				midLocal.z = 0.0;
				cenPt = obbM * midLocal;
				
				norm = sphPt[i].normal.GetVector();
				Vector perp = VNorm(VCross(VNorm(commonNorm), VNorm(mpoint-refPt)));
				Vector cNorm = VNorm(VCross(VNorm(mpoint-refPt), perp));
				
				sphPt[i].center.SetDelta(cenPt,refPt);
				padr[mInd] = cenPt + cNorm * Len(refPt - cenPt);
			}
		}
		tData->SetBool(MT_MIDPOINT_EXISTS,true);
		ptNorm.Free();
	}
}

Bool CDMorphTagPlugin::MCEqual(CDMSpPoint sp)
{
	Vector m, c;
	
	c = sp.center.GetVector();
	m = sp.normal.GetVector();
	
	if(VEqual(m, c, 0.001)) return true;
	else return false;
}

Vector CDMorphTagPlugin::CalcCenter(Vector a, Vector b, Vector c)
{
	Vector center, vA, vB, vC, offP, dirP;
	Matrix m;
	
	if(VEqual(a, b, 0.001) || VEqual(c, b, 0.001)) return b;
	
	Real dot = VDot(VNorm(a-b), VNorm(c-b));
	if(dot > -0.996)
	{
		center.z = 0.0;
		
		m.off = a;
		m.v2 = VNorm(b-a);
		m.v3 = VNorm(VCross((c-a), m.v2));
		m.v1 = VNorm(VCross(m.v2, m.v3));
		
		vA = MInv(m) * a;
		vB = MInv(m) * b;
		vC = MInv(m) * c;
		
		offP = vB + VNorm(vC-vB) * (Len(vC-vB) * 0.5);
		dirP = VNorm(VCross((vC-vB), Vector(0,0,1)));
		
		center.y = Len(vB-vA)*0.5;
		
		Real k = (center.y - offP.y)/dirP.y;
		
		center.x = offP.x + dirP.x * k;
		
		return (m * center);
	}
	else
	{
		return b;
	}	
}

Vector CDMorphTagPlugin::MorphSlerp(Vector a, Vector b, CDMSpPoint sp, Real t)
{
	Vector output, dir, c, mp, v0, v1, v2;
	
	c = sp.center.GetDelta(a);
	mp = c + sp.normal.GetVector() * Len(a-c);
	
	Real dotP = VDot(VNorm(b-a), VNorm(b-mp));
	if(dotP > -0.9999 && dotP < 0.9999)
	{
		v0 = VNorm(a - c);
		v1 = VNorm(mp - c);
		v2 = VNorm(b - c);
		Real w = ACos(VDot(v0, v1)) + ACos(VDot(v1, v2));
		
		dir = (Sin((1-t)*w)/Sin(w))*v0 + (Sin(t*w)/Sin(w))*v2;
		
		output = (c + VNorm(dir) * Len(c-a))-a;
	}
	else
	{
		output = (b-a)*t;
	}
	
	return output;
}

Real CDMorphTagPlugin::GetJointRotationMix(BaseContainer *tData, BaseObject *op)
{
	Real mix = 0, value=0;
	BaseContainer *oData = op->GetDataInstance();
	
	if(op->GetType() == ID_CDJOINTOBJECT && oData)
	{
		Real minVal = tData->GetReal(MT_MIN_VALUE);
		Real maxVal = tData->GetReal(MT_MAX_VALUE);
		
		Vector opRot = oData->GetVector(JNT_TRANS_ROT);
		
		switch (tData->GetLong(MT_ROTATION_AXIS))
		{
			case MT_ROTATION_H:	
				value = opRot.x;	// Get the H Rotation
				break;
			case MT_ROTATION_P:
				value = opRot.y;	// Get the P Rotation
				break;
			case MT_ROTATION_B:	
				value = opRot.z;	// Get the B Rotation
				break;
				break;
		}
		if(minVal < maxVal)
		{
			if(tData->GetBool(MT_CLAMP_MIN) && value < minVal) value = minVal;
			if(tData->GetBool(MT_CLAMP_MAX) && value > maxVal) value = maxVal;
		}
		else if(minVal > maxVal)
		{
			if(tData->GetBool(MT_CLAMP_MIN) && value > minVal) value = minVal;
			if(tData->GetBool(MT_CLAMP_MAX) && value < maxVal) value = maxVal;
		}
		
		if((maxVal-minVal) != 0) //Check for division by zero
		{
			mix = (value-minVal)/(maxVal-minVal);
		}
		else
		{
			mix = 0;
		}
	}
	else
	{
		Real minVal = tData->GetReal(MT_MIN_VALUE);
		Real maxVal = tData->GetReal(MT_MAX_VALUE);
		Vector opRot;
		
		BaseTag *ztTag = op->GetTag(ID_CDZEROTRANSTAG);
		if(ztTag)
		{
			BaseContainer *ztData = ztTag->GetDataInstance();
			if(ztData) opRot = ztData->GetVector(ZT_TRANS_ROT);
		}
		else opRot = CDGetRot(op);
		
		switch (tData->GetLong(MT_ROTATION_AXIS))
		{
			case MT_ROTATION_H:	
				value = opRot.x;	// Get the H Rotation
				break;
			case MT_ROTATION_P:
				value = opRot.y;	// Get the P Rotation
				break;
			case MT_ROTATION_B:	
				value = opRot.z;	// Get the B Rotation
				break;
				break;
		}
		if(minVal < maxVal)
		{
			if(tData->GetBool(MT_CLAMP_MIN) && value < minVal) value = minVal;
			if(tData->GetBool(MT_CLAMP_MAX) && value > maxVal) value = maxVal;
		}
		else if(minVal > maxVal)
		{
			if(tData->GetBool(MT_CLAMP_MIN) && value > minVal) value = minVal;
			if(tData->GetBool(MT_CLAMP_MAX) && value < maxVal) value = maxVal;
		}
		
		if((maxVal-minVal) != 0) //Check for division by zero
		{
			mix = (value-minVal)/(maxVal-minVal);
		}
		else
		{
			mix = 0;
		}
	}
	
	return mix;
}

Bool CDMorphTagPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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

		tData->SetLink(MT_DEST_LINK,tData->GetLink(MT_DEST_LINK+T_LST,doc));
		tData->SetBool(MT_USE_MIDPOINT,tData->GetBool(MT_USE_MIDPOINT+T_LST));
		
		tData->SetBool(MT_USE_BONE_ROTATION,tData->GetBool(MT_USE_BONE_ROTATION+T_LST));
		tData->SetLong(MT_ROTATION_AXIS,tData->GetLong(MT_ROTATION_AXIS+T_LST));
		tData->SetReal(MT_MIN_VALUE,tData->GetReal(MT_MIN_VALUE+T_LST));
		tData->SetReal(MT_MAX_VALUE,tData->GetReal(MT_MAX_VALUE+T_LST));
		tData->SetBool(MT_CLAMP_MAX,tData->GetBool(MT_CLAMP_MAX+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDMorphTagPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	if(!tData->GetBool(MT_MIDPOINT_EXISTS)) tData->SetBool(MT_SHOW_GUIDE,false);
	
	BaseObject *mesh = GetMesh(doc,op,tData); if(!mesh) return false;
	if(!IsValidPointObject(mesh)) return false;
	
	Vector	*padr = GetPointArray(mesh); if(!padr) return false;
	Vector	mpoint, newMPt;
	LONG	ptCount = ToPoint(mesh)->GetPointCount();
	LONG	i, bsPtCount = tData->GetLong(MT_BS_POINT_COUNT);
	
	BaseTag *refTag = mesh->GetTag(ID_CDMORPHREFPLUGIN); if(!refTag) return false;
	BaseContainer *rfData = refTag->GetDataInstance(); if(!rfData) return false;
	
	BaseTag *skTag = mesh->GetTag(ID_CDSKINPLUGIN);
	BaseContainer *skData = NULL;
	if(skTag)  skData = skTag->GetDataInstance();
	
	CDMRefPoint	*refadr = CDGetMorphReference(refTag); if(!refadr) return false;
	CDMRefPoint	*mpadr = CDGetMorphCache(refTag); if(!mpadr) return false;
	
	Vector refPt, theScale = rfData->GetVector(1005);
	if(mesh == op) tData->SetBool(MT_USE_BONE_ROTATION,false);
	
	if(doc->IsEditMode() && (mesh->GetBit(BIT_ACTIVE)))
	{
		if(tData->GetBool(MT_MIDPOINT_EDITING) && tData->GetBool(MT_USE_SLIDERS))
		{
			if(!tData->GetBool(MT_MIDPOINT_EXISTS)) InitMidpointNormals(mesh,refadr,tData);
			LONG mInd;
			for (i=0; i<bsPtCount; i++)
			{
				mInd = msadr[i].ind;
				if(mInd < ptCount)
				{
					refPt = refadr[mInd].GetVector();
					mpoint = msadr[i].GetDelta(refPt);
					padr[mInd] = CalcSphericalMidPoint(tData,refPt,mpoint,sphPt[i]);
				}
			}
			mesh->Message(MSG_UPDATE);
		}
	}
	else
	{
		tData->SetBool(MT_MIDPOINT_EDITING,false);
		if(doc->GetMode() == Mmodel && doc->GetAction() == ID_MODELING_SCALE)
		{
			if(theScale != Vector(1,1,1))
			{
				tData->SetBool(MT_M_SCALING,true);
			}
			else tData->SetBool(MT_M_SCALING,false);
			
			return false;
		}
		if(tData->GetBool(MT_M_SCALING))
		{
			tData->SetBool(MT_M_SCALING,false);
			return false;
		}
		
		Real theMix=0;
		if(tData->GetBool(MT_USE_BONE_ROTATION))
		{
			theMix = GetJointRotationMix(tData,op);
		}
		else
		{
			theMix = tData->GetReal(MT_MIX_SLIDER);
		}
		
		LONG mInd;
		for (i=0; i<bsPtCount; i++)
		{
			mInd = msadr[i].ind;
			if(mInd < ptCount)
			{
				refPt = refadr[mInd].GetVector();
				mpoint = msadr[i].GetDelta(refPt);
				
				if(skTag && skData->GetBool(SKN_BOUND))
				{
					Vector mrPt;
					mrPt = mpadr[mInd].GetVector();
					if(tData->GetBool(MT_USE_MIDPOINT) && tData->GetBool(MT_MIDPOINT_EXISTS))
					{
						if(!MCEqual(sphPt[i]))
						{
							mrPt += MorphSlerp(refPt,mpoint,sphPt[i],theMix);
						}
						else
						{
							mrPt += (mpoint-refPt)*theMix;
						}
					}
					else
					{
						mrPt += (mpoint-refPt)*theMix;
					}
					mpadr[mInd].SetVector(mrPt);
				}
				else
				{
					if(tData->GetBool(MT_USE_MIDPOINT) && tData->GetBool(MT_MIDPOINT_EXISTS))
					{
						if(!MCEqual(sphPt[i]))
						{
							padr[mInd] += MorphSlerp(refPt,mpoint,sphPt[i],theMix);
						}
						else
						{
							padr[mInd] += (mpoint-refPt)*theMix;
						}
					}
					else
					{
						padr[mInd] += (mpoint-refPt)*theMix;
					}
					mpadr[mInd].SetVector(padr[mInd]);
				}
			}
		}
		mesh->Message(MSG_UPDATE);
		tData->SetMatrix(MT_REF_MATRIX,mesh->GetMg());
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Vector CDMorphTagPlugin::CalcSphericalMidPoint(BaseContainer *tData, Vector rPt, Vector mPt, CDMSpPoint sPt)
{
	Vector sphCen, cenPt, midPt, halfPt, cNorm, norm, mdNorm, mixNorm, localCen;
	Vector commonNorm = tData->GetVector(MT_COMMON_NORM);
	Real cw = tData->GetReal(MT_COMMON_WIDTH), h = tData->GetReal(MT_MID_OFFSET);
	Matrix obbM = tData->GetMatrix(MT_COMMON_PARENT);
	
	mdNorm = sPt.normal.GetVector();
	Vector perp = VNorm(VCross(commonNorm, VNorm(mPt-rPt)));
	cNorm = VNorm(VCross(VNorm(mPt-rPt), perp));
	
	sphCen = sPt.center.GetDelta(rPt);
	cenPt = sphCen - commonNorm * cw * h;
	midPt = cenPt + cNorm * Len(rPt-cenPt);
	
	halfPt = rPt + (mPt-rPt) * 0.5;
	Real midLen = Len(midPt - halfPt);
	norm = VNorm(CDBlend(cNorm,mdNorm,tData->GetReal(MT_MID_WIDTH)));
	midPt = halfPt + norm * midLen;
	
	return midPt;
}

Bool CDMorphTagPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(MT_MID_CONTROLS), ar);
	if(bc)
	{
		if(!tData->GetBool(MT_MIDPOINT_EDITING)) bc->SetBool(DESC_HIDE, true);
		else bc->SetBool(DESC_HIDE, false);
	}
	bc = description->GetParameterI(DescLevel(MT_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	bc = description->GetParameterI(DescLevel(MT_SELECTED_ONLY), ar);
	if(bc)
	{
		if(!tData->GetBool(MT_MIDPOINT_EDITING)) bc->SetBool(DESC_HIDE, true);
		else bc->SetBool(DESC_HIDE, false);
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDMorphTagPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case MT_DEST_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MT_SET_SELECTION:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(!tData->GetBool(MT_SELECTION_EXISTS) || tData->GetBool(MT_SELECTION_EDITING)) return true;
				else return false;
			}
		case MT_SET_OFFSET:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(MT_SELECTION_EXISTS) && !tData->GetBool(MT_OFFSET_EXISTS) && !tData->GetBool(MT_SELECTION_EDITING)) return true;
				else if(tData->GetBool(MT_SELECTION_EXISTS) && tData->GetBool(MT_OFFSET_EDITING)) return true;
				else return false;
			}
		case MT_RESTORE_SELECTION:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_SELECTION_EXISTS);
		case MT_UNHIDE_ALL:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_SELECTION_EXISTS);
		case MT_SELECT_POINTS:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_SELECTION_EXISTS);
		case MT_EDIT_SELECTION:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(MT_SELECTION_EXISTS) && !tData->GetBool(MT_SELECTION_EDITING) && !tData->GetBool(MT_OFFSET_EDITING))return true;
				else return false;
			}
		case MT_HIDE_UNSELECTED:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_SELECTION_EXISTS);
		case MT_EDIT_OFFSET:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(MT_OFFSET_EXISTS) && !tData->GetBool(MT_SELECTION_EDITING) && !tData->GetBool(MT_OFFSET_EDITING))return true;
				else return false;
			}
		case MT_ERASE_OFFSET:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(MT_OFFSET_EXISTS) && !tData->GetBool(MT_SELECTION_EDITING) && !tData->GetBool(MT_OFFSET_EDITING))return true;
				else return false;
			}
		case MT_USE_MIDPOINT:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_OFFSET_EXISTS);
		case MT_SET_MIDPOINT:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(MT_USE_MIDPOINT) && doc->IsEditMode() && tData->GetBool(MT_MIDPOINT_EDITING)) return true;
				else return false;
			}
		case MT_EDIT_MIDPOINT:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(MT_USE_MIDPOINT) && !tData->GetBool(MT_MIDPOINT_EDITING)) return true;
				else return false;
			}
		case MT_ERASE_MIDPOINT:
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetBool(MT_MIDPOINT_EXISTS) && !tData->GetBool(MT_MIDPOINT_EDITING))return true;
				else return false;
			}
		case MT_USE_SLIDERS:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_MIDPOINT_EDITING);
		case MT_MID_OFFSET:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_USE_SLIDERS);
		case MT_MID_WIDTH:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_USE_SLIDERS);
		case MT_SHOW_GUIDE:	
			return tData->GetBool(MT_MIDPOINT_EXISTS);
		case MT_SELECTED_ONLY:	
			return tData->GetBool(MT_MIDPOINT_EDITING);
		case MT_GUIDE_COLOR:	
			return tData->GetBool(MT_MIDPOINT_EXISTS);
		case MT_MIX_SLIDER:
			if(!tData->GetBool(MT_USE_BONE_ROTATION)) return true;
			else return false;
		case MT_USE_BONE_ROTATION:	
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				if(tData->GetObjectLink(MT_DEST_LINK,doc) == op) return false;
				else return true;
			}
		case MT_ROTATION_AXIS:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_USE_BONE_ROTATION);
		case MT_MIN_VALUE:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_USE_BONE_ROTATION);
		case MT_MAX_VALUE:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_USE_BONE_ROTATION);
		case MT_CLAMP_MIN:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_USE_BONE_ROTATION);
		case MT_CLAMP_MAX:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MT_USE_BONE_ROTATION);
	}
	return true;
}

Bool RegisterCDMorphTagPlugin(void)
{
	if(CDFindPlugin(ID_CDMORPHTAGPLUGIN,CD_TAG_PLUGIN)) return true;

	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDM_SERIAL_SIZE];
	String cdmnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDMORPH,data,CDM_SERIAL_SIZE)) reg = false;
	
	cdmnr.SetCString(data,CDM_SERIAL_SIZE-1);
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
	
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_MULTIPLE|TAG_VISIBLE;
	
	String name=GeLoadString(IDS_CDMORPHTAG); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDMORPHTAGPLUGIN,name,info,CDMorphTagPlugin::Alloc,"tCDMorphTag","CDMorphTag.tif",3);
}

// library functions
CDMPoint* iCDGetMorphPoints(BaseTag *tag)
{
	CDMorphTagPlugin *mTag = static_cast<CDMorphTagPlugin*>(tag->GetNodeData());
	return mTag ? mTag->GetMorphPoints() : NULL;
}

CDMSpPoint* iCDGetMorphSphericalPoints(BaseTag *tag)
{
	CDMorphTagPlugin *mTag = static_cast<CDMorphTagPlugin*>(tag->GetNodeData());
	return mTag ? mTag->GetSphericalPoints() : NULL;
}
