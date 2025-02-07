//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "CDTagData.h"

#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"
#include "CDJoint.h"
#include "CDClusterTag.h"
#include "CDPaintSkin.h"
//#include "CDDebug.h"

#include "tCDSkin.h"
#include "tCDCluster.h"

// cdm includes
#include "CDMorphRef.h"
#include "tCDMorphRef.h"

enum // CD IK Tools
{
	IK_SQUASH_N_STRETCH			= 3004
};

Bool CDSkinPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(SKN_BOUND, false);
	tData->SetBool(SKN_W_SET,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);
	
	jCountChange = false;
	
	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_EXPRESSION);
			pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,200);
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	CDJSData skd;
	PluginMessage(ID_CDSKINPLUGIN,&skd);
	if(skd.list) skd.list->Append(node);
	
	return true;
}

void CDSkinPlugin::Free(GeListNode *node)
{
	BaseTag	*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	jIndM.Free();
	pIndM.Free();
	jM.Free();
	J.Free();
	skinWeight.Free();

	BaseObject *op = tag->GetObject();
	if(op)
	{
		BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN);
		if(skrTag)
		{
			BaseContainer *skrData = skrTag->GetDataInstance();
			skrData->SetLink(SKR_TAG_LINK,tag);
			
			DescriptionCommand dc;
			dc.id = DescID(SKR_DOUBLE_CHECK);
			skrTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
		}
	}
	
	CDJSData skd;
	PluginMessage(ID_CDSKINPLUGIN,&skd);
	if(skd.list) skd.list->Remove(node);
}

Bool CDSkinPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag	*tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG i, pCnt = tData->GetLong(SKN_S_POINT_COUNT);
	LONG j, jCnt = tData->GetLong(SKN_J_COUNT);
	
	if(level < 1)
	{
		if(pCnt > 0)
		{
			for(i=0; i<pCnt; i++)
			{
				Real temp;
				CDHFReadFloat(hf, &temp);
			}
		}
		if(pCnt > 0 && jCnt > 0)
		{
			if(!skinWeight.Alloc(pCnt)) return false;
			for(i=0; i<pCnt; i++)
			{
				if(!skinWeight[i].jw.Alloc(jCnt)) return false;
				if(!skinWeight[i].jPlus.Alloc(jCnt)) return false;
				
				CDHFReadFloat(hf, &skinWeight[i].taw);
				CDHFReadLong(hf, &skinWeight[i].jn);
				for(j=0; j<jCnt; j++)
				{
					CDHFReadFloat(hf, &skinWeight[i].jw[j]);
					CDHFReadLong(hf, &skinWeight[i].jPlus[j]);
				}
			}
		}
	}
	else
	{
		if(pCnt > 0 && jCnt > 0)
		{
			if(!skinWeight.Alloc(pCnt)) return false;
			for(i=0; i<pCnt; i++)
			{
				if(!skinWeight[i].jw.Alloc(jCnt)) return false;
				if(!skinWeight[i].jPlus.Alloc(jCnt)) return false;
				
				CDHFReadFloat(hf, &skinWeight[i].taw);
				CDHFReadLong(hf, &skinWeight[i].jn);
				for(j=0; j<jCnt; j++)
				{
					CDHFReadFloat(hf, &skinWeight[i].jw[j]);
					CDHFReadLong(hf, &skinWeight[i].jPlus[j]);
				}
			}
		}
	}
	
	return true;
}

Bool CDSkinPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag	*tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG i, pCnt = tData->GetLong(SKN_S_POINT_COUNT);
	LONG j, jCnt = tData->GetLong(SKN_J_COUNT);

	//Level 1
	if (pCnt > 0 && jCnt > 0)
	{
		for (i=0; i<pCnt; i++)
		{
			CDHFWriteFloat(hf, skinWeight[i].taw);
			CDHFWriteLong(hf, skinWeight[i].jn);
			for (j=0; j<jCnt; j++)
			{
				CDHFWriteFloat(hf, skinWeight[i].jw[j]);
				CDHFWriteLong(hf, skinWeight[i].jPlus[j]);
			}
		}
	}
	
	return true;
}

Bool CDSkinPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	BaseTag	*tag  = (BaseTag*)snode; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	LONG i, pCnt = tData->GetLong(SKN_S_POINT_COUNT);
	LONG jCnt = tData->GetLong(SKN_J_COUNT);
	
	if(pCnt > 0 && jCnt > 0)
	{
		skinWeight.Copy(((CDSkinPlugin*)dest)->skinWeight);
		jM.Copy(((CDSkinPlugin*)dest)->jM);
		J.Copy(((CDSkinPlugin*)dest)->J);
	}

	return true;
}

Bool CDSkinPlugin::GetWeight(BaseContainer *tData, Real *tWeight, LONG pCnt)
{
	BaseDocument *doc = GetActiveDocument(); if(!doc) return false;
	
	if(skinWeight.IsEmpty()) return false;
	
	LONG i, jntID = tData->GetLong(SKN_J_LINK_ID);
	if(!tData->GetObjectLink(SKN_J_LINK+jntID,doc)) return false;
	
	for(i=0; i<pCnt; i++)
	{
		tWeight[i] = skinWeight[i].jw[jntID];
	}
	
	return true;
}

