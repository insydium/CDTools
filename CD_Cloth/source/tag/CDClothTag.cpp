//	Cactus Dan's Cloth
//	Copyright 2013 by Cactus Dan Libisch
//	CDClothTag.cpp

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "customgui_priority.h"

#include "CDGeneral.h"
#include "CDMessages.h"
#include "CDCompatibility.h"
//#include "CDDebug.h"

#include "tCDClothTag.h"
#include "tCDClothCollider.h"
#include "oCDJoint.h"

#include "CDClothTag.h"
#include "CDClothCollider.h"

#include "CDSkinRefTag.h"
#include "CDSkinTag.h"

Bool CDClothTagPlugin::Init(GeListNode *node)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetReal(CL_DAMPING, 0.01);
	tData->SetReal(CL_STIFFNESS, 0.5);
	tData->SetReal(CL_TIME_STEP, 2.5);
	tData->SetLong(CL_ITERATIONS, 25);
	
	tData->SetBool(CL_USE_GRAVITY, true);
	tData->SetReal(CL_GRAVITY,-9.8);
	
	tData->SetLong(CL_POINT_COUNT, 0);
	tData->SetLong(CL_CONSTR_COUNT, 0);
	tData->SetBool(CL_POINTS_ADDED,false);
	
	tData->SetVector(CL_WIND_DIRECTION, Vector(0,0,1));
	tData->SetReal(CL_TURBULENCE_FREQUENCY,1.6);
	tData->SetReal(CL_TURBULENCE_AMPLITUDE, 13.8);
	tData->SetReal(CL_TURBULENCE_SPEED, 2.5);
	
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

void CDClothTagPlugin::Free(GeListNode *node)
{
	if(wtChange.Size() > 0) wtChange.Free();
	if(particles.Size() > 0) particles.Free();
	if(constraints.Size() > 0) constraints.Free();
}

Bool CDClothTagPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	//GePrint("CDClothTagPlugin::Read()");
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG i, prtCnt = tData->GetLong(CL_POINT_COUNT);
	if(prtCnt > 0)
	{
		if(!wtChange.Alloc(prtCnt)) return false;
		for(i=0; i<prtCnt; i++)
		{
			hf->ReadLong(&wtChange[i]);
		}
		
		if(!particles.Alloc(prtCnt)) return false;
		for(i=0; i<prtCnt; i++)
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
	
	LONG cCnt = tData->GetLong(CL_CONSTR_COUNT);
	if(cCnt > 0)
	{
		constraints.Alloc(cCnt);
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

Bool CDClothTagPlugin::Write(GeListNode* node, HyperFile* hf)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	//Level 1
	LONG i, prtCnt = tData->GetLong(CL_POINT_COUNT);
	if(prtCnt > 0)
	{
		for(i=0; i<prtCnt; i++)
		{
			hf->WriteLong(wtChange[i]);
		}
		
		for(i=0; i<prtCnt; i++)
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

	LONG cCnt = tData->GetLong(CL_CONSTR_COUNT);
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

Bool CDClothTagPlugin::CDCopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, LONG flags, AliasTrans* trn)
{
	//GePrint("CDClothTagPlugin::CDCopyto()");
	BaseTag *tag  = (BaseTag*)snode; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	LONG prtCnt = tData->GetLong(CL_POINT_COUNT);
	if(prtCnt > 0)
	{
		wtChange.Copy(((CDClothTagPlugin*)dest)->wtChange);
		particles.Copy(((CDClothTagPlugin*)dest)->particles);
	}
	
	if(constraints.Size() > 0)
	{
		constraints.Copy(((CDClothTagPlugin*)dest)->constraints);
	}
	
	return true;
}

Bool CDClothTagPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	//GePrint("CDClothTagPlugin::CDDraw()");
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	if(!tData->GetBool(CL_DEBUG_DRAW)) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	Matrix opM = op->GetMg();
	
	Vector *padr = GetPointArray(op);
	
	LONG i, a, b, cCnt = constraints.Size(), prtCnt = tData->GetLong(CL_POINT_COUNT);
	for(i=0; i<cCnt; i++)
	{
		a = constraints[i].aInd;
		b = constraints[i].bInd;
		if(a < prtCnt && b < prtCnt)
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
			CDDrawLine(bd, opM * padr[a], opM * padr[b]);
		}
	}

	//GePrint("CDClothTagPlugin::CDDraw() -> return");
	return true;
}

Bool CDClothTagPlugin::InitParticles(BaseObject *op, BaseContainer *tData)
{
	//GePrint("CDClothTagPlugin::InitParticles()");
	if(!op || !tData) return false;
	if(!IsValidPointObject(op)) return false;
	Vector *padr = GetPointArray(op); if(!padr) return false;
	
	LONG ptCnt = ToPoint(op)->GetPointCount(); if(ptCnt < 1) return false;
	
	Matrix opM = op->GetMg();
	
	if(!particles.Alloc(ptCnt)) return false;
	if(!wtChange.Alloc(ptCnt)) return false;

	StatusSetSpin();
	
	LONG i;
	for(i=0; i<ptCnt; i++)
	{
		Vector pt = opM * padr[i];
		particles[i].SetParticle(pt,pt,Vector(0,0,0),pt,1.0,0.0);
		wtChange[i] = 0;
	}
	tData->SetLong(CL_POINT_COUNT, ptCnt);
	
	StatusClear();

	constraints.Init();
	
	return true;
}

