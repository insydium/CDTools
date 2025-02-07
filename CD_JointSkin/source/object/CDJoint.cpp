//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "customgui_priority.h"

#if API_VERSION >= 13000
	#include "ocharacterbase.h"
#endif

#include "CDCompatibility.h"
#include "CDObjectData.h"
#include "CDArray.h"

#include "CDJointSkin.h"
#include "CDJoint.h"

#include "oCDJoint.h"

#define HANDLE_CNT 6

extern Vector colorH, colorCH;

Bool CDJoint::Init(GeListNode *node)
{	
	//GePrint("CDJoint::Init");
	BaseObject		*op   = (BaseObject*)node;
	BaseContainer *data = op->GetDataInstance();

	ObjectColorProperties prop;
	prop.usecolor = 0;
	prop.color = Vector(1,1,1);
	prop.xray = true;
	op->SetColorProperties(&prop);
	
	data->SetReal(JNT_JOINT_RADIUS,10.0);
	data->SetBool(JNT_CONNECTED,true);
	
	data->SetBool(JNT_SHOW_ENVELOPE,false);
	data->SetBool(JNT_SHOW_PROXY,false);
	
	data->SetVector(JNT_ROOT_OFFSET,Vector(10,10,-5));
	data->SetVector(JNT_TIP_OFFSET,Vector(10,10,5));
	data->SetReal(JNT_SKEW_RX,0);
	data->SetReal(JNT_SKEW_RY,0);
	data->SetReal(JNT_SKEW_TX,0);
	data->SetReal(JNT_SKEW_TY,0);
	
	data->SetReal(JNT_STRETCH_VALUE,1);
	data->SetReal(JNT_SQUASH_VALUE,1);
	data->SetBool(JNT_USE_SPLINE,false);
	
	GeData d(CUSTOMGUI_PRIORITY_DATA,DEFAULTVALUE);
	PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
	if(!pd) return false;
	
	pd->SetPriorityValue(PRIORITYVALUE_MODE,CYCLE_EXPRESSION);
	pd->SetPriorityValue(PRIORITYVALUE_PRIORITY,10);
	pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
	
	data->SetData(JNT_PRIORITY,d);
	
	execute = false;
	
	CDJSData jd;
	PluginMessage(ID_CDJOINTOBJECT,&jd);
	if(jd.list) jd.list->Append(node);

	return true;
}

void CDJoint::Free(GeListNode* node)
{
	CDJSData jd;
	PluginMessage(ID_CDJOINTOBJECT,&jd);
	if(jd.list) jd.list->Remove(node);
}

void CDJoint::CheckOpReg(BaseContainer *oData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);

	CHAR b;
	String kb, cdjnr = oData->GetString(T_STR);
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
	
	oData->SetBool(T_REG,reg);
	if(!reg)
	{
		if(!oData->GetBool(T_SET))
		{
			oData->SetBool(JNT_CONNECTED+T_LST,oData->GetBool(JNT_CONNECTED));
			oData->SetBool(JNT_SHOW_LOCAL_AXIS+T_LST,oData->GetBool(JNT_SHOW_LOCAL_AXIS));
			oData->SetReal(JNT_JOINT_RADIUS+T_LST,oData->GetReal(JNT_JOINT_RADIUS));

			oData->SetBool(JNT_LOCK_ZERO_TRANS+T_LST,oData->GetBool(JNT_LOCK_ZERO_TRANS));

			oData->SetBool(JNT_SHOW_ENVELOPE+T_LST,oData->GetBool(JNT_SHOW_ENVELOPE));
			oData->SetBool(JNT_SHOW_PROXY+T_LST,oData->GetBool(JNT_SHOW_PROXY));
			oData->SetVector(JNT_ROOT_OFFSET+T_LST,oData->GetVector(JNT_ROOT_OFFSET));
			oData->SetVector(JNT_TIP_OFFSET+T_LST,oData->GetVector(JNT_TIP_OFFSET));
			oData->SetReal(JNT_SKEW_RX+T_LST,oData->GetReal(JNT_SKEW_RX));
			oData->SetReal(JNT_SKEW_RY+T_LST,oData->GetReal(JNT_SKEW_RY));
			oData->SetReal(JNT_SKEW_TX+T_LST,oData->GetReal(JNT_SKEW_TX));
			oData->SetReal(JNT_SKEW_TY+T_LST,oData->GetReal(JNT_SKEW_TY));
			
			oData->SetBool(T_SET,true);
		}
	}
}