Bool CDSkinPlugin::SetWeight(BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt)
{
	Bool normPaint = tData->GetBool(SKN_NORMALIZED_WEIGHT);
	Real totalWeight,totalWt;
	
	BaseObject *jnt = NULL;
	LONG i, j, jCnt = tData->GetLong(SKN_J_COUNT), jIndex = SKN_J_LINK;
	for(i=0; i<pCnt; i++)
	{
		if(skinWeight[i].jw[tData->GetLong(SKN_J_LINK_ID)] != tWeight[i])
		{
			skinWeight[i].jw[tData->GetLong(SKN_J_LINK_ID)] = tWeight[i];
			skinWeight[i].jn = 0;
			totalWeight = 0.0;
			for(j=0; j<jCnt; j++)
			{
				jnt = tData->GetObjectLink(jIndex+j,doc);
				if(jnt)
				{							
					if(skinWeight[i].jw[j] > 0.01)
					{
						totalWeight += skinWeight[i].jw[j];
						skinWeight[i].jPlus[skinWeight[i].jn] = j;
						skinWeight[i].jn += 1;
					}
				}
			}
			if(!normPaint)
			{
				skinWeight[i].taw = totalWeight;
			}
			else
			{
				LONG j, jCnt, indPlus;
				if(tWeight[i] < 1.0)
				{
					totalWt = totalWeight - tWeight[i];
					if(totalWt > 0.0)
					{
						jCnt = skinWeight[i].jn;
						for(j=0; j<jCnt; j++)
						{
							indPlus = skinWeight[i].jPlus[j];
							jnt = tData->GetObjectLink(jIndex+indPlus,doc);
							if(jnt && jnt != tData->GetObjectLink(jIndex+tData->GetLong(SKN_J_LINK_ID),doc))
							{
								Real wt = (1.0 - tWeight[i]) * (skinWeight[i].jw[indPlus]/totalWt);
								skinWeight[i].jw[indPlus] = wt;
							}
						}
						skinWeight[i].taw = 1.0;
					}
					else
					{
						skinWeight[i].jPlus[0] = tData->GetLong(SKN_J_LINK_ID);
						skinWeight[i].jw[tData->GetLong(SKN_J_LINK_ID)] = tWeight[i];
						skinWeight[i].jn = 1;
						skinWeight[i].taw = tWeight[i];
					}
				}
				else
				{
					jCnt = skinWeight[i].jn;
					for(j=0; j<jCnt; j++)
					{
						indPlus = skinWeight[i].jPlus[j];
						jnt = tData->GetObjectLink(jIndex+indPlus,doc);
						if(jnt && jnt != tData->GetObjectLink(jIndex+tData->GetLong(SKN_J_LINK_ID),doc))
						{
							skinWeight[i].jw[indPlus] = 0.0;
						}
					}
					skinWeight[i].jPlus[0] = tData->GetLong(SKN_J_LINK_ID);
					skinWeight[i].jw[tData->GetLong(SKN_J_LINK_ID)] = 1.0;
					skinWeight[i].jn = 1;
					skinWeight[i].taw = 1.0;
				}
				
			}
		}
		else
		{
			if(normPaint && tWeight[i] == 1.0)
			{
				LONG jCnt, indPlus;
				jCnt = skinWeight[i].jn;
				for(j=0; j<jCnt; j++)
				{
					indPlus = skinWeight[i].jPlus[j];
					jnt = tData->GetObjectLink(jIndex+indPlus,doc);
					if(jnt && jnt != tData->GetObjectLink(jIndex+tData->GetLong(SKN_J_LINK_ID),doc))
					{
						skinWeight[i].jw[indPlus] = 0.0;
					}
				}
				skinWeight[i].jPlus[0] = tData->GetLong(SKN_J_LINK_ID);
				skinWeight[i].jw[tData->GetLong(SKN_J_LINK_ID)] = 1.0;
				skinWeight[i].jn = 1;
				skinWeight[i].taw = 1.0;
			}
		}
	}
	
	return true;
}

Bool CDSkinPlugin::GetJointColors(BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt)
{
	LONG i, j, jCnt, indPlus;
	BaseObject *jnt = NULL;
	
	for(i=0; i<pCnt; i++)
	{
		jColor[i] = Vector(0,0,0);
		jCnt = skinWeight[i].jn;
		for(j=0; j<jCnt; j++)
		{
			indPlus = skinWeight[i].jPlus[j];
			jnt = tData->GetObjectLink(SKN_J_LINK+indPlus,doc);
			if(jnt)
			{
				ObjectColorProperties prop;
				jnt->GetColorProperties(&prop);
				if(prop.usecolor > 0)
				{
					if(CDMinVector(prop.color,0.1))  jColor[i] += ((Vector(0.1,0.1,0.1))/Real(jCnt));
					else  jColor[i] += (prop.color/Real(jCnt));
				}
				else jColor[i] += ((Vector(0,0,0.75))/Real(jCnt));
			}
		}
	}
	
	return TRUE;
}

void CDSkinPlugin::AccumulateWeights(BaseDocument *doc, BaseContainer *tData, LONG pCnt, LONG jCnt)
{
	if(!doc) doc = GetActiveDocument();
	Real totalWt;
	LONG i, j;
	for(i=0; i<pCnt; i++)
	{
		skinWeight[i].jn = 0;
		totalWt = 0.0;
		for(j=0; j<jCnt; j++)
		{
			BaseObject *jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
			if(jnt)
			{							
				if(skinWeight[i].jw[j] > 0)
				{
					totalWt += skinWeight[i].jw[j];
					skinWeight[i].jPlus[skinWeight[i].jn] = j;
					skinWeight[i].jn += 1;
				}
			}
		}
		skinWeight[i].taw = totalWt;
	}
}

void CDSkinPlugin::NormalizeAllWeights(BaseDocument *doc, BaseContainer *tData, LONG pCnt, LONG jCnt, Real *minWt)
{
	Real min = 0.0;
	LONG i, j;
	
	if(!skinWeight.IsEmpty())
	{
		if(minWt) min = *minWt;
		
		if(min > 0.0)
		{
			for(i=0; i<pCnt; i++)
			{
				if(skinWeight[i].taw > 0)
				{
					LONG jwCnt = skinWeight[i].jn;
					for(j=0; j<jwCnt; j++)
					{
						LONG indPlus = skinWeight[i].jPlus[j];
						BaseObject *jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
						if(jnt)
						{							
							Real wt = skinWeight[i].jw[indPlus];
							if(wt < min) wt = 0.0;
							skinWeight[i].jw[indPlus] = wt;
						}
					}
				}
			}
		}
		AccumulateWeights(doc,tData,pCnt,jCnt);
		
		for(i=0; i<pCnt; i++)
		{
			if(skinWeight[i].taw > 0)
			{
				LONG jwCnt = skinWeight[i].jn;
				Real tfw = 0.0;
				for(j=0; j<jwCnt; j++)
				{
					LONG indPlus = skinWeight[i].jPlus[j];
					BaseObject *jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
					if(jnt)
					{							
						Real wt = TruncatePercentFractions(skinWeight[i].jw[indPlus] / skinWeight[i].taw);
						tfw += wt;
						if(j == jwCnt-1)
						{
							Real rem = 1 - tfw;
							if(rem > 0) wt += rem;
						}
						skinWeight[i].jw[indPlus] = wt;
					}
				}
				skinWeight[i].taw = 1.0;
			}
		}
	}
}

