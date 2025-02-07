//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDDConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	10

enum
{
	//DC_PURCHASE			= 1000,
	
	DC_TARGET_OLD				= 1010,
	DC_DISTANCE_OLD			= 1011,
	//DC_STRENGTH			= 1012,
	//DC_SHOW_LINES			= 1013,
	//DC_LINE_COLOR			= 1014,
	
	DC_CLAMP_DISTANCE_OLD		= 1015,
		DC_CLAMP_MIN_OLD		= 1016,
		DC_CLAMP_MAX_OLD		= 1017,
		DC_CLAMP_BOTH_OLD		= 1018,
	
	DC_INIT_DISTANCE_OLD		= 1019,
	
	DC_DIST_COUNT			= 1030,
	//DC_ADD_DIST			= 1031,
	//DC_SUB_DIST			= 1032,
	DC_DISK_LEVEL			= 1033,					
	
	DC_TARGET_COUNT			= 1040,
	
	//DC_ID_TARGET			= 1050,
	
	//DC_LINK_GROUP			= 2000,
	//DC_LINE_ADD			= 2100,
	//DC_CLAMP_GROUP			= 2200,

	//DC_TARGET			= 2300,
	//DC_DIST_MIX			= 2400,
	//DC_DISTANCE			= 2500,
	
	DC_INIT_DISTANCE		= 3000,
	//DC_CLAMP_DISTANCE	= 3100,
		//DC_CLAMP_MIN		= 3200,
		//DC_CLAMP_MAX		= 3300,
		//DC_CLAMP_BOTH	= 3400,
	
	DC_CONTACT_HIT				= 5000,
	DC_CONTACT_PT				= 5100,
	DC_CONTACT_NORM			= 5200,
	DC_CONTACT_CNST			= 5300
};

class CDDConstraintPlugin : public CDTagData
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
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDDConstraintPlugin); }
};

Bool CDDConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(DC_SHOW_LINES,false);
	tData->SetVector(DC_LINE_COLOR, Vector(1,0,0));
	tData->SetReal(DC_STRENGTH,1.00);
	
	tData->SetLong(DC_TARGET_COUNT,1);
	tData->SetLong(DC_CLAMP_DISTANCE,DC_CLAMP_MAX);
	tData->SetReal(DC_DIST_MIX,1.00);
		
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

Bool CDDConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(DC_DISK_LEVEL,level);
	
	return true;
}

Bool CDDConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;
	
	if (!tData->GetBool(DC_SHOW_LINES)) return true;

	BaseObject *goal = NULL;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	LONG i, dCnt = tData->GetLong(DC_TARGET_COUNT);
	for(i=0; i<dCnt; i++)
	{
		goal = tData->GetObjectLink(DC_TARGET+i,doc);
		if(goal)
		{
			Vector h;
			Real dist = tData->GetReal(DC_DISTANCE+i);
			
			Matrix m = goal->GetMg();
			
			m.v1*=dist;
			m.v2*=dist;
			m.v3*=dist;

			bd->SetPen(tData->GetVector(DC_LINE_COLOR));
			CDDrawCircle(bd,m);
			
			h=m.v2;
			m.v2=m.v3;
			m.v3=h;
			CDDrawCircle(bd,m);
			
			h=m.v1;
			m.v1=m.v3;
			m.v3=h;
			CDDrawCircle(bd,m);
			
		}
	}
	
	
	return true;
}

void CDDConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLong(DC_TARGET_COUNT+T_LST,tData->GetLong(DC_TARGET_COUNT));
			LONG i, clCnt = tData->GetLong(DC_TARGET_COUNT);
			for(i=0; i<clCnt; i++)
			{
				tData->SetLink(DC_TARGET+i+T_LST,tData->GetLink(DC_TARGET+i,doc));
				tData->SetReal(DC_DISTANCE+i+T_LST,tData->GetReal(DC_DISTANCE+i));
				tData->SetLong(DC_CLAMP_DISTANCE+i+T_LST,tData->GetLong(DC_CLAMP_DISTANCE+i));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDDConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLong(DC_TARGET_COUNT,tData->GetLong(DC_TARGET_COUNT+T_LST));
		LONG i, clCnt = tData->GetLong(DC_TARGET_COUNT);
		for(i=0; i<clCnt; i++)
		{
			tData->SetLink(DC_TARGET+i,tData->GetLink(DC_TARGET+i+T_LST,doc));
			tData->SetReal(DC_DISTANCE+i,tData->GetReal(DC_DISTANCE+i+T_LST));
			tData->SetLong(DC_CLAMP_DISTANCE+i,tData->GetLong(DC_CLAMP_DISTANCE+i+T_LST));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

void CDDConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(DC_TARGET+i,NULL);
	tData->SetReal(DC_DIST_MIX+i,1.00);
	tData->SetReal(DC_DISTANCE+i,0.0);
	tData->SetLong(DC_CLAMP_DISTANCE+i,DC_CLAMP_MAX+i);
}

void CDDConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(DC_TARGET+dst,tData->GetObjectLink(DC_TARGET+src,doc));
	tData->SetReal(DC_DIST_MIX+dst,tData->GetReal(DC_DIST_MIX+src));
	tData->SetReal(DC_DISTANCE+dst,tData->GetReal(DC_DISTANCE+src));
	tData->SetLong(DC_CLAMP_DISTANCE+dst,tData->GetLong(DC_CLAMP_DISTANCE+src)-(src-dst));
}