Bool CDJoint::CreateKeys(BaseDocument *doc, BaseList2D *op, const BaseTime &time, LONG index, Vector value, Vector prev)
{
	Bool setPrevKey = (time != doc->GetMinTime());
	
	DescID dscID;

	// X value
	dscID = DescID(DescLevel(index,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
	CDSetKey(doc,op,time,dscID,value.x);
	if(setPrevKey) CDSetKey(doc,op,doc->GetMinTime(),dscID,prev.x);
	
	// Y value
	dscID = DescID(DescLevel(index,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
	CDSetKey(doc,op,time,dscID,value.y);
	if(setPrevKey) CDSetKey(doc,op,doc->GetMinTime(),dscID,prev.y);
	
	// Z value
	dscID = DescID(DescLevel(index,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
	CDSetKey(doc,op,time,dscID,value.z);
	if(setPrevKey) CDSetKey(doc,op,doc->GetMinTime(),dscID,prev.z);
	
	return true;
}

Bool CDJoint::AutoKey(BaseDocument *doc, BaseObject *opClone, BaseContainer *ocData, BaseObject *op, BaseContainer *oData)
{
	if(CDIsAxisMode(doc)) return false;
	
	switch(doc->GetAction())
	{
		case ID_MODELING_SCALE:
		{
			if(!VEqual(ocData->GetVector(JNT_TRANS_SCA),oData->GetVector(JNT_TRANS_SCA),0.01) && !CDIsCommandChecked(IDM_A_SIZE))
			{
				CreateKeys(doc,op,doc->GetTime(),JNT_TRANS_SCA,oData->GetVector(JNT_TRANS_SCA),ocData->GetVector(JNT_TRANS_SCA));
			}
			break;
		}
		case ID_MODELING_ROTATE:
		{
			if(!VEqual(ocData->GetVector(JNT_TRANS_ROT),oData->GetVector(JNT_TRANS_ROT),0.001) && !CDIsCommandChecked(IDM_A_DIR))
			{
				CreateKeys(doc,op,doc->GetTime(),JNT_TRANS_ROT,oData->GetVector(JNT_TRANS_ROT),ocData->GetVector(JNT_TRANS_ROT));
			}
			break;
		}
		default:
		{
			if(!VEqual(ocData->GetVector(JNT_TRANS_POS),oData->GetVector(JNT_TRANS_POS),0.001) && !CDIsCommandChecked(IDM_A_POS))
			{
				CreateKeys(doc,op,doc->GetTime(),JNT_TRANS_POS,oData->GetVector(JNT_TRANS_POS),ocData->GetVector(JNT_TRANS_POS));
			}
			break;
		}
	}
	
	return true;
}

BaseObject* CDJoint::GetCharacterObjectParent(BaseObject *op)
{
#if API_VERSION >= 13000
    BaseObject *pr = op->GetUp();
    while(pr)
    {
        if(pr->IsInstanceOf(Ocharacter)) return pr;
        pr = pr->GetUp();
    }
#endif
    
    return NULL;
}

Bool CDJoint::IsAdjustMode(BaseObject *caOp)
{
#if API_VERSION >= 13000
    BaseContainer *caData = caOp->GetDataInstance();
    if(caData)
    {
        if(caData->GetLong(ID_CA_CHARACTER_MODE) == ID_CA_CHARACTER_MODE_ADJUST) return true;
    }
#endif
    
    return false;
}

Bool CDJoint::Message(GeListNode *node, LONG type, void *data)
{
	//GePrint("CDJoint::Message");
	BaseObject *op = (BaseObject*)node, *ch = NULL, *chAtm = NULL, *pred = NULL;
	BaseContainer *oData = op->GetDataInstance(); if(!oData) return true;
		
	BaseDocument *doc = node->GetDocument(); 
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			oData->SetBool(T_SET,false);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			oData->SetString(T_STR,cdjnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			oData->SetBool(T_SET,false);
			CHAR snData[CDJS_SERIAL_SIZE];
			String cdjnr;
			
			CDReadPluginInfo(ID_CDJOINTSANDSKIN,snData,CDJS_SERIAL_SIZE);
			cdjnr.SetCString(snData,CDJS_SERIAL_SIZE-1);
			oData->SetString(T_STR,cdjnr);
			break;
		}
		
	}
	if(!doc) return true;
	
	CheckOpReg(oData);
	
	if(oData->GetBool(JNT_LOCAL_MSET)) execute = true;
	
	Vector theScale;
	Matrix opM = op->GetMg(), prM = Matrix(), chM, targM;
	switch (type)
	{
		case MSG_MOVE_FINISHED:
        {
            BaseObject *caOp = GetCharacterObjectParent(op);
            if(caOp && IsAdjustMode(caOp))
            {
                BaseObject *pr = op->GetUp();
                if(pr && pr->GetType() == ID_CDJOINTOBJECT)
                {
                    if(pr->GetDown() == op)
                    {
                        DescriptionCommand dc;
                        dc.id = DescID(JNT_ORIENT_JOINT);
                        pr->Message(MSG_DESCRIPTION_COMMAND,&dc);
					}
                }
            }
            break;
        }
		case MSG_DESCRIPTION_CHECKUPDATE:
		{
			DescriptionCheckUpdate *dch = (DescriptionCheckUpdate*) data;
			DescID descID = *(dch->descid);
			if(descID[0].id  == JNT_TRANS_POS)
			{
				oData->SetBool(JNT_PARAMETER_CHANGE,true);
				if(oData->GetBool(JNT_ADD_KEY)) oData->SetBool(JNT_ADD_KEY,false);
			}
			else if(descID[0].id  == JNT_TRANS_SCA)
			{
				oData->SetBool(JNT_PARAMETER_CHANGE,true);
				if(oData->GetBool(JNT_ADD_KEY)) oData->SetBool(JNT_ADD_KEY,false);
			}
			else if(descID[0].id  == JNT_TRANS_ROT)
			{
				oData->SetBool(JNT_PARAMETER_CHANGE,true);
				if(oData->GetBool(JNT_ADD_KEY)) oData->SetBool(JNT_ADD_KEY,false);
				
				Vector oldRot = oData->GetVector(JNT_OLD_ROT);
				Vector newRot = CDGetRot(op);
				Vector opRot = CDGetOptimalAngle(oldRot,newRot,op);
				CDSetRot(op,opRot);
				oData->SetVector(JNT_OLD_ROT,opRot);
			}
			
			break;
		}
		case CD_MSG_UPDATE:
		{
			if(IsCommandEnabled(IDM_AUTOKEYS) && CDIsCommandChecked(IDM_AUTOKEYS))
			{
				if(CDIsCommandChecked(IDM_A_PARAMETER))
				{
					if(oData->GetBool(JNT_ADD_KEY))
					{
						AutoAlloc<AliasTrans> trans;
						BaseObject *opClone = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_0,trans);
						if(opClone)
						{
							BaseContainer *ocData = opClone->GetDataInstance();
							if(ocData)
							{
								ocData->SetVector(JNT_TRANS_POS,oData->GetVector(JNT_AUTO_POS));
								ocData->SetVector(JNT_TRANS_SCA,oData->GetVector(JNT_AUTO_SCA));
								ocData->SetVector(JNT_TRANS_ROT,oData->GetVector(JNT_AUTO_ROT));
								
								AutoKey(doc,opClone,ocData,op,oData);
							}
							BaseObject::Free(opClone);
						}
					}
				}
			}
			
			oData->SetVector(JNT_AUTO_POS,oData->GetVector(JNT_TRANS_POS));
			oData->SetVector(JNT_AUTO_SCA,oData->GetVector(JNT_TRANS_SCA));
			oData->SetVector(JNT_AUTO_ROT,oData->GetVector(JNT_TRANS_ROT));
			
			oData->SetBool(JNT_PARAMETER_CHANGE,false);
			oData->SetBool(JNT_ADD_KEY,false);
			
			Vector oldRot = oData->GetVector(JNT_OLD_ROT);
			Vector newRot = CDGetRot(op);
			Vector opRot = CDGetOptimalAngle(oldRot,newRot,op);
			CDSetRot(op,opRot);
			oData->SetVector(JNT_OLD_ROT,opRot);
			
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==JNT_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==JNT_ORIENT_JOINT)
			{
				if(oData->GetBool(T_REG))
				{
					doc->StartUndo();
					
					ch = op->GetDown();
					if(ch)
					{
						chM = ch->GetMg();
						if(VEqual(opM.off,chM.off,0.001))
						{
							pred = ch;
							ch = ch->GetDown();
							if(ch) chM = ch->GetMg();
							while(ch && VEqual(opM.off,chM.off,0.001))
							{
								pred = ch;
								ch = ch->GetDown();
								if(ch) chM = ch->GetMg();
							}
							ch = pred;
						}
						CDAddUndo(doc,CD_UNDO_CHANGE,op);
						CDAddUndo(doc,CD_UNDO_CHANGE,ch);

						Vector chPos = chM.off;
						opM.v3 = VNorm(chPos - opM.off);
						opM.v1 = VNorm(VCross(opM.v2, opM.v3));
						opM.v2 = VNorm(VCross(opM.v3, opM.v1));
						theScale = CDGetScale(op);
						op->SetMg(opM);
						CDSetScale(op,theScale);
						
						theScale = CDGetScale(ch);
						chM.off = chPos;
						ch->SetMg(chM);
						CDSetScale(ch,theScale);
					}
					doc->EndUndo();
				}
			}
			else if (dc->id[0].id==JNT_TOOL_ORIENT_JOINT)
			{
				if(oData->GetBool(T_REG))
				{
					ch = op->GetDown();
					if (ch)
					{
						AutoAlloc<AtomArray> childList; if (!childList) return false;
						while(ch)
						{
							childList->Append(ch);
							ch = ch->GetNext();
						}
						LONG i, chCnt = childList->GetCount();
						CDArray<Matrix> chMatrices;
						chMatrices.Alloc(chCnt);
						
						for(i=0; i<chCnt; i++)
						{
							chAtm = static_cast<BaseObject*>(childList->GetIndex(i));
							chMatrices[i] = chAtm->GetMg();
						}
						
						ch = op->GetDown();
						if(ch->GetType() != ID_CDJOINTOBJECT)
						{
							Bool childJointFound = false;
							for(i=1; i<chCnt; i++)
							{
								if(!childJointFound)
								{
									ch = ch->GetNext();
									if(ch && ch->GetType() == ID_CDJOINTOBJECT) childJointFound = true;
								}
							}
						}
						if(ch->GetType() == ID_CDJOINTOBJECT)
						{
							chM = ch->GetMg();
							if(VEqual(opM.off,chM.off,0.001))
							{
								pred = ch;
								ch = ch->GetDown();
								if(ch) chM = ch->GetMg();
								while(ch && VEqual(opM.off,chM.off,0.001))
								{
									pred = ch;
									ch = ch->GetDown();
									if(ch) chM = ch->GetMg();
								}
								ch = pred;
							}

							BaseContainer *cData = ch->GetDataInstance();
							if(cData->GetBool(JNT_CONNECTED))
							{
								CDAddUndo(doc,CD_UNDO_CHANGE,op);
								CDAddUndo(doc,CD_UNDO_CHANGE,ch);

								Vector endDir = VNorm(chM.off - opM.off);
								targM.off = opM.off;
								targM.v3 = endDir;
								
								BaseObject *pr = op->GetUp();
								if(pr && oData->GetBool(JNT_CONNECTED))
								{
									Vector toParent = VNorm(pr->GetMg().off - opM.off);
									Real upAngle = VDot(endDir,toParent);
									if(upAngle>-0.9999 && upAngle<0.9999)
									{
										Vector upV = VNorm(VCross(endDir, toParent));
										Real v1Angle = VDot(VNorm(pr->GetMg().v1),upV);
										if(v1Angle > 0.707 || v1Angle < -0.707)
										{
											targM.v1 = VNorm(pr->GetMg().v1);
											targM.v2 = VNorm(VCross(targM.v3, targM.v1));
											targM.v1 = VNorm(VCross(targM.v2, targM.v3));
										}
										else
										{
											targM.v2 = VNorm(pr->GetMg().v2);
											targM.v1 = VNorm(VCross(targM.v2, targM.v3));
											targM.v2 = VNorm(VCross(targM.v3, targM.v1));
										}
									}
									else
									{
										targM.v2 = pr->GetMg().v2;
										targM.v1 = VNorm(VCross(targM.v2, targM.v3));
										targM.v2 = VNorm(VCross(targM.v3, targM.v1));
									}
									theScale = CDGetScale(op);
									op->SetMg(targM);
									CDSetScale(op,theScale);
									for(i=0; i<chCnt; i++)
									{
										chAtm = static_cast<BaseObject*>(childList->GetIndex(i));
										theScale = CDGetScale(chAtm);
										chAtm->SetMg(chMatrices[i]);
										CDSetScale(chAtm,theScale);
									}
									oData->SetMatrix(JNT_JOINT_MATRIX,targM);
								}
								else
								{
									Real upAngle = VDot(endDir,opM.v2);
									if(upAngle>-0.9999 && upAngle<0.9999)
									{
										targM.v1 = VNorm(VCross(opM.v2, targM.v3));
										targM.v2 = VNorm(VCross(targM.v3, targM.v1));
									}
									else
									{
										targM.v2 = VNorm(VCross(targM.v3, opM.v1));
										targM.v1 = VNorm(VCross(targM.v2, targM.v3));
									}
									theScale = CDGetScale(op);
									op->SetMg(targM);
									CDSetScale(op,theScale);
									for(i=0; i<chCnt; i++)
									{
										chAtm = static_cast<BaseObject*>(childList->GetIndex(i));
										theScale = CDGetScale(chAtm);
										chAtm->SetMg(chMatrices[i]);
										CDSetScale(chAtm,theScale);
									}
									oData->SetMatrix(JNT_JOINT_MATRIX,targM);
								}
							}
							else
							{
								BaseObject *pr = op->GetUp();
								if(pr && oData->GetBool(JNT_CONNECTED))
								{
									CDAddUndo(doc,CD_UNDO_CHANGE,op);
									prM = pr->GetMg();
									theScale = CDGetScale(op);
									op->SetMg(Matrix(opM.off,prM.v1,prM.v2,prM.v3));
									CDSetScale(op,theScale);
									
									//reset the children
									for(i=0; i<chCnt; i++)
									{
										chAtm = static_cast<BaseObject*>(childList->GetIndex(i));
										theScale = CDGetScale(chAtm);
										chAtm->SetMg(chMatrices[i]);
										CDSetScale(chAtm,theScale);
									}
									oData->SetMatrix(JNT_JOINT_MATRIX,targM);
								}
							}
						}
						chMatrices.Free();
					}
					else
					{
						BaseObject *pr = op->GetUp();
						if(pr && oData->GetBool(JNT_CONNECTED))
						{
							CDAddUndo(doc,CD_UNDO_CHANGE,op);
							prM = pr->GetMg();
							theScale = CDGetScale(op);
							op->SetMg(Matrix(opM.off,prM.v1,prM.v2,prM.v3));
							CDSetScale(op,theScale);
							oData->SetMatrix(JNT_JOINT_MATRIX,targM);
						}
					}
				}
			}
			else if (dc->id[0].id==JNT_ZERO_COORDS)
			{
				if(oData->GetBool(T_REG))
				{
					BaseObject *pr = op->GetUp();
					if(pr)
					{
						prM = pr->GetMg();
					}
					targM = MInv(prM) * opM;

					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					oData->SetMatrix(JNT_LOCAL_MATRIX,targM);
					oData->SetBool(JNT_LOCAL_MSET,true);
					oData->SetVector(JNT_TRANS_POS,Vector(0,0,0));
					oData->SetVector(JNT_POS_VECTOR,Vector(0,0,0));
					
					oData->SetVector(JNT_TRANS_SCA,Vector(1,1,1));
					oData->SetVector(JNT_SCA_VECTOR,Vector(1,1,1));
					
					oData->SetVector(JNT_TRANS_ROT,Vector(0,0,0));
					oData->SetVector(JNT_ROT_VECTOR,Vector(0,0,0));
					oData->SetVector(JNT_OLD_TRANS_ROT,Vector(0,0,0));
					oData->SetVector(JNT_OLD_ROT,CDGetRot(op));
					oData->SetVector(JNT_SET_ROT,CDGetRot(op));
				}
			}
			else if(dc->id[0].id==JNT_RETURN_TO_ZERO)
			{
				if(oData->GetBool(T_REG))
				{
					CDAddUndo(doc,CD_UNDO_CHANGE,op);
					oData->SetVector(JNT_TRANS_POS,Vector(0,0,0));
					oData->SetVector(JNT_TRANS_SCA,Vector(1,1,1));
					oData->SetVector(JNT_TRANS_ROT,Vector(0,0,0));
					oData->SetVector(JNT_OLD_TRANS_ROT,Vector(0,0,0));
					oData->SetVector(JNT_OLD_ROT,oData->GetVector(JNT_SET_ROT));
					CDSetRot(op,oData->GetVector(JNT_SET_ROT));
				}
			}
			break;
		}
	}
	
	return true;
}

void CDJoint::CDGetDimension(BaseObject *op, Vector *mp, Vector *rad)
{
	Real jDspSize = 1.0;
	BaseContainer *wpData = GetWorldPluginData(ID_CDJOINTDISPLAYSIZE);
	if(wpData) jDspSize = wpData->GetReal(IDC_JOINT_SIZE);

	BaseContainer *oData = op->GetDataInstance(), *cData;
	Real opLen, opRad = oData->GetReal(JNT_JOINT_RADIUS) * jDspSize, childRad;

	*mp = Vector(0.0,0.0,0.0);
	*rad = Vector(opRad,opRad,opRad);
	BaseObject *ch = op->GetDown();
	if(ch && ch->GetType() == ID_CDJOINTOBJECT)
	{
		cData = ch->GetDataInstance();
		if(cData->GetBool(JNT_CONNECTED))
		{
			childRad = cData->GetReal(JNT_JOINT_RADIUS) * jDspSize;
			Vector cDir = VNorm(ch->GetMg().off - op->GetMg().off);
			opLen = Len(ch->GetMl().off);
			
			if(oData->GetBool(JNT_SHOW_PROXY) && !oData->GetBool(JNT_SHOW_ENVELOPE))
			{
				*mp = Vector(0,0,opLen/2);
				Vector rOff = oData->GetVector(JNT_ROOT_OFFSET);
				*rad = Vector(rOff.x,rOff.y,opLen/2);
			}
			else
			{
				*mp = Vector(0,0,(opRad+opLen-childRad)/2-opRad);
				*rad = Vector(opRad,opRad,(opRad+opLen-childRad)/2);
			} 
		}
	}
}

Bool CDJoint::CDAddToExecution(BaseObject* op, PriorityList* list)
{
	LONG pMode = CYCLE_EXPRESSION, pVal = 0;
	
	GeData d;
	if(CDGetParameter(op,DescLevel(JNT_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd)
		{
			pMode = pd->GetPriorityValue(PRIORITYVALUE_MODE).GetLong();
			pVal = pd->GetPriorityValue(PRIORITYVALUE_PRIORITY).GetLong();
		}
	}
	
	xPrty = CD_EXECUTION_PRIORITY_EXPRESSION;
	switch(pMode)
	{
		case CYCLE_INITIAL:
			xPrty = CD_EXECUTION_PRIORITY_INITIAL;
			break;
		case CYCLE_ANIMATION:
			xPrty = CD_EXECUTION_PRIORITY_ANIMATION;
			break;
		case CYCLE_EXPRESSION:
			xPrty = CD_EXECUTION_PRIORITY_EXPRESSION;
			break;
		case CYCLE_DYNAMICS:
			xPrty = CD_EXECUTION_PRIORITY_DYNAMICS;
			break;
		case CYCLE_GENERATORS:
			xPrty = CD_EXECUTION_PRIORITY_GENERATOR;
			break;
	}
	CDPriorityListAdd(list, op, xPrty, CD_EXECUTION_0);
	if(pVal != xPrty) CDPriorityListAdd(list, op, xPrty + pVal, CD_EXECUTION_0);
	
	return true;
}

LONG CDJoint::CDExecute(BaseObject* op, BaseDocument* doc, BaseThread* bt, LONG priority, LONG flags)
{	
	BaseContainer *oData = op->GetDataInstance();
	
	if(!oData->GetBool(T_REG))
	{
		oData->SetBool(JNT_CONNECTED,oData->GetBool(JNT_CONNECTED+T_LST));
		oData->SetBool(JNT_SHOW_LOCAL_AXIS,oData->GetBool(JNT_SHOW_LOCAL_AXIS+T_LST));
		oData->SetReal(JNT_JOINT_RADIUS,oData->GetReal(JNT_JOINT_RADIUS+T_LST));

		oData->SetBool(JNT_LOCK_ZERO_TRANS,oData->GetBool(JNT_LOCK_ZERO_TRANS+T_LST));

		oData->SetBool(JNT_SHOW_ENVELOPE,oData->GetBool(JNT_SHOW_ENVELOPE+T_LST));
		oData->SetBool(JNT_SHOW_PROXY,oData->GetBool(JNT_SHOW_PROXY+T_LST));
		oData->SetVector(JNT_ROOT_OFFSET,oData->GetVector(JNT_ROOT_OFFSET+T_LST));
		oData->SetVector(JNT_TIP_OFFSET,oData->GetVector(JNT_TIP_OFFSET+T_LST));
		oData->SetReal(JNT_SKEW_RX,oData->GetReal(JNT_SKEW_RX+T_LST));
		oData->SetReal(JNT_SKEW_RY,oData->GetReal(JNT_SKEW_RY+T_LST));
		oData->SetReal(JNT_SKEW_TX,oData->GetReal(JNT_SKEW_TX+T_LST));
		oData->SetReal(JNT_SKEW_TY,oData->GetReal(JNT_SKEW_TY+T_LST));
	}
	else oData->SetBool(T_SET,false);
	
	Matrix opM = op->GetMg(), transM = Matrix(), targM, prM = Matrix();
	Vector newTRot, localTRot, theScale, theColor, oldTRot = oData->GetVector(JNT_OLD_TRANS_ROT);
	Vector nRot, oRot = oData->GetVector(JNT_OLD_ROT);
	Bool opT = false;
	
	if(!op->GetDown())
	{
		oData->SetBool(JNT_SHOW_ENVELOPE,false);
		oData->SetBool(JNT_SHOW_PROXY,false);
	}

	BaseObject *pr = op->GetUp();
	if(pr)
	{
		if(pr->GetType() == ID_CDJOINTOBJECT)
		{
			BaseContainer *prData = pr->GetDataInstance();
			if(prData) oData->SetBool(JNT_SHOW_XRAY,prData->GetBool(JNT_SHOW_XRAY));
		}
		else oData->SetBool(JNT_CONNECTED,false);
		prM = pr->GetMg();
		transM = prM;
	}
	if(oData->GetBool(JNT_LOCAL_MSET))
	{
		transM = prM * oData->GetMatrix(JNT_LOCAL_MATRIX);
	}
	targM = MInv(transM) * opM;
	
	//Check if changed
	if(!VEqual(oData->GetVector(JNT_TRANS_POS),oData->GetVector(JNT_POS_VECTOR),0.001)) opT = true;
	if(!VEqual(oData->GetVector(JNT_TRANS_SCA),oData->GetVector(JNT_SCA_VECTOR),0.001)) opT = true;
	if(!VEqual(oData->GetVector(JNT_TRANS_ROT),oData->GetVector(JNT_ROT_VECTOR),0.001)) opT = true;

	if(opT)
	{
		theScale = CDGetScale(op);
		newTRot = oData->GetVector(JNT_TRANS_ROT);
		targM = CDHPBToMatrix(newTRot, op);
		
		targM.off = oData->GetVector(JNT_TRANS_POS);
		opM = transM * targM;
		op->SetMg(opM);
		
		nRot = CDGetRot(op);
		Vector optRot = CDGetOptimalAngle(oRot,nRot,op);
		CDSetRot(op,optRot);
		
		CDSetScale(op,theScale);
		theScale = oData->GetVector(JNT_TRANS_SCA);
		CDSetScale(op,theScale);
	}
	
	LONG curFrame = doc->GetTime().GetFrame(doc->GetFps());
	LONG prvFrame = oData->GetLong(JNT_PREV_FRAME);
	
	//Set the Local position readout
	oData->SetVector(JNT_TRANS_POS,targM.off);
	
	//Set the Local scale readout
	theScale.x = Len(targM.v1);
	theScale.y = Len(targM.v2);
	theScale.z = Len(targM.v3);
	oData->SetVector(JNT_TRANS_SCA,theScale);
	
	//Set the Local rotation readout
	newTRot = CDMatrixToHPB(targM, op);
	localTRot = CDGetOptimalAngle(oldTRot, newTRot, op);
	
	oData->SetVector(JNT_TRANS_ROT,localTRot);
	
	// check for auto key
	if(priority < xPrty + 500)
	{
		Bool addKey = false;
		if(curFrame == prvFrame && op->GetBit(BIT_ACTIVE))
		{
			BaseList2D *bsl = doc->GetUndoPtr();
			if(bsl)
			{
				if(bsl->IsInstanceOf(Obase))
				{
					if(CDIsDirty(op,CD_DIRTY_MATRIX) && !oData->GetBool(JNT_PARAMETER_CHANGE))
					{
						if(!VEqual(oData->GetVector(JNT_TRANS_POS),oData->GetVector(JNT_AUTO_POS),0.001))
						{
							addKey = true;
						}
						if(!VEqual(oData->GetVector(JNT_TRANS_SCA),oData->GetVector(JNT_AUTO_SCA),0.001))
						{
							addKey = true;
						}
						if(!VEqual(oData->GetVector(JNT_TRANS_ROT),oData->GetVector(JNT_AUTO_ROT),0.001))
						{
							addKey = true;
						}
					}
				}
			}
		}
		oData->SetBool(JNT_ADD_KEY,addKey);
	}
	oData->SetVector(JNT_POS_VECTOR,targM.off);
	oData->SetVector(JNT_SCA_VECTOR,theScale);
	oData->SetVector(JNT_ROT_VECTOR,localTRot);
	
	oData->SetVector(JNT_OLD_TRANS_ROT,localTRot);
	oData->SetLong(JNT_PREV_FRAME,curFrame);
	
	oData->SetBool(JNT_HIGHLITE,false);
	
	if(op->GetBit(BIT_ACTIVE)) oData->SetBool(JNT_HIGHLITE,true);
	
	Bool highlight = oData->GetBool(JNT_HIGHLITE);
	while(pr && !highlight)
	{
		if(pr->GetType() == ID_CDJOINTOBJECT)
		{
			BaseContainer *prData = pr->GetDataInstance();
			if(prData)
			{
				if(prData->GetBool(JNT_HIGHLITE))
				{
					highlight = true;
					oData->SetBool(JNT_HIGHLITE,true);
				}
			}
		}
		else
		{
			if(pr->GetBit(BIT_ACTIVE))
			{
				highlight = true;
				oData->SetBool(JNT_HIGHLITE,true);
			}
		}
		pr = pr->GetUp();
	}
	
	return CD_EXECUTION_RESULT_OK;
}


Vector CDJoint::GetRTHandle(BaseObject *op, LONG id)
{
	BaseContainer *data = op->GetDataInstance();

	if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
	{
		if(data->GetBool(JNT_SHOW_ENVELOPE) || data->GetBool(JNT_SHOW_PROXY))
		{
			Matrix tipM, rootM, skRM, skTM, opM = op->GetMg();
			rootM = opM;
			tipM = rootM;
			
			BaseObject *ch = op->GetDown();
			Vector opPos = op->GetMg().off, cPos = ch->GetMg().off;
			Real bnLen = Len(cPos - opPos);
			tipM.off = cPos;
			
			Vector rSk, tSk, rOff=data->GetVector(JNT_ROOT_OFFSET), tOff=data->GetVector(JNT_TIP_OFFSET);
			rSk.x = data->GetReal(JNT_SKEW_RX);
			rSk.y = data->GetReal(JNT_SKEW_RY);
			tSk.x = data->GetReal(JNT_SKEW_TX);
			tSk.y = data->GetReal(JNT_SKEW_TY);
			// Calculate the root skew position
			Vector skrPos = (rootM * Vector(rSk.x,rSk.y,0)) - cPos;
			rootM.off = cPos + VNorm(skrPos) * (bnLen - rOff.z);
			// Calculate the tip skew position
			Vector sktPos = (tipM * Vector(tSk.x,tSk.y,bnLen)) - opPos;
			tipM.off = opPos + VNorm(sktPos) * (bnLen + tOff.z);
			
			Vector theScale = GetGlobalScale(op);
			rootM.v3 = VNorm(tipM.off - rootM.off) * theScale.z;
			rootM.v2 = VNorm(VCross(rootM.v3, rootM.v1)) * theScale.y;
			rootM.v1 = VNorm(VCross(rootM.v2, rootM.v3)) * theScale.x;
			tipM.v1 = rootM.v1;
			tipM.v2 = rootM.v2;
			tipM.v3 = rootM.v3;
			
			skRM = MInv(opM) * rootM;
			skTM = MInv(opM) * tipM;

			Vector rOffset = data->GetVector(JNT_ROOT_OFFSET);
			Vector tOffset = data->GetVector(JNT_TIP_OFFSET);

			switch (id)
			{
				case 0: return skRM * Vector(rOffset.x,0.0,0.0); break;
				case 1: return skRM * Vector(0.0,rOffset.y,0.0); break;
				case 2: return skRM * Vector(0.0,0.0,0.0); break;
				case 3: return skTM * Vector(tOffset.x,0.0,0.0); break;
				case 4: return skTM * Vector(0.0,tOffset.y,0.0); break;
				case 5: return skTM * Vector(0.0,0.0,0.0); break;
			}
		}
	}

	return Vector(0.0,0.0,0.0);
}

Bool CDJoint::CDDraw(BaseObject *op, LONG drawpass, BaseDraw *bd, BaseDrawHelp *bh)
{
	Real jDspSize = 1.0;
	BaseContainer *wpData = GetWorldPluginData(ID_CDJOINTDISPLAYSIZE);
	if(wpData) jDspSize = wpData->GetReal(IDC_JOINT_SIZE);
	
	GeData dispSData = bd->GetParameterData(BASEDRAW_DATA_SDISPLAYACTIVE);
	BaseContainer *data = op->GetDataInstance();
	ObjectColorProperties prop;
	BaseObject *ch = NULL;
	
	Matrix chNewMatrix, opNewMatrix;
	bd->SetMatrix_Matrix(op, opNewMatrix);

	if(drawpass == CD_DRAWPASS_OBJECT)
	{ 
		CDSetBaseDrawMatrix(bd,NULL,Matrix());
		
		Real   rad = data->GetReal(JNT_JOINT_RADIUS) * jDspSize;
		Vector h, p1, p2, p3, p4, endDir, theColor;
		Matrix opM = op->GetMg(), chM, bM, jM = bh->GetMg(), scaleM, targetM;

		if(data->GetBool(JNT_SHOW_LOCAL_AXIS))
		{
			bd->SetPen(Vector(1,0,0));
			CDDrawLine(bd,opM.off,opM.off + VNorm(opM.v1)*(rad +3));
			bd->SetPen(Vector(0,1,0));
			CDDrawLine(bd,opM.off,opM.off + VNorm(opM.v2)*(rad +3));
			bd->SetPen(Vector(0,0,1));
			CDDrawLine(bd,opM.off,opM.off + VNorm(opM.v3)*(rad +3));
		}
		ch = op->GetDown();
		while(ch)
		{
			if(ch->GetType() == ID_CDJOINTOBJECT)
			{
				BaseContainer *cData = ch->GetDataInstance();
				bd->SetMatrix_Matrix(ch, chNewMatrix);
				if(cData->GetBool(JNT_CONNECTED))
				{
					op->GetColorProperties(&prop);
					if(prop.usecolor > 0)  theColor = prop.color;
					else theColor = Vector(0,0,0.75);
					bd->SetPen(theColor);
					
					if(data->GetBool(JNT_HIGHLITE) && op->GetBit(BIT_ACTIVE)) bd->SetPen(colorH);
					else if(data->GetBool(JNT_HIGHLITE)) bd->SetPen(colorCH);
					
					Real opRad = data->GetReal(JNT_JOINT_RADIUS) * jDspSize;
					chM = ch->GetMg();
					endDir = VNorm(chM.off - opM.off);
					if(opM.off != chM.off)
					{
						bM.off = opM.off + endDir * opRad;
						bM.v3 = endDir;
						Real upAngle = VDot(endDir,opM.v2);
						if(upAngle>-0.9999 && upAngle<0.9999)
						{
							bM.v1 = VNorm(VCross(opM.v2, bM.v3));
							bM.v2 = VNorm(VCross(bM.v3, bM.v1));
						}
						else
						{
							bM.v2 = VNorm(VCross(bM.v3, opM.v1));
							bM.v1 = VNorm(VCross(bM.v2, bM.v3));
						}
						
						Matrix afM = GetAffineMatrix(opM);
						p1 = opM * MInv(afM) * bM * Vector(opRad,0.0,0.0);
						p2 = opM * MInv(afM) * bM * Vector(0.0,opRad,0.0);
						p3 = opM * MInv(afM) * bM * Vector(-opRad,0.0,0.0);
						p4 = opM * MInv(afM) * bM * Vector(0.0,-opRad,0.0);
						
						if(!data->GetBool(JNT_SHOW_PROXY) || data->GetBool(JNT_SHOW_ENVELOPE))
						{
							CDDrawLine(bd,p1,p2);
							CDDrawLine(bd,p2,p3);
							CDDrawLine(bd,p3,p4);
							CDDrawLine(bd,p4,p1);
							
							Vector tipPos = chM * ((VNorm(MInv(chM) * opM.off)) * (cData->GetReal(JNT_JOINT_RADIUS) * jDspSize));// 
							CDDrawLine(bd,tipPos,p1);
							CDDrawLine(bd,tipPos,p2);
							CDDrawLine(bd,tipPos,p3);
							CDDrawLine(bd,tipPos,p4);
							
						}
					}
				}
			}
			ch = ch->GetNext();
		}
		
		op->GetColorProperties(&prop);
		if(prop.usecolor > 0)  theColor = prop.color;
		else theColor = Vector(0,0,0.75);
		bd->SetPen(theColor);
		
		if(data->GetBool(JNT_HIGHLITE) && op->GetBit(BIT_ACTIVE)) bd->SetPen(colorH);
		else if(data->GetBool(JNT_HIGHLITE)) bd->SetPen(colorCH);
		
		jM.v1*=rad;
		jM.v2*=rad;
		jM.v3*=rad;
		
		ch = op->GetDown();
		if(!data->GetBool(JNT_SHOW_PROXY) || data->GetBool(JNT_SHOW_ENVELOPE))
		{
			if(!op->GetDown())
			{
				if(!op->GetUp())
				{
					CDDrawCircle(bd,jM);
					h=jM.v2; jM.v2=jM.v3; jM.v3=h;
					CDDrawCircle(bd,jM);
					h=jM.v1; jM.v1=jM.v3; jM.v3=h;
					CDDrawCircle(bd,jM);
				}
				else
				{
					BaseObject *pr = op->GetUp();
					BaseContainer *prData = pr->GetDataInstance();
					if(prData)
					{
						if(!prData->GetBool(JNT_SHOW_PROXY) || prData->GetBool(JNT_SHOW_ENVELOPE))
						{
							CDDrawCircle(bd,jM);
							h=jM.v2; jM.v2=jM.v3; jM.v3=h;
							CDDrawCircle(bd,jM);
							h=jM.v1; jM.v1=jM.v3; jM.v3=h;
							CDDrawCircle(bd,jM);
						}
					}
				}
			}
			else
			{
				CDDrawCircle(bd,jM);
				h=jM.v2; jM.v2=jM.v3; jM.v3=h;
				CDDrawCircle(bd,jM);
				h=jM.v1; jM.v1=jM.v3; jM.v3=h;
				CDDrawCircle(bd,jM);
			}
		}

		if(ch && ch->GetType() == ID_CDJOINTOBJECT)
		{
			Matrix rootM = op->GetMg();
			LONG i;
			
			BaseObject *theChild = op->GetDown();
			Matrix tipM = rootM;
			Vector opPos = op->GetMg().off, cPos = theChild->GetMg().off;
			Real bnLen = Len(cPos - opPos);
			tipM.off = cPos;
			
			Vector rSk, tSk, rOff=data->GetVector(JNT_ROOT_OFFSET), tOff=data->GetVector(JNT_TIP_OFFSET);
			rSk.x = data->GetReal(JNT_SKEW_RX);
			rSk.y = data->GetReal(JNT_SKEW_RY);
			tSk.x = data->GetReal(JNT_SKEW_TX);
			tSk.y = data->GetReal(JNT_SKEW_TY);
			// Calculate the root skew position
			Vector skrPos = (rootM * Vector(rSk.x,rSk.y,0)) - cPos;
			rootM.off = cPos + VNorm(skrPos) * (bnLen - rOff.z);
			// Calculate the tip skew position
			Vector sktPos = (tipM * Vector(tSk.x,tSk.y,bnLen)) - opPos;
			tipM.off = opPos + VNorm(sktPos) * (bnLen + tOff.z);
			
			rootM.v3 = VNorm(tipM.off - rootM.off);
			rootM.v2 = VNorm(VCross(rootM.v3, rootM.v1));
			rootM.v1 = VNorm(VCross(rootM.v2, rootM.v3));
			tipM.v1 = rootM.v1;
			tipM.v2 = rootM.v2;
			tipM.v3 = rootM.v3;
			rootM.v1 *= rOff.x;
			rootM.v2 *= rOff.y;
			tipM.v1 *= tOff.x;
			tipM.v2 *= tOff.y;
			
			if(data->GetBool(JNT_SHOW_ENVELOPE))
			{
				
				Vector ePts[9] = {
						Vector(1,0,0),
						Vector(0.707,0.707,0),
						Vector(0,1,0),
						Vector(-0.707,0.707,0),
						Vector(-1,0,0),
						Vector(-0.707,-0.707,0),
						Vector(0,-1,0),
						Vector(0.707,-0.707,0),
						Vector(1,0,0),
						};
						
				if(data->GetBool(JNT_SHOW_PROXY) && dispSData != BASEDRAW_SDISPLAY_NOSHADING)
				{
					Vector vL, vR, vN;
					Real lig;
					
					for(i=0; i<8; i++)
					{
						p1 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i]);
						p2 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i]);
						p3 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i+1]);
						p4 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i+1]);
						vL = VNorm(p4 - p1);
						vR = VNorm(p2 - p1);
						vN = VNorm(VCross(vL, vR));
						lig = bd->SimpleShade(p1,vN);
						Vector p[4] = { p1,p2,p3,p4 };
						Vector f[4] = { theColor*lig, theColor*lig, theColor*lig,  theColor*lig };
						CDDrawPolygon(bd,p, f, true);
					}
				}
				else
				{
					CDDrawCircle(bd,rootM);
					CDDrawCircle(bd,tipM);
					
					// draw the lines between the ends
					for(i=0; i<8; i++)
					{
						p1 = rootM * ePts[i];
						p2 = tipM * ePts[i];
						CDDrawLine(bd,p1,p2);
					}
				}
			}
			else
			{
				if(data->GetBool(JNT_SHOW_PROXY))
				{
					Vector ePts[5] = {
							Vector(1,1,0),
							Vector(-1,1,0),
							Vector(-1,-1,0),
							Vector(1,-1,0),
							Vector(1,1,0),
							};
					
					if(dispSData != BASEDRAW_SDISPLAY_NOSHADING)
					{
						Vector vL, vR, vN;
						Real lig;
						
						//Draw sides
						for(i=0; i<4; i++)
						{
							p1 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i]);
							p2 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i]);
							p3 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i+1]);
							p4 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i+1]);
							vL = VNorm(p4 - p1);
							vR = VNorm(p2 - p1);
							vN = VNorm(VCross(vL, vR));
							lig = bd->SimpleShade(p1,vN);
							Vector p[4] = { p1,p2,p3,p4 };
							Vector f[4] = { theColor*lig, theColor*lig, theColor*lig,  theColor*lig };
							CDDrawPolygon(bd,p, f, true);
						}
						
						//Draw top
						p1 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[0]);
						p2 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[1]);
						p3 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[2]);
						p4 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[3]);
						vL = VNorm(p4 - p1);
						vR = VNorm(p2 - p1);
						vN = VNorm(VCross(vL, vR));
						lig = bd->SimpleShade(p1,vN);
						Vector tp[4] = { p1,p2,p3,p4 };
						Vector tf[4] = { theColor*lig, theColor*lig, theColor*lig,  theColor*lig };
						CDDrawPolygon(bd,tp, tf, true);
						
						//Draw bottom
						p1 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[3]);
						p2 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[2]);
						p3 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[1]);
						p4 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[0]);
						vL = VNorm(p4 - p1);
						vR = VNorm(p2 - p1);
						vN = VNorm(VCross(vL, vR));
						lig = bd->SimpleShade(p1,vN);
						Vector bp[4] = { p1,p2,p3,p4 };
						Vector bf[4] = { theColor*lig, theColor*lig, theColor*lig,  theColor*lig };
						CDDrawPolygon(bd,bp, bf, true);
					}
					else
					{
						//Draw sides
						for(i=0; i<4; i++)
						{
							p1 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i]);
							p2 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i]);
							CDDrawLine(bd,p1,p2);
						}
						
						//Draw top
						for(i=0; i<4; i++)
						{
							p1 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i]);
							p2 = rootM * (MatrixScale(GetGlobalScale(op)) * ePts[i+1]);
							CDDrawLine(bd,p1,p2);
						}
						
						//Draw bottom
						for(i=0; i<4; i++)
						{
							p1 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i]);
							p2 = tipM * (MatrixScale(GetGlobalScale(ch)) * ePts[i+1]);
							CDDrawLine(bd,p1,p2);
						}
					}
				}
			}
		}
	}
	else if(drawpass == CD_DRAWPASS_HANDLES)
	{
		if(data->GetBool(T_REG))
		{
			ch = op->GetDown();
			if(ch && ch->GetType() == ID_CDJOINTOBJECT)
			{
				if(data->GetBool(JNT_SHOW_ENVELOPE) || data->GetBool(JNT_SHOW_PROXY))
				{
					LONG   i;
					Matrix m=op->GetMg();
					Vector cPos = CDGetPos(op->GetDown());

					LONG   hitid = op->GetHighlightHandle(bd);

					for (i=0; i<HANDLE_CNT; i++)
					{
						if(i == hitid)  bd->SetPen(CDGetViewColor(CD_VIEWCOLOR_SELECTION_PREVIEW));
						else  bd->SetPen(CDGetViewColor(CD_VIEWCOLOR_ACTIVEPOINT));
						CDDrawHandle3D(bd, m * GetRTHandle(op,i), CD_DRAWHANDLE_BIG, 0);
					}
					bd->SetPen(CDGetViewColor(CD_VIEWCOLOR_ACTIVEPOINT));
					CDDrawLine(bd, m * GetRTHandle(op,2), m.off);
					CDDrawLine(bd, m * GetRTHandle(op,5), m.off + m.v3 * cPos.z);
				}
			}
		}
	}
	
	return CDSuperDrawReturn(op, drawpass, bd, bh);
}