void CDSkinPlugin::InitSkinWeight(BaseContainer *tData, LONG pCnt, LONG jCnt)
{
	LONG i, j;
	
	skinWeight.Free();
	
	skinWeight.Alloc(pCnt);
	for(i=0; i<pCnt; i++)
	{
		skinWeight[i].jw.Alloc(jCnt);
		skinWeight[i].jPlus.Alloc(jCnt);
		for(j=0; j<jCnt; j++)
		{
			skinWeight[i].jw[j] = 0;
		}
	}
	
	tData->SetLong(SKN_S_POINT_COUNT,pCnt);
	tData->SetBool(SKN_W_SET,true);
}

void CDSkinPlugin::InitJointMirrorIndex(BaseDocument *doc, BaseObject *op, BaseContainer *tData, LONG *jIndM, LONG jCnt)
{
	BaseObject *jnt=NULL, *jm=NULL, *firstOp = tData->GetObjectLink(SKN_MJ_ID,doc);
	Matrix opMatrix = op->GetMg(), jMatrix, targetMatrix, mirMatrix;
	
	LONG j, mj, axis = tData->GetLong(SKN_MIRROR_AXIS), jIndex = SKN_J_LINK;
	Real dif, jtolerance = tData->GetReal(SKN_J_TOLERANCE);
	Bool mirrored = false;
	
	tData->SetBool(SKN_J_MIRRORED,true);

	StatusSetText("Checking Joint Symmetry");
	for(j=0; j<jCnt; j++)
	{
		StatusSetBar(LONG(Real(j)/Real(jCnt)*100.0));
		jIndM[j] = -1;
		jnt = tData->GetObjectLink(jIndex+j,doc);
		if(jnt)
		{
			mirrored = false;
			
			BaseContainer *jData = jnt->GetDataInstance();
			if(jData)
			{
				jm = jData->GetObjectLink(JNT_JOINT_M,doc);
				if(!jm)
				{
					jMatrix = jnt->GetMg();
					targetMatrix = MInv(opMatrix) * jMatrix;
					switch (axis)
					{
						case 0:
							targetMatrix.off.x *= -1;
							break;
						case 1:
							targetMatrix.off.y *= -1;
							break;
						case 2:
							targetMatrix.off.z *= -1;
							break;
						default:
						break;
					}
					mirMatrix = opMatrix * targetMatrix;
					
					Real prvDif = CDMAXREAL;
					for(mj=0; mj<jCnt; mj++)
					{
						jm = tData->GetObjectLink(jIndex+mj,doc);
						if(jm)
						{
							dif = Len(jm->GetMg().off - mirMatrix.off);
							if(dif<jtolerance)
							{
								if(dif < prvDif)
								{
									jIndM[j] = mj;
									if(jnt == firstOp) tData->SetLink(SKN_MJ_ID,jm);
									
									prvDif = dif;
									mirrored = true;
								}
							}
						}
					}
				}
				else
				{
					for(mj=0; mj<jCnt; mj++)
					{
						BaseObject *jMir = tData->GetObjectLink(jIndex+mj,doc);
						if(jMir == jm)
						{
							jIndM[j] = mj;
							if(jnt == firstOp) tData->SetLink(SKN_MJ_ID,jm);
						}
					}
					mirrored = true;
				}
				
				if(!mirrored)  tData->SetBool(SKN_J_MIRRORED,false);
			}
		}
	}
	StatusClear();
}

void CDSkinPlugin::InitPointMirrorIndex(BaseObject *op, BaseContainer *tData, CDMRefPoint *skinRef, LONG *pIndM, LONG pCnt)
{
	Bool useSymtag = false;
	LONG i, *symadr = NULL;
	
	tData->SetBool(SKN_P_MIRRORED,true);
	
	BaseTag *symTag = op->GetTag(ID_CDMIRRORPOINTSTAG);
	if(symTag)
	{
		BaseContainer *stData = symTag->GetDataInstance();
		if(stData)
		{
			if(stData->GetBool(10002))
			{
				symadr = CDSymGetMirrorPoints(symTag);
				if(symadr) useSymtag = true;
			}
		}
	}
	
	if(useSymtag)
	{
		for(i=0; i<pCnt; i++)
		{
			pIndM[i] = symadr[i];
		}
	}
	else
	{
		LONG mi, axis = tData->GetLong(SKN_MIRROR_AXIS);
		Real dif, ptolerance = tData->GetReal(SKN_P_TOLERANCE);
		Bool mirrored = false;
		Vector pt,mpt;

		StatusSetText("Checking Mesh Symmetry");
		for(i=0; i<pCnt; i++)
		{
			StatusSetBar(LONG(Real(i)/Real(pCnt)*100.0));
			pIndM[i] = -1;
			pt.x = skinRef[i].x;
			pt.y = skinRef[i].y;
			pt.z = skinRef[i].z;
			switch (axis)
			{
				case 0:
					pt.x *= -1;
					break;
				case 1:
					pt.y *= -1;
					break;
				case 2:
					pt.z *= -1;
					break;
				default:
				break;
			}
			
			mirrored = false;
			Real prvDif = CDMAXREAL;
			for(mi=0; mi<pCnt; mi++)
			{
				mpt.x = skinRef[mi].x;
				mpt.y = skinRef[mi].y;
				mpt.z = skinRef[mi].z;
				dif = Len(mpt - pt);
				if(dif<ptolerance)
				{
					if(dif < prvDif)
					{
						pIndM[i] = mi;
						
						prvDif = dif;
						mirrored = true;
					}
				}
			}
			if(!mirrored)  tData->SetBool(SKN_P_MIRRORED,false);
		}
		StatusClear();
	}
}

void CDSkinPlugin::CheckTagReg(BaseObject *op, BaseContainer *tData)
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
			tData->SetLink(SKN_DEST_ID,op);
			tData->SetBool(T_SET,true);
		}
	}
}

