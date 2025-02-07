//	Cactus Dan's Transfer Tools 1.0 plugin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"

#include "CDTransfer.h"

class CDCoordinatesDialog : public GeDialog
{
	private:
		void DoApply(void);
		void DoEnable(Bool async);
		
		void GetInputGlobals(void);
		void UpdatePSRValues(void);
		
		void GetPSRValues(Bool async);
		void SetPSRValues(void);
		
		Real GetUnitFactor(LONG unit, LONG inv);
		Vector GetInputScale(BaseObject *op, Real ufct, Real ifct);
		void AddChildBounds(BaseObject *op, CDAABB *bnd);
		
		void DoModelToolScale(BaseObject *op, Vector oldSca);
		void DoModelToolScaleChildren(BaseObject *op, Vector scale);
	
	public:
		Vector pos, sca, rot;
		LONG c4dunit;
		
		CDCoordinatesDialog(void);

		virtual Bool CoreMessage(LONG id, const BaseContainer& msg);
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
		
};

CDCoordinatesDialog::CDCoordinatesDialog(void)
{ 
}

Bool CDCoordinatesDialog::CoreMessage(LONG id, const BaseContainer& msg)
{
	switch(id)
	{
		case EVMSG_CHANGE:
		{
			GetPSRValues(false);
			UpdatePSRValues();
			DoEnable(false);
			break;
		}
		case EVMSG_ASYNCEDITORMOVE:
		{
			GetPSRValues(true);
			UpdatePSRValues();
			DoEnable(true);
			break;
		}
		case CD_MSG_SELECTION_UPDATE:
		{
			GetPSRValues(true);
			UpdatePSRValues();
			DoEnable(true);
			break;
		}
	}
	
	return true;
}


Bool CDCoordinatesDialog::CreateLayout(void)
{
	Bool res = GeDialog::CreateLayout();
	if(res)
	{
		SetTitle(GeLoadString(CDCM_TITLE));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
			{
				GroupBorderNoTitle(BORDER_NONE);
				GroupBorderSpace(10,5,10,5);
				
				GroupBegin(0,BFH_LEFT,4,0,"",0);
				{
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_UNITS),0);
					AddComboBox(CDCM_UNITS, BFH_SCALEFIT);
						AddChild(CDCM_UNITS, 0, GeLoadString(CDCM_KM));
						AddChild(CDCM_UNITS, 1, GeLoadString(CDCM_M));
						AddChild(CDCM_UNITS, 2, GeLoadString(CDCM_CM));
						AddChild(CDCM_UNITS, 3, GeLoadString(CDCM_MM));
						AddChild(CDCM_UNITS, 4, GeLoadString(CDCM_MI));
						AddChild(CDCM_UNITS, 5, GeLoadString(CDCM_YD));
						AddChild(CDCM_UNITS, 6, GeLoadString(CDCM_FT));
						AddChild(CDCM_UNITS, 7, GeLoadString(CDCM_IN));
					
					AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_UNIT_SCALE),0);
					AddEditNumberArrows(CDCM_UNIT_SCALE,BFH_LEFT);
					
				}
				GroupEnd();
				
			}
			GroupEnd();
			
			GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
			{
				GroupBorderNoTitle(BORDER_NONE);
				GroupBorderSpace(10,0,10,5);
				
				GroupBegin(CDCM_COORDS,BFH_SCALEFIT,3,0,"",0);
				{
					GroupBegin(CDCM_P_GROUP,BFH_SCALEFIT,2,0,"",0);
					{
						AddStaticText(0,BFH_LEFT,0,0," ",0);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_P_GROUP),0);
						
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_X_POS),0);
						AddEditNumberArrows(CDCM_X_POS,BFH_SCALEFIT);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_Y_POS),0);
						AddEditNumberArrows(CDCM_Y_POS,BFH_SCALEFIT);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_Z_POS),0);
						AddEditNumberArrows(CDCM_Z_POS,BFH_SCALEFIT);
						
						AddStaticText(0,BFH_LEFT,0,0," ",0);
						AddComboBox(CDCM_GLOBAL, BFH_SCALEFIT);
							AddChild(CDCM_GLOBAL, 0, GeLoadString(CDCM_LOCAL));
							AddChild(CDCM_GLOBAL, 1, GeLoadString(CDCM_GLOBAL));
							AddChild(CDCM_GLOBAL, 2, GeLoadString(CDCM_OBJECT_PLUS));
							
					}
					GroupEnd();
					
					GroupBegin(CDCM_S_GROUP,BFH_SCALEFIT,2,0,"",0);
					{
						AddStaticText(0,BFH_LEFT,0,0," ",0);
						AddStaticText(CDCM_S_GRP_NAME,BFH_LEFT,0,0,GeLoadString(CDCM_SCALE),0);
						
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_X_SCA),0);
						AddEditNumberArrows(CDCM_X_SCA,BFH_SCALEFIT);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_Y_SCA),0);
						AddEditNumberArrows(CDCM_Y_SCA,BFH_SCALEFIT);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_Z_SCA),0);
						AddEditNumberArrows(CDCM_Z_SCA,BFH_SCALEFIT);
							
						AddStaticText(0,BFH_LEFT,0,0," ",0);
						AddComboBox(CDCM_SCALE, BFH_SCALEFIT);
							AddChild(CDCM_SCALE, 0, GeLoadString(CDCM_SCALE));
							AddChild(CDCM_SCALE, 1, GeLoadString(CDCM_SIZE));
							AddChild(CDCM_SCALE, 2, GeLoadString(CDCM_SIZE_PLUS));
							AddChild(CDCM_SCALE, 3, GeLoadString(CDCM_SCALE_THREE));
							AddChild(CDCM_SCALE, 4, GeLoadString(CDCM_SIZE_THREE));
					}
					GroupEnd();
					
					GroupBegin(CDCM_R_GROUP,BFH_SCALEFIT,2,0,"",0);
					{
						AddStaticText(0,BFH_LEFT,0,0," ",0);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_R_GROUP),0);
						
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_H_ROT),0);
						AddEditNumberArrows(CDCM_H_ROT,BFH_SCALEFIT);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_P_ROT),0);
						AddEditNumberArrows(CDCM_P_ROT,BFH_SCALEFIT);
						AddStaticText(0,BFH_LEFT,0,0,GeLoadString(CDCM_B_ROT),0);
						AddEditNumberArrows(CDCM_B_ROT,BFH_SCALEFIT);

						AddStaticText(0,BFH_LEFT,0,0," ",0);
						AddComboBox(CDCM_DEGREE, BFH_SCALEFIT);
							AddChild(CDCM_DEGREE, 0, GeLoadString(CDCM_RADIAN));
							AddChild(CDCM_DEGREE, 1, GeLoadString(CDCM_DEGREE));
					}
					GroupEnd();
					
				}
				GroupEnd();
				
				AddButton(CDCM_APPLY,BFH_CENTER,0,0,GeLoadString(CDCM_APPLY));
			}
			GroupEnd();
		}
		GroupEnd();

	}

	
	return res;
}