Bool CDClothTagPlugin::GetWeight(BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	if(!tWeight) return false;
	
	ClearMem(tWeight,(sizeof(Real)*tCnt),0);
	//if(particles)
	if(!particles.IsEmpty())
	{
		LONG i;
		for(i=0; i<tCnt; i++)
		{
			tWeight[i] = particles[i].wt;
		}
	}
	
	return true;
}

Bool CDClothTagPlugin::SetWeight(BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	if(!op) return false;
	if(!IsValidPolygonObject(op)) return false;
	if(!tWeight) return false;
	//if(!particles || !wtChange) return false;
	if(particles.IsEmpty() || wtChange.IsEmpty()) return false;
	
	LONG i;
	for(i=0; i<tCnt; i++)
	{
		if(particles[i].wt != tWeight[i])
		{
			if(particles[i].wt > 0.0 && tWeight[i] == 0.0)
				wtChange[i] = -1;
			else if(particles[i].wt == 0.0 && tWeight[i] > 0.0)
				wtChange[i] = 1;
		}
		Real wt = tWeight[i];
		particles[i].wt = TruncatePercentFractions(wt);
	}
	
	return true;
}

void CDClothTagPlugin::RemoveUnusedConstraints(void)
{
	//GePrint("CDClothTagPlugin::RemoveUnusedConstraints()");
	LONG i, cnt = constraints.Size()-1;
	for(i=cnt; i>-1; i--)
	{
		CLConstraint c = constraints[i];
		if(particles[c.aInd].wt == 0.0 && particles[c.bInd].wt == 0.0)
			constraints.Remove(i);
	}
}

void CDClothTagPlugin::AddConstraint(LONG a, LONG b, LONG type)
{
	//GePrint("CDClothTagPlugin::AddConstraint()");
	LONG ind = constraints.GetIndex(a,b);
	if(ind < 0)
	{
		if(particles[a].wt > 0.0 || particles[b].wt > 0.0)
		{
			CLConstraint c = CLConstraint(particles.Array(), a, b, type);
			constraints.Append(c);
		}
	}
}

void CDClothTagPlugin::AddEdgeBendConstraint(CPolygon *vadr, LONG plyInd, LONG start, LONG a, LONG b)
{
	LONG edge, aPt, bPt;
	for(edge=0; edge<4; edge++)
	{
		switch(edge)
		{
			case 0:
			{
				aPt = vadr[plyInd].a;
				bPt = vadr[plyInd].b;
				if(aPt == a && bPt != b)
				{
					if(start > -1 && bPt > -1)
						AddConstraint(start, bPt, CD_BEND_CONSTRAINT);
				}
				else if(bPt == a && aPt != b)
				{
					if(start > -1 && aPt > -1)
						AddConstraint(start, aPt, CD_BEND_CONSTRAINT);
				}
				break;
			}
			case 1:
			{
				aPt = vadr[plyInd].b;
				bPt = vadr[plyInd].c;
				if(aPt == a && bPt != b)
				{
					if(start > -1 && bPt > -1)
						AddConstraint(start, bPt, CD_BEND_CONSTRAINT);
				}
				else if(bPt == a && aPt != b)
				{
					if(start > -1 && aPt > -1)
						AddConstraint(start, aPt, CD_BEND_CONSTRAINT);
				}
				break;
			}
			case 2:
			{
				aPt = vadr[plyInd].c;
				bPt = vadr[plyInd].d;
				if(aPt == a && bPt != b)
				{
					if(start > -1 && bPt > -1)
						AddConstraint(start, bPt, CD_BEND_CONSTRAINT);
				}
				else if(bPt == a && aPt != b)
				{
					if(start > -1 && aPt > -1)
						AddConstraint(start, aPt, CD_BEND_CONSTRAINT);
				}
				break;
			}
			case 3:
			{
				aPt = vadr[plyInd].d;
				bPt = vadr[plyInd].a;
				if(aPt == a && bPt != b)
				{
					if(start > -1 && bPt > -1)
						AddConstraint(start, bPt, CD_BEND_CONSTRAINT);
				}
				else if(bPt == a && aPt != b)
				{
					if(start > -1 && aPt > -1)
						AddConstraint(start, aPt, CD_BEND_CONSTRAINT);
				}
				break;
			}
		}
	}
}

