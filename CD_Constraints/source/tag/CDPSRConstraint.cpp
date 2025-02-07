//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDPSRConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	10

enum
{
	//PSRC_PURCHASE			= 1000,
	
	//PSRC_SHOW_LINES			= 1010,
	//PSRC_LINE_COLOR			= 1011,
	
	//PSRC_USE_P			= 1012,
	//PSRC_USE_R			= 1013,
	//PSRC_USE_S			= 1014,
	
	PSRC_TARGET_A_OLD			= 1015,
	PSRC_TARGET_B_OLD			= 1016,
	//PSRC_AB_MIX				= 1017,
	
	//PSRC_STRENGTH			= 1018,
	
	//PSRC_LOCAL_PSR			= 1022,
	//PSRC_USE_AB_MIX			= 1023,
	
	PSRC_COUNT			= 1030,
	//PSRC_ADD_PSR				= 1031,
	//PSRC_SUB_PSR				= 1032,
	PSRC_DISK_LEVEL				= 1033,					
	
	PSRC_TARGET_COUNT			= 1034,
	
	//PSRC_INTERPOLATION		= 1040,
		//PSRC_AVERAGE			= 1041,
		//PSRC_SHORTEST		= 1042,					
	
	//PSRC_ID_TARGET			= 1050,
	
	//PSRC_LINK_GROUP			= 2000,
	//PSRC_LINE_ADD			= 3000,

	//PSRC_TARGET				= 4000,
	
	//PSRC_PSR_MIX				= 5000,

	// Reserve containers 10000 to 10400
	PSRC_TOGGLE					= 10000,
	PSRC_OLD_W					= 10100,
	PSRC_OLD_XYZ					= 10200,
	PSRC_OLD_HPB					= 10300,

	PSRC_REST_ROTATION_SET		= 11000,
	PSRC_REST_ROTATION			= 11001
};

class CDPSRConstraintPlugin : public CDTagData
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
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDPSRConstraintPlugin); }
};

Bool CDPSRConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(P_AXIS_X,true);
	tData->SetBool(P_AXIS_Y,true);
	tData->SetBool(P_AXIS_Z,true);
	tData->SetBool(S_AXIS_X,true);
	tData->SetBool(S_AXIS_Y,true);
	tData->SetBool(S_AXIS_Z,true);
	tData->SetBool(R_AXIS_X,true);
	tData->SetBool(R_AXIS_Y,true);
	tData->SetBool(R_AXIS_Z,true);

	tData->SetLong(PSRC_INTERPOLATION,PSRC_SHORTEST);

	tData->SetBool(PSRC_USE_P,true);
	tData->SetBool(PSRC_USE_S,true);
	tData->SetBool(PSRC_USE_R,true);
	tData->SetBool(PSRC_SHOW_LINES,true);
	tData->SetVector(PSRC_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(PSRC_STRENGTH,1.00);
	tData->SetReal(PSRC_AB_MIX,0.5);
		
	tData->SetLong(PSRC_TARGET_COUNT,1);
	tData->SetReal(PSRC_PSR_MIX,1.00);

	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		if(pd) pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDPSRConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(PSRC_DISK_LEVEL,level);
	
	return true;
}

void CDPSRConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetBool(PSRC_LOCAL_PSR+T_LST,tData->GetBool(PSRC_LOCAL_PSR));
			tData->SetBool(PSRC_USE_P+T_LST,tData->GetBool(PSRC_USE_P));
			tData->SetBool(PSRC_USE_S+T_LST,tData->GetBool(PSRC_USE_S));
			tData->SetBool(PSRC_USE_R+T_LST,tData->GetBool(PSRC_USE_R));
			
			tData->SetBool(PSRC_USE_AB_MIX+T_LST,tData->GetBool(PSRC_USE_AB_MIX));
			
			tData->SetLong(PSRC_INTERPOLATION+T_LST,tData->GetLong(PSRC_INTERPOLATION));
			tData->SetLong(PSRC_TARGET_COUNT+T_LST,tData->GetLong(PSRC_TARGET_COUNT));
			LONG i, psrCnt = tData->GetLong(PSRC_TARGET_COUNT);
			for(i=0; i<psrCnt; i++)
			{
				tData->SetLink(PSRC_TARGET+i+T_LST,tData->GetLink(PSRC_TARGET+i,doc));
			}
			
			tData->SetBool(P_AXIS_X+T_LST,tData->GetBool(P_AXIS_X));
			tData->SetBool(P_AXIS_Y+T_LST,tData->GetBool(P_AXIS_Y));
			tData->SetBool(P_AXIS_Z+T_LST,tData->GetBool(P_AXIS_Z));
			tData->SetReal(P_OFFSET_X+T_LST,tData->GetReal(P_OFFSET_X));
			tData->SetReal(P_OFFSET_Y+T_LST,tData->GetReal(P_OFFSET_Y));
			tData->SetReal(P_OFFSET_Z+T_LST,tData->GetReal(P_OFFSET_Z));
			
			tData->SetBool(S_AXIS_X+T_LST,tData->GetBool(S_AXIS_X));
			tData->SetBool(S_AXIS_Y+T_LST,tData->GetBool(S_AXIS_Y));
			tData->SetBool(S_AXIS_Z+T_LST,tData->GetBool(S_AXIS_Z));
			tData->SetReal(S_OFFSET_X+T_LST,tData->GetReal(S_OFFSET_X));
			tData->SetReal(S_OFFSET_Y+T_LST,tData->GetReal(S_OFFSET_Y));
			tData->SetReal(S_OFFSET_Z+T_LST,tData->GetReal(S_OFFSET_Z));
			
			tData->SetBool(R_AXIS_X+T_LST,tData->GetBool(R_AXIS_X));
			tData->SetBool(R_AXIS_Y+T_LST,tData->GetBool(R_AXIS_Y));
			tData->SetBool(R_AXIS_Z+T_LST,tData->GetBool(R_AXIS_Z));
			tData->SetReal(R_OFFSET_X+T_LST,tData->GetReal(R_OFFSET_X));
			tData->SetReal(R_OFFSET_Y+T_LST,tData->GetReal(R_OFFSET_Y));
			tData->SetReal(R_OFFSET_Z+T_LST,tData->GetReal(R_OFFSET_Z));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDPSRConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetBool(PSRC_LOCAL_PSR,tData->GetBool(PSRC_LOCAL_PSR+T_LST));
		tData->SetBool(PSRC_USE_P,tData->GetBool(PSRC_USE_P+T_LST));
		tData->SetBool(PSRC_USE_S,tData->GetBool(PSRC_USE_S+T_LST));
		tData->SetBool(PSRC_USE_R,tData->GetBool(PSRC_USE_R+T_LST));
		
		tData->SetBool(PSRC_USE_AB_MIX,tData->GetBool(PSRC_USE_AB_MIX+T_LST));
		
		tData->SetLong(PSRC_INTERPOLATION,tData->GetLong(PSRC_INTERPOLATION+T_LST));
		tData->SetLong(PSRC_TARGET_COUNT,tData->GetLong(PSRC_TARGET_COUNT+T_LST));
		LONG i, psrCnt = tData->GetLong(PSRC_TARGET_COUNT);
		for(i=0; i<psrCnt; i++)
		{
			tData->SetLink(PSRC_TARGET+i,tData->GetLink(PSRC_TARGET+i+T_LST,doc));
		}
		
		tData->SetBool(P_AXIS_X,tData->GetBool(P_AXIS_X+T_LST));
		tData->SetBool(P_AXIS_Y,tData->GetBool(P_AXIS_Y+T_LST));
		tData->SetBool(P_AXIS_Z,tData->GetBool(P_AXIS_Z+T_LST));
		tData->SetReal(P_OFFSET_X,tData->GetReal(P_OFFSET_X+T_LST));
		tData->SetReal(P_OFFSET_Y,tData->GetReal(P_OFFSET_Y+T_LST));
		tData->SetReal(P_OFFSET_Z,tData->GetReal(P_OFFSET_Z+T_LST));
		
		tData->SetBool(S_AXIS_X,tData->GetBool(S_AXIS_X+T_LST));
		tData->SetBool(S_AXIS_Y,tData->GetBool(S_AXIS_Y+T_LST));
		tData->SetBool(S_AXIS_Z,tData->GetBool(S_AXIS_Z+T_LST));
		tData->SetReal(S_OFFSET_X,tData->GetReal(S_OFFSET_X+T_LST));
		tData->SetReal(S_OFFSET_Y,tData->GetReal(S_OFFSET_Y+T_LST));
		tData->SetReal(S_OFFSET_Z,tData->GetReal(S_OFFSET_Z+T_LST));
		
		tData->SetBool(R_AXIS_X,tData->GetBool(R_AXIS_X+T_LST));
		tData->SetBool(R_AXIS_Y,tData->GetBool(R_AXIS_Y+T_LST));
		tData->SetBool(R_AXIS_Z,tData->GetBool(R_AXIS_Z+T_LST));
		tData->SetReal(R_OFFSET_X,tData->GetReal(R_OFFSET_X+T_LST));
		tData->SetReal(R_OFFSET_Y,tData->GetReal(R_OFFSET_Y+T_LST));
		tData->SetReal(R_OFFSET_Z,tData->GetReal(R_OFFSET_Z+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDPSRConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	if(!tData->GetBool(PSRC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	LONG i, psrCnt = tData->GetLong(PSRC_TARGET_COUNT);
	BaseObject *goal = NULL;
	
	bd->SetPen(tData->GetVector(PSRC_LINE_COLOR));
	Matrix goalM, opM;
	bd->SetMatrix_Matrix(op, opM);
	
	for(i=0; i<psrCnt; i++)
	{
		goal = tData->GetObjectLink(PSRC_TARGET+i,doc);
		if(goal)
		{
			bd->SetMatrix_Matrix(goal, goalM);
			CDDrawLine(bd,op->GetMg().off, goal->GetMg().off);
		}
	}
	
	return true;
}

void CDPSRConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(PSRC_TARGET+i,NULL);
	tData->SetReal(PSRC_PSR_MIX+i,1.00);
}

void CDPSRConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(PSRC_TARGET+dst,tData->GetObjectLink(PSRC_TARGET+src,doc));
	tData->SetReal(PSRC_PSR_MIX+dst,tData->GetReal(PSRC_PSR_MIX+src));
}

void CDPSRConstraintPlugin::ConvertOldData(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, oldCnt = 0;
	
	BaseObject *goalA = tData->GetObjectLink(PSRC_TARGET_A_OLD,doc);
	if(goalA)
	{
		tData->SetLink(PSRC_TARGET,goalA);
		tData->SetReal(PSRC_PSR_MIX,1.00);
		
		Real strength = 1.0;
		if(tData->GetReal(P_STRENGTH) < 1.0) strength = tData->GetReal(P_STRENGTH);
		if(tData->GetReal(R_STRENGTH) < strength) strength = tData->GetReal(R_STRENGTH);
		if(tData->GetReal(S_STRENGTH) < strength) strength = tData->GetReal(S_STRENGTH);
		tData->SetReal(PSRC_STRENGTH,strength);
		oldCnt++;
		tData->SetLong(PSRC_TARGET_COUNT,oldCnt);
	}
	BaseObject *goalB = tData->GetObjectLink(PSRC_TARGET_B_OLD,doc);
	if(goalB)
	{
		tData->SetLink(PSRC_TARGET+1,goalB);
		tData->SetReal(PSRC_PSR_MIX,1.0-tData->GetReal(PSRC_AB_MIX));
		tData->SetReal(PSRC_PSR_MIX+1,tData->GetReal(PSRC_AB_MIX));
		tData->SetBool(PSRC_USE_AB_MIX,true);
		oldCnt++;
		tData->SetLong(PSRC_TARGET_COUNT,oldCnt);
	}
	
	if(oldCnt == 0) tData->SetLong(PSRC_TARGET_COUNT,tData->GetLong(PSRC_COUNT) + 1);
	else
	{
		for(i=oldCnt; i<MAX_ADD; i++)
		{
			if(tData->GetObjectLink(PSRC_TARGET+i,doc)) oldCnt++;
		}
		tData->SetLong(PSRC_TARGET_COUNT,oldCnt);
	}
}

Bool CDPSRConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(PSRC_DISK_LEVEL) < 1) ConvertOldData(doc,tData);
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
			
			tData->SetVector(PSRC_REST_ROTATION,CDGetRot(op));
			tData->SetBool(PSRC_REST_ROTATION_SET,true);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	LONG psrCnt = tData->GetLong(PSRC_TARGET_COUNT);
	switch (type)
	{
		case ID_CDFREEZETRANSFORMATION:
		{
			if(tData->GetBool(PSRC_USE_S))
			{
				Vector *trnsSca = (Vector*)data;
				if(trnsSca)
				{
					Vector sca = *trnsSca, offV;
					offV.x = tData->GetReal(S_OFFSET_X) * sca.x;
					offV.y = tData->GetReal(S_OFFSET_Y) * sca.y;
					offV.z = tData->GetReal(S_OFFSET_Z) * sca.z;
					
					if(tData->GetBool(S_AXIS_X)) tData->SetReal(S_OFFSET_X,offV.x);
					if(tData->GetBool(S_AXIS_Y)) tData->SetReal(S_OFFSET_Y,offV.y);
					if(tData->GetBool(S_AXIS_Z)) tData->SetReal(S_OFFSET_Z,offV.z);
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==PSRC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==PSRC_ADD_PSR)
			{
				if(tData->GetBool(T_REG))
				{
					if(psrCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,psrCnt);
						psrCnt++;
						tData->SetLong(PSRC_TARGET_COUNT,psrCnt);
					}
				}
			}
			else if(dc->id[0].id==PSRC_SUB_PSR)
			{
				if(tData->GetBool(T_REG))
				{
					if(psrCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,psrCnt);
						psrCnt--;
						tData->SetLong(PSRC_TARGET_COUNT,psrCnt);
					}
				}
			}
			break;
		}
	}

	return true;
}