Bool CDCoordinatesDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeDialog::InitValues()) return false;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDCOORDINATES);
	if(!wpData)
	{
		SetLong(CDCM_GLOBAL,0);
		SetLong(CDCM_SCALE,1);
		SetLong(CDCM_DEGREE,1);
	}
	else
	{
		SetLong(CDCM_GLOBAL,wpData->GetLong(CDCM_GLOBAL));
		SetLong(CDCM_SCALE,wpData->GetLong(CDCM_SCALE));
		SetLong(CDCM_DEGREE,wpData->GetLong(CDCM_DEGREE));
		
	}
	
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *docData = doc->GetDataInstance()->GetContainerInstance(ID_CDCOORDINATES);
	if(!docData)
	{
		SetLong(CDCM_UNITS,3);
		SetReal(CDCM_UNIT_SCALE,1.0,0.1,CDMAXREAL,0.1);
	}
	else
	{
		SetLong(CDCM_UNITS,docData->GetLong(CDCM_UNITS));
		SetReal(CDCM_UNIT_SCALE,docData->GetReal(CDCM_UNIT_SCALE),0.1,CDMAXREAL,0.1);
	}
	
	GetPSRValues(false);
	UpdatePSRValues();
	DoEnable(false);

	return true;
}

void CDCoordinatesDialog::DoApply(void)
{
	LONG i, opCnt = 0;
	
	BaseDocument *doc = GetActiveDocument();
	AutoAlloc<AtomArray> opActiveList;
	AutoAlloc<AtomArray> opUndoList;
	if(opActiveList)
	{
		CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
		opCnt = opActiveList->GetCount();
	}
	
	if(IsCommandEnabled(12425) && CDIsCommandChecked(12425))
	{
		if(opActiveList && opUndoList)
		{
			opUndoList->Flush();
			BaseObject *op = NULL, *undoOp = NULL;
			for(i=0; i<opCnt; i++)
			{
				op = static_cast<BaseObject*>(opActiveList->GetIndex(i));
				undoOp = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY,NULL);
				if(undoOp) opUndoList->Append(undoOp);
			}
			SetPSRValues();
			for(i=0; i<opCnt; i++)
			{
				op = static_cast<BaseObject*>(opActiveList->GetIndex(i));
				undoOp = static_cast<BaseObject*>(opUndoList->GetIndex(i));
				doc->AutoKey(undoOp,op,false,CDIsCommandChecked(12417),CDIsCommandChecked(12418),CDIsCommandChecked(12419),CDIsCommandChecked(12422),CDIsCommandChecked(12421));
			}
			for(i=0; i<opCnt; i++)
			{
				undoOp = static_cast<BaseObject*>(opUndoList->GetIndex(0));
				if(undoOp)
				{
					opUndoList->Remove(undoOp);
					BaseObject::Free(undoOp);
				}
			}
		}
		else SetPSRValues();
	}
	else SetPSRValues();
}

Bool CDCoordinatesDialog::Command(LONG id,const BaseContainer &msg)
{

	LONG cmUnit, cmWorld, cmScale, cmDegree;
	Real cmUScale;
	LONG opCnt = 0;
	
	BaseDocument *doc = GetActiveDocument();
	AutoAlloc<AtomArray> opActiveList;
	if(opActiveList)
	{
		CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
		opCnt = opActiveList->GetCount();
	}

	GetReal(CDCM_UNIT_SCALE,cmUScale);
	GetLong(CDCM_UNITS,cmUnit);
	GetLong(CDCM_GLOBAL,cmWorld);
	GetLong(CDCM_SCALE,cmScale);
	GetLong(CDCM_DEGREE,cmDegree);
	
	BaseContainer *docData = doc->GetDataInstance();
	BaseContainer dbc;
	dbc.SetLong(CDCM_UNITS,cmUnit);
	dbc.SetReal(CDCM_UNIT_SCALE,cmUScale);
	docData->SetContainer(ID_CDCOORDINATES,dbc);
	
	BaseContainer bc;
	bc.SetLong(CDCM_DEGREE,cmDegree);
	if(opCnt == 1)
	{
		bc.SetLong(CDCM_GLOBAL,cmWorld);
		bc.SetLong(CDCM_SCALE,cmScale);
	}
	else
	{
		BaseContainer *wpData = GetWorldPluginData(ID_CDCOORDINATES);
		if(wpData)
		{
			bc.SetLong(CDCM_GLOBAL,wpData->GetLong(CDCM_GLOBAL));
			bc.SetLong(CDCM_SCALE,wpData->GetLong(CDCM_SCALE));
		}
	}
	SetWorldPluginData(ID_CDCOORDINATES,bc,false);
	
	switch(id)
	{
		case CDCM_APPLY:
			GetInputGlobals();
			DoApply();
			UpdatePSRValues();
			EventAdd(EVENT_FORCEREDRAW);
			break;
		case 1:
			GetInputGlobals();
			DoApply();
			UpdatePSRValues();
			EventAdd(EVENT_FORCEREDRAW);
			break;
		case CDCM_UNITS:
			GetPSRValues(false);
			UpdatePSRValues();
			break;
		case CDCM_UNIT_SCALE:
			GetPSRValues(false);
			UpdatePSRValues();
			break;
		case CDCM_DEGREE:
			GetPSRValues(false);
			UpdatePSRValues();
			break;
		case CDCM_SCALE:
			GetPSRValues(false);
			UpdatePSRValues();
			break;
		case CDCM_GLOBAL:
			GetPSRValues(false);
			UpdatePSRValues();
			break;
	}
	
	DoEnable(false);
	
	return true;
}