LONG CDJoint::CDDetectHandle(BaseObject *op, BaseDraw *bd, LONG x, LONG y, LONG qualifier)
{
	BaseContainer *data = op->GetDataInstance();
	if(!data->GetBool(T_REG)) return NOTOK;

	if (qualifier&QUALIFIER_CTRL) return NOTOK;

	Matrix	mg = op->GetMg();
	LONG    i,ret=NOTOK;
	Vector	p;

	if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
	{
		if(data->GetBool(JNT_SHOW_ENVELOPE) || data->GetBool(JNT_SHOW_PROXY))
		{
			for (i=0; i<HANDLE_CNT; i++)
			{
				p = GetRTHandle(op,i);
				if (bd->PointInRange(mg * p, x, y))
				{
					ret=i;
					if (!(qualifier&QUALIFIER_SHIFT)) break;
				}
			}
		}
	}
		
	return ret;
}

Bool CDJoint::CDMoveHandle(BaseObject *op, BaseObject *undo, const Vector &mouse_pos, LONG hit_id, LONG qualifier, BaseDraw *bd)
{
	BaseContainer *src = undo->GetDataInstance();
	BaseContainer *dst = op->GetDataInstance();
	if(!dst->GetBool(T_REG)) return true;
	
	if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
	{
		if(dst->GetBool(JNT_SHOW_ENVELOPE) || dst->GetBool(JNT_SHOW_PROXY))
		{
			Vector handle_dir, shandle_pos, dhandle_pos;
			Vector cPos = CDGetPos(op->GetDown());
			
			switch (hit_id)
			{
				case 0: handle_dir = Vector(1.0,0.0,0.0); break;
				case 1: handle_dir = Vector(0.0,1.0,0.0); break;
				case 2: handle_dir = Vector(0.0,0.0,1.0); break;
				case 3: handle_dir = Vector(1.0,0.0,0.0); break;
				case 4: handle_dir = Vector(0.0,1.0,0.0); break;
				case 5: handle_dir = Vector(0.0,0.0,1.0); break;
			}

			Real val = VDot(mouse_pos, handle_dir);

			switch (hit_id)
			{
				case 0:
					{
					 	shandle_pos = src->GetVector(JNT_ROOT_OFFSET);
						val+=shandle_pos.x+val;
						FCut((LReal)val,(LReal)0.0,(LReal)MAXRANGE);
					 	dhandle_pos = dst->GetVector(JNT_ROOT_OFFSET);
					 	dhandle_pos.x = val;
						dst->SetVector(JNT_ROOT_OFFSET,dhandle_pos); 
					}
					break;
				case 1: 
					{
					 	shandle_pos = src->GetVector(JNT_ROOT_OFFSET);
						val+=shandle_pos.y+val;
						FCut((LReal)val,(LReal)0.0,(LReal)MAXRANGE);
					 	dhandle_pos = dst->GetVector(JNT_ROOT_OFFSET);
					 	dhandle_pos.y = val;
						dst->SetVector(JNT_ROOT_OFFSET,dhandle_pos); 
					}
					break;
				case 2: 
					{
					 	shandle_pos = src->GetVector(JNT_ROOT_OFFSET);
						val+=shandle_pos.z+val;
						FCut((LReal)val,(LReal)0.0,(LReal)MAXRANGE);
					 	dhandle_pos = dst->GetVector(JNT_ROOT_OFFSET);
					 	dhandle_pos.z = val;
						dst->SetVector(JNT_ROOT_OFFSET,dhandle_pos); 
					}
					break;
				case 3:
					{
					 	shandle_pos = src->GetVector(JNT_TIP_OFFSET);
						val+=shandle_pos.x+val;
						FCut((LReal)val,(LReal)0.0,(LReal)MAXRANGE);
					 	dhandle_pos = dst->GetVector(JNT_TIP_OFFSET);
					 	dhandle_pos.x = val;
						dst->SetVector(JNT_TIP_OFFSET,dhandle_pos); 
					}
					break;
				case 4: 
					{
					 	shandle_pos = src->GetVector(JNT_TIP_OFFSET);
						val+=shandle_pos.y+val;
						FCut((LReal)val,(LReal)0.0,(LReal)MAXRANGE);
					 	dhandle_pos = dst->GetVector(JNT_TIP_OFFSET);
					 	dhandle_pos.y = val;
						dst->SetVector(JNT_TIP_OFFSET,dhandle_pos); 
					}
					break;
				case 5:
					{
					 	shandle_pos = src->GetVector(JNT_TIP_OFFSET);
						val+=shandle_pos.z+val;
						FCut((LReal)val,(LReal)0.0,(LReal)MAXRANGE);
					 	dhandle_pos = dst->GetVector(JNT_TIP_OFFSET);
					 	dhandle_pos.z = val;
						dst->SetVector(JNT_TIP_OFFSET,dhandle_pos); 
					} 
					break;
			}
		}
	}
		
	return true;
}