void CDSkinPlugin::RemapSkinWeights(BaseContainer *tData, TranslationMaps *map)
{
	if(!skinWeight.IsEmpty() && map)
	{
		LONG oCnt = map->m_oPointCount, nCnt = map->m_nPointCount;
		LONG i, oInd, nInd, mCnt = map->m_mPointCount;
		LONG j, jCnt = tData->GetLong(SKN_J_COUNT);
		
		CDArray<CDSkinVertex> tempSkinWt;
		skinWeight.Copy(tempSkinWt);
		tempSkinWt.Resize(nCnt);
		
		if(nCnt > oCnt)
		{
			for(i=oCnt; i<nCnt; i++)
			{
				tempSkinWt[i].jw.Alloc(jCnt);
				tempSkinWt[i].jPlus.Alloc(jCnt);
			}
		}
		
		for(i=0; i<mCnt; i++)
		{
			oInd = map->m_pPointMap[i].oIndex;
			nInd = map->m_pPointMap[i].nIndex;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_DELETED) continue;
			
			if(map->m_pPointMap[i].lFlags & TRANSMAP_FLAG_NEW)
			{
				for(j=0; j<jCnt; j++)
				{
					tempSkinWt[nInd].jw[j] = 0.0;
				}
			}
			else if(nInd > -1 && nInd < nCnt)
			{
				for(j=0; j<jCnt; j++)
				{
					tempSkinWt[nInd].jw[j] = skinWeight[oInd].jw[j];
					tempSkinWt[nInd].jPlus[j] = skinWeight[oInd].jPlus[j];
				}
			}
		}
		skinWeight.Free();
		tempSkinWt.Copy(skinWeight);
		tempSkinWt.Free();
	}
}

Bool CDSkinPlugin::TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg)
{
	LONG i, jCnt = tData->GetLong(SKN_J_COUNT);
	
	LONG nCnt = vchg->new_cnt, oCnt = vchg->old_cnt;
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
			RemapSkinWeights(tData,&tMap);
			
			AccumulateWeights(doc,tData,nCnt,jCnt);
			tData->SetLong(SKN_S_POINT_COUNT,nCnt);
			
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
			RemapSkinWeights(tData,&tMap);
			
			AccumulateWeights(doc,tData,nCnt,jCnt);
			tData->SetLong(SKN_S_POINT_COUNT,nCnt);
			
			pMap.Free();
		}
	}
	else
	{
		if(nCnt > oCnt)
		{
			//GePrint("nCnt > oCnt");
			TranslationMaps tMap;
			
			tMap.m_oPointCount = oCnt; // old point count
			tMap.m_nPointCount = nCnt; // new point count
			tMap.m_mPointCount = nCnt - oCnt; // map point count
			
			// Allocate the m_pPointMap array
			CDArray<TransMapData> pMap;
			
			LONG mCnt = 0;
			for(i=oCnt; i<nCnt; i++)
			{
				TransMapData tmd = TransMapData(-1,i);
				tmd.mIndex = mCnt;
				tmd.lFlags |= TRANSMAP_FLAG_NEW;
				pMap.Append(tmd);
				mCnt++;
			}
			tMap.m_pPointMap = pMap.Array();
			
			if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
			RemapSkinWeights(tData,&tMap);
			
			AccumulateWeights(doc,tData,nCnt,jCnt);
			tData->SetLong(SKN_S_POINT_COUNT,nCnt);
			
			pMap.Free();
		}
	}
	
	return true;
}

static Bool IsSquashAndStretchOn(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, jCnt = tData->GetLong(SKN_J_COUNT);
	for(i=0; i<jCnt; i++)
	{
		BaseObject *jnt = tData->GetObjectLink(SKN_J_LINK+i,doc);
		if(jnt)
		{
			BaseTag *ikTag = jnt->GetTag(ID_CDLIMBIKPLUGIN);
			if(ikTag && ikTag->GetData().GetBool(IK_SQUASH_N_STRETCH)) return true;
			
			ikTag = jnt->GetTag(ID_CDQUADLEGPLUGIN);
			if(ikTag && ikTag->GetData().GetBool(IK_SQUASH_N_STRETCH)) return true;
			
			ikTag = jnt->GetTag(ID_CDSPLINEIKPLUGIN);
			if(ikTag && ikTag->GetData().GetBool(IK_SQUASH_N_STRETCH)) return true;
			
			ikTag = jnt->GetTag(ID_CDSPINALPLUGIN);
			if(ikTag && ikTag->GetData().GetBool(IK_SQUASH_N_STRETCH)) return true;
			
			ikTag = jnt->GetTag(ID_CDDUALTARGETPLUGIN);
			if(ikTag && ikTag->GetData().GetBool(IK_SQUASH_N_STRETCH)) return true;
		}
	}
	
	return false;
}

void CDSkinPlugin::InitJointStorage(LONG jCnt)
{
	jM.Alloc(jCnt);
	J.Alloc(jCnt);
}

Bool CDSkinPlugin::UpdateJointStorage(BaseDocument *doc, BaseContainer *tData)
{
	if(!tData || !doc || jM.IsEmpty()) return false;
	
	LONG j, jCnt = tData->GetLong(SKN_J_COUNT);
	
	if(jCnt > 0)
	{
		for(j=0; j<jCnt; j++)
		{
			jM[j].jnt = NULL;
			BaseObject *jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
			if(jnt)
			{
				// store joint pointer
				jM[j].jnt = jnt;
				
				//store translation matrices
				jM[j].m  = tData->GetMatrix(SKN_J_MATRIX+j);
				jM[j].trnsM = jnt->GetMg();
				
				// store ss info
				jM[j].base = false;
				jM[j].useSpline = false;
				jM[j].stretch = 1.0;
				jM[j].squash = 1.0;
				if(jnt->GetType() == ID_CDJOINTOBJECT)
				{
					BaseContainer *jData = jnt->GetDataInstance();
					
					jM[j].base = jData->GetBool(JNT_J_BASE);
					jM[j].stretch = jData->GetReal(JNT_STRETCH_VALUE);
					jM[j].squash = jData->GetReal(JNT_SQUASH_VALUE);
					
					jM[j].useSpline = jData->GetBool(JNT_USE_SPLINE);
					if(jM[j].useSpline)
					{
						jM[j].splStart = jData->GetReal(JNT_SPL_START);
						jM[j].splEnd = jData->GetReal(JNT_SPL_END);
						jM[j].length = jData->GetReal(JNT_J_LENGTH);
						jM[j].spline = (SplineData*)jData->GetCustomDataType(JNT_S_AND_S_SPLINE,CUSTOMDATATYPE_SPLINE);
					}
				}
			}
		}
	}
	return true;
}

