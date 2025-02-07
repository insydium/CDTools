//
//  CDRopeTag.cpp
//	Cactus Dan's Cloth
//	Copyright 2013 by Cactus Dan Libisch
//

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "customgui_priority.h"

#include "CDGeneral.h"
#include "CDMessages.h"
#include "CDCompatibility.h"
//#include "CDDebug.h"

#include "tCDRopeTag.h"
#include "tCDClothCollider.h"
#include "CDRopeTag.h"
#include "CDCloth.h"


Bool CDRopeTagPlugin::Init(GeListNode *node)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetReal(RP_DAMPING, 0.01);
	tData->SetReal(RP_STIFFNESS, 0.0);
	tData->SetReal(RP_TIME_STEP, 2.5);
	tData->SetLong(RP_ITERATIONS, 25);
	
	tData->SetBool(RP_USE_GRAVITY, true);
	tData->SetReal(RP_GRAVITY,-9.8);
	
	tData->SetLong(RP_POINT_COUNT, 0);
	tData->SetLong(RP_CONSTR_COUNT, 0);
	tData->SetBool(RP_POINTS_ADDED, false);
	
	tData->SetVector(RP_WIND_DIRECTION, Vector(0,0,1));
	tData->SetReal(RP_TURBULENCE_FREQUENCY,1.6);
	tData->SetReal(RP_TURBULENCE_AMPLITUDE, 13.8);
	tData->SetReal(RP_TURBULENCE_SPEED, 2.5);
	
	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd)
		{
			pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_EXPRESSION);
			pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,300);
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}
	
	return true;
}

void CDRopeTagPlugin::Free(GeListNode *node)
{
	if(ref.Size() > 0) ref.Free();
	if(particles.Size() > 0) particles.Free();
	if(constraints.Size() > 0) constraints.Free();
}

Bool CDRopeTagPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG i;
	LONG ptCnt = tData->GetLong(RP_POINT_COUNT);
	if(ptCnt > 0)
	{
		if(!ref.Alloc(ptCnt)) return false;
		for(i=0; i<ptCnt; i++)
		{
			hf->ReadReal(&ref[i].x);
			hf->ReadReal(&ref[i].y);
			hf->ReadReal(&ref[i].z);
		}
		
		if(!particles.Alloc(ptCnt)) return false;
		for(i=0; i<ptCnt; i++)
		{
			hf->ReadReal(&particles[i].mass);
			hf->ReadReal(&particles[i].wt);
			hf->ReadReal(&particles[i].pos.x);
			hf->ReadReal(&particles[i].pos.y);
			hf->ReadReal(&particles[i].pos.z);
			hf->ReadReal(&particles[i].old_pos.x);
			hf->ReadReal(&particles[i].old_pos.y);
			hf->ReadReal(&particles[i].old_pos.z);
			hf->ReadReal(&particles[i].acceleration.x);
			hf->ReadReal(&particles[i].acceleration.y);
			hf->ReadReal(&particles[i].acceleration.z);
		}
	}
	
	LONG cCnt = tData->GetLong(RP_CONSTR_COUNT);
	if(cCnt > 0)
	{
		if(!constraints.Alloc(cCnt)) return false;
		for(i=0; i<cCnt; i++)
		{
			hf->ReadLong(&constraints[i].aInd);
			hf->ReadLong(&constraints[i].bInd);
			hf->ReadLong(&constraints[i].type);
			hf->ReadReal(&constraints[i].rest_distance);
		}
	}
	
	return true;
}

