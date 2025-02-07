//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDRConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	10

enum
{
	//RC_PURCHASE			= 1000,
	
	//RC_OFFSETS				= 1009,
	
	RC_TARGET_A_OLD			= 1010, // Old target link
	RC_TARGET_B_OLD			= 1011, // Old target link
	//RC_AB_MIX				= 1012,
		
	//RC_STRENGTH			= 1013,
	//RC_SHOW_LINES			= 1014,
	//RC_LINE_COLOR			= 1015,

	//RC_AXIS_X				= 1016,
	//RC_OFFSET_X			= 1017,
	//RC_AXIS_Y				= 1018,
	//RC_OFFSET_Y			= 1019,
	//RC_AXIS_Z				= 1020,
	//RC_OFFSET_Z			= 1021,
	
	//RC_LOCAL_ROT			= 1022,
	//RC_USE_AB_MIX			= 1023,
	
	RC_COUNT			= 1030,
	//RC_ADD_ROT				= 1031,
	//RC_SUB_ROT				= 1032,
	
	// Temporary reserve containter 1034 for fix in beta
	RC_DISK_LEVEL				= 1035,
	
	RC_TARGET_COUNT			= 1036,
	
	//RC_INTERPOLATION		= 1040,
		//RC_AVERAGE			= 1041,
		//RC_SHORTEST		= 1042,					
	
	//RC_ID_TARGET			= 1050,
	
	//RC_LINK_GROUP			= 2000,
	//RC_LINE_ADD			= 3000,

	//RC_TARGET				= 4000,
	
	//RC_ROT_MIX				= 5000,

	RC_TOGGLE					= 10000,
	RC_OLD_W					= 10100,
	RC_OLD_XYZ					= 10200,
	RC_OLD_HPB					= 10300,
	RC_LINK_USED				= 10400,

	RC_REST_ROTATION_SET		= 11000,
	RC_REST_ROTATION			= 11001
};

class CDRConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

	void ConvertOldData(BaseDocument *doc, BaseContainer *tData);
	LONG ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData);

	void SetDefaultTargetParameters(BaseContainer *tData, LONG n);
	void CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst);

public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDRConstraintPlugin); }
};