void CDDConstraintPlugin::ConvertOldData(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, oldCnt = 0;
	
	BaseObject *goal = tData->GetObjectLink(DC_TARGET_OLD,doc);
	if(goal)
	{
		tData->SetLink(DC_TARGET,goal);
		tData->SetReal(DC_DIST_MIX,1.00);
		tData->SetBool(DC_DISTANCE,tData->GetBool(DC_DISTANCE_OLD));
		tData->SetBool(DC_INIT_DISTANCE,tData->GetBool(DC_INIT_DISTANCE_OLD));
		switch(tData->GetLong(DC_CLAMP_DISTANCE_OLD))
		{
			case DC_CLAMP_MIN_OLD:	
				tData->SetLong(DC_CLAMP_DISTANCE,DC_CLAMP_MIN);
				break;
			case DC_CLAMP_MAX_OLD:
				tData->SetLong(DC_CLAMP_DISTANCE,DC_CLAMP_MAX);
				break;
			case DC_CLAMP_BOTH_OLD:
				tData->SetLong(DC_CLAMP_DISTANCE,DC_CLAMP_BOTH);
				break;
			default:
			break;
		}
		oldCnt++;
		tData->SetLong(DC_TARGET_COUNT,oldCnt);
	}
	
	if(oldCnt == 0) tData->SetLong(DC_TARGET_COUNT,tData->GetLong(DC_DIST_COUNT) + 1);
	else
	{
		for(i=oldCnt; i<MAX_ADD; i++)
		{
			if(tData->GetObjectLink(DC_TARGET+i,doc)) oldCnt++;
		}
		tData->SetLong(DC_TARGET_COUNT,oldCnt);
	}
}

Bool CDDConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(DC_DISK_LEVEL) < 1) ConvertOldData(doc,tData);
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
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	LONG dCnt = tData->GetLong(DC_TARGET_COUNT);
	
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==DC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==DC_ADD_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					if (dCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						
						SetDefaultTargetParameters(tData,dCnt);
						dCnt++;
						tData->SetLong(DC_TARGET_COUNT,dCnt);
					}
				}
			}
			else if (dc->id[0].id==DC_SUB_DIST)
			{
				if(tData->GetBool(T_REG))
				{
					if (dCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						
						SetDefaultTargetParameters(tData,dCnt);
						dCnt--;
						tData->SetLong(DC_TARGET_COUNT,dCnt);
					}
				}
			}
			break;
		}
	}

	return true;
}

LONG CDDConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, dCnt = tData->GetLong(DC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<dCnt; i++)
	{
		trg = tData->GetObjectLink(DC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < dCnt)
			{
				j++;
				trg = tData->GetObjectLink(DC_TARGET+j,doc);
			}
			if(trg)
			{
				chkCnt++;
				CopyTargetParameters(doc,tData,j,i);
				SetDefaultTargetParameters(tData,dCnt);
			}
		}
		else chkCnt++;
	}
	
	return chkCnt;
}