Bool CDRopeTagPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG i;
	
	//Level 1
	LONG ptCnt = tData->GetLong(RP_POINT_COUNT);
	if(ptCnt > 0)
	{
		for(i=0; i<ptCnt; i++)
		{
			hf->WriteReal(ref[i].x);
			hf->WriteReal(ref[i].y);
			hf->WriteReal(ref[i].z);
		}
		
		for(i=0; i<ptCnt; i++)
		{
			hf->WriteReal(particles[i].mass);
			hf->WriteReal(particles[i].wt);
			hf->WriteReal(particles[i].pos.x);
			hf->WriteReal(particles[i].pos.y);
			hf->WriteReal(particles[i].pos.z);
			hf->WriteReal(particles[i].old_pos.x);
			hf->WriteReal(particles[i].old_pos.y);
			hf->WriteReal(particles[i].old_pos.z);
			hf->WriteReal(particles[i].acceleration.x);
			hf->WriteReal(particles[i].acceleration.y);
			hf->WriteReal(particles[i].acceleration.z);
		}
	}
	
	LONG cCnt = tData->GetLong(RP_CONSTR_COUNT);
	if(cCnt > 0)
	{
		for(i=0; i<cCnt; i++)
		{
			hf->WriteLong(constraints[i].aInd);
			hf->WriteLong(constraints[i].bInd);
			hf->WriteLong(constraints[i].type);
			hf->WriteReal(constraints[i].rest_distance);
		}
	}
	return true;
}

Bool CDRopeTagPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	//GePrint("CDRopeTagPlugin::CDCopyto()");
	BaseTag *tag  = (BaseTag*)snode; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG ptCnt = tData->GetLong(RP_POINT_COUNT);
	if(ptCnt > 0)
	{
		ref.Copy(((CDRopeTagPlugin*)dest)->ref);
		particles.Copy(((CDRopeTagPlugin*)dest)->particles);
	}
	
	if(constraints.Size() > 0)
	{
		constraints.Copy(((CDRopeTagPlugin*)dest)->constraints);
	}
	
	return true;
}

Bool CDRopeTagPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	//GePrint("CDRopeTagPlugin::CDDraw()");
	return true;
	
	// debug drawing
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	if(!op->GetBit(BIT_ACTIVE)) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	Matrix opM = op->GetMg();
	
	Vector *padr = GetPointArray(op);
	
	LONG i, a, b, cCnt = constraints.Size(), ptCnt = tData->GetLong(RP_POINT_COUNT);
	for(i=0; i<cCnt; i++)
	{
		a = constraints[i].aInd;
		b = constraints[i].bInd;
		if(a < ptCnt && b < ptCnt)
		{
			switch(constraints[i].type)
			{
				case CD_EDGE_CONSTRAINT:
					bd->SetPen(Vector(1,1,0));
					break;
				case CD_SHEAR_CONSTRAINT:
					bd->SetPen(Vector(0,1,1));
					break;
				case CD_BEND_CONSTRAINT:
					bd->SetPen(Vector(1,0,1));
					break;
			}
			CDDrawLine(bd, opM * padr[i], opM * padr[i]);
		}
	}
	
	return true;
}

Bool CDRopeTagPlugin::InitParticles(BaseObject *op, BaseContainer *tData)
{
	if(!op || !tData) return false;
	if(!IsValidPointObject(op)) return false;
	Vector *padr = GetPointArray(op); if(!padr) return false;
	
	LONG ptCnt = ToPoint(op)->GetPointCount(); if(ptCnt < 1) return false;
	
	Matrix opM = op->GetMg();
	
	if(!ref.Alloc(ptCnt)) return false;
	if(!particles.Alloc(ptCnt)) return false;
	
	LONG i;
	for(i=0; i<ptCnt; i++)
	{
		ref[i] = padr[i];
		Vector pt = opM * padr[i];
		particles[i].SetParticle(pt,pt,Vector(0,0,0),pt,1.0,0.0);
	}
	tData->SetLong(RP_POINT_COUNT, ptCnt);
	constraints.Init();
	
	return CreateConstraints(op, tData);
}

Bool CDRopeTagPlugin::GetWeight(BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	if(!tWeight) return false;
	
	ClearMem(tWeight,(sizeof(Real)*tCnt),0);
	//if(particles)
	if(!particles.IsEmpty())
	{
		LONG i;
		for(i=0; i<tCnt; i++)
		{
			if(i < tCnt) tWeight[i] = particles[i].wt;
		}
	}
	
	return true;
}

Bool CDRopeTagPlugin::SetWeight(BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	if(!op) return false;
	if(!IsValidPointObject(op)) return false;
	if(!tWeight) return false;
	
	//if(!particles) return false;
	if(particles.IsEmpty()) return false;
	
	LONG i;
	for(i=0; i<tCnt; i++)
	{
		Real wt = tWeight[i];
		particles[i].wt = TruncatePercentFractions(wt);
	}
	
	return true;
}