Bool CDClothTagPlugin::CreateConstraints(BaseObject *op, BaseContainer *tData)
{
	//GePrint("CDClothTagPlugin::CreateConstraints()");
	if(!op || !tData) return false;
	if(!IsValidPointObject(op)) return false;
	
	Vector *padr = GetPointArray(op); if(!padr) return false;
	LONG i, ptCnt = ToPoint(op)->GetPointCount();
	
	if(IsValidPolygonObject(op))
	{
		CPolygon *vadr = GetPolygonArray(op);
		LONG vCnt = ToPoly(op)->GetPolygonCount();
		
		Neighbor n;
		if(n.Init(ptCnt,vadr,vCnt,NULL))
		{
			StatusSetText("Building Cloth");
			for (i=0; i<ptCnt; i++)
			{
				LONG status = LONG(Real(i)/Real(ptCnt)*100.0);
				StatusSetBar(status);
				
				if(wtChange[i] > 0)
				{
					LONG *dadr=NULL, dCnt=0, p;
					n.GetPointPolys(i,&dadr,&dCnt);
					for(p=0; p<dCnt; p++)
					{
						LONG vInd = dadr[p];
						CPolygon ply = vadr[vInd];
						LONG plyA, plyB, plyInd;
						if(i == ply.a)
						{
							if(ply.b > -1)
							{
								AddConstraint(i, ply.b, CD_EDGE_CONSTRAINT);
								n.GetEdgePolys(ply.b, ply.c, &plyA, &plyB);
								if(plyA != dadr[p]) plyInd = plyA;
								else plyInd = plyB;
								if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.a, ply.b, ply.c);
							}
							if(ply.c > -1)
							{
								AddConstraint(i, ply.c, CD_SHEAR_CONSTRAINT);
								LONG *qadr=NULL, qCnt=0, q;
								n.GetPointPolys(ply.c,&qadr,&qCnt);
								for(q=0; q<qCnt; q++)
								{
									CPolygon pB = vadr[qadr[q]];
									if(GetSharedPolyPointsCount(ply, pB) == 1)
									{
										if(pB.a == ply.c && pB.c > -1)
											AddConstraint(i, pB.c, CD_BEND_CONSTRAINT);
										else if(pB.b == ply.c && pB.d > -1)
											AddConstraint(i, pB.d, CD_BEND_CONSTRAINT);
										else if(pB.c == ply.c && pB.a > -1)
											AddConstraint(i, pB.a, CD_BEND_CONSTRAINT);
										else if(pB.d == ply.c && pB.b > -1)
											AddConstraint(i, pB.b, CD_BEND_CONSTRAINT);
									}
								}
							}
							if(ply.d > -1 && ply.c != ply.d)
							{
								AddConstraint(i, ply.d, CD_EDGE_CONSTRAINT);
								n.GetEdgePolys(ply.c, ply.d, &plyA, &plyB);
								if(plyA != dadr[p]) plyInd = plyA;
								else plyInd = plyB;
								if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.a, ply.d, ply.c);
							}
						}
						else if(i == ply.b)
						{
							if(ply.a > -1)
							{
								AddConstraint(i, ply.a, CD_EDGE_CONSTRAINT);
								n.GetEdgePolys(ply.a, ply.d, &plyA, &plyB);
								if(plyA != dadr[p]) plyInd = plyA;
								else plyInd = plyB;
								if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.b, ply.a, ply.d);
							}
							if(ply.c > -1)
							{
								AddConstraint(i, ply.c, CD_EDGE_CONSTRAINT);
								if(ply.c != ply.d)
								{
									n.GetEdgePolys(ply.c, ply.d, &plyA, &plyB);
									if(plyA != dadr[p]) plyInd = plyA;
									else plyInd = plyB;
									if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.b, ply.c, ply.d);
								}
							}
							if(ply.d > -1 && ply.c != ply.d)
							{
								AddConstraint(i, ply.d, CD_SHEAR_CONSTRAINT);
								LONG *qadr=NULL, qCnt=0, q;
								n.GetPointPolys(ply.d,&qadr,&qCnt);
								for(q=0; q<qCnt; q++)
								{
									CPolygon pB = vadr[qadr[q]];
									if(GetSharedPolyPointsCount(ply, pB) == 1)
									{
										if(pB.a == ply.d && pB.c > -1)
											AddConstraint(i, pB.c, CD_BEND_CONSTRAINT);
										else if(pB.b == ply.d && pB.d > -1)
											AddConstraint(i, pB.d, CD_BEND_CONSTRAINT);
										else if(pB.c == ply.d && pB.a > -1)
											AddConstraint(i, pB.a, CD_BEND_CONSTRAINT);
										else if(pB.d == ply.d && pB.b > -1)
											AddConstraint(i, pB.b, CD_BEND_CONSTRAINT);
									}
								}
							}
						}
						else if(i == ply.c)
						{
							if(ply.b > -1)
							{
								AddConstraint(i, ply.b, CD_EDGE_CONSTRAINT);
								n.GetEdgePolys(ply.b, ply.a, &plyA, &plyB);
								if(plyA != dadr[p]) plyInd = plyA;
								else plyInd = plyB;
								if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.c, ply.b, ply.a);
							}
							if(ply.a > -1)
							{
								AddConstraint(i, ply.a, CD_SHEAR_CONSTRAINT);
								LONG *qadr=NULL, qCnt=0, q;
								n.GetPointPolys(ply.a,&qadr,&qCnt);
								for(q=0; q<qCnt; q++)
								{
									CPolygon pB = vadr[qadr[q]];
									if(GetSharedPolyPointsCount(ply, pB) == 1)
									{
										if(pB.a == ply.a && pB.c > -1)
											AddConstraint(i, pB.c, CD_BEND_CONSTRAINT);
										else if(pB.b == ply.a && pB.d > -1)
											AddConstraint(i, pB.d, CD_BEND_CONSTRAINT);
										else if(pB.c == ply.a && pB.a > -1)
											AddConstraint(i, pB.a, CD_BEND_CONSTRAINT);
										else if(pB.d == ply.a && pB.b > -1)
											AddConstraint(i, pB.b, CD_BEND_CONSTRAINT);
									}
								}
							}
							if(ply.d > -1 && ply.c != ply.d)
							{
								AddConstraint(i, ply.d, CD_EDGE_CONSTRAINT);
								n.GetEdgePolys(ply.d, ply.a, &plyA, &plyB);
								if(plyA != dadr[p]) plyInd = plyA;
								else plyInd = plyB;
								if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.c, ply.d, ply.a);
							}
						}
						else if(ply.c != ply.d && i == ply.d)
						{
							
							if(ply.a > -1)
							{
								AddConstraint(i, ply.a, CD_EDGE_CONSTRAINT);
								n.GetEdgePolys(ply.a, ply.b, &plyA, &plyB);
								if(plyA != dadr[p]) plyInd = plyA;
								else plyInd = plyB;
								if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.d, ply.a, ply.b);
							}
							if(ply.b > -1)
							{
								AddConstraint(i, ply.b, CD_SHEAR_CONSTRAINT);
								LONG *qadr=NULL, qCnt=0, q;
								n.GetPointPolys(ply.b,&qadr,&qCnt);
								for(q=0; q<qCnt; q++)
								{
									CPolygon pB = vadr[qadr[q]];
									if(GetSharedPolyPointsCount(ply, pB) == 1)
									{
										if(pB.a == ply.b && pB.c > -1)
											AddConstraint(i, pB.c, CD_BEND_CONSTRAINT);
										else if(pB.b == ply.b && pB.d > -1)
											AddConstraint(i, pB.d, CD_BEND_CONSTRAINT);
										else if(pB.c == ply.b && pB.a > -1)
											AddConstraint(i, pB.a, CD_BEND_CONSTRAINT);
										else if(pB.d == ply.b && pB.b > -1)
											AddConstraint(i, pB.b, CD_BEND_CONSTRAINT);
									}
								}
							}
							if(ply.c > -1)
							{
								AddConstraint(i, ply.c, CD_EDGE_CONSTRAINT);
								n.GetEdgePolys(ply.c, ply.b, &plyA, &plyB);
								if(plyA != dadr[p]) plyInd = plyA;
								else plyInd = plyB;
								if(plyInd > -1) AddEdgeBendConstraint(vadr, plyInd, ply.d, ply.c, ply.b);
							}
						}
					}
				}
				//else GePrint("wtChange = 0");
			}
			tData->SetLong(CL_CONSTR_COUNT, constraints.Size());
			StatusClear();
		}
		//else GePrint("Neighbor failed to initialize");
		//GePrint("constraints.Size() = "+LongToString(constraints.Size()));
	}
	
	return true;
}