void CDCoordinatesDialog::DoEnable(Bool async)
{
	BaseDocument *doc = GetActiveDocument();
	LONG opCnt = 0;
	
	AutoAlloc<AtomArray> opActiveList;
	if(opActiveList)
	{
		CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
		opCnt = opActiveList->GetCount();
	}

	Bool oMode = true;
	if(doc->IsEditMode()) oMode = false;
	
	if(opCnt > 1)
	{
		Enable(CDCM_X_POS,oMode);
		Enable(CDCM_Y_POS,oMode);
		Enable(CDCM_Z_POS,oMode);
		
		Enable(CDCM_X_SCA,oMode);
		Enable(CDCM_Y_SCA,false);
		Enable(CDCM_Z_SCA,false);
		
		if(!async)
		{
			Enable(CDCM_H_ROT,oMode);
			Enable(CDCM_P_ROT,oMode);
			Enable(CDCM_B_ROT,oMode);
		}
		else
		{
			Enable(CDCM_H_ROT,true);
			Enable(CDCM_P_ROT,true);
			Enable(CDCM_B_ROT,true);
		}	
		
		Enable(CDCM_GLOBAL,false);
		Enable(CDCM_SCALE,false);
		Enable(CDCM_DEGREE,oMode);
		
		Enable(CDCM_APPLY,oMode);
	}
	else if(opCnt == 1)
	{
		Enable(CDCM_X_POS,true);
		Enable(CDCM_Y_POS,true);
		Enable(CDCM_Z_POS,true);
		
		Enable(CDCM_X_SCA,true);
		Enable(CDCM_Y_SCA,true);
		Enable(CDCM_Z_SCA,true);
		
		Enable(CDCM_H_ROT,true);
		Enable(CDCM_P_ROT,true);
		Enable(CDCM_B_ROT,true);
		
		Enable(CDCM_GLOBAL,true);
		Enable(CDCM_SCALE,oMode);
		Enable(CDCM_DEGREE,true);
		
		Enable(CDCM_APPLY,true);
	}
	else
	{
		Enable(CDCM_X_POS,false);
		Enable(CDCM_Y_POS,false);
		Enable(CDCM_Z_POS,false);
		
		Enable(CDCM_X_SCA,false);
		Enable(CDCM_Y_SCA,false);
		Enable(CDCM_Z_SCA,false);
		
		Enable(CDCM_H_ROT,false);
		Enable(CDCM_P_ROT,false);
		Enable(CDCM_B_ROT,false);
		
		Enable(CDCM_GLOBAL,false);
		Enable(CDCM_SCALE,false);
		Enable(CDCM_DEGREE,false);
		
		Enable(CDCM_APPLY,false);
	}
}


void CDCoordinatesDialog::GetInputGlobals(void)
{
	GetReal(CDCM_X_POS,pos.x);
	GetReal(CDCM_Y_POS,pos.y);
	GetReal(CDCM_Z_POS,pos.z);

	// to do: add a constrain proportions here
	GetReal(CDCM_X_SCA,sca.x);
	GetReal(CDCM_Y_SCA,sca.y);
	GetReal(CDCM_Z_SCA,sca.z);

	GetReal(CDCM_H_ROT,rot.x);
	GetReal(CDCM_P_ROT,rot.y);
	GetReal(CDCM_B_ROT,rot.z);
}