Bool CDRopeTagPlugin::ConstraintExists(LONG a, LONG b)
{
	LONG cCnt = constraints.Size();
	if(cCnt > 0)
	{
		LONG i;
		for(i=0; i<cCnt; i++)
		{
			CLConstraint ct = constraints[i];
			if(ct.aInd == a && ct.bInd == b) return true;
			else if(ct.aInd == b && ct.bInd == a) return true;
		}
	}
	
	return false;
}

void CDRopeTagPlugin::AddConstraint(LONG a, LONG b, LONG type)
{
	if(!ConstraintExists(a,b))
	{
		CLConstraint c = CLConstraint(particles.Array(), a, b, type);
		constraints.Append(c);
	}
}

Bool CDRopeTagPlugin::CreateConstraints(BaseObject *op, BaseContainer *tData)
{
	if(!op || !tData) return false;
	if(!IsValidPointObject(op)) return false;
	
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, ptCnt = ToPoint(op)->GetPointCount();
	
	if(IsValidPointObject(op))
	{
		for(i=0; i<ptCnt; i++)
		{
			if(i > 0) AddConstraint(i, i-1, CD_EDGE_CONSTRAINT);
			if(i > 1) AddConstraint(i, i-2, CD_BEND_CONSTRAINT);
			//AddConstraint(i, i, CD_REF_CONSTRAINT);
		}
		tData->SetLong(RP_CONSTR_COUNT, constraints.Size());
	}
	
	return true;
}

void CDRopeTagPlugin::TimeStep(BaseContainer *tData)
{
	LONG ptCnt = tData->GetLong(RP_POINT_COUNT);
	if(ptCnt > 0)
	{
		LONG i;
		Real damp = tData->GetReal(RP_DAMPING);
		Real step = tData->GetReal(RP_TIME_STEP) * 0.1;//
		
		for(i=0; i<ptCnt; i++)
		{
			particles[i].TimeStep(damp, step); // calculate the position of each particle at the next time step.
		}
	}
}

void CDRopeTagPlugin::SatisfyConstraints(BaseDocument *doc, BaseContainer *tData, BaseObject *op)
{
	if(constraints.Size() > 0)
	{
		LONG i, j, cCnt = constraints.Size();
		LONG iter = tData->GetLong(RP_ITERATIONS);
		LONG stf = iter * tData->GetReal(RP_STIFFNESS);
		Matrix opM = op->GetMg();
		for(i=0; i<iter; i++) // iterate over all constraints several times
		{
			for(j=0; j<cCnt; j++)
			{
				if(constraints[j].GetType() == CD_BEND_CONSTRAINT)
				{
					if(i < stf) constraints[j].SatisfyConstraint(particles.Array()); // satisfy constraint.
				}
				else constraints[j].SatisfyConstraint(particles.Array()); // satisfy constraint.
			}
			Collision(doc,tData);
		}
	}
}

void CDRopeTagPlugin::AddShapeForce(BaseContainer *tData, BaseObject *op)
{
	LONG ptCnt = tData->GetLong(RP_POINT_COUNT);
	if(ptCnt > 0)
	{
		Matrix opM = op->GetMg();
		LONG i;
		for(i=0; i<ptCnt; i++)
		{
			Vector pPos = particles[i].pos;
			Vector rPos = opM * ref[i];
			Vector f = rPos - pPos;
			particles[i].AddForce(f); // add the forces to each particle
		}
	}
}

void CDRopeTagPlugin::AddForce(BaseContainer *tData, Vector f)
{
	LONG ptCnt = tData->GetLong(RP_POINT_COUNT);
	if(ptCnt > 0)
	{
		LONG i;
		for(i=0; i<ptCnt; i++)
		{
			particles[i].AddForce(f); // add the forces to each particle
		}
	}
}