Bool CDRConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(RC_SHOW_LINES,true);
	tData->SetVector(RC_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(RC_STRENGTH,1.00);
	tData->SetReal(RC_AB_MIX,0.5);
	
	tData->SetBool(RC_AXIS_X,true);
	tData->SetReal(RC_OFFSET_X,0.0);
	tData->SetBool(RC_AXIS_Y,true);
	tData->SetReal(RC_OFFSET_Y,0.0);
	tData->SetBool(RC_AXIS_Z,true);
	tData->SetReal(RC_OFFSET_Z,0.0);
	
	tData->SetLong(RC_INTERPOLATION,RC_SHORTEST);

	tData->SetLong(RC_TARGET_COUNT,1);
	tData->SetReal(RC_ROT_MIX,1.00);
	tData->SetBool(RC_TOGGLE,false);
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if (CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if (pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDRConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetLong(RC_DISK_LEVEL,level);
	
	return true;
}

void CDRConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetBool(RC_LOCAL_ROT+T_LST,tData->GetBool(RC_LOCAL_ROT));
			
			tData->SetBool(RC_USE_AB_MIX+T_LST,tData->GetBool(RC_USE_AB_MIX));
			
			tData->SetBool(RC_AXIS_X+T_LST,tData->GetBool(RC_AXIS_X));
			tData->SetBool(RC_AXIS_Y+T_LST,tData->GetBool(RC_AXIS_Y));
			tData->SetBool(RC_AXIS_Z+T_LST,tData->GetBool(RC_AXIS_Z));
			tData->SetReal(RC_OFFSET_X+T_LST,tData->GetReal(RC_OFFSET_X));
			tData->SetReal(RC_OFFSET_Y+T_LST,tData->GetReal(RC_OFFSET_Y));
			tData->SetReal(RC_OFFSET_Z+T_LST,tData->GetReal(RC_OFFSET_Z));
			
			tData->SetLong(RC_INTERPOLATION+T_LST,tData->GetLong(RC_INTERPOLATION));
			tData->SetLong(RC_TARGET_COUNT+T_LST,tData->GetLong(RC_TARGET_COUNT));
			LONG i, rCnt = tData->GetLong(RC_TARGET_COUNT);
			for(i=0; i<rCnt; i++)
			{
				tData->SetLink(RC_TARGET+i+T_LST,tData->GetLink(RC_TARGET+i,doc));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDRConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetBool(RC_LOCAL_ROT,tData->GetBool(RC_LOCAL_ROT+T_LST));
		
		tData->SetBool(RC_USE_AB_MIX,tData->GetBool(RC_USE_AB_MIX+T_LST));
		
		tData->SetBool(RC_AXIS_X,tData->GetBool(RC_AXIS_X+T_LST));
		tData->SetBool(RC_AXIS_Y,tData->GetBool(RC_AXIS_Y+T_LST));
		tData->SetBool(RC_AXIS_Z,tData->GetBool(RC_AXIS_Z+T_LST));
		tData->SetReal(RC_OFFSET_X,tData->GetReal(RC_OFFSET_X+T_LST));
		tData->SetReal(RC_OFFSET_Y,tData->GetReal(RC_OFFSET_Y+T_LST));
		tData->SetReal(RC_OFFSET_Z,tData->GetReal(RC_OFFSET_Z+T_LST));
		
		tData->SetLong(RC_INTERPOLATION,tData->GetLong(RC_INTERPOLATION+T_LST));
		tData->SetLong(RC_TARGET_COUNT,tData->GetLong(RC_TARGET_COUNT+T_LST));
		LONG i, rCnt = tData->GetLong(RC_TARGET_COUNT);
		for(i=0; i<rCnt; i++)
		{
			tData->SetLink(RC_TARGET+i,tData->GetLink(RC_TARGET+i+T_LST,doc));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDRConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(RC_TARGET+i,NULL);
	tData->SetReal(RC_ROT_MIX+i,1.00);
}

void CDRConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(RC_TARGET+dst,tData->GetObjectLink(RC_TARGET+src,doc));
	tData->SetReal(RC_ROT_MIX+dst,tData->GetReal(RC_ROT_MIX+src));
}

void CDRConstraintPlugin::ConvertOldData(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, oldCnt = 0;
	
	BaseObject *goalA = tData->GetObjectLink(RC_TARGET_A_OLD,doc);
	if(goalA)
	{
		tData->SetLink(RC_TARGET,goalA);
		tData->SetReal(RC_ROT_MIX,1.00);
		oldCnt++;
		tData->SetLong(RC_TARGET_COUNT,oldCnt);
	}
	BaseObject *goalB = tData->GetObjectLink(RC_TARGET_B_OLD,doc);
	if(goalB)
	{
		tData->SetLink(RC_TARGET+1,goalB);
		tData->SetReal(RC_ROT_MIX,1.0-tData->GetReal(RC_AB_MIX));
		tData->SetReal(RC_ROT_MIX+1,tData->GetReal(RC_AB_MIX));
		tData->SetBool(RC_USE_AB_MIX,true);
		oldCnt++;
		tData->SetLong(RC_TARGET_COUNT,oldCnt);
	}
	tData->SetBool(RC_AXIS_X,true);
	tData->SetReal(RC_OFFSET_X,0.0);
	tData->SetBool(RC_AXIS_Y,true);
	tData->SetReal(RC_OFFSET_Y,0.0);
	tData->SetBool(RC_AXIS_Z,true);
	tData->SetReal(RC_OFFSET_Z,0.0);
	
	if(oldCnt == 0) tData->SetLong(RC_TARGET_COUNT,tData->GetLong(RC_COUNT) + 1);
	else
	{
		for(i=oldCnt; i<MAX_ADD; i++)
		{
			if(tData->GetObjectLink(RC_TARGET+i,doc)) oldCnt++;
		}
		tData->SetLong(RC_TARGET_COUNT,oldCnt);
	}
}

Bool CDRConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(RC_DISK_LEVEL) < 1) ConvertOldData(doc,tData);
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
			
			tData->SetVector(RC_REST_ROTATION,CDGetRot(op));
			tData->SetBool(RC_REST_ROTATION_SET,true);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	LONG rCnt = tData->GetLong(RC_TARGET_COUNT);
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==RC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==RC_ADD_ROT)
			{
				if(tData->GetBool(T_REG))
				{
					if (rCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,rCnt);
						rCnt++;
						tData->SetLong(RC_TARGET_COUNT,rCnt);
					}
				}
			}
			else if (dc->id[0].id==RC_SUB_ROT)
			{
				if(tData->GetBool(T_REG))
				{
					if (rCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,rCnt);
						rCnt--;
						tData->SetLong(RC_TARGET_COUNT,rCnt);
					}
				}
			}
			break;
		}
	}

	return true;
}