LONG CDPSRConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, psrCnt = tData->GetLong(PSRC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<psrCnt; i++)
	{
		trg = tData->GetObjectLink(PSRC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < psrCnt)
			{
				j++;
				trg = tData->GetObjectLink(PSRC_TARGET+j,doc);
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

LONG CDPSRConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	//GePrint("CDPSRConstraintPlugin::Execute");
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, psrCnt = ConfirmTargetCount(doc,tData);
	BaseObject *goal = tData->GetObjectLink(PSRC_TARGET,doc);if(!goal) return false;
	BaseObject *opA = goal;
	
	if(!tData->GetBool(PSRC_REST_ROTATION_SET))
	{
		tData->SetVector(PSRC_REST_ROTATION,CDGetRot(op));
		tData->SetBool(PSRC_REST_ROTATION_SET,true);
	}
	
	Vector mixPos, trgPos;
	Vector gScale, opSca, mixSca, trgSca;
	Matrix opM, goalM, targM, transM, pM = goal->GetMg();
	
	Bool local = tData->GetBool(PSRC_LOCAL_PSR);
	if(local) opM = op->GetMl();
	else opM = op->GetMg();
	trgPos = opM.off;

	CDQuaternion opQ, trgQ, mixQ, qA, qB, q25, q50, q75;
	if(local) opQ.SetMatrix(op->GetMl());
	else opQ.SetMatrix(MInv(pM) * op->GetMg());

	mixPos = Vector(0,0,0);
	
	opSca = CDGetScale(op);
	trgSca = opSca;
	mixSca = Vector(0,0,0);

	// Get the total constraint mix
	Bool ABMixAvailable = true;
	Real totalMix = 0.0;
	
	if(psrCnt < 2) ABMixAvailable = false;
	for(i=0; i<psrCnt; i++)
	{
		goal = tData->GetObjectLink(PSRC_TARGET+i,doc);
		if(goal)
		{
			totalMix += tData->GetReal(PSRC_PSR_MIX+i);
		}
		else ABMixAvailable = false;
	}
	if(totalMix == 0.0) return false;
	
	if(!ABMixAvailable) tData->SetBool(PSRC_USE_AB_MIX,false);

	if(psrCnt == 2)
	{
		BaseObject *opB = tData->GetObjectLink(PSRC_TARGET+1,doc);
		
		Real theMix, totalMix = tData->GetReal(PSRC_PSR_MIX) + tData->GetReal(PSRC_PSR_MIX+1);
		
		gScale += GetGlobalScale(opA) * (tData->GetReal(PSRC_PSR_MIX)/totalMix);
		gScale += GetGlobalScale(opB) * (tData->GetReal(PSRC_PSR_MIX+1)/totalMix);
		
		if(tData->GetBool(PSRC_USE_AB_MIX)) theMix = tData->GetReal(PSRC_AB_MIX);
		else theMix = tData->GetReal(PSRC_PSR_MIX+1)/totalMix;
		
		if(tData->GetBool(PSRC_USE_R))
		{
			switch (tData->GetLong(PSRC_INTERPOLATION))
			{
				case PSRC_SHORTEST:
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
				case PSRC_AVERAGE:
				{
					qA = CDQuaternion();
					qB.SetHPB(GetRotationalDifference(opA,opB,local));
					break;
				}
			}
			
			if(tData->GetLong(PSRC_INTERPOLATION) == PSRC_SHORTEST)
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
		if(tData->GetBool(PSRC_USE_P))
		{
			Vector aPos, bPos;
			
			if(local)
			{
				aPos = CDGetPos(opA);
				bPos = CDGetPos(opB);
			}
			else
			{
				aPos = opA->GetMg().off;
				bPos = opB->GetMg().off;
			}
			mixPos = CDBlend(aPos,bPos,theMix);
		}
		
		if(tData->GetBool(PSRC_USE_S))
		mixSca = CDBlend(CDGetScale(opA),CDGetScale(opB),theMix);
	}
	else
	{
		Real angle = 0;
		Vector axis = Vector(0,0,0), oldHPB;
		for(i=0; i<psrCnt; i++)
		{
			Real maxVal = 0.0, minVal = 1.0;
			if(psrCnt-1 != 0) minVal = 1.0/(Real)(psrCnt-1);//Check for division by zero
			
			Real theMix, mixVal, value = Abs((minVal*i) - tData->GetReal(PSRC_AB_MIX));
			if(value > minVal) value = minVal;
			if((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
			else mixVal = 0.0;
			
			goal = tData->GetObjectLink(PSRC_TARGET+i,doc);
			if(goal)
			{
				gScale += GetGlobalScale(goal) * (tData->GetReal(PSRC_PSR_MIX+i)/totalMix);
				if(local) goalM = goal->GetMl();
				else goalM = goal->GetMg();

				if(tData->GetBool(PSRC_USE_R))
				{
					if(tData->GetBool(PSRC_USE_AB_MIX)) theMix = mixVal;
					else theMix = tData->GetReal(PSRC_PSR_MIX+i)/totalMix;
					
					switch (tData->GetLong(PSRC_INTERPOLATION))
					{
						case PSRC_SHORTEST:
						{
							if(local) qA.SetMatrix(goal->GetMl());
							else qA.SetMatrix(MInv(pM) * goal->GetMg());
							break;
						}
						case PSRC_AVERAGE:
						{
							if(local) qA.SetHPB(CDGetRot(goal));
							else qA.SetHPB(GetRotationalDifference(opA,goal,local));
							break;
						}
					}
					axis += qA.v * theMix;
					angle += qA.w * theMix;
				}
				
				if(tData->GetBool(PSRC_USE_P))
				{
					if(tData->GetBool(PSRC_USE_AB_MIX)) mixPos += goalM.off * mixVal;
					else mixPos += goalM.off * (tData->GetReal(PSRC_PSR_MIX+i)/totalMix);
				}
				
				if(tData->GetBool(PSRC_USE_S))
				{
					if(tData->GetBool(PSRC_USE_AB_MIX)) mixSca += CDGetScale(goal) * mixVal;
					else mixSca += CDGetScale(goal) * (tData->GetReal(PSRC_PSR_MIX+i)/totalMix);
				}
			}
		}
		qB = CDQuaternion(angle, axis.x, axis.y, axis.z);
		trgQ = !qB;
	}
	
	Vector oldRot = CDGetRot(op), newRot, opRot, trgRot;
	Real strength = tData->GetReal(PSRC_STRENGTH);
	
	if(tData->GetBool(PSRC_USE_P))
	{
		mixPos.x += (tData->GetReal(P_OFFSET_X) * gScale.x);
		mixPos.y += (tData->GetReal(P_OFFSET_Y) * gScale.y);
		mixPos.z += (tData->GetReal(P_OFFSET_Z) * gScale.z);
	}
	else mixPos = opM.off;
	
	if(tData->GetBool(P_AXIS_X)) trgPos.x = CDBlend(opM.off.x,mixPos.x,strength);
	if(tData->GetBool(P_AXIS_Y)) trgPos.y = CDBlend(opM.off.y,mixPos.y,strength);
	if(tData->GetBool(P_AXIS_Z)) trgPos.z = CDBlend(opM.off.z,mixPos.z,strength);

	if(tData->GetBool(PSRC_USE_S))
	{
		mixSca.x += tData->GetReal(S_OFFSET_X);
		mixSca.y += tData->GetReal(S_OFFSET_Y);
		mixSca.z += tData->GetReal(S_OFFSET_Z);
	}
	else mixSca = opSca;
	
	if(tData->GetBool(S_AXIS_X)) trgSca.x = CDBlend(opSca.x,mixSca.x,strength);
	if(tData->GetBool(S_AXIS_Y)) trgSca.y = CDBlend(opSca.y,mixSca.y,strength);
	if(tData->GetBool(S_AXIS_Z)) trgSca.z = CDBlend(opSca.z,mixSca.z,strength);

	if(tData->GetBool(PSRC_USE_R))
	{
		targM = trgQ.GetMatrix();
		transM = targM;
		if(tData->GetBool(R_AXIS_X))
		{
			transM = targM * RotAxisToMatrix(Vector(0,1,0),tData->GetReal(R_OFFSET_X));
			targM = transM;
		}
		if(tData->GetBool(R_AXIS_Y))
		{
			transM = targM * RotAxisToMatrix(Vector(1,0,0),tData->GetReal(R_OFFSET_Y));
			targM = transM;
		}
		if(tData->GetBool(R_AXIS_Z))
		{
			transM = targM * RotAxisToMatrix(Vector(0,0,1),tData->GetReal(R_OFFSET_Z));
		}
		trgQ.SetMatrix(transM);
		
		mixQ = CDQSlerp(opQ,trgQ,strength);
		if(local) targM = mixQ.GetMatrix();
		else targM = pM * mixQ.GetMatrix();
	}
	else targM = opM;
	
	targM.off = trgPos;
	transM = targM;
	if(strength > 0.0)
	{
		if(!MatrixEqual(transM,opM,0.001) || !VEqual(opSca,trgSca,0.001))
		{
			
			if(local) op->SetMl(transM);
			else op->SetMg(transM);
			
			CDSetScale(op,trgSca);
			
			newRot = CDGetRot(op);
			opRot = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,opRot);
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDPSRConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag			*tag  = (BaseTag*)node;
	BaseContainer	*tData = tag->GetDataInstance();
	LONG i, psrCnt = tData->GetLong(PSRC_TARGET_COUNT);
	
	if(!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(PSRC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	for (i=0; i<psrCnt; i++)
	{
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if(!description->SetParameter(DescLevel(PSRC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(PSRC_ID_TARGET))) return false;

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(PSRC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(PSRC_ID_TARGET))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(PSRC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(PSRC_LINK_GROUP+i))) return false;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,1.0);
		if(!description->SetParameter(DescLevel(PSRC_PSR_MIX+i, DTYPE_REAL, 0), bc3, DescLevel(PSRC_LINK_GROUP+i))) return true;
	}

	if(tData->GetBool(PSRC_USE_P))
	{
		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetString(DESC_NAME,GeLoadString(P_GROUP));
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(P_GROUP, DTYPE_GROUP, 0), subgroup1, DESCID_ROOT)) return true;
		
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc1.SetString(DESC_NAME,GeLoadString(P_AXIS_X));
		bc1.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc1.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(P_AXIS_X,DTYPE_BOOL,0),bc1,DescLevel(P_GROUP))) return false;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc2.SetString(DESC_NAME, GeLoadString(P_OFFSET_X));
		bc2.SetLong(DESC_UNIT, DESC_UNIT_METER);
		if(!description->SetParameter(DescLevel(P_OFFSET_X, DTYPE_REAL, 0), bc2, DescLevel(P_GROUP))) return true;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc3.SetString(DESC_NAME,GeLoadString(P_AXIS_Y));
		bc3.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc3.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(P_AXIS_Y,DTYPE_BOOL,0),bc3,DescLevel(P_GROUP))) return false;
		
		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc4.SetString(DESC_NAME, GeLoadString(P_OFFSET_Y));
		bc4.SetLong(DESC_UNIT, DESC_UNIT_METER);
		if(!description->SetParameter(DescLevel(P_OFFSET_Y, DTYPE_REAL, 0), bc4, DescLevel(P_GROUP))) return true;
		
		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc5.SetString(DESC_NAME,GeLoadString(P_AXIS_Z));
		bc5.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc5.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(P_AXIS_Z,DTYPE_BOOL,0),bc5,DescLevel(P_GROUP))) return false;
		
		BaseContainer bc6 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc6.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc6.SetString(DESC_NAME, GeLoadString(P_OFFSET_Z));
		bc6.SetLong(DESC_UNIT, DESC_UNIT_METER);
		if(!description->SetParameter(DescLevel(P_OFFSET_Z, DTYPE_REAL, 0), bc6, DescLevel(P_GROUP))) return true;
	}
	if(tData->GetBool(PSRC_USE_S))
	{
		BaseContainer subgroup2 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup2.SetString(DESC_NAME,GeLoadString(S_GROUP));
		subgroup2.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(S_GROUP, DTYPE_GROUP, 0), subgroup2, DESCID_ROOT)) return true;
		
		BaseContainer bc8 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc8.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc8.SetString(DESC_NAME,GeLoadString(S_AXIS_X));
		bc8.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc8.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(S_AXIS_X,DTYPE_BOOL,0),bc8,DescLevel(S_GROUP))) return false;
		
		BaseContainer bc9 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc9.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc9.SetString(DESC_NAME, GeLoadString(S_OFFSET_X));
		bc9.SetLong(DESC_UNIT, DESC_UNIT_REAL);
		bc9.SetReal(DESC_STEP,0.01);
		if(!description->SetParameter(DescLevel(S_OFFSET_X, DTYPE_REAL, 0), bc9, DescLevel(S_GROUP))) return true;
		
		BaseContainer bc10 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc10.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc10.SetString(DESC_NAME,GeLoadString(S_AXIS_Y));
		bc10.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc10.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(S_AXIS_Y,DTYPE_BOOL,0),bc10,DescLevel(S_GROUP))) return false;
		
		BaseContainer bc11 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc11.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc11.SetString(DESC_NAME, GeLoadString(S_OFFSET_Y));
		bc11.SetLong(DESC_UNIT, DESC_UNIT_REAL);
		bc11.SetReal(DESC_STEP,0.01);
		if(!description->SetParameter(DescLevel(S_OFFSET_Y, DTYPE_REAL, 0), bc11, DescLevel(S_GROUP))) return true;
		
		BaseContainer bc12 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc12.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc12.SetString(DESC_NAME,GeLoadString(S_AXIS_Z));
		bc12.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc12.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(S_AXIS_Z,DTYPE_BOOL,0),bc12,DescLevel(S_GROUP))) return false;
		
		BaseContainer bc13 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc13.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc13.SetString(DESC_NAME, GeLoadString(S_OFFSET_Z));
		bc13.SetLong(DESC_UNIT, DESC_UNIT_REAL);
		bc13.SetReal(DESC_STEP,0.01);
		if(!description->SetParameter(DescLevel(S_OFFSET_Z, DTYPE_REAL, 0), bc13, DescLevel(S_GROUP))) return true;
	}
	if(tData->GetBool(PSRC_USE_R))
	{
		BaseContainer subgroup3 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup3.SetString(DESC_NAME,GeLoadString(R_GROUP));
		subgroup3.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(R_GROUP, DTYPE_GROUP, 0), subgroup3, DESCID_ROOT)) return true;
		
		BaseContainer bc15 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc15.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc15.SetString(DESC_NAME,GeLoadString(R_AXIS_X));
		bc15.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc15.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(R_AXIS_X,DTYPE_BOOL,0),bc15,DescLevel(R_GROUP))) return false;
		
		BaseContainer bc16 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc16.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc16.SetString(DESC_NAME, GeLoadString(R_OFFSET_X));
		bc16.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
		bc16.SetReal(DESC_STEP,0.01);
		if(!description->SetParameter(DescLevel(R_OFFSET_X, DTYPE_REAL, 0), bc16, DescLevel(R_GROUP))) return true;
		
		BaseContainer bc17 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc17.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc17.SetString(DESC_NAME,GeLoadString(R_AXIS_Y));
		bc17.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc17.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(R_AXIS_Y,DTYPE_BOOL,0),bc17,DescLevel(R_GROUP))) return false;
		
		BaseContainer bc18 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc18.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc18.SetString(DESC_NAME, GeLoadString(R_OFFSET_Y));
		bc18.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
		bc18.SetReal(DESC_STEP,0.01);
		if(!description->SetParameter(DescLevel(R_OFFSET_Y, DTYPE_REAL, 0), bc18, DescLevel(R_GROUP))) return true;
		
		BaseContainer bc19 = GetCustomDataTypeDefault(DTYPE_BOOL);
		bc19.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_BOOL);
		bc19.SetString(DESC_NAME,GeLoadString(R_AXIS_Z));
		bc19.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc19.SetBool(DESC_REMOVEABLE,true);
		if(!description->SetParameter(DescLevel(R_AXIS_Z,DTYPE_BOOL,0),bc19,DescLevel(R_GROUP))) return false;
		
		BaseContainer bc20 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc20.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc20.SetString(DESC_NAME, GeLoadString(R_OFFSET_Z));
		bc20.SetLong(DESC_UNIT, DESC_UNIT_DEGREE);
		bc20.SetReal(DESC_STEP,0.01);
		if(!description->SetParameter(DescLevel(R_OFFSET_Z, DTYPE_REAL, 0), bc20, DescLevel(R_GROUP))) return true;
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDPSRConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	LONG i, psrCnt = tData->GetLong(PSRC_TARGET_COUNT);
	switch (id[0].id)
	{
		case PSRC_LOCAL_PSR:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSRC_USE_P:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSRC_USE_R:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSRC_USE_S:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSRC_USE_AB_MIX:
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				Bool ABMixAvailable = true;
				if(psrCnt < 2) ABMixAvailable = false;
				for(i=0; i<psrCnt; i++)
				{
					BaseObject *goal = tData->GetObjectLink(PSRC_TARGET+i,doc);
					if(!goal) ABMixAvailable = false;
				}
				return ABMixAvailable;
			}
		}
		case PSRC_AB_MIX:
			return tData->GetBool(PSRC_USE_AB_MIX);
		case PSRC_ADD_PSR:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSRC_SUB_PSR:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PSRC_INTERPOLATION:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case P_AXIS_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case P_AXIS_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case P_AXIS_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case S_AXIS_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case S_AXIS_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case S_AXIS_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case R_AXIS_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case R_AXIS_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case R_AXIS_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case P_OFFSET_X:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(P_AXIS_X);
		case P_OFFSET_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(P_AXIS_Y);
		case P_OFFSET_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(P_AXIS_Z);
		case S_OFFSET_X:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(S_AXIS_X);
		case S_OFFSET_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(S_AXIS_Y);
		case S_OFFSET_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(S_AXIS_Z);
		case R_OFFSET_X:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(R_AXIS_X);
		case R_OFFSET_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(R_AXIS_Y);
		case R_OFFSET_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(R_AXIS_Z);
	}
	for(i=0; i<psrCnt; i++)
	{
		if(id[0].id == PSRC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == PSRC_PSR_MIX+i)
		{
			BaseObject *goal = tData->GetObjectLink(PSRC_TARGET+i,doc);
			if(!goal) return false;
			else if(tData->GetBool(PSRC_USE_AB_MIX)) return false;
			else return true;
		}
	}
	return true;
}

Bool RegisterCDPSRConstraintPlugin(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDPSRCONSTRAINT); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDPSRCONSTRAINTPLUGIN,name,info,CDPSRConstraintPlugin::Alloc,"tCDPSRConstraint","CDPSRConstraint.tif",1);
}