Bool CDJoint::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseObject *op = (BaseObject*)node;
	BaseContainer *data = op->GetDataInstance();
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(909), ar);
	if(bc) bc->SetBool(DESC_HIDE, false);

	bc = description->GetParameterI(DescLevel(JNT_PURCHASE), ar);
	if(bc)
	{
		if(!data->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= DESCFLAGS_DESC_LOADED;

	return CDSuperGetDDescriptionReturn(node, description, flags);
}

Bool CDJoint::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseObject	*op  = (BaseObject*)node; if(!op) return false;
	BaseContainer *oData = op->GetDataInstance(); if(!oData) return false;
	
	switch (id[0].id)
	{
		case JNT_CONNECTED:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetUp() && op->GetUp()->GetType() == ID_CDJOINTOBJECT) return true;
			else return false;
		case JNT_SHOW_LOCAL_AXIS:
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case JNT_JOINT_RADIUS:
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case JNT_ORIENT_JOINT:
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case JNT_ZERO_COORDS:
			if(!oData->GetBool(T_REG)) return false;
			else if(oData->GetBool(JNT_LOCK_ZERO_TRANS)) return false;
			else return true;
		case JNT_RETURN_TO_ZERO:
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case JNT_LOCK_ZERO_TRANS:
			if(!oData->GetBool(T_REG)) return false;
			else return true;
		case JNT_SHOW_ENVELOPE:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT) return true;
			else return false;
		case JNT_SHOW_PROXY:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT) return true;
			else return false;
		case JNT_ROOT_OFFSET:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
			{
				if(oData->GetBool(JNT_SHOW_ENVELOPE) || oData->GetBool(JNT_SHOW_PROXY)) return true;
				else return false;
			}
			else return false;
		case JNT_TIP_OFFSET:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
			{
				if(oData->GetBool(JNT_SHOW_ENVELOPE) || oData->GetBool(JNT_SHOW_PROXY)) return true;
				else return false;
			}
			else return false;
		case JNT_SKEW_RX:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
			{
				if(oData->GetBool(JNT_SHOW_ENVELOPE) || oData->GetBool(JNT_SHOW_PROXY)) return true;
				else return false;
			}
			else return false;
		case JNT_SKEW_RY:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
			{
				if(oData->GetBool(JNT_SHOW_ENVELOPE) || oData->GetBool(JNT_SHOW_PROXY)) return true;
				else return false;
			}
			else return false;
		case JNT_SKEW_TX:	
			if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
			{
				if(oData->GetBool(JNT_SHOW_ENVELOPE) || oData->GetBool(JNT_SHOW_PROXY)) return true;
				else return false;
			}
			else return false;
		case JNT_SKEW_TY:	
			if(!oData->GetBool(T_REG)) return false;
			else if(op->GetDown() && op->GetDown()->GetType() == ID_CDJOINTOBJECT)
			{
				if(oData->GetBool(JNT_SHOW_ENVELOPE) || oData->GetBool(JNT_SHOW_PROXY)) return true;
				else return false;
			}
			else return false;
	}
	return true;
}

Bool RegisterCDJoint(void)
{
	if(CDFindPlugin(ID_CDJOINTOBJECT,CD_OBJECT_PLUGIN)) return true;
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDJOINTOBJECT); if (!name.Content()) return true;
	return CDRegisterObjectPlugin(ID_CDJOINTOBJECT,name,CD_OBJECT_CALL_ADDEXECUTION|PLUGINFLAG_HIDE,CDJoint::Alloc,"oCDJoint","CDJoint.tif","CDJoint_small.tif",0);
}