void CDCoordinatesDialog::UpdatePSRValues(void)
{
	Vector theScale;
	
	SetReal(CDCM_X_POS,pos.x);
	SetReal(CDCM_Y_POS,pos.y);
	SetReal(CDCM_Z_POS,pos.z);

	LONG cmDegree;
	GetLong(CDCM_DEGREE,cmDegree);
	switch(cmDegree)
	{
		case 0:
			SetReal(CDCM_H_ROT,rot.x);
			SetReal(CDCM_P_ROT,rot.y);
			SetReal(CDCM_B_ROT,rot.z);
			break;
		case 1:
			SetDegree(CDCM_H_ROT,rot.x);
			SetDegree(CDCM_P_ROT,rot.y);
			SetDegree(CDCM_B_ROT,rot.z);
			break;
	}
	
	BaseDocument *doc = GetActiveDocument();
	LONG opCnt = 0, cmScale, cmWorld;
	
	AutoAlloc<AtomArray> opActiveList;
	if(opActiveList)
	{
		CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
		opCnt = opActiveList->GetCount();
	}
	
	if(opCnt > 1)
	{
		cmScale = 0;
		cmWorld = 1;
		
		theScale.x = 1.0;
		theScale.y = 0.0;
		theScale.z = 0.0;
	}
	else if(opCnt == 1)
	{
		theScale.x = sca.x;
		theScale.y = sca.y;
		theScale.z = sca.z;
			
		BaseContainer *wpData = GetWorldPluginData(ID_CDCOORDINATES);
		if(wpData)
		{
			cmScale = wpData->GetLong(CDCM_SCALE);
			cmWorld = wpData->GetLong(CDCM_GLOBAL);
		}
		else
		{
			GetLong(CDCM_SCALE,cmScale);
			GetLong(CDCM_GLOBAL,cmWorld);
		}
	}
	else 
	{
		cmScale = 0;
		cmWorld = 1;
		
		theScale.x = 0.0;
		theScale.y = 0.0;
		theScale.z = 0.0;
	}
	SetLong(CDCM_SCALE,cmScale);
	SetLong(CDCM_GLOBAL,cmWorld);
	
	SetReal(CDCM_X_SCA,theScale.x);
	SetReal(CDCM_Y_SCA,theScale.y);
	SetReal(CDCM_Z_SCA,theScale.z);
	switch(cmScale)
	{
		case 0:
			SetString(CDCM_S_GRP_NAME,GeLoadString(CDCM_SCALE));
			break;
		default:
			SetString(CDCM_S_GRP_NAME,GeLoadString(CDCM_SIZE));
			break;
	}
	
	BaseContainer *docData = doc->GetDataInstance()->GetContainerInstance(ID_CDCOORDINATES);
	if(!docData)
	{
		SetLong(CDCM_UNITS,3);
		SetReal(CDCM_UNIT_SCALE,1.0,0.1,CDMAXREAL,0.1);
	}
	else
	{
		SetLong(CDCM_UNITS,docData->GetLong(CDCM_UNITS));
		SetReal(CDCM_UNIT_SCALE,docData->GetReal(CDCM_UNIT_SCALE),0.1,CDMAXREAL,0.1);
	}
}

Real CDCoordinatesDialog::GetUnitFactor(LONG unit, LONG inv)
{
	Real r = 1;
	
	switch(inv)
	{
		case 0:
			switch(unit)
			{
				case 0:
					r = 0.000001;
					break;
				case 1:
					r = 0.001;
					break;
				case 2:
					r = 0.1;
					break;
				case 3:
					r = 1.0;
					break;
				case 4:
					r = 0.000000621;
					break;
				case 5:
					r = 0.001093613;
					break;
				case 6:
					r = 0.003280839;
					break;
				case 7:
					r = 0.039370078;
					break;
			}
			break;
		case 1:
			switch(unit)
			{
				case 0:
					r = 1000000.0;
					break;
				case 1:
					r = 1000.0;
					break;
				case 2:
					r = 10.0;
					break;
				case 3:
					r = 1.0;
					break;
				case 4:
					r = 1609344;
					break;
				case 5:
					r = 914.4;
					break;
				case 6:
					r = 304.8;
					break;
				case 7:
					r = 25.4;
					break;
			}
			break;
	}
	
	return r;
}

void CDCoordinatesDialog::AddChildBounds(BaseObject *op, CDAABB *bnd)
{
	Matrix opM;
	
	while(op)
	{
		opM = op->GetMg();
		bnd->AddPoint(opM * (op->GetMp() + op->GetRad()));
		bnd->AddPoint(opM * (op->GetMp() - op->GetRad()));
		
		AddChildBounds(op->GetDown(), bnd);
		
		op = op->GetNext();
	}
}