Bool CDSkinPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;
	
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
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	CheckTagReg(op,tData);

	// Check for SkinRef tag
	DescriptionCommand skrdc;
	BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN);
	if(!skrTag) return true;
	
	LONG i, j, jCnt = tData->GetLong(SKN_J_COUNT);
	LONG oldPCnt = tData->GetLong(SKN_S_POINT_COUNT);
	
	BaseObject *jnt = NULL;
	if(doc)
	{
		jnt = tData->GetObjectLink(SKN_J_LINK,doc);
		tData->SetBool(SKN_SQUASH_N_STRETCH_ON, IsSquashAndStretchOn(doc,tData));
	}

	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			if(tData->GetBool(SKN_REF_EDITING))
			{
				if(skrTag)
				{
					BaseContainer *skrData = skrTag->GetDataInstance();
					if(skrData) skrData->SetBool(SKN_REF_EDITING,true);
				}
			}
			InitJointStorage(jCnt);
			break;
		}
		case MSG_MENUPREPARE:
		{
			LONG pCnt = ToPoint(op)->GetPointCount();
			InitSkinWeight(tData,pCnt,jCnt);
			InitJointStorage(jCnt);
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
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					RemapSkinWeights(tData,tMap);
					AccumulateWeights(doc,tData,nCnt,jCnt);
					tData->SetLong(SKN_S_POINT_COUNT,nCnt);
				}
			}
			break;
		}
		case  CD_MSG_UPDATE:
		{
			LONG njCnt = 0;
			for(j=0; j<jCnt; j++)
			{
				jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
				if(jnt)
				{
					njCnt++;
				}
			}
			
			if(njCnt < jCnt)
			{
				if(njCnt == 0)
				{
					DescriptionCommand dc;
					dc.id = DescID(SKR_RESTORE_REFERENCE);
					skrTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
				}
				
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				LONG skCnt = tData->GetLong(SKN_S_POINT_COUNT);
				
				// Create temp array
				CDArray<CDSkinVertex> tempSkinWeight;
				skinWeight.Copy(tempSkinWeight);
				
				LONG ind, mind;
				Matrix jMatrix;
				for(i=0; i<skCnt; i++)
				{
					skinWeight[i].jw.Resize(njCnt);
					skinWeight[i].jPlus.Resize(njCnt);
					
					LONG cnt = 0;
					for(j=0; j<jCnt; j++)
					{
						jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
						if(jnt && cnt<njCnt)
						{
							skinWeight[i].jw[cnt] = tempSkinWeight[i].jw[j];
							cnt++;
						}
					}
				}
				for(j=0; j<njCnt; j++)
				{
					ind = SKN_J_LINK+j;
					mind = SKN_J_MATRIX+j;
					jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
					if(!jnt)
					{
						while(!jnt && (ind < SKN_J_LINK+jCnt))
						{
							ind++;
							mind++;
							jnt = tData->GetObjectLink(ind,doc);
							jMatrix = tData->GetMatrix(mind);
						}
						if(jnt)
						{
							tData->SetLink(SKN_J_LINK+j,jnt);
							tData->SetMatrix(SKN_J_MATRIX+j,jMatrix);
							tData->SetLink(ind,NULL);
						}
					}
				}
				tempSkinWeight.Free();
				
				AccumulateWeights(doc,tData,skCnt,njCnt);
				tData->SetLong(SKN_J_COUNT,njCnt);
				
				InitJointStorage(njCnt);
				
				BaseContainer *tbc=GetToolData(GetActiveDocument(),ID_SKINWEIGHTTOOL);
				if(tbc) tbc->SetLong(WP_SKIN_JOINT_COUNT,njCnt);
			}
			break;
		}
		case CD_MSG_RECALC_REFERENCE:
		{
			CDTransMatrixData *tmData = (CDTransMatrixData*)data;
			if(tmData)
			{
				tData->SetMatrix(SKN_SKIN_MATRIX,tmData->nM);
			}
			break;
		}
		case CD_MSG_NORMALIZE_WEIGHT:
		{
			Real *minWt = (Real*)data;
			LONG pCnt = ToPoint(op)->GetPointCount();
			NormalizeAllWeights(doc,tData,pCnt,jCnt,minWt);
			break;
		}
		case CD_MSG_FREEZE_TRANSFORMATION:
		{
			Vector *trnsSca = (Vector*)data;
			if(trnsSca)
			{
				Vector sca = *trnsSca;
				Matrix refM;
				
				refM = tData->GetMatrix(SKN_SKIN_MATRIX);
				refM.off.x *= sca.x;
				refM.off.y *= sca.y;
				refM.off.z *= sca.z;
				tData->SetMatrix(SKN_SKIN_MATRIX,refM);
				
				for(i=0; i<jCnt; i++)
				{
					refM = tData->GetMatrix(SKN_J_MATRIX+i);
					refM.off.x *= sca.x;
					refM.off.y *= sca.y;
					refM.off.z *= sca.z;
					tData->SetMatrix(SKN_J_MATRIX+i,refM);
					
					refM = tData->GetMatrix(SKN_JL_MATRIX+i);
					refM.off.x *= sca.x;
					refM.off.y *= sca.y;
					refM.off.z *= sca.z;
					tData->SetMatrix(SKN_JL_MATRIX+i,refM);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SKN_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==SKN_UNBIND_SKIN)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				tData->SetBool(SKN_BOUND,false);
				if(skrTag)
				{
					skrdc.id = DescID(SKR_RESTORE_REFERENCE);
					skrTag->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
				}
				
				CDJSData scd;
				PluginMessage(ID_CDCLUSTERTAGPLUGIN,&scd);
				if(scd.list)
				{
					LONG i, scCnt = scd.list->GetCount();
					BaseContainer *scData = NULL;
					BaseTag *scTag = NULL;
					for(i=0; i<scCnt; i++)
					{
						scTag = static_cast<BaseTag*>(scd.list->GetIndex(i));
						if(scTag->GetDocument())
						{
							scData = scTag->GetDataInstance();
							if(scData)
							{
								if(op == scData->GetObjectLink(CLST_DEST_LINK,doc))
								{
									DescriptionCommand scdc;
									scdc.id = DescID(CLST_UNBIND_POINTS);
									scTag->Message(MSG_DESCRIPTION_COMMAND,&scdc);
								}
							}
						}
					}
				}
			}
			else if (dc->id[0].id==SKN_BIND_SKIN)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				tData->SetMatrix(SKN_SKIN_MATRIX,op->GetMg());
				
				for(i=0; i<jCnt; i++)
				{
					jnt = tData->GetObjectLink(SKN_J_LINK+i,doc);
					if(jnt)
					{
						tData->SetMatrix(SKN_J_MATRIX+i,jnt->GetMg());
						tData->SetMatrix(SKN_JL_MATRIX+i,jnt->GetMl());
					}
				}
				tData->SetBool(SKN_BOUND,true);
				
				CDJSData scd;
				PluginMessage(ID_CDCLUSTERTAGPLUGIN,&scd);
				if(scd.list)
				{
					LONG i, scCnt = scd.list->GetCount();
					BaseContainer *scData = NULL;
					BaseTag *scTag = NULL;
					for(i=0; i<scCnt; i++)
					{
						scTag = static_cast<BaseTag*>(scd.list->GetIndex(i));
						if(scTag->GetDocument())
						{
							scData = scTag->GetDataInstance();
							if(scData)
							{
								if(op == scData->GetObjectLink(CLST_DEST_LINK,doc))
								{
									DescriptionCommand scdc;
									scdc.id = DescID(CLST_BIND_POINTS);
									scTag->Message(MSG_DESCRIPTION_COMMAND,&scdc);
								}
							}
						}
					}
				}
			}
			else if (dc->id[0].id==SKN_AUTO_BIND_SKIN)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				tData->SetMatrix(SKN_SKIN_MATRIX,op->GetMg());
				
				for(i=0; i<jCnt; i++)
				{
					jnt = tData->GetObjectLink(SKN_J_LINK+i,doc);
					if(jnt)
					{
						tData->SetMatrix(SKN_J_MATRIX+i,jnt->GetMg());
						tData->SetMatrix(SKN_JL_MATRIX+i,jnt->GetMl());
					}
				}
				tData->SetBool(SKN_BOUND,true);
			}
			else if (dc->id[0].id==SKN_EDIT_REF)
			{
				CDAddUndo(doc,CD_UNDO_CHANGE,tag);
				doc->SetActiveObject(op);
				doc->SetMode(Mpoints);
				if(skrTag)
				{
					skrdc.id = DescID(SKR_RESTORE_REFERENCE);
					skrTag->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
					BaseContainer *skrData = skrTag->GetDataInstance();
					if(skrData) skrData->SetBool(SKN_REF_EDITING,true);
				}
				
				BaseTag *refTag = op->GetTag(ID_CDMORPHREFPLUGIN);
				if(refTag)
				{
					DescriptionCommand mrdc;
					mrdc.id = DescID(MR_EDIT_MESH);
					refTag->Message(MSG_DESCRIPTION_COMMAND,&mrdc);
					BaseContainer *rtData = refTag->GetDataInstance();
					if(rtData) rtData->SetBool(MR_EDITING,true);
				}
				tData->SetBool(SKN_REF_EDITING,true);
				doc->SetActiveTag(tag);
			}
			else if (dc->id[0].id==SKN_RESET_REF)
			{
				doc->StartUndo();
				
				if(skrTag)
				{
					skrdc.id = DescID(SKR_UPDATE_REFERENCE);
					skrTag->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
					BaseContainer *skrData = skrTag->GetDataInstance();
					if(skrData) skrData->SetBool(SKN_REF_EDITING,false);
				}
				
				BaseTag *refTag = op->GetTag(ID_CDMORPHREFPLUGIN);
				if(refTag)
				{
					DescriptionCommand mrdc;
					mrdc.id = DescID(MR_RESET_REF);
					refTag->Message(MSG_DESCRIPTION_COMMAND,&mrdc);
					BaseContainer *rtData = refTag->GetDataInstance();
					if(rtData) rtData->SetBool(MR_EDITING,false);
				}
				doc->EndUndo();
				
				tData->SetBool(SKN_REF_EDITING,false);
				doc->SetMode(Mobject);
			}
			else if (dc->id[0].id==SKN_ACCUMULATE)
			{
				LONG pCnt = ToPoint(op)->GetPointCount();
				AccumulateWeights(doc,tData,pCnt,jCnt);
			}
			else if (dc->id[0].id==SKN_ADD_JOINT)
			{
				//GePrint("SKN_ADD_JOINT");
				LONG jCntChg = tData->GetLong(SKN_J_COUNT_CHANGE);
				if(jCntChg > jCnt)
				{
					LONG pCnt = ToPoint(op)->GetPointCount();
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					//Create temp array
					
					// Store existing array in temp array
					for(i=0; i<pCnt; i++)
					{
						skinWeight[i].jw.Resize(jCntChg);
						skinWeight[i].jPlus.Resize(jCntChg);
						
						for(j=jCnt; j<jCntChg; j++)
						{
							skinWeight[i].jw[j] = 0;
						}
					}
					
					tData->SetLong(SKN_J_COUNT,jCntChg);
					InitJointStorage(jCntChg);
					
					AccumulateWeights(doc,tData,pCnt,jCntChg);
				}
			}
			else if (dc->id[0].id==SKN_DELETE_JOINT)
			{
				LONG jCntChg = tData->GetLong(SKN_J_COUNT_CHANGE);
				if(jCntChg < jCnt)
				{
					LONG pCnt = ToPoint(op)->GetPointCount();
					CDAddUndo(doc,CD_UNDO_CHANGE,tag);
					
					CDArray<CDSkinVertex> tempSkinWeight;
					skinWeight.Copy(tempSkinWeight);
					
					LONG cnt;
					for(i=0; i<pCnt; i++)
					{
						skinWeight[i].jw.Resize(jCntChg);
						skinWeight[i].jPlus.Resize(jCntChg);
						
						cnt = 0;
						for(j=0; j<jCnt; j++)
						{
							jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
							if(jnt && cnt<jCntChg)
							{
								skinWeight[i].jw[cnt] = tempSkinWeight[i].jw[j];
								cnt++;
							}
						}
					}
					LONG ind;
					for(j=0; j<jCntChg; j++)
					{
						ind = SKN_J_LINK+j;
						jnt = tData->GetObjectLink(SKN_J_LINK+j,doc);
						if(!jnt)
						{
							while(!jnt && (ind < SKN_J_LINK+jCnt))
							{
								ind++;
								jnt = tData->GetObjectLink(ind,doc);
							}
							if(jnt)
							{
								tData->SetLink(SKN_J_LINK+j,jnt);
								tData->SetLink(ind,NULL);
							}
						}
					}
					tempSkinWeight.Free();
					
					tData->SetLong(SKN_J_COUNT,jCntChg);
					InitJointStorage(jCntChg);
					
					AccumulateWeights(doc,tData,pCnt,jCntChg);
				}				
			}
			else if (dc->id[0].id==SKN_INIT_MJ_INDEX)
			{
				// Init mirror jnt index array
				jIndM.Alloc(jCnt);
				InitJointMirrorIndex(doc,op,tData,jIndM.Array(),jCnt);
			}
			else if (dc->id[0].id==SKN_INIT_MP_INDEX)
			{
				CDMRefPoint  *skinRef = CDGetSkinReference(skrTag);
				
				// Init mirror point index array
				LONG pCnt = ToPoint(op)->GetPointCount();
				pIndM.Alloc(pCnt);
				InitPointMirrorIndex(op,tData,skinRef,pIndM.Array(),pCnt);
			}
			else if (dc->id[0].id==SKN_FREE_MIRROR_IND)
			{
				// Free mirror index arrays
				pIndM.Free();
				jIndM.Free();
			}
			else if (dc->id[0].id==SKN_MIRROR_WEIGHT)
			{
				CDMRefPoint  *skinRef = CDGetSkinReference(skrTag);
				
				LONG skCnt = tData->GetLong(SKN_S_POINT_COUNT);
				
				LONG axis = tData->GetLong(SKN_MIRROR_AXIS);
				LONG direction = tData->GetLong(SKN_MIRROR_DIRECTION);
				LONG jInd = tData->GetLong(SKN_J_LINK_ID);
				if(jIndM[jInd] > -1)
				{
					Vector mPt;
					for(i=0; i<skCnt; i++)
					{
						mPt.x = skinRef[i].x;
						mPt.y = skinRef[i].y;
						mPt.z = skinRef[i].z;
						if(pIndM[i] > -1)
						{
							switch (direction)
							{
								case 0:
								{
									switch (axis)
									{
										case 0:
										{
											if(mPt.x >= 0.0)
											{
												skinWeight[pIndM[i]].jw[jIndM[jInd]] = skinWeight[i].jw[jInd];
											}
											break;
										}
										case 1:
										{
											if(mPt.y >= 0.0)
											{
												skinWeight[pIndM[i]].jw[jIndM[jInd]] = skinWeight[i].jw[jInd];
											}
											break;
										}
										case 2:
										{
											if(mPt.z >= 0.0)
											{
												skinWeight[pIndM[i]].jw[jIndM[jInd]] = skinWeight[i].jw[jInd];
											}
											break;
										}
										default:
											break;
									}
									break;
								}
								case 1:
								{
									switch (axis)
									{
										case 0:
										{
											if(mPt.x <= 0.0)
											{
												skinWeight[pIndM[i]].jw[jIndM[jInd]] = skinWeight[i].jw[jInd];
											}
											break;
										}
										case 1:
										{
											if(mPt.y <= 0.0)
											{
												skinWeight[pIndM[i]].jw[jIndM[jInd]] = skinWeight[i].jw[jInd];
											}
											break;
										}
										case 2:
										{
											if(mPt.z <= 0.0)
											{
												skinWeight[pIndM[i]].jw[jIndM[jInd]] = skinWeight[i].jw[jInd];
											}
											break;
										}
										default:
											break;
									}
									break;
								}
								default:
									break;
							}
						}
					}
					
				}
			}
			else if (dc->id[0].id==SKN_GO_TO_BIND_POSE)
			{
				for(i=0; i<jCnt; i++)
				{
					jnt = tData->GetObjectLink(SKN_J_LINK+i,doc);
					if(jnt)
					{
						jnt->SetMl(tData->GetMatrix(SKN_JL_MATRIX+i));
					}
				}
				skrdc.id = DescID(SKR_RESTORE_REFERENCE);
				skrTag->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
			}
			break;
		}
	}
	
	UpdateJointStorage(doc,tData);

	return true;
}