void CDRopeTagPlugin::WindForces(BaseContainer *tData, Real t)
{
	Real strength = tData->GetReal(RP_WIND_STRENGTH);
	if(strength > 0.0)
	{
		Vector direction = tData->GetVector(RP_WIND_DIRECTION);
		Vector windForce = VNorm(direction) * strength;
		
		if(tData->GetBool(RP_USE_TURBULENCE))
		{
			Real frq = tData->GetReal(RP_TURBULENCE_FREQUENCY) * 0.1;
			Real amp = tData->GetReal(RP_TURBULENCE_AMPLITUDE) * 0.1;
			Real spd = tData->GetReal(RP_TURBULENCE_SPEED);
			Real turb = Abs(amp * Sin(spd * frq * t));
			windForce *= turb;
		}
		AddForce(tData, windForce);
	}
}

Bool CDRopeTagPlugin::PlanerCollision(BaseContainer *tData, BaseContainer *ctData, BaseObject *op)
{
	if(!op) return false;
	
	Vector norm;
	Matrix opM = op->GetMg();
	switch(ctData->GetLong(CLD_PLANE_NORM))
	{
		case CLD_NORM_XP:
			norm = VNorm(opM.v1);
			break;
		case CLD_NORM_XN:
			norm = VNorm(opM.v1 * -1);
			break;
		case CLD_NORM_YP:
			norm = VNorm(opM.v2);
			break;
		case CLD_NORM_YN:
			norm = VNorm(opM.v2 * -1);
			break;
		case CLD_NORM_ZP:
			norm = VNorm(opM.v3);
			break;
		case CLD_NORM_ZN:
			norm = VNorm(opM.v3 * -1);
			break;
		default:
			norm = VNorm(opM.v2);
			break;
	}
	
	Real planeConstant = VDot(opM.off, norm);
	
	LONG i, ptCnt = tData->GetLong(RP_POINT_COUNT);
	for(i=0; i<ptCnt; i++)
	{
		Real penetration = planeConstant - VDot(particles[i].pos, norm);
		if(penetration > 0.0)
		{
			particles[i].OffsetPosition(norm * penetration);
		}
	}
	
	return true;
}

Bool CDRopeTagPlugin::SphericalCollision(BaseContainer *tData, BaseContainer *ctData, BaseObject *op)
{
	if(!op) return false;
	
	if(op->IsInstanceOf(Osphere) || op->IsInstanceOf(Onull))
	{
		Real rad = ctData->GetReal(CLD_RADIUS);
		Matrix opM = op->GetMg();
		
		LONG i, ptCnt = tData->GetLong(RP_POINT_COUNT);
		for(i=0; i<ptCnt; i++)
		{
			Vector dir = particles[i].pos - opM.off;
			Real len = Len(dir);
			if(len < rad)
			{
				particles[i].OffsetPosition(VNorm(dir) * (rad-len));
			}
		}
	}
	
	return true;
}

void CDRopeTagPlugin::Collision(BaseDocument *doc, BaseContainer *tData)
{
	CDCLData cld;
	PluginMessage(ID_CDCLOTHCOLLIDER,&cld);
	if(cld.list)
	{
		LONG i, cnt = cld.list->GetCount();
		for(i=0; i<cnt; i++)
		{
			BaseTag *cldTag = static_cast<BaseTag*>(cld.list->GetIndex(i));
			if(cldTag && cldTag->GetDocument())
			{
				BaseObject *cldOp = cldTag->GetObject();
				BaseContainer *ctData = cldTag->GetDataInstance();
				if(ctData)
				{
					switch(ctData->GetLong(CLD_COLLIDER_TYPE))
					{
						case CLD_PLANE:
							PlanerCollision(tData, ctData,cldOp);
							break;
						case CLD_SPHERE:
							SphericalCollision(tData, ctData, cldOp);
							break;
						case CLD_JOINT:
							break;
						case CLD_POLYGON:
							break;
					}
				}
			}
		}
	}
}

Bool CDRopeTagPlugin::RestoreReference(BaseObject *op)
{
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG ptCnt = ToPoint(op)->GetPointCount(); if(ptCnt < 1) return false;
	
	LONG i;
	for(i=0; i<ptCnt; i++)
	{
		padr[i] = ref[i];
	}
	
	return true;
}

void CDRopeTagPlugin::RemapRopeWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map)
{
	//GePrint("CDClusterTagPlugin::RemapClusterWeights");
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
			
			// remap any changes
			
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
			ref.Free();
			particles.Free();
			InitParticles(tag->GetObject(),tData);
			SetWeight(tag->GetObject(),tData,rmWeight.Array(),nCnt);
			
			constraints.Free();
			CreateConstraints(tag->GetObject(), tData);
		}
		rmWeight.Free();
		tWeight.Free();
	}
}