void CDCoordinatesDialog::GetPSRValues(Bool async)
{
	LONG cmUnit, cmWorld, cmScale, cmDegree;
	Matrix opM, bM;
	Real unitFactor, scaleFactor, cmUScale;
	Vector bounds, gScale;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDCOORDINATES);
	if(!wpData)
	{
		GetLong(CDCM_GLOBAL,cmWorld);
		GetLong(CDCM_SCALE,cmScale);
		GetLong(CDCM_DEGREE,cmDegree);
	}
	else
	{
		cmWorld = wpData->GetLong(CDCM_GLOBAL);
		cmScale = wpData->GetLong(CDCM_SCALE);
		cmDegree = wpData->GetLong(CDCM_DEGREE);
	}
	
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *docData = doc->GetDataInstance()->GetContainerInstance(ID_CDCOORDINATES);
	if(!docData)
	{
		GetLong(CDCM_UNITS,cmUnit);
		GetReal(CDCM_UNIT_SCALE,cmUScale);
	}
	else
	{
		cmUnit = docData->GetLong(CDCM_UNITS);
		cmUScale = docData->GetReal(CDCM_UNIT_SCALE);
	}
	
	if(cmUScale < 0.01) cmUScale = 0.01;
	scaleFactor = 1/cmUScale;
	
	BaseObject *grp = NULL;
	LONG opCnt = 0;
	
	AutoAlloc<AtomArray> opActiveList;
	if(opActiveList)
	{
		CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
		opCnt = opActiveList->GetCount();
	}
	
	if(opCnt > 1) grp = doc->GetHelperAxis();
	else if(opCnt == 1) grp = doc->GetActiveObject();
	if(grp)
	{
		unitFactor = GetUnitFactor(cmUnit,0) * scaleFactor;
		
		if(doc->IsEditMode())
		{
			if(opCnt == 1)
			{
				LONG i;
				if(IsValidPointObject(grp))
				{
					switch(cmWorld)
					{
						case 2:
						{
							rot = Vector(0,0,0);
							pos = Vector(0,0,0);
							sca = Vector(0,0,0);
							break;
						}
						default:
						{
							Vector *padr = GetPointArray(grp);
							BaseSelect *ptSel = ToPoint(grp)->GetPointS();
							
							AutoAlloc<BaseSelect> bs;
							if(bs)
							{
								switch(doc->GetMode())
								{
									case Mpoints:
									{
										bs->Merge(ptSel);
										break;
									}
									case Medges:;
									{
										if(IsValidPolygonObject(grp))
										{
											BaseSelect *edgeS = ToPoly(grp)->GetEdgeS();
											if(edgeS) ConvertToPointSelection(edgeS,bs,grp,Medges);
										}
										break;
									}
									case Mpolygons:
									{
										if(IsValidPolygonObject(grp))
										{
											BaseSelect *plyS = ToPoly(grp)->GetPolygonS();
											if(plyS) ConvertToPointSelection(plyS,bs,grp,Mpolygons);
										}
										break;
									}
								}
								
								LONG bsCnt = bs->GetCount(), ptCnt = ToPoint(grp)->GetPointCount();
								if(bsCnt > 0)
								{
									CDAABB ptBounds;
									ptBounds.Empty();
									
									LONG seg=0,a,b;
									while(CDGetRange(bs,seg++,&a,&b))
									{
										for (i=a; i<=b; ++i)
										{
											if(i >= 0 && i < ptCnt)
											{
												Vector pt = padr[i];
												ptBounds.AddPoint(pt);
											}
										}
									}
									if(cmWorld == 1) pos = (grp->GetMg() * ptBounds.GetCenterPoint()) * unitFactor;
									else pos = ptBounds.GetCenterPoint() * unitFactor;
									sca = ((ptBounds.max - ptBounds.min) * unitFactor);
								}
								else
								{
									pos = Vector(0,0,0);
									sca = Vector(0,0,0);
									rot = Vector(0,0,0);
								}
							}
							break;
						}
					}
				}
				else
				{
					pos = Vector(0,0,0);
					sca = Vector(0,0,0);
					if(!async) rot = Vector(0,0,0);
					else rot = CDGetRot(grp);
				}
			}
			else
			{
				pos = Vector(0,0,0);
				sca = Vector(0,0,0);
				if(!async) rot = Vector(0,0,0);
				else rot = CDGetRot(grp);
			}
		}
		else
		{
			switch(cmWorld)
			{
				case 0:
					opM = grp->GetMl();
					rot = CDGetRot(grp);
					pos = CDGetPos(grp) * unitFactor;
					break;
				case 1:
					opM = grp->GetMg();
					if(opCnt == 1)
					{
						rot = GetGlobalRotation(grp);
					}
					else
					{
						rot = CDMatrixToHPB(opM,grp);
					}
					pos = opM.off * unitFactor;
					break;
				case 2:
					rot = Vector(0,0,0);
					pos = Vector(0,0,0);
					break;
			}
			
			if(opCnt == 1)
			{
				Matrix scM = MatrixScale(GetGlobalScale(grp));
				bounds = scM * (grp->GetRad() * 2);
				switch(cmScale)
				{
					case 0:
					{
						switch(cmWorld)
						{
							case 0:
								sca = CDGetScale(grp);
								break;
							case 1:
								sca = GetGlobalScale(grp);
								break;
							case 2:
								sca = Vector(0,0,0);
								break;
						}
						break;
					}
					case 1:
					{
						switch(cmWorld)
						{
							case 2:
								sca = Vector(0,0,0);
								break;
							default:
								sca = bounds * unitFactor;
								break;
						}
						break;
					}
					case 2:
					{
						switch(cmWorld)
						{
							case 2:
								sca = Vector(0,0,0);
								break;
							default:
								if(grp->GetDown())
								{
									CDAABB hBounds;
									hBounds.Empty();
									
									opM = grp->GetMg();
									hBounds.AddPoint(opM * (grp->GetMp() + grp->GetRad()));
									hBounds.AddPoint(opM * (grp->GetMp() - grp->GetRad()));
									
									AddChildBounds(grp->GetDown(),&hBounds);
									Vector hSize = (hBounds.max - hBounds.min) * unitFactor;
									sca = scM * hSize;
								}
								else
								{
									sca = bounds * unitFactor;
								}
								break;
						}
						break;
					}
					case 3:
					{
						switch(cmWorld)
						{
							case 0:
								sca = CDGetScale(grp);
								break;
							case 1:
								sca = GetGlobalScale(grp);
								break;
							case 2:
								sca = Vector(0,0,0);
								break;
						}
						break;
					}
					case 4:
					{
						switch(cmWorld)
						{
							case 2:
								sca = Vector(0,0,0);
								break;
							default:
								sca = bounds * unitFactor;
								break;
						}
						break;
					}
				}
			}
		}
	}
	else
	{
		pos = Vector(0,0,0);
		rot = Vector(0,0,0);
		sca = Vector(0,0,0);
	}	
}