LONG CDSkinPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return false;
	LONG i, pCnt = ToPoint(op)->GetPointCount(), skCnt = tData->GetLong(SKN_S_POINT_COUNT);	
	
	Bool tagMoved = false;
	if(op != tData->GetObjectLink(SKN_DEST_ID,doc))
	{
		BaseObject *tOp = tData->GetObjectLink(SKN_DEST_ID,doc);
		if(tOp)
		{
			if(tOp->GetDocument())
			{
				BaseTag *skrTag = tOp->GetTag(ID_CDSKINREFPLUGIN);
				if(skrTag)
				{
					DescriptionCommand sktdc;
					sktdc.id = DescID(SKR_RESTORE_REFERENCE);
					skrTag->Message(MSG_DESCRIPTION_COMMAND,&sktdc);
				}
				tagMoved = true;
				tData->SetBool(T_MOV,true);
			}
		}
		if(!tagMoved && !tData->GetBool(T_MOV) && pCnt == skCnt)  tData->SetLink(SKN_DEST_ID,op);
	}
	else tData->SetBool(T_MOV,false);
	if(tagMoved || tData->GetBool(T_MOV)) return false;
	
	BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN); if(!skrTag) return false;
	if(!IsValidPointObject(op)) return false;
	
	Vector  refPt, *padr = GetPointArray(op); if(!padr) return false;
	BaseObject	*jnt = tData->GetObjectLink(SKN_J_LINK,doc); 
	if(!jnt) return false;
	
	if(!tData->GetBool(SKN_W_SET)) return false;
	
	LONG j, jCnt, indPlus;
	if(!tData->GetBool(SKN_BOUND)) return true;

	// Calculate the mesh
	if(pCnt == skCnt)
	{
		CDMRefPoint  *skinRef=NULL;
		skinRef = CDGetSkinReference(skrTag);
		
		BaseContainer *refTData = NULL;
		BaseTag *refTag = op->GetTag(ID_CDMORPHREFPLUGIN);
		if(refTag)
		{
			refTData = refTag->GetDataInstance();
			if(refTData->GetBool(MR_REFERENCE_NEW))
			{
				skinRef = CDGetMorphCache(refTag);
			}
		}
		if(!skinRef) return false;
		
		if(doc->GetMode() == Mmodel && doc->GetAction() == ID_MODELING_SCALE)
		{
			if(ActiveModelToolScaling(op)) return false;
		}
		if(doc->IsEditMode() && (op->GetBit(BIT_ACTIVE)))
		{
			if(refTData)
			{
				if(refTData->GetLong(MR_CDMR_LEVEL) < 2) return false;
				else
				{
					if(refTData->GetBool(MR_DELTA_EDITING)) return false;
				}
			}
		}
		
		Bool ssOn = tData->GetBool(SKN_SQUASH_N_STRETCH_ON);

		// Precalculate jnt info
		jCnt = tData->GetLong(SKN_J_COUNT);
		Matrix  jTransM, sMatrix = tData->GetMatrix(SKN_SKIN_MATRIX), opMatrix=op->GetMg();

		BaseContainer *jData = NULL;
		for(j=0; j<jCnt; j++)
		{
			jnt = jM[j].jnt;
			if(jnt)
			{
				jM[j].trnsM = jnt->GetMg();
				jM[j].stretch = 1.0;
				jM[j].squash = 1.0;
				if(jnt->GetType() == ID_CDJOINTOBJECT)
				{
					jData = jnt->GetDataInstance();
					if(jData)
					{
						jM[j].stretch = jData->GetReal(JNT_STRETCH_VALUE);
						jM[j].squash = jData->GetReal(JNT_SQUASH_VALUE);
					}
				}
				J[j] = MInv(opMatrix) * jM[j].trnsM * MInv(jM[j].m) * sMatrix;
			}
		}
			
		Vector skinVert, theColor;
		for(i=0; i<pCnt; i++)
		{
			skinVert = Vector();
			
			refPt = skinRef[i].GetVector();
			
			jCnt = skinWeight[i].jn;
			if(skinWeight[i].taw > 0)
			{
				for(j=0; j<jCnt; j++)
				{
					indPlus = skinWeight[i].jPlus[j];
					if(indPlus < tData->GetLong(SKN_J_COUNT))
					{
						jnt = jM[indPlus].jnt;
						if(jnt)
						{
							if(ssOn)
							{
								jTransM = jM[indPlus].trnsM;
								Vector htY, ptV = ((sMatrix * refPt) - jM[indPlus].m.off);
								Real bias = 1.0, adj, dotProd = VDot(VNorm(ptV), VNorm(jM[indPlus].m.v3));
								
								if(dotProd > 0 || !(jM[indPlus].base))
								{
									jTransM.v3 *= jM[indPlus].stretch;
									bias = jM[indPlus].squash;
									
									if(jM[indPlus].useSpline && jM[indPlus].spline)
									{
										adj = dotProd * Len(ptV);
										Real splMix, sqMix, bMix;
										
										splMix = CDBlend(jM[indPlus].splStart,jM[indPlus].splEnd,adj/jM[indPlus].length);
										htY = jM[indPlus].spline->GetPoint(splMix);
										sqMix = CDBlend(1.0,jM[indPlus].squash,htY.y);
										
										if(jM[indPlus].squash > 1.0 && jnt->GetDown())
										{
											bMix = Len(CDGetPos(jnt->GetDown()))/jM[indPlus].length;
											bias = CDBlend(sqMix,1.0,bMix);
										}
										else if(jM[indPlus].squash < 1.0 && jnt->GetDown())
										{
											bMix = jM[indPlus].length/Len(CDGetPos(jnt->GetDown()));
											bias = CDBlend(sqMix,1.0,bMix);
										}
									}
								}
								
								jTransM.v1 *= bias;
								jTransM.v2 *= bias;
								J[indPlus] = MInv(opMatrix) * jTransM * MInv(jM[indPlus].m) * sMatrix;
							}
							skinVert += J[indPlus] * refPt * (skinWeight[i].jw[indPlus] / skinWeight[i].taw);
						}
					}
				}
			}
			else skinVert = refPt;
			padr[i] = skinVert;
		}
		op->Message(MSG_UPDATE);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSkinPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SKN_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSkinPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(tData->GetBool(T_MOV)) return false;
	
	switch (id[0].id)
	{
		case SKN_BIND_SKIN:	
			if(!tData->GetBool(SKN_BOUND) && !tData->GetBool(SKN_REF_EDITING)) return true;
			else return false;
		case SKN_UNBIND_SKIN:	
			if (tData->GetBool(SKN_BOUND))return true;
			else return false;
		case SKN_EDIT_REF:	
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(SKN_BOUND) && !tData->GetBool(SKN_REF_EDITING)) return true;
			else return false;
		case SKN_RESET_REF:	
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(SKN_BOUND) && tData->GetBool(SKN_REF_EDITING)) return true;
			else return false;
	}
	return true;
}