Bool CDRConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;
	
	BaseObject *goal = NULL;

	if (!tData->GetBool(RC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	bd->SetPen(tData->GetVector(RC_LINE_COLOR));
	
	LONG i, rCnt = tData->GetLong(RC_TARGET_COUNT);
	for(i=0; i<rCnt; i++)
	{
		goal = tData->GetObjectLink(RC_TARGET+i,doc);
		if(goal)
		{
			CDDrawLine(bd,goal->GetMg().off, op->GetMg().off);
		}
	}

	return true;
}

LONG CDRConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, rCnt = tData->GetLong(RC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<rCnt; i++)
	{
		trg = tData->GetObjectLink(RC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < rCnt)
			{
				j++;
				trg = tData->GetObjectLink(RC_TARGET+j,doc);
			}
			if(trg)
			{
				chkCnt++;
				CopyTargetParameters(doc,tData,j,i);
				SetDefaultTargetParameters(tData,j);
			}
		}
		else chkCnt++;
	}
	
	return chkCnt;
}

LONG CDRConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, rCnt = ConfirmTargetCount(doc,tData);
	BaseObject *goal = tData->GetObjectLink(RC_TARGET,doc); if(!goal) return false;
	BaseObject *opA = goal;
	
	if(!tData->GetBool(RC_REST_ROTATION_SET))
	{
		tData->SetVector(RC_REST_ROTATION,CDGetRot(op));
		tData->SetBool(RC_REST_ROTATION_SET,true);
	}
	
	Vector theScale = CDGetScale(op), opPos = CDGetPos(op);
	Matrix opM, targM, transM, pM = goal->GetMg();
	Bool local = tData->GetBool(RC_LOCAL_ROT);
	
	if(local) opM = op->GetMl();
	else opM = op->GetMg();
	
	CDQuaternion opQ, trgQ, mixQ, qA, qB, q25, q50, q75;
	if(local) opQ.SetMatrix(op->GetMl());
	else opQ.SetMatrix(MInv(pM) * op->GetMg());

	// Get the total constraint mix
	Bool ABMixAvailable = true;
	Real totalMix = 0.0;
	
	if(rCnt < 2) ABMixAvailable = false;
	for(i=0; i<rCnt; i++)
	{
		goal = tData->GetObjectLink(RC_TARGET+i,doc);
		if(goal)
		{
			totalMix += tData->GetReal(RC_ROT_MIX+i);
		}
		else ABMixAvailable = false;
	}
	if(totalMix == 0.0) return false;
	
	if(!ABMixAvailable) tData->SetBool(RC_USE_AB_MIX,false);

	if(rCnt == 2)
	{
		BaseObject *opB = tData->GetObjectLink(RC_TARGET+1,doc);
		
		Real theMix, totalMix = tData->GetReal(RC_ROT_MIX) + tData->GetReal(RC_ROT_MIX+1);
		
		if(tData->GetBool(RC_USE_AB_MIX)) theMix = tData->GetReal(RC_AB_MIX);
		else theMix = tData->GetReal(RC_ROT_MIX+1)/totalMix;
		
		switch (tData->GetLong(RC_INTERPOLATION))
		{
			case RC_SHORTEST:
			{
				if(local)
				{
					qA.SetMatrix(opA->GetMl());
					qB.SetMatrix(opB->GetMl());
				}
				else
				{
					qA.SetMatrix(MInv(pM) * opA->GetMg());
					qB.SetMatrix(MInv(pM) * opB->GetMg());
				}
				break;
			}
			case RC_AVERAGE:
			{
				qA = CDQuaternion();
				qB.SetHPB(GetRotationalDifference(opA,opB,local));
				break;
			}
		}
		
		if(tData->GetLong(RC_INTERPOLATION) == RC_SHORTEST)
		{
			trgQ = CDQSlerp(qA,qB,theMix);
		}
		else
		{
			q50 = !(qA + qB); // calculate the 50 quat
			q25 = !(q50 + qA); // calculate the 25 quat
			q75 = !(q50 + qB); // calculate the 75 quat
			trgQ = CDQSlerpBezier(qA,q25,q50,q75,qB,theMix);
		}
	}
	else
	{
		Real angle = 0;
		Vector axis = Vector(0,0,0), oldHPB;
		for(i=0; i<rCnt; i++)
		{
			Real maxVal = 0.0, minVal = 1.0;
			if(rCnt-1 != 0) minVal = 1.0/(Real)(rCnt-1);//Check for division by zero
		
			Real theMix, mixVal, value = Abs((minVal*i) - tData->GetReal(RC_AB_MIX));
			if(value > minVal) value = minVal;
			if ((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
			else mixVal = 0.0;
			
			goal = tData->GetObjectLink(RC_TARGET+i,doc);
			if(goal)
			{
				if(tData->GetBool(RC_USE_AB_MIX)) theMix = mixVal;
				else theMix = tData->GetReal(RC_ROT_MIX+i)/totalMix;
				
				switch (tData->GetLong(RC_INTERPOLATION))
				{
					case RC_SHORTEST:
					{
						if(local) qA.SetMatrix(goal->GetMl());
						else qA.SetMatrix(MInv(pM) * goal->GetMg());
						break;
					}
					case RC_AVERAGE:
					{
						if(local) qA.SetHPB(CDGetRot(goal));
						else qA.SetHPB(GetRotationalDifference(opA,goal,local));
						break;
					}
				}
				axis += qA.v * theMix;
				angle += qA.w * theMix;
			}
		}
		qB = CDQuaternion(angle, axis.x, axis.y, axis.z);
		trgQ = !qB;
	}
	
	targM = trgQ.GetMatrix();
	targM.off = opM.off;
	
	transM = opM;
	if(tData->GetBool(RC_AXIS_X))
	{
		transM = targM * RotAxisToMatrix(Vector(0,1,0),tData->GetReal(RC_OFFSET_X));
		targM = transM;
	}
	if(tData->GetBool(RC_AXIS_Y))
	{
		transM = targM * RotAxisToMatrix(Vector(1,0,0),tData->GetReal(RC_OFFSET_Y));
		targM = transM;
	}
	if(tData->GetBool(RC_AXIS_Z))
	{
		transM = targM * RotAxisToMatrix(Vector(0,0,1),tData->GetReal(RC_OFFSET_Z));
	}
	
	trgQ.SetMatrix(transM);
	
	Vector oldRot = CDGetRot(op), newRot, opRot, targetRot;
	Real theStrength = tData->GetReal(RC_STRENGTH);
	mixQ = CDQSlerp(opQ,trgQ,theStrength);
		
	if(local) transM = mixQ.GetMatrix();
	else transM = pM * mixQ.GetMatrix();
	transM.off = opM.off;
	
	if(theStrength > 0.0)
	{
		if(!MatrixEqual(transM,opM,0.001))
		{
			
			if(local) op->SetMl(transM);
			else op->SetMg(transM);
			
			CDSetScale(op,theScale);
			
			newRot = CDGetRot(op);
			opRot = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,opRot);
			CDSetPos(op,opPos);
		}
	}

	return CD_EXECUTION_RESULT_OK;
}