LONG CDDConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, dCnt = ConfirmTargetCount(doc,tData);
	BaseObject *goal = NULL;
	
	Vector theScale = CDGetScale(op);
	Matrix opM = op->GetMg(), goalM;
	Vector opPos, trgPos, mixPos = Vector(0,0,0), dir;
	Vector oldRot, newRot, rotSet;
	Real dist, mix;
	
	Real totalMix = 0.0;
	
	for(i=0; i<dCnt; i++)
	{
		goal = tData->GetObjectLink(DC_TARGET+i,doc);
		if(goal)
		{
			totalMix += tData->GetReal(DC_DIST_MIX+i);
		}
	}
	if(totalMix == 0.0) return false;
	
	for(i=0; i<dCnt; i++)
	{
		CDContact contact;
		
		Bool doClamp = false;
		goal = tData->GetObjectLink(DC_TARGET+i,doc);
		if(goal)
		{
			goalM = goal->GetMg();
			dist = tData->GetReal(DC_DISTANCE+i);
			mix = tData->GetReal(DC_DIST_MIX+i)/totalMix;
			if (!tData->GetBool(DC_INIT_DISTANCE+i))
			{
				dist = Len(opM.off - goalM.off);
				tData->SetReal(DC_DISTANCE+i,dist);
				tData->SetBool(DC_INIT_DISTANCE+i,true);
			}
			mixPos = opM.off;
			
			Vector localPos, localDir, gScale = GetGlobalScale(goal);
			Matrix sclM = MatrixScale(gScale);
			
			localDir = VNorm(MInv(goalM) * opM.off);
			localPos = sclM * (localDir * dist);
			dist = Len(localPos);

			contact.hit = false;
			
			Real gLen = Len(opM.off - goalM.off);
			if(tData->GetLong(DC_CLAMP_DISTANCE+i) == DC_CLAMP_MIN+i)
			{
				if(gLen < dist)
				{
					doClamp = true;
					dir = VNorm(opM.off - goalM.off);
					mixPos = goalM.off + dir * dist;
					
					contact.hit = true;
					contact.normal = dir;
					contact.point = trgPos;
					contact.constant = VDot(contact.normal, contact.point);
				}
			}
			else if (tData->GetLong(DC_CLAMP_DISTANCE+i) == DC_CLAMP_MAX+i)
			{
				if(gLen > dist)
				{
					doClamp = true;
					dir = VNorm(opM.off - goalM.off);
					mixPos = goalM.off + dir * dist;
					
					contact.hit = true;
					contact.normal = -dir;
					contact.point = trgPos;
					contact.constant = VDot(contact.normal, contact.point);
				}
			}
			else if (tData->GetLong(DC_CLAMP_DISTANCE+i) == DC_CLAMP_BOTH+i)
			{
				doClamp = true;
				dir = VNorm(opM.off - goalM.off);
				mixPos = goalM.off + dir * dist;
					
				contact.hit = true;
				if(gLen > dist)
				{
					contact.normal = -dir;
				}
				else
				{
					contact.normal = dir;
				}
				contact.point = trgPos;
				contact.constant = VDot(contact.normal, contact.point);
			}
			tData->SetBool(DC_CONTACT_HIT+i,contact.hit);
			tData->SetVector(DC_CONTACT_PT+i,contact.point);
			tData->SetVector(DC_CONTACT_NORM+i,contact.normal);
			tData->SetReal(DC_CONTACT_CNST+i,contact.constant);
		}
		
		Real strength = tData->GetReal(DC_DIST_MIX+i);
		opPos = CDBlend(opM.off, mixPos, strength);
		
		if(doClamp)
		{
			opM.off = opPos;
			oldRot = CDGetRot(op);
			op->SetMg(opM);
			newRot = CDGetRot(op);
			rotSet = CDGetOptimalAngle(oldRot, newRot, op);
			CDSetRot(op,rotSet);
			CDSetScale(op,theScale);
		}
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDDConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, dCnt = tData->GetLong(DC_TARGET_COUNT);
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(DC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	for (i=0; i<dCnt; i++)
	{
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if (!description->SetParameter(DescLevel(DC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(DC_ID_TARGET))) return false;

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(DC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(DC_ID_TARGET))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(DC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(DC_LINK_GROUP+i))) return false;
		
		BaseContainer bc4 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc4.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc4.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i));
		bc4.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc4.SetReal(DESC_MIN,0.0);
		bc4.SetReal(DESC_MAX,1.0);
		bc4.SetReal(DESC_STEP,0.01);
		bc4.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(DC_DIST_MIX+i, DTYPE_REAL, 0), bc4, DescLevel(DC_LINK_GROUP+i))) return true;
		
		BaseContainer subgroup2 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup2.SetLong(DESC_COLUMNS, 3);
		if(!description->SetParameter(DescLevel(DC_CLAMP_GROUP+i, DTYPE_GROUP, 0), subgroup2, DescLevel(DC_ID_TARGET))) return true;

		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_DISTANCE)+"."+CDLongToString(i));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_METER);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_STEP,0.1);
		if (!description->SetParameter(DescLevel(DC_DISTANCE+i, DTYPE_REAL, 0), bc3, DescLevel(DC_CLAMP_GROUP+i))) return true;

		BaseContainer bc5 = GetCustomDataTypeDefault(DTYPE_LONG);
		bc5.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LONG);
		bc5.SetString(DESC_NAME, GeLoadString(IDS_CLAMP)+"."+CDLongToString(i));
		BaseContainer cycle;
		cycle.SetString(DC_CLAMP_MIN+i, GeLoadString(IDS_CLAMP_MIN));
		cycle.SetString(DC_CLAMP_MAX+i, GeLoadString(IDS_CLAMP_MAX));
		cycle.SetString(DC_CLAMP_BOTH+i, GeLoadString(IDS_CLAMP_BOTH));
		bc5.SetContainer(DESC_CYCLE, cycle);
		if (!description->SetParameter(DescLevel(DC_CLAMP_DISTANCE+i, DTYPE_LONG, 0), bc5, DescLevel(DC_CLAMP_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDDConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	LONG i, dCnt = tData->GetLong(DC_TARGET_COUNT);
	switch (id[0].id)
	{
		case DC_ADD_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case DC_SUB_DIST:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	
	for(i=0; i<dCnt; i++)
	{
		if(id[0].id == DC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == DC_DISTANCE+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == DC_CLAMP_DISTANCE+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == IDS_CLAMP_BOTH)
		{
			if(dCnt > 1) return false;
			else return true;
		}
	}

	return true;
}
Bool RegisterCDDConstraintPlugin(void)
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
	
	// decide by name if the plugin shall be registered - just for user convenience  TAG_MULTIPLE|
	String name=GeLoadString(IDS_CDDCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDDCONSTRAINTPLUGIN,name,info,CDDConstraintPlugin::Alloc,"tCDDConstraint","CDDConstraint.tif",1);
}