Vector CDCoordinatesDialog::GetInputScale(BaseObject *op, Real ufct, Real ifct)
{
	Matrix opM, scM = MatrixScale(GetGlobalScale(op));
	Vector bounds = scM * (op->GetRad() * 2);
	Vector oldSca, addSca, newSca = CDGetScale(op);
	Real scale3 = 1;
	
	LONG cmWorld, cmScale;
	GetLong(CDCM_GLOBAL,cmWorld);
	GetLong(CDCM_SCALE,cmScale);

	switch(cmScale)
	{
		case 0:
		{
			switch(cmWorld)
			{
				case 0:
					newSca = sca;
					break;
				case 1:
					oldSca = GetGlobalScale(op);
					if(oldSca.x != 0.0) newSca.x *= sca.x / oldSca.x;
					if(oldSca.y != 0.0) newSca.y *= sca.y / oldSca.y;
					if(oldSca.z != 0.0) newSca.z *= sca.z / oldSca.z;
					break;
				case 2:
					newSca += sca;
					break;
			}
			break;
		}
		case 1:
		{
			oldSca = bounds * ifct;
			switch(cmWorld)
			{
				case 2:
					addSca = oldSca + sca;
					if(oldSca.x != 0.0) newSca.x *= addSca.x / oldSca.x;
					if(oldSca.y != 0.0) newSca.y *= addSca.y / oldSca.y;
					if(oldSca.z != 0.0) newSca.z *= addSca.z / oldSca.z;
					break;
				default:
					if(oldSca.x != 0.0) newSca.x *= sca.x / oldSca.x;
					if(oldSca.y != 0.0) newSca.y *= sca.y / oldSca.y;
					if(oldSca.z != 0.0) newSca.z *= sca.z / oldSca.z;
					break;
			}
			break;
		}
		case 2:
		{
			if(op->GetDown())
			{
				CDAABB hBounds;
				hBounds.Empty();
				
				opM = op->GetMg();
				hBounds.AddPoint(opM * (op->GetMp() + op->GetRad()));
				hBounds.AddPoint(opM * (op->GetMp() - op->GetRad()));
				
				AddChildBounds(op->GetDown(),&hBounds);
				Vector hSize = (hBounds.max - hBounds.min) * ifct;
				oldSca = scM * hSize;
			}
			else
			{
				oldSca = bounds * ifct;
			}
			switch(cmWorld)
			{
				case 2:
					addSca = oldSca + sca;
					if(oldSca.x != 0.0) newSca.x *= addSca.x / oldSca.x;
					if(oldSca.y != 0.0) newSca.y *= addSca.y / oldSca.y;
					if(oldSca.z != 0.0) newSca.z *= addSca.z / oldSca.z;
					break;
				default:
					if(oldSca.x != 0.0) newSca.x *= sca.x / oldSca.x;
					if(oldSca.y != 0.0) newSca.y *= sca.y / oldSca.y;
					if(oldSca.z != 0.0) newSca.z *= sca.z / oldSca.z;
					break;
			}
			break;
		}
		case 3:
		{
			switch(cmWorld)
			{
				case 0:
					oldSca = CDGetScale(op);
					if(oldSca.x != sca.x && oldSca.x != 0.0)
					{
						scale3 = sca.x / oldSca.x;
					}
					else if(oldSca.y != sca.y && oldSca.y != 0.0)
					{
						scale3 = sca.y / oldSca.y;
					}
					else if(oldSca.z != sca.z && oldSca.z != 0.0)
					{
						scale3 = sca.z / oldSca.z;
					}
					newSca *= scale3;
					break;
				case 1:
					oldSca = GetGlobalScale(op);
					if(oldSca.x != sca.x && oldSca.x != 0.0)
					{
						scale3 = sca.x / oldSca.x;
					}
					else if(oldSca.y != sca.y && oldSca.y != 0.0)
					{
						scale3 = sca.y / oldSca.y;
					}
					else if(oldSca.z != sca.z && oldSca.z != 0.0)
					{
						scale3 = sca.z / oldSca.z;
					}
					newSca *= scale3;
					break;
				case 2:
					if(sca.x != 0.0)
					{
						scale3 = sca.x;
					}
					else if(sca.y != 0.0)
					{
						scale3 = sca.y;
					}
					else if(sca.z != 0.0)
					{
						scale3 = sca.z;
					}
					newSca.x += scale3;
					newSca.y += scale3;
					newSca.z += scale3;
					break;
			}
			break;
		}
		case 4:
		{
			oldSca = bounds * ifct;
			switch(cmWorld)
			{
				case 2:
					addSca = oldSca + sca;
					if(addSca.x != oldSca.x && oldSca.x != 0.0)
					{
						scale3 = addSca.x / oldSca.x;
					}
					else if(addSca.y != oldSca.y && oldSca.y != 0.0)
					{
						scale3 = addSca.y / oldSca.y;
					}
					else if(addSca.z != oldSca.z && oldSca.z != 0.0)
					{
						scale3 = addSca.z / oldSca.z;
					}
					newSca *= scale3;
					break;
				default:
					if(oldSca.x != sca.x && oldSca.x != 0.0)
					{
						scale3 = sca.x / oldSca.x;
					}
					else if(oldSca.y != sca.y && oldSca.y != 0.0)
					{
						scale3 = sca.y / oldSca.y;
					}
					else if(oldSca.z != sca.z && oldSca.z != 0.0)
					{
						scale3 = sca.z / oldSca.z;
					}
					newSca *= scale3;
					break;
			}
			break;
		}
	}
	
	return newSca;
}