Bool CDRConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, rCnt = tData->GetLong(RC_TARGET_COUNT);
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(RC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	for (i=0; i<rCnt; i++)
	{
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if (!description->SetParameter(DescLevel(RC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(RC_ID_TARGET))) return false;

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(RC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(RC_ID_TARGET))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(RC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(RC_LINK_GROUP+i))) return false;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(RC_ROT_MIX+i, DTYPE_REAL, 0), bc3, DescLevel(RC_LINK_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDRConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	LONG i, rCnt = tData->GetLong(RC_TARGET_COUNT);
	switch (id[0].id)
	{
		case RC_LOCAL_ROT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RC_AXIS_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RC_AXIS_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RC_AXIS_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RC_OFFSET_X:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(RC_AXIS_X);
		case RC_OFFSET_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(RC_AXIS_Y);
		case RC_OFFSET_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(RC_AXIS_Z);
		case RC_USE_AB_MIX:
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				Bool ABMixAvailable = true;
				if(rCnt < 2) ABMixAvailable = false;
				for(i=0; i<rCnt; i++)
				{
					BaseObject *goal = tData->GetObjectLink(RC_TARGET+i,doc);
					if(!goal) ABMixAvailable = false;
				}
				return ABMixAvailable;
			}
		}
		case RC_AB_MIX:
			return tData->GetBool(RC_USE_AB_MIX);
		case RC_ADD_ROT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RC_SUB_ROT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case RC_INTERPOLATION:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	for(i=0; i<rCnt; i++)
	{
		if(id[0].id == RC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == RC_ROT_MIX+i)
		{
			BaseObject *goal = tData->GetObjectLink(RC_TARGET+i,doc);
			if(!goal) return false;
			else if(tData->GetBool(RC_USE_AB_MIX)) return false;
			else return true;
		}
	}
	return true;
}

Bool RegisterCDRConstraintPlugin(void)
{
	if(CDFindPlugin(ID_CDRCONSTRAINTPLUGIN,CD_TAG_PLUGIN)) return true;
	
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
	String name=GeLoadString(IDS_CDRCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDRCONSTRAINTPLUGIN,name,info,CDRConstraintPlugin::Alloc,"tCDRConstraint","CDRConstraint.tif",1);
}