void CDClothTagPlugin::TimeStep(BaseDocument *doc, BaseContainer *tData, BaseObject *op)
{
	LONG prtCnt = tData->GetLong(CL_POINT_COUNT);
	if(prtCnt > 0)
	{
		LONG i;
		Real damp = tData->GetReal(CL_DAMPING);
		Real step = tData->GetReal(CL_TIME_STEP) * 0.1;
		
		if(step > 0)
		{
			for(i=0; i<prtCnt; i++)
			{
				particles[i].TimeStep(damp, step);
			}
			Collision(doc, tData, op);
		}
	}
}

void CDClothTagPlugin::SatisfyConstraints(BaseDocument *doc, BaseContainer *tData, BaseObject *op)
{
	//GePrint("CDClothTagPlugin::SatisfyConstraints()");
	LONG cCnt = constraints.Size();
	if(cCnt > 0)
	{
		LONG i, j;
		LONG iter = tData->GetLong(CL_ITERATIONS);
		LONG stf = iter * tData->GetReal(CL_STIFFNESS);
		for(i=0; i<iter; i++)
		{
			for(j=0; j<cCnt; j++)
			{
				if(constraints[j].GetType() == CD_BEND_CONSTRAINT)
				{
					if(i < stf) constraints[j].SatisfyConstraint(particles.Array());
				}
				else constraints[j].SatisfyConstraint(particles.Array());
			}
			if(i % 5 == 0) Collision(doc, tData, op);
			//Collision(doc, tData, op);
		}
	}
}

void CDClothTagPlugin::AddSkinForce(BaseContainer *tData, BaseObject *op, Vector *ref, CDSkinVertex *skWt)
{
	LONG ptCnt = tData->GetLong(CL_POINT_COUNT);
	if(ptCnt > 0)
	{
		Matrix opM = op->GetMg();
		LONG i;
		for(i=0; i<ptCnt; i++)
		{
			if(skWt[i].taw > 0.0)
			{
				Vector pPos = particles[i].pos;
				Vector rPos = opM * ref[i];
				Vector f = rPos - pPos;
				particles[i].AddForce(f);
			}
		}
	}
}

void CDClothTagPlugin::AddForce(BaseContainer *tData, Vector f)
{
	LONG prtCnt = tData->GetLong(CL_POINT_COUNT);
	if(prtCnt > 0)
	{
		LONG i;
		for(i=0; i<prtCnt; i++)
		{
			particles[i].AddForce(f);
		}
	}
}