void CDCoordinatesDialog::SetPSRValues(void)
{
	LONG cmUnit, cmWorld, cmScale, cmDegree;
	Matrix opM, pM, sM, rM, tM, bM;
	Real unitFactor, scaleFactor, invUnitFactor, cmUScale;
	Vector bounds, gScale;
	
	GetReal(CDCM_UNIT_SCALE,cmUScale);
	GetLong(CDCM_UNITS,cmUnit);
	GetLong(CDCM_GLOBAL,cmWorld);
	GetLong(CDCM_SCALE,cmScale);
	GetLong(CDCM_DEGREE,cmDegree);
	
	if(cmUScale < 0.01) cmUScale = 0.01;
	scaleFactor = 1/cmUScale;

	BaseObject *op = NULL, *grp = NULL;
	BaseDocument *doc = GetActiveDocument();
	LONG i, opCnt = 0;
	
	AutoAlloc<AtomArray> opActiveList;
	if(opActiveList)
	{
		CDGetActiveObjects(doc,opActiveList,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
		opCnt = opActiveList->GetCount();
	}
	
	if(opCnt > 1) grp = doc->GetHelperAxis();
	else if(opCnt == 1) grp = doc->GetActiveObject();
	if(grp)
	{
		unitFactor = GetUnitFactor(cmUnit,1) * cmUScale;
		invUnitFactor = GetUnitFactor(cmUnit,0) * scaleFactor;
	
		doc->StartUndo();
		
		if(doc->IsEditMode())
		{
			if(opCnt == 1)
			{
				LONG i;
				if(IsValidPointObject(grp))
				{
					Vector *padr = GetPointArray(grp);
					BaseSelect *ptSel = ToPoint(grp)->GetPointS();
					Matrix opM = grp->GetMg();
					
					AutoAlloc<BaseSelect> bs;
					if(bs)
					{
						switch(doc->GetMode())
						{
							case Mpoints:
							{
								bs->Merge(ptSel);
								break;
							}
							case Medges:;
							{
								if(IsValidPolygonObject(grp))
								{
									BaseSelect *edgeS = ToPoly(grp)->GetEdgeS();
									if(edgeS) ConvertToPointSelection(edgeS,bs,grp,Medges);
								}
								break;
							}
							case Mpolygons:
							{
								if(IsValidPolygonObject(grp))
								{
									BaseSelect *plyS = ToPoly(grp)->GetPolygonS();
									if(plyS) ConvertToPointSelection(plyS,bs,grp,Mpolygons);
								}
								break;
							}
						}
						LONG bsCnt = bs->GetCount();
						if(bsCnt > 0)
						{
							Tangent *tngt = NULL;
							BaseContainer *oData = grp->GetDataInstance();
							if(grp->IsInstanceOf(Ospline) && oData->GetLong(SPLINEOBJECT_TYPE) == SPLINEOBJECT_TYPE_BEZIER)
							{
								tngt = GetTangentArray(grp);
							}
							
							Vector oldSca, addSca, newSca;
							CDAABB ptBounds;
							ptBounds.Empty();
							
							bM = Matrix();
							LONG seg=0,a,b;
							while(CDGetRange(bs,seg++,&a,&b))
							{
								for (i=a; i<=b; ++i)
								{
									Vector pt = padr[i];
									ptBounds.AddPoint(pt);
								}
							}
							bM.off = ptBounds.GetCenterPoint();
							
							rM = CDHPBToMatrix(rot,grp);
							
							newSca = Vector(1,1,1);
							oldSca = ((ptBounds.max - ptBounds.min) * invUnitFactor);
							switch(cmWorld)
							{
								case 2:
									addSca = oldSca + sca;
									if(oldSca.x != 0.0) newSca.x = addSca.x / oldSca.x;
									if(oldSca.y != 0.0) newSca.y = addSca.y / oldSca.y;
									if(oldSca.z != 0.0) newSca.z = addSca.z / oldSca.z;
									break;
								default:
									if(oldSca.x != 0.0) newSca.x = sca.x / oldSca.x;
									if(oldSca.y != 0.0) newSca.y = sca.y / oldSca.y;
									if(oldSca.z != 0.0) newSca.z = sca.z / oldSca.z;
									break;
							}
							sM = MatrixScale(newSca);
							
							pM = bM * rM * sM;
							Vector pOff = pM.off;
							switch(cmWorld)
							{
								case 0:
									pOff = (pos * unitFactor);
									break;
								case 1:
									pOff = MInv(grp->GetMg()) * (pos * unitFactor);
									break;
								case 2:
									pOff += (pos * unitFactor);
									break;
							}
							
							Matrix modAxis = MInv(opM) * grp->GetModelingAxis(doc);
							if(!VEqual(bM.off,modAxis.off,0.001))
							{
								rM.off = modAxis.off;
								pM.off = rM * (MInv(modAxis) * pOff);
							}
							else pM.off = pOff;
								
							CDAddUndo(doc,CD_UNDO_CHANGE,grp);
							
							seg=0;
							while(CDGetRange(bs,seg++,&a,&b))
							{
								for (i=a; i<=b; ++i)
								{
									Vector pt = opM * padr[i];
									padr[i] = MInv(opM) * (pM * (MInv(bM) * pt));
									
									if(tngt)
									{
										Vector tngL = opM * tngt[i].vl;
										Vector tngR = opM * tngt[i].vr;
										tngt[i].vl = MInv(opM) * (pM * (MInv(bM) * tngL));
										tngt[i].vr = MInv(opM) * (pM * (MInv(bM) * tngR));
									}
								}
							}
							rot = Vector(0,0,0);
						}
						else
						{
							pos = Vector(0,0,0);
							rot = Vector(0,0,0);
							sca = Vector(0,0,0);
						}
						
					}
					grp->Message(MSG_UPDATE);
				}
			}
		}
		else
		{
			if(opCnt > 1)
			{
				pM = grp->GetMg();
				rM = CDHPBToMatrix(rot,grp);
				tM = Matrix() * rM;
				tM.off = pos;
				 
				for(i=0; i<opCnt; i++)
				{
					op = static_cast<BaseObject*>(opActiveList->GetIndex(i));
					if(op)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						opM = MInv(pM) * op->GetMg();
						op->SetMg(tM * opM);
					}
				}
				CDAddUndo(doc,CD_UNDO_CHANGE,grp);
				grp->SetMg(tM);
			}
			else if(opCnt == 1)
			{
				if(CDIsAxisMode(doc))
				{
					opM = grp->GetMg();
					rM = CDHPBToMatrix(rot,grp);
					Vector oldSca, newSca, mSca = Vector(0,0,0);
					newSca = GetInputScale(grp,unitFactor,invUnitFactor);
					
					CDAddUndo(doc,CD_UNDO_CHANGE,grp);
					switch(cmWorld)
					{
						case 0:
						{
							CDSetPos(grp,pos * unitFactor);
							CDSetRot(grp,rot);
							CDSetScale(grp,newSca);
							break;
						}
						case 1:
						{
							pM = Matrix();
							tM = pM * rM;
							tM.off = (pos * unitFactor);
							grp->SetMg(tM);
							CDSetScale(grp,newSca);
							break;
						}
						case 2:
						{
							tM = opM * rM;
							tM.off = opM * (pos * unitFactor);
							grp->SetMg(tM);
							CDSetScale(grp,newSca);
							break;
						}
					}
					grp->Message(MSG_UPDATE);
					
					if(IsValidPointObject(grp))
					{
						if(grp->GetTag(ID_CDSKINREFPLUGIN) || grp->GetTag(ID_CDMORPHREFPLUGIN))
						{
							RecalculateReference(doc,grp,grp->GetMg(),opM);
						}
						else RecalculatePoints(grp,grp->GetMg(),opM);
					}
					
					if(grp->GetDown()) RepositionChildren(doc,grp->GetDown(),grp->GetMg(),opM,false);
					grp->Message(MSG_UPDATE);
				}
				else
				{
					Matrix oldM = grp->GetMg();
					Vector oldSca = CDGetScale(grp);
					
					CDAddUndo(doc,CD_UNDO_CHANGE,grp);
					switch(cmWorld)
					{
						case 0:
						{
							CDSetPos(grp,pos * unitFactor);
							CDSetRot(grp,rot);
							CDSetScale(grp,GetInputScale(grp,unitFactor,invUnitFactor));
							if(doc->GetMode() == Mmodel) DoModelToolScale(grp,oldSca);
							break;
						}
						case 1:
						{
							opM = CDHPBToMatrix(rot,grp);
							opM.off = (pos * unitFactor);
							Vector newSca = GetInputScale(grp,unitFactor,invUnitFactor);
							grp->SetMg(opM);
							CDSetScale(grp,newSca);
							if(doc->GetMode() == Mmodel) DoModelToolScale(grp,oldSca);
							break;
						}
						case 2:
						{
							opM = grp->GetMg();
							rM = CDHPBToMatrix(rot,grp);
							tM = opM * rM;
							tM.off = opM * (pos * unitFactor);
							grp->SetMg(tM);
							CDSetScale(grp,GetInputScale(grp,unitFactor,invUnitFactor));
							if(doc->GetMode() == Mmodel) DoModelToolScale(grp,oldSca);
							break;
						}
					}
				}
			}
		}
		doc->EndUndo();
	}
}

