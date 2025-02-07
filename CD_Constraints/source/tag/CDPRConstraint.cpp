//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDPRConstraint.h"
#include "CDConstraint.h"

enum
{
	//PRC_PURCHASE			= 1000,
	
	//PRC_TARGET				= 1010,
	//PRC_EDIT_POSITION		= 1011,
	//PRC_RESET_POSITION		= 1012,
	PRC_REF_MATRIX			= 1013,
	//PRC_SHOW_LINES			= 1014,
	//PRC_TARGET_SCALE		= 1015,
	//PRC_LINE_COLOR			= 1016,
	//PRC_STRENGTH			= 1017,
	PRC_POSITION_SET		= 1018,
	//PRC_LINEAR_INTERP		= 1019,
	//PRC_TARGET_ID			= 1020,
	PRC_GOAL_MATRIX			= 1021,
	//PRC_TARGET_ROT			= 1022,
	
	PRC_G_SCALE				= 1100
};

class CDPRConstraintPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);

public:
	Bool	editPosition;
	Bool	setPosition;
	
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDPRConstraintPlugin); }
};

Bool CDPRConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	editPosition = false;
	setPosition = false;
	tData->SetReal(PRC_STRENGTH,1.00);
	tData->SetBool(PRC_SHOW_LINES,true);
	tData->SetBool(PRC_TARGET_ROT,true);
	tData->SetBool(PRC_TARGET_SCALE,true);
	tData->SetVector(PRC_LINE_COLOR, Vector(1,0,0));
	tData->SetBool(PRC_LINEAR_INTERP,true);
			
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

Bool CDPRConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	BaseObject *goal = tData->GetObjectLink(PRC_TARGET,doc); if (!goal) return true;
	
	if (!tData->GetBool(PRC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	Vector opPosition = op->GetMg().off;
	Vector goalPosition = goal->GetMg().off;
	
	bd->SetPen(tData->GetVector(PRC_LINE_COLOR));
	CDDrawLine(bd,opPosition, goalPosition);
	
	return true;
}

void CDPRConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetLink(PRC_TARGET+T_LST,tData->GetLink(PRC_TARGET,doc));
			tData->SetBool(PRC_TARGET_ROT+T_LST,tData->GetBool(PRC_TARGET_ROT));
			tData->SetBool(PRC_TARGET_SCALE+T_LST,tData->GetBool(PRC_TARGET_SCALE));
			tData->SetBool(PRC_LINEAR_INTERP+T_LST,tData->GetBool(PRC_LINEAR_INTERP));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDPRConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	BaseDocument *doc = node->GetDocument();
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
	
	switch (type)
	{
		case ID_CDFREEZETRANSFORMATION:
		{
			Vector *trnsSca = (Vector*)data;
			if(trnsSca)
			{
				BaseObject *goal = tData->GetObjectLink(PRC_TARGET,doc);
				if(goal)
				{
					Vector gScale = GetGlobalScale(goal);
					if(!VEqual(gScale,tData->GetVector(PRC_G_SCALE),0.001))
					{
						Matrix refM = MatrixScale(tData->GetVector(PRC_G_SCALE)) * tData->GetMatrix(PRC_REF_MATRIX);
						tData->SetMatrix(PRC_REF_MATRIX,refM);
					}
				}
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==PRC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==PRC_EDIT_POSITION)
			{
				if(tData->GetBool(T_REG)) editPosition = true;
			}
			else if (dc->id[0].id==PRC_RESET_POSITION)
			{
				if(tData->GetBool(T_REG))
				{
					tData->SetBool(PRC_POSITION_SET, false);
					setPosition = false;
					editPosition = false;
				}
			}
			break;
		}
	}

	return true;
}

Bool CDPRConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetLink(PRC_TARGET,tData->GetLink(PRC_TARGET+T_LST,doc));
		tData->SetBool(PRC_TARGET_ROT,tData->GetBool(PRC_TARGET_ROT+T_LST));
		tData->SetBool(PRC_TARGET_SCALE,tData->GetBool(PRC_TARGET_SCALE+T_LST));
		tData->SetBool(PRC_LINEAR_INTERP,tData->GetBool(PRC_LINEAR_INTERP+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

LONG CDPRConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	BaseObject *goal = tData->GetObjectLink(PRC_TARGET,doc);
	if (!goal) 
	{
		tData->SetBool(PRC_POSITION_SET, false);
		return false;
	}
	Vector gScale = GetGlobalScale(goal);
	tData->SetVector(PRC_G_SCALE,gScale);
	
	Matrix refM, opM, goalM, trgM, rotM;
	Vector dir, opSca = CDGetScale(op);
	Vector oldRot, newRot, rotSet;
	CDQuaternion opQ, trgQ, newQ;
	Real length, strength = tData->GetReal(PRC_STRENGTH);
	
	opM = op->GetMg();
	opQ.SetMatrix(opM);

	goalM = goal->GetMg();
	
	if (goal != tData->GetObjectLink(PRC_TARGET_ID,doc))
	{
		tData->SetBool(PRC_POSITION_SET, false);
		tData->SetLink(PRC_TARGET_ID, goal);
	}
	
	if (!tData->GetBool(PRC_POSITION_SET))
	{
		refM = MInv(goalM) * opM;
		CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
		tData->SetMatrix(PRC_REF_MATRIX,refM);
		tData->SetBool(PRC_POSITION_SET, true);
		setPosition = true;
	}
	
	if (!editPosition)
	{
		trgM = goalM * tData->GetMatrix(PRC_REF_MATRIX);
		
		if (!tData->GetBool(PRC_TARGET_ROT))
		{
			rotM.off = trgM.off;
			rotM.v1 = opM.v1;
			rotM.v2 = opM.v2;
			rotM.v3 = opM.v3;
		}
		else
		{
			trgQ.SetMatrix(trgM);
			newQ = CDQSlerp(opQ, trgQ, strength);
			rotM = newQ.GetMatrix();
		}
		
		rotM.off = CDBlend(opM.off, trgM.off, strength);
		if (!tData->GetBool(PRC_LINEAR_INTERP))
		{
			length = Len(trgM.off - goalM.off);
			dir = VNorm(rotM.off - goalM.off);
			rotM.off = goalM.off + dir * length;
		}
		
		oldRot = CDGetRot(op);
		op->SetMg(rotM);
		newRot = CDGetRot(op);
		rotSet = CDGetOptimalAngle(oldRot, newRot, op);
		CDSetRot(op,rotSet);
		
		if (!tData->GetBool(PRC_TARGET_SCALE)) CDSetScale(op,opSca);
		else SetGlobalScale(op,gScale);
	}
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDPRConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();

	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(PRC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDPRConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case PRC_TARGET:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PRC_EDIT_POSITION:
			if(!tData->GetBool(T_REG)) return false;
			else if(editPosition) return false;
			else return true;
		case PRC_RESET_POSITION:
			if(!tData->GetBool(T_REG)) return false;
			else return editPosition;
		case PRC_TARGET_ROT:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PRC_TARGET_SCALE:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case PRC_LINEAR_INTERP:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDPRConstraintPlugin(void)
{
	if(CDFindPlugin(ID_CDPRCONSTRAINTPLUGIN,CD_TAG_PLUGIN)) return true;
	
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
	String name=GeLoadString(IDS_CDPRCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDPRCONSTRAINTPLUGIN,name,info,CDPRConstraintPlugin::Alloc,"tCDPRConstraint","CDPRConstraint.tif",0);
}