Bool CDRopeTagPlugin::TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg)
{
	//GePrint("CDSkinRefPlugin::TransferTMaps");
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, ptCnt = ToPoint(op)->GetPointCount();
	
	TranslationMaps tMap;
	
	LONG nCnt = vchg->new_cnt, oCnt = vchg->old_cnt, mCnt, size;
	if(vchg->map)
	{
		//GePrint("vchg->map : nCnt < oCnt");
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
			RemapRopeWeights(tag, tData, &tMap);
			
			tData->SetLong(RP_POINT_COUNT,nCnt);
			pMap.Free();
		}
		else if(nCnt > oCnt)
		{
			//GePrint("vchg->map : nCnt > oCnt");
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
			RemapRopeWeights(tag, tData, &tMap);
			
			tData->SetLong(RP_POINT_COUNT,nCnt);
			pMap.Free();
		}
	}
	else
	{
		if(nCnt < oCnt)
		{
			//GePrint("nCnt < oCnt");
			if(!particles.IsEmpty())
			{
				particles.Resize(nCnt);
				tData->SetLong(RP_POINT_COUNT,nCnt);
			}
		}
		else if(nCnt > oCnt)
		{
			//GePrint("nCnt > oCnt");
			
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
			RemapRopeWeights(tag, tData, &tMap);
			
			tData->SetBool(RP_POINTS_ADDED,true);
			pMap.Free();
		}
		else if(nCnt == oCnt)
		{
			//GePrint("nCnt == oCnt");
			if(tData->GetBool(RP_POINTS_ADDED))
			{
				LONG oldCnt = tData->GetLong(RP_POINT_COUNT);
				
				for(i=oldCnt; i<ptCnt; i++)
				{
					CLParticle p;
					particles[i] = p;
				}
				
				tData->SetBool(RP_POINTS_ADDED,false);
				tData->SetLong(RP_POINT_COUNT,ptCnt);
			}
		}
	}
	
	return true;
}

void CDRopeTagPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdclnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdclnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdclnr.FindFirst("-",&pos);
	while(h)
	{
		cdclnr.Delete(pos,1);
		h = cdclnr.FindFirst("-",&pos);
	}
	cdclnr.ToUpper();
	
	kb = cdclnr.SubStr(pK,2);
	
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
			
			//tData->SetLink(TARGET+T_LST,tData->GetLink(TARGET,doc));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDRopeTagPlugin::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("CDRopeTagPlugin::Message()");
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	
	//ListReceivedMessages("CDRopeTagPlugin",type,data);
	switch(type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDCL_SERIAL_SIZE];
			String cdclnr;
			
			CDReadPluginInfo(ID_CDCLOTH,snData,CDCL_SERIAL_SIZE);
			cdclnr.SetCString(snData,CDCL_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdclnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDCL_SERIAL_SIZE];
			String cdclnr;
			
			CDReadPluginInfo(ID_CDCLOTH,snData,CDCL_SERIAL_SIZE);
			cdclnr.SetCString(snData,CDCL_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdclnr);
			break;
		}
	}
	BaseObject *op = tag->GetObject(); if(!op) return true;
	BaseDocument *doc = node->GetDocument(); if(!doc) return true;
	
	CheckTagReg(doc,op,tData);
	
	if(!IsValidPointObject(op)) return false;
	
	LONG oldPCnt = tData->GetLong(RP_POINT_COUNT);
	switch(type)
	{
		case MSG_MENUPREPARE:
		{
			if(!InitParticles(op,tData)) return false;
			break;
		}
		case MSG_POINTS_CHANGED:
		{
			//GePrint("CDRopeTagPlugin::Message = MSG_POINTS_CHANGED");
			VariableChanged *vchg = (VariableChanged*)data;
			if(vchg)
			{
				if(vchg->old_cnt == oldPCnt)
				{
					TransferTMaps(tag,doc,tData,op,vchg);
				}
			}
			break;
		}
		case  MSG_TRANSLATE_POINTS:
		{
			//GePrint("CDRopeTagPlugin::Message = MSG_TRANSLATE_POINTS");
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
						RemapRopeWeights(tag, tData, tMap);
						tData->SetLong(RP_POINT_COUNT,nCnt);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==RP_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

LONG CDRopeTagPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	//GePrint("CDRopeTagPlugin::CDExecute()");
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	LONG i, pCnt = ToPoint(op)->GetPointCount(), prtCnt = tData->GetLong(RP_POINT_COUNT);
	
	Bool tagMoved = false;
	if(op != tData->GetObjectLink(T_OID,doc))
	{
		BaseObject *tOp = tData->GetObjectLink(T_OID,doc);
		if(tOp)
		{
			if(tOp->GetDocument())
			{
				RestoreReference(op);
				tagMoved = true;
				tData->SetBool(T_MOV,true);
			}
		}
		if(!tagMoved && !tData->GetBool(T_MOV) && pCnt == prtCnt)  tData->SetLink(T_OID,op);
	}
	else tData->SetBool(T_MOV,false);
	if(tagMoved || tData->GetBool(T_MOV)) return false;
	
	if(!IsValidPointObject(op)) return false;
	if(!(op->GetInfo() & OBJECT_ISSPLINE)) return false;
	
	//return CD_EXECUTION_RESULT_OK;// debug exit
	
	if(particles.IsEmpty() || ref.IsEmpty()) return false;
	
	Vector *padr = GetPointArray(op);
	Matrix opM = op->GetMg();
	
	if(prtCnt > 0)
	{
		BaseTime time = doc->GetTime();
		Real fps = doc->GetFps();
		LONG frm = time.GetFrame(fps);
		
		if(tData->GetBool(RP_USE_GRAVITY))
		{
			Real g = tData->GetReal(RP_GRAVITY);
			AddForce(tData, Vector(0.0,g,0.0));
		}
		
		if(tData->GetBool(RP_KEEP_SHAPE)) AddShapeForce(tData, op);
		
		Real t = CDGetNumerator(&time)/fps;
		WindForces(tData, t);
				
		TimeStep(tData);
		SatisfyConstraints(doc,tData,op);
		
		for(i=0; i<prtCnt; i++)
		{
			if(frm == 0)
			{
				particles[i].pos = opM * ref[i];
				particles[i].old_pos = opM * ref[i];
				particles[i].ResetAcceleration();
			}
			if(particles[i].wt > 0.0) //movable
			{
				Vector v = MInv(opM) * particles[i].pos;
				padr[i] = v;
			}
			else particles[i].pos = opM * padr[i];
		}
		op->Message(MSG_UPDATE);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDRopeTagPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(RP_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDRopeTagPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	return true;
}

Bool RegisterCDRopeTagPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDCL_SERIAL_SIZE];
	String cdclnr, kb;
	
	if(!CDReadPluginInfo(ID_CDCLOTH,data,CDCL_SERIAL_SIZE)) reg = false;
	
	cdclnr.SetCString(data,CDCL_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdclnr)) reg = false;
	
	SerialInfo si;
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdclnr.FindFirst("-",&pos);
	while(h)
	{
		cdclnr.Delete(pos,1);
		h = cdclnr.FindFirst("-",&pos);
	}
	cdclnr.ToUpper();
	kb = cdclnr.SubStr(pK,2);
	
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
	
	LONG info;
	if(!reg) info = TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE;
	else info = TAG_EXPRESSION|TAG_VISIBLE;

	// decide by name if the plugin shall be registered - just for user convenience  
	String name=GeLoadString(IDS_CDRPTAG); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDROPETAG,name,info,CDRopeTagPlugin::Alloc,"tCDRopeTag","CDRopeTag.tif",0);
}

// library functions
Bool iCDRopeGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDRopeTagPlugin *rTag = static_cast<CDRopeTagPlugin*>(tag->GetNodeData());
	return rTag ? rTag->GetWeight(tData, tWeight, tCnt) : false;
}

Bool iCDRopeSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDRopeTagPlugin *rTag = static_cast<CDRopeTagPlugin*>(tag->GetNodeData());
	return rTag ? rTag->SetWeight(op, tData, tWeight, tCnt) : false;
}