void CDCoordinatesDialog::DoModelToolScaleChildren(BaseObject *op, Vector scale)
{
	while(op)
	{
		Vector opPos = CDGetPos(op);
		opPos.x *= scale.x;
		opPos.y *= scale.y;
		opPos.z *= scale.z;
		CDSetPos(op,opPos);

		if(IsValidPointObject(op))
		{
			Vector pt, *padr = GetPointArray(op);
			LONG i, ptCnt = ToPoint(op)->GetPointCount();
			for(i=0; i<ptCnt; i++)
			{
				pt.x = padr[i].x * scale.x;
				pt.y = padr[i].y * scale.y;
				pt.z = padr[i].z * scale.z;
				padr[i] = pt;
			}
		}
		else ScaleAttributes(op,scale);
		
		op->Message(MSG_UPDATE);
		DoModelToolScaleChildren(op->GetDown(),scale);
		
		op = op->GetNext();
	}
}

void CDCoordinatesDialog::DoModelToolScale(BaseObject *op, Vector oldSca)
{
	Vector mtSca, newSca = CDGetScale(op);
	mtSca.x = newSca.x / oldSca.x;
	mtSca.y = newSca.y / oldSca.y;
	mtSca.z = newSca.z / oldSca.z;
		
	if(IsValidPointObject(op))
	{
		Vector pt, *padr = GetPointArray(op);
		LONG i, ptCnt = ToPoint(op)->GetPointCount();
		for(i=0; i<ptCnt; i++)
		{
			pt.x = padr[i].x * mtSca.x;
			pt.y = padr[i].y * mtSca.y;
			pt.z = padr[i].z * mtSca.z;
			padr[i] = pt;
		}
	}
	else ScaleAttributes(op,mtSca);
	
	CDSetScale(op,oldSca);
	op->Message(MSG_UPDATE);
	DoModelToolScaleChildren(op->GetDown(),mtSca);
}

class CDCoordinatesPlugin : public CommandData
{
	private:
		CDCoordinatesDialog dlg;
		
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
		#if API_VERSION < 12000
			return dlg.Open(true,ID_CDCOORDINATES,-1,-1);
		#else
			return dlg.Open(DLG_TYPE_ASYNC,ID_CDCOORDINATES,-1,-1);
		#endif
		}
		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDCOORDINATES,0,secret);
		}
};

class CDCoordinatesDialogR : public GeDialog
{
	public:
		virtual Bool CreateLayout(void)
		{
			Bool res = GeDialog::CreateLayout();
			if(res)
			{
				SetTitle(GeLoadString(CDCM_TITLE));
				
				GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
				{
					GroupBorderNoTitle(BORDER_NONE);
					GroupBorderSpace(10,5,10,5);
					
					AddStaticText(0,BFH_LEFT,0,0,"CD Transfer Tools v"+CDRealToString(GetCDPluginVersion(ID_CDTRANSFERTOOLS),-1,3),0);
				}
				GroupEnd();

			}
			return res;
		}
};

class CDCoordinatesPluginR : public CommandData
{
	private:
		CDCoordinatesDialogR dlg;
		
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDCOORDINATES,0,secret);
		}
};

Bool RegisterCDCoordinatesPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDTR_SERIAL_SIZE];
	String cdtnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDTRANSFERTOOLS,data,CDTR_SERIAL_SIZE)) reg = false;
	
	cdtnr.SetCString(data,CDTR_SERIAL_SIZE-1);
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
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDCOORDS); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDCOORDINATES,name,PLUGINFLAG_HIDE,"CDCoords.tif","CD Coordinates",CDDataAllocator(CDCoordinatesPluginR));
	else return CDRegisterCommandPlugin(ID_CDCOORDINATES,name,0,"CDCoords.tif","CD Coordinates",CDDataAllocator(CDCoordinatesPlugin));
}