Bool RegisterCDSkinPlugin(void)
{
	if(CDFindPlugin(ID_CDSKINPLUGIN,CD_TAG_PLUGIN)) return true;
	
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
	
	// decide by name if the plugin shall be registered - just for user convenience 
	String name=GeLoadString(IDS_CDSKIN); if (!name.Content()) return true;

	return CDRegisterTagPlugin(ID_CDSKINPLUGIN,name,TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE,CDSkinPlugin::Alloc,"tCDSkin","CDSkin.tif",1);
}


// library functions
Bool iCDGetJointWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG pCnt)
{
	CDSkinPlugin *skTag = static_cast<CDSkinPlugin*>(tag->GetNodeData());
	return skTag ? skTag->GetWeight(tData, tWeight, pCnt) : false;
}

Bool iCDSetJointWeight(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Real *tWeight, LONG pCnt)
{
	CDSkinPlugin *skTag = static_cast<CDSkinPlugin*>(tag->GetNodeData());
	return skTag ? skTag->SetWeight(doc, tData, tWeight, pCnt) : false;
}

Bool iCDGetSkinJointColors(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, Vector *jColor, LONG pCnt)
{
	CDSkinPlugin *skTag = static_cast<CDSkinPlugin*>(tag->GetNodeData());
	return skTag ? skTag->GetJointColors(doc, tData, jColor, pCnt) : false;
}

CDSkinVertex* iCDGetSkinWeights(BaseTag *tag)
{
	CDSkinPlugin *skTag = static_cast<CDSkinPlugin*>(tag->GetNodeData());
	return skTag ? skTag->GetSkinWeight() : NULL;
}