void CDClothTagPlugin::WindForces(BaseContainer *tData, Real t)
{
	Real strength = tData->GetReal(CL_WIND_STRENGTH);
	if(strength > 0.0)
	{
		Vector direction = tData->GetVector(CL_WIND_DIRECTION);
		Vector windForce = VNorm(direction) * strength;
		
		if(tData->GetBool(CL_USE_TURBULENCE))
		{
			Real frq = tData->GetReal(CL_TURBULENCE_FREQUENCY) * 0.1;
			Real amp = tData->GetReal(CL_TURBULENCE_AMPLITUDE) * 0.1;
			Real spd = tData->GetReal(CL_TURBULENCE_SPEED);
			Real turb = Abs(amp * Sin(spd * frq * t));
			windForce *= turb;
		}
		AddForce(tData, windForce);
	}
}

Bool CDClothTagPlugin::BoundsIntersection(CDAABB *b, Matrix bM, Matrix opM)
{
	Vector min = bounds.min;
	Vector max = bounds.max;
	
	CDAABB bnds;
	
	bnds.Empty();
	
	Vector pt = opM * Vector(min.x-20.0, min.y-20.0, min.z-20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	pt = opM * Vector(min.x-20.0, max.y+20.0, min.z-20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	pt = opM * Vector(min.x-20.0, min.y-20.0, max.z+20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	pt = opM * Vector(min.x-20.0, max.y+20.0, max.z+20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	pt = opM * Vector(max.x+20.0, min.y-20.0, min.z-20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	pt = opM * Vector(max.x+20.0, max.y+20.0, min.z-20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	pt = opM * Vector(max.x+20.0, min.y-20.0, max.z+20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	pt = opM * Vector(max.x+20.0, max.y+20.0, max.z+20.0);
	bnds.AddPoint(MInv(bM) * pt);
	
	return b->IntersectionTest(&bnds);
}

Bool CDClothTagPlugin::PlanerCollision(BaseContainer *tData, BaseTag *tag)
{
	BaseObject *cldOp = tag->GetObject(); if(!cldOp) return false;
	BaseContainer *ctData = tag->GetDataInstance(); if(!ctData) return false;
	Real offset = ctData->GetReal(CLD_OFFSET);
	
	Vector norm;
	Matrix cldM = cldOp->GetMg();
	switch(ctData->GetLong(CLD_PLANE_NORM))
	{
		case CLD_NORM_XP:
			norm = VNorm(cldM.v1);
			break;
		case CLD_NORM_XN:
			norm = VNorm(cldM.v1 * -1);
			break;
		case CLD_NORM_YP:
			norm = VNorm(cldM.v2);
			break;
		case CLD_NORM_YN:
			norm = VNorm(cldM.v2 * -1);
			break;
		case CLD_NORM_ZP:
			norm = VNorm(cldM.v3);
			break;
		case CLD_NORM_ZN:
			norm = VNorm(cldM.v3 * -1);
			break;
		default:
			norm = VNorm(cldM.v2);
			break;
	}
	
	Real planeConstant = VDot(cldM.off + (norm * offset), norm);
	
	LONG i, ptCnt = tData->GetLong(CL_POINT_COUNT);
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

Bool CDClothTagPlugin::SphericalCollision(BaseContainer *tData, BaseTag *tag, BaseObject *op)
{
	BaseObject *cldOp = tag->GetObject(); if(!cldOp) return false;
	BaseContainer *ctData = tag->GetDataInstance(); if(!ctData) return false;
	Real offset = ctData->GetReal(CLD_OFFSET);
	
	//CDClothCollider *cldTag = static_cast<CDClothCollider*>(tag->GetNodeData());
	//if(!cldTag) return false;
	
	//CDAABB *cldBnds = cldTag->GetBounds();
	CDAABB *cldBnds = CDColliderGetBounds(tag);
	
	Matrix cldM = cldOp->GetMg();
	Matrix opM = op->GetMg();
	
	if(BoundsIntersection(cldBnds, cldM, opM))
	{
		Real rad = ctData->GetReal(CLD_RADIUS) + offset;
		LONG i, ptCnt = tData->GetLong(CL_POINT_COUNT);
		for(i=0; i<ptCnt; i++)
		{
			Vector pt = particles[i].pos;
			if(cldBnds->PointInBounds(MInv(cldM) * pt))
			{
				Vector bDir = pt - cldM.off;
				Real bLen = Len(bDir);
				if(bLen < rad)
				{
					particles[i].OffsetPosition(VNorm(bDir) * (rad-bLen));
					
					/*Vector stPt = particles[i].start_pos;
					Vector aDir = stPt - pt;
					Real cAng = ACos(VDot(VNorm(bDir), VNorm(-aDir)));
					Real bAng = ((Sin(cAng) * bLen)/rad);
					Real aAng = pi - (cAng + bAng);
					Real aLen = ((rad * Sin(aAng)/Sin(cAng)));
					particles[i].OffsetPosition(VNorm(aDir) * aLen);*/
				}
			}
		}
	}
	
	return true;
}

Bool CDClothTagPlugin::CDJointCollision(BaseContainer *tData, BaseTag *tag, BaseObject *op)
{
	BaseObject *cldOp = tag->GetObject(); if(!cldOp) return false;
	BaseContainer *ctData = tag->GetDataInstance(); if(!ctData) return false;
	
	//CDClothCollider *cldTag = static_cast<CDClothCollider*>(tag->GetNodeData());
	//if(!cldTag) return false;
	
	//CDAABB *cldBnds = cldTag->GetBounds();
	CDAABB *cldBnds = CDColliderGetBounds(tag);
	
	Matrix cldM = cldOp->GetMg();
	Matrix opM = op->GetMg();

	if(BoundsIntersection(cldBnds, cldM, opM))
	{
		BaseContainer *jData = cldOp->GetDataInstance();
		Real rRad = jData->GetReal(JNT_JOINT_RADIUS);
		
		BaseObject *ch = cldOp->GetDown();
		if(ch)
		{
			Matrix rootM = cldOp->GetMg();
			Matrix tipM = ch->GetMg();
			
			tipM.v1 = rootM.v1 = VNorm(rootM.v1);
			tipM.v2 = rootM.v2 = VNorm(rootM.v2);
			tipM.v3 = rootM.v3 = VNorm(rootM.v3);
			
			rootM.v1 *= rRad;
			rootM.v2 *= rRad;
			
			BaseContainer *chData = ch->GetDataInstance();
			Real tRad = chData->GetReal(JNT_JOINT_RADIUS);
			tipM.v1 *= tRad;
			tipM.v2 *= tRad;
		
			LONG i, ptCnt = tData->GetLong(CL_POINT_COUNT);
			for(i=0; i<ptCnt; i++)
			{
				Vector pt = particles[i].pos;
				if(cldBnds->PointInBounds(MInv(cldM) * pt))
				{
					Vector d1 = pt - rootM.off;
					Vector d2 = tipM.off - rootM.off;
					Real dot = VDot(VNorm(d1), VNorm(d2));
					if(dot < 0)
					{
						Vector dir = pt - rootM.off;
						Real len = Len(dir);
						if(len < rRad)
						{
							particles[i].OffsetPosition(VNorm(dir) * (rRad-len));
						}
					}
					else
					{
						Real adj = dot * Len(d1);
						if(adj > Len(d2))
						{
							Vector dir = pt - tipM.off;
							Real len = Len(dir);
							if(len < tRad)
							{
								particles[i].OffsetPosition(VNorm(dir) * (tRad-len));
							}
						}
						else
						{
							Real mix = adj / Len(d2);
							Matrix secM = rootM;
							Vector vPos = rootM.off + VNorm(d2) * adj;
							secM.off = vPos;
							secM.v1 = CDBlend(rootM.v1,tipM.v1,mix);
							secM.v2 = CDBlend(rootM.v2,tipM.v2,mix);
							secM.v3 = rootM.v3;
							Real rad = Len((secM * VNorm(MInv(secM) * pt)) - vPos);
							
							Vector dir = pt - vPos;
							Real len = Len(dir);
							if(len < rad)
							{
								particles[i].OffsetPosition(VNorm(dir) * (rad-len));
							}
						}
					}
				}
			}
		}
		else
		{
			LONG i, ptCnt = tData->GetLong(CL_POINT_COUNT);
			for(i=0; i<ptCnt; i++)
			{
				Vector pt = particles[i].pos;
				if(cldBnds->PointInBounds(MInv(cldM) * pt))
				{
					Vector dir = pt - cldM.off;
					Real len = Len(dir);
					if(len < rRad)
					{
						particles[i].OffsetPosition(VNorm(dir) * (rRad-len));
					}
				}
			}
		}
	}
	
	return true;
}

Vector CDClothTagPlugin::GetPolygonCenter(const CPolygon &v)
{
	if(v.c==v.d)
		return (particles[v.a].pos + particles[v.b].pos + particles[v.c].pos)/3;
	else
		return (particles[v.a].pos + particles[v.b].pos + particles[v.c].pos + particles[v.d].pos)/4;
}

Bool CDClothTagPlugin::PolygonalCollision(BaseContainer *tData, BaseTag *tag, BaseObject *op)
{
	BaseObject *cldOp = tag->GetObject(); if(!cldOp) return false;
	BaseContainer *ctData = tag->GetDataInstance(); if(!ctData) return false;
	Real offset = ctData->GetReal(CLD_OFFSET);
	
	CDClothCollider *cldTag = static_cast<CDClothCollider*>(tag->GetNodeData());
	if(!cldTag) return false;
	
	//CDAABB *cldBnds = cldTag->GetBounds();
	CDAABB *cldBnds = CDColliderGetBounds(tag);
	
	Matrix cldM = cldOp->GetMg();
	Matrix opM = op->GetMg();
	
	if(BoundsIntersection(cldBnds, cldM, opM))
	{		
		/*LONG i, ptCnt = tData->GetLong(CL_POINT_COUNT);
		for(i=0; i<ptCnt; i++)
		{
			Vector pt = particles[i].pos;
			if(cldBnds->PointInBounds(VNorm(cldM) * pt))
			{
				Vector norm;
				//Vector srfPt = cldTag->GetNearestSurfacePoint(cldOp, pt, norm);
				Vector srfPt = CDGetNearestSurfacePoint(cldTag, cldOp, pt, norm);
				Vector aDir = (srfPt + norm * offset) - pt;
				Real dot = VNorm(norm) * VNorm(aDir);
				if(dot > 0)
				{
					Real alen = Len(aDir);
					particles[i].OffsetPosition(VNorm(aDir) * alen);
				}
			}
		}*/
		CPolygon *vadr = GetPolygonArray(op);
		Vector *padr = GetPointArray(op);

		if(padr && vadr)
		{
			LONG i, vCnt = ToPoly(op)->GetPolygonCount();
			for(i=0; i<vCnt; i++)
			{
				CPolygon ply = vadr[i];
				Vector pt = GetPolygonCenter(ply);
				if(cldBnds->PointInBounds(MInv(cldM) * pt))
				{
					Vector norm;
					//Vector srfPt = cldTag->GetNearestSurfacePoint(cldOp, pt, norm);
					Vector srfPt = CDGetNearestSurfacePoint(cldTag, cldOp, pt, norm);
					Vector aDir = srfPt - pt;
					Real dot = VDot(VNorm(norm), VNorm(aDir));
					if(dot > 0)
					{
						Real alen = Len(aDir);
						Vector offsetV = VNorm(aDir) * (alen + offset);
						particles[ply.a].OffsetPosition(offsetV);
						particles[ply.b].OffsetPosition(offsetV);
						particles[ply.c].OffsetPosition(offsetV);
						if(ply.c != ply.d)
							particles[ply.d].OffsetPosition(offsetV);
					}
				}
			}
		}
	}
	
	return true;
}

void CDClothTagPlugin::Collision(BaseDocument *doc, BaseContainer *tData, BaseObject *op)
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
				BaseContainer *ctData = cldTag->GetDataInstance();
				if(ctData)
				{
					switch(ctData->GetLong(CLD_COLLIDER_TYPE))
					{
						case CLD_PLANE:
							PlanerCollision(tData, cldTag);
							break;
						case CLD_SPHERE:
							SphericalCollision(tData, cldTag, op);
							break;
						case CLD_JOINT:
							CDJointCollision(tData, cldTag, op);
							break;
						case CLD_POLYGON:
							PolygonalCollision(tData, cldTag, op);
							break;
					}
				}
				
			}
		}
	}
}

void CDClothTagPlugin::RemapClothWeights(BaseTag *tag, BaseContainer *tData, TranslationMaps *map)
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
			wtChange.Free();
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

Bool CDClothTagPlugin::TransferTMaps(BaseTag *tag, BaseDocument *doc, BaseContainer *tData, BaseObject *op, VariableChanged *vchg)
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
			RemapClothWeights(tag, tData, &tMap);
			
			tData->SetLong(CL_POINT_COUNT,nCnt);
			pMap.Free();
		}
		else if(nCnt > oCnt)
		{
			//GePrint("vchg->map : nCnt > oCnt");
			// Get remap count
			size = 0;
			for(i=0; i<nCnt; i++)
			{
				if(i < oCnt)
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
			RemapClothWeights(tag, tData, &tMap);
			
			tData->SetLong(CL_POINT_COUNT,nCnt);
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
				tData->SetLong(CL_POINT_COUNT,nCnt);
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
			RemapClothWeights(tag, tData, &tMap);
			
			tData->SetBool(CL_POINTS_ADDED,true);
			pMap.Free();
		}
		else if(nCnt == oCnt)
		{
			//GePrint("nCnt == oCnt");
			if(tData->GetBool(CL_POINTS_ADDED))
			{
				LONG oldCnt = tData->GetLong(CL_POINT_COUNT);
				
				for(i=oldCnt; i<ptCnt; i++)
				{
					CLParticle p;
					particles[i] = p;
				}
				
				tData->SetBool(CL_POINTS_ADDED,false);
				tData->SetLong(CL_POINT_COUNT,ptCnt);
			}
		}
	}
	
	return true;
}

void CDClothTagPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdtnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdtnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdtnr.FindFirst("-",&pos);
	while(h)
	{
		cdtnr.Delete(pos,1);
		h = cdtnr.FindFirst("-",&pos);
	}
	cdtnr.ToUpper();
	
	kb = cdtnr.SubStr(pK,2);
	
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

Bool CDClothTagPlugin::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("CDClothTagPlugin::Message()");
	BaseTag *tag  = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;

	//ListReceivedMessages("CDClothTagPlugin",type,data);
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
	BaseDocument *doc = tag->GetDocument(); if(!doc) return false;
	BaseObject *op = tag->GetObject(); if(!op) return false;
	
	CheckTagReg(doc,op,tData);
	
	if(!IsValidPolygonObject(op)) return false;
	
	LONG prtCnt = tData->GetLong(CL_POINT_COUNT);
	switch(type)
	{
		case MSG_MENUPREPARE:
		{
			//GePrint("CDClothTagPlugin::Message = MSG_MENUPREPARE");
			if(!InitParticles(op,tData)) return false;
			break;
		}
		case CD_MSG_UPDATE:
		{
			//GePrint("CDClothTagPlugin::Message = CD_MSG_UPDATE");
			CreateConstraints(op, tData);
			RemoveUnusedConstraints();
			wtChange.Fill(0);
			break;
		}
		case MSG_POINTS_CHANGED:
		{
			//GePrint("CDClothTagPlugin::Message = MSG_POINTS_CHANGED");
			VariableChanged *vchg = (VariableChanged*)data;
			if(vchg)
			{
				if(vchg->old_cnt == prtCnt)
				{
					TransferTMaps(tag,doc,tData,op,vchg);
				}
			}
			break;
		}
		case MSG_TRANSLATE_POINTS:
		{
			//GePrint("CDClothTagPlugin::Message = MSG_TRANSLATE_POINTS");
			TranslationMaps *tMap = (TranslationMaps*)data;
			if(tMap) 
			{
				Vector *padr = GetPointArray(op);
				if(padr)
				{
					LONG oCnt = tMap->m_oPointCount, nCnt = tMap->m_nPointCount;
					
					if(oCnt == prtCnt && oCnt != nCnt)
					{
						if(doc) CDAddUndo(doc,CD_UNDO_CHANGE,tag);
						RemapClothWeights(tag, tData, tMap);
						tData->SetLong(CL_POINT_COUNT,nCnt);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==CL_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			break;
		}
	}
	
	return true;
}

LONG CDClothTagPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	//GePrint("CDClothTagPlugin::CDExecute()");
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	LONG i, ptCnt = ToPoint(op)->GetPointCount(), prtCnt = tData->GetLong(CL_POINT_COUNT);
	
	Bool tagMoved = false;
	if(op != tData->GetObjectLink(T_OID,doc))
	{
		BaseObject *tOp = tData->GetObjectLink(T_OID,doc);
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
		if(!tagMoved && !tData->GetBool(T_MOV) && ptCnt == prtCnt)  tData->SetLink(T_OID,op);
	}
	else tData->SetBool(T_MOV,false);
	if(tagMoved || tData->GetBool(T_MOV)) return false;
	
	if(!IsValidPolygonObject(op)) return false;
	
	if(particles.IsEmpty()) return false;
	
	BaseTag *skrTag = op->GetTag(ID_CDSKINREFPLUGIN); if(!skrTag) return false;
	CDSkinRefPlugin *sRef = static_cast<CDSkinRefPlugin*>(skrTag->GetNodeData());
	CDMRefPoint *skinRef = sRef->GetSkinReference();
	
	BaseTag *sknTag = op->GetTag(ID_CDSKINPLUGIN);
	if(sknTag)
	{
		BaseContainer *sknData = sknTag->GetDataInstance();
		if(sknData)
		{
			if(!sknData->GetBool(SKN_BOUND)) return false;
		}
	}
	
	Vector *padr = GetPointArray(op);
	Matrix opM = op->GetMg();
	
	Vector mp = op->GetMp();
	Vector rad = op->GetRad();
	bounds.Empty();
	bounds.AddPoint(mp - rad);
	bounds.AddPoint(mp + rad);
	
	if(prtCnt > 0)
	{
		BaseTime time = doc->GetTime();
		Real fps = doc->GetFps();
		LONG frm = time.GetFrame(fps);

		if(tData->GetBool(CL_USE_GRAVITY))
		{
			Real g = tData->GetReal(CL_GRAVITY);
			AddForce(tData, Vector(0.0,g,0.0));
		}
		if(sknTag)
		{
			CDSkinPlugin *sTag = static_cast<CDSkinPlugin*>(sknTag->GetNodeData());
			CDSkinVertex *skinWeight = sTag->GetSkinWeight(); if(!skinWeight) return false;
			AddSkinForce(tData, op, padr, skinWeight);
		}
		
		Real t = CDGetNumerator(&time)/fps;
		WindForces(tData, t);
		
		TimeStep(doc, tData, op);
		SatisfyConstraints(doc,tData,op);
		Collision(doc, tData, op);
		
		for(i=0; i<prtCnt; i++)
		{
			if(frm == 0)
			{
				Vector pt;
				if(!sknTag) pt = skinRef[i].GetVector();
				else pt = padr[i];
				
				particles[i].pos = opM * pt;
				particles[i].old_pos = opM * pt;
				particles[i].ResetAcceleration();
			}
			if(particles[i].wt > 0.0)
			{
				padr[i] = MInv(opM) * particles[i].pos;
			}
			else particles[i].pos = opM * padr[i];
			
			particles[i].SetStartPosition();
		}
		op->Message(MSG_UPDATE);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDClothTagPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(CL_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDClothTagPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	return true;
}

Bool RegisterCDClothTagPlugin(void)
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

	// decide by name if the plugin shall be registered - just for user convenience  
	String name=GeLoadString(IDS_CDCLTAG); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDCLOTHTAG,name,TAG_EXPRESSION|TAG_VISIBLE|PLUGINFLAG_HIDE,CDClothTagPlugin::Alloc,"tCDClothTag","CDClothTag.tif",0);
}

// library functions
Bool iCDClothGetWeight(BaseTag *tag, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClothTagPlugin *cTag = static_cast<CDClothTagPlugin*>(tag->GetNodeData());
	return cTag ? cTag->GetWeight(tData, tWeight, tCnt) : false;
}

Bool iCDClothSetWeight(BaseTag *tag, BaseObject *op, BaseContainer *tData, Real *tWeight, LONG tCnt)
{
	CDClothTagPlugin *cTag = static_cast<CDClothTagPlugin*>(tag->GetNodeData());
	return cTag ? cTag->SetWeight(op, tData, tWeight, tCnt) : false;
}

