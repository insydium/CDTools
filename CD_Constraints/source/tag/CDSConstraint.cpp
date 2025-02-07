//	Cactus Dan's Constraints plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

//#include "CDCompatibility.h"
#include "tCDSConstraint.h"
#include "CDConstraint.h"

#define MAX_ADD 	10

enum
{
	//SC_PURCHASE			= 1000,
	
	//SC_OFFSETS				= 1009,
	
	SC_TARGET_A_OLD			= 1010,
	SC_TARGET_B_OLD			= 1011,
	//SC_AB_MIX				= 1012,
	
	//SC_AXIS_X				= 1013,
	//SC_OFFSET_X			= 1014,
	//SC_AXIS_Y				= 1015,
	//SC_OFFSET_Y			= 1016,
	//SC_AXIS_Z				= 1017,
	//SC_OFFSET_Z			= 1018,
	
	//SC_STRENGTH			= 1019,
	//SC_SHOW_LINES			= 1020,
	//SC_LINE_COLOR			= 1021,

	//SC_USE_AB_MIX			= 1023,

	SC_COUNT			= 1030,
	//SC_ADD_SCA				= 1031,
	//SC_SUB_SCA				= 1032,
	SC_DISK_LEVEL				= 1033,					
	
	SC_TARGET_COUNT			= 1040,
	
	//SC_ID_TARGET			= 1050,

	//SC_LINK_GROUP			= 2000,
	//SC_LINE_ADD			= 3000,

	//SC_TARGET				= 4000,
	
	//SC_SCA_MIX				= 5000,
};

class CDSConstraintPlugin : public CDTagData
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
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDSConstraintPlugin); }
};

Bool CDSConstraintPlugin::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(SC_AXIS_X,true);
	tData->SetBool(SC_AXIS_Y,true);
	tData->SetBool(SC_AXIS_Z,true);
	tData->SetBool(SC_SHOW_LINES,true);
	tData->SetVector(SC_LINE_COLOR, Vector(1,0,0));

	tData->SetReal(SC_STRENGTH,1.00);
	tData->SetReal(SC_AB_MIX,0.5);

	tData->SetLong(SC_TARGET_COUNT,1);
	tData->SetReal(SC_SCA_MIX,1.00);
		
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

Bool CDSConstraintPlugin::Read(GeListNode* node, HyperFile* hf, LONG level)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	tData->SetLong(SC_DISK_LEVEL,level);
	
	return true;
}

void CDSConstraintPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
			
			tData->SetBool(SC_USE_AB_MIX+T_LST,tData->GetBool(SC_USE_AB_MIX));
			tData->SetBool(SC_AXIS_X+T_LST,tData->GetBool(SC_AXIS_X));
			tData->SetBool(SC_AXIS_Y+T_LST,tData->GetBool(SC_AXIS_Y));
			tData->SetBool(SC_AXIS_Z+T_LST,tData->GetBool(SC_AXIS_Z));
			tData->SetReal(SC_OFFSET_X+T_LST,tData->GetReal(SC_OFFSET_X));
			tData->SetReal(SC_OFFSET_Y+T_LST,tData->GetReal(SC_OFFSET_Y));
			tData->SetReal(SC_OFFSET_Z+T_LST,tData->GetReal(SC_OFFSET_Z));
			
			tData->SetLong(SC_TARGET_COUNT+T_LST,tData->GetLong(SC_TARGET_COUNT));
			LONG i, scCnt = tData->GetLong(SC_TARGET_COUNT);
			for(i=0; i<scCnt; i++)
			{
				tData->SetLink(SC_TARGET+i+T_LST,tData->GetLink(SC_TARGET+i,doc));
			}
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDSConstraintPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetBool(SC_USE_AB_MIX,tData->GetBool(SC_USE_AB_MIX+T_LST));
		tData->SetBool(SC_AXIS_X,tData->GetBool(SC_AXIS_X+T_LST));
		tData->SetBool(SC_AXIS_Y,tData->GetBool(SC_AXIS_Y+T_LST));
		tData->SetBool(SC_AXIS_Z,tData->GetBool(SC_AXIS_Z+T_LST));
		tData->SetReal(SC_OFFSET_X,tData->GetReal(SC_OFFSET_X+T_LST));
		tData->SetReal(SC_OFFSET_Y,tData->GetReal(SC_OFFSET_Y+T_LST));
		tData->SetReal(SC_OFFSET_Z,tData->GetReal(SC_OFFSET_Z+T_LST));
		
		tData->SetLong(SC_TARGET_COUNT,tData->GetLong(SC_TARGET_COUNT+T_LST));
		LONG i, scCnt = tData->GetLong(SC_TARGET_COUNT);
		for(i=0; i<scCnt; i++)
		{
			tData->SetLink(SC_TARGET+i,tData->GetLink(SC_TARGET+i+T_LST,doc));
		}
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Bool CDSConstraintPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseDocument *doc = bh->GetDocument();
	BaseContainer *tData = tag->GetDataInstance();
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return true;

	if (!tData->GetBool(SC_SHOW_LINES)) return true;

	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	LONG i, scCnt = tData->GetLong(SC_TARGET_COUNT);
	BaseObject *goal = NULL;
	
	bd->SetPen(tData->GetVector(SC_LINE_COLOR));
	Matrix goalMatrix, opMatrix;
	bd->SetMatrix_Matrix(op, opMatrix);
	
	for(i=0; i<scCnt; i++)
	{
		goal = tData->GetObjectLink(SC_TARGET+i,doc);
		if(goal)
		{
			bd->SetMatrix_Matrix(goal, goalMatrix);
			CDDrawLine(bd,op->GetMg().off, goal->GetMg().off);
		}
	}
	
	return true;
}

void CDSConstraintPlugin::SetDefaultTargetParameters(BaseContainer *tData, LONG i)
{
	tData->SetLink(SC_TARGET+i,NULL);
	tData->SetReal(SC_SCA_MIX+i,1.00);
}

void CDSConstraintPlugin::CopyTargetParameters(BaseDocument *doc, BaseContainer *tData, LONG src, LONG dst)
{
	tData->SetLink(SC_TARGET+dst,tData->GetObjectLink(SC_TARGET+src,doc));
	tData->SetReal(SC_SCA_MIX+dst,tData->GetReal(SC_SCA_MIX+src));
}

void CDSConstraintPlugin::ConvertOldData(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, oldCnt = 0;
	
	BaseObject *goalA = tData->GetObjectLink(SC_TARGET_A_OLD,doc);
	if(goalA)
	{
		tData->SetLink(SC_TARGET,goalA);
		tData->SetReal(SC_SCA_MIX,1.00);
		oldCnt++;
		tData->SetLong(SC_TARGET_COUNT,oldCnt);
	}
	BaseObject *goalB = tData->GetObjectLink(SC_TARGET_B_OLD,doc);
	if(goalB)
	{
		tData->SetLink(SC_TARGET+1,goalB);
		tData->SetReal(SC_SCA_MIX,1.0-tData->GetReal(SC_AB_MIX));
		tData->SetReal(SC_SCA_MIX+1,tData->GetReal(SC_AB_MIX));
		tData->SetBool(SC_USE_AB_MIX,true);
		oldCnt++;
		tData->SetLong(SC_TARGET_COUNT,oldCnt);
	}
	
	if(oldCnt == 0) tData->SetLong(SC_TARGET_COUNT,tData->GetLong(SC_COUNT) + 1);
	else
	{
		for(i=oldCnt; i<MAX_ADD; i++)
		{
			if(tData->GetObjectLink(SC_TARGET+i,doc)) oldCnt++;
		}
		tData->SetLong(SC_TARGET_COUNT,oldCnt);
	}
}

Bool CDSConstraintPlugin::Message(GeListNode *node, LONG type, void *data)
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
			
			if(doc && tData->GetLong(SC_DISK_LEVEL) < 1) ConvertOldData(doc,tData);
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
	
	LONG scCnt = tData->GetLong(SC_TARGET_COUNT);
	switch (type)
	{
		case ID_CDFREEZETRANSFORMATION:
		{
			Vector *trnsSca = (Vector*)data;
			if(trnsSca)
			{
				Vector sca = *trnsSca, offV;
				offV.x = tData->GetReal(SC_OFFSET_X) * sca.x;
				offV.y = tData->GetReal(SC_OFFSET_Y) * sca.y;
				offV.z = tData->GetReal(SC_OFFSET_Z) * sca.z;
				
				if(tData->GetBool(SC_AXIS_X)) tData->SetReal(SC_OFFSET_X,offV.x);
				if(tData->GetBool(SC_AXIS_Y)) tData->SetReal(SC_OFFSET_Y,offV.y);
				if(tData->GetBool(SC_AXIS_Z)) tData->SetReal(SC_OFFSET_Z,offV.z);
			}
			break;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==SC_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if (dc->id[0].id==SC_ADD_SCA)
			{
				if(tData->GetBool(T_REG))
				{
					if (scCnt < MAX_ADD)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,scCnt);
						scCnt++;
						tData->SetLong(SC_TARGET_COUNT,scCnt);
					}
				}
			}
			else if (dc->id[0].id==SC_SUB_SCA)
			{
				if(tData->GetBool(T_REG))
				{
					if (scCnt > 1)
					{
						CDAddUndo(doc,CD_UNDO_CHANGE_SMALL, tag);
						SetDefaultTargetParameters(tData,scCnt);
						scCnt--;
						tData->SetLong(SC_TARGET_COUNT,scCnt);
					}
				}
			}
			break;
		}
	}

	return true;
}

LONG CDSConstraintPlugin::ConfirmTargetCount(BaseDocument *doc, BaseContainer *tData)
{
	LONG i, scCnt = tData->GetLong(SC_TARGET_COUNT);
	BaseObject *trg = NULL;
	
	LONG chkCnt = 0;
	for(i=0; i<scCnt; i++)
	{
		trg = tData->GetObjectLink(SC_TARGET+i,doc);
		if(!trg)
		{
			LONG j = i;
			while(!trg && j < scCnt)
			{
				j++;
				trg = tData->GetObjectLink(SC_TARGET+j,doc);
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

LONG CDSConstraintPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;

	LONG i, scCnt = ConfirmTargetCount(doc,tData);
	BaseObject *goal = NULL;
	
	Vector theScale, opScale = CDGetScale(op), mixScale = Vector(0,0,0);
	
	// Get the total constraint mix
	Bool ABMixAvailable = true;
	Real totalMix = 0.0;
	if(scCnt < 2) ABMixAvailable = false;
	for(i=0; i<scCnt; i++)
	{
		goal = tData->GetObjectLink(SC_TARGET+i,doc);
		if(goal)
		{
			totalMix += tData->GetReal(SC_SCA_MIX+i);
		}
		else ABMixAvailable = false;
	}
	if(totalMix == 0.0) return false;
	
	if(!ABMixAvailable) tData->SetBool(SC_USE_AB_MIX,false);

	for(i=0; i<scCnt; i++)
	{
		Real maxVal = 0.0, minVal = 1.0;
		if(scCnt-1 != 0) minVal = 1.0/(Real)(scCnt-1);//Check for division by zero
		
		Real mixVal, value = Abs((minVal*i) - tData->GetReal(SC_AB_MIX));
		if(value > minVal) value = minVal;
		if ((maxVal-minVal) != 0) mixVal = (value-minVal)/(maxVal-minVal);//Check for division by zero
		else mixVal = 0.0;

		goal = tData->GetObjectLink(SC_TARGET+i,doc);
		if(goal)
		{
			if(tData->GetBool(SC_USE_AB_MIX)) mixScale += CDGetScale(goal) * mixVal;
			else mixScale += CDGetScale(goal) * (tData->GetReal(SC_SCA_MIX+i)/totalMix);
		}
	}
	mixScale.x += tData->GetReal(SC_OFFSET_X);
	mixScale.y += tData->GetReal(SC_OFFSET_Y);
	mixScale.z += tData->GetReal(SC_OFFSET_Z);

	theScale = opScale;
	Real theStrength = tData->GetReal(SC_STRENGTH);
	if(tData->GetBool(SC_AXIS_X)) theScale.x = CDBlend(opScale.x, mixScale.x, theStrength);
	if(tData->GetBool(SC_AXIS_Y)) theScale.y = CDBlend(opScale.y, mixScale.y, theStrength);
	if(tData->GetBool(SC_AXIS_Z)) theScale.z = CDBlend(opScale.z, mixScale.z, theStrength);
	
	if(!VEqual(theScale,CDGetScale(op),0.001)) CDSetScale(op,theScale);
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDSConstraintPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag *tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	LONG i, scCnt = tData->GetLong(SC_TARGET_COUNT);
	
	if (!description->LoadDescription(node->GetType())) return false;

	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(SC_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}
	
	for (i=0; i<scCnt; i++)
	{
		BaseContainer bc1 = GetCustomDataTypeDefault(DTYPE_SEPARATOR);
		bc1.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_SEPARATOR);
		bc1.SetBool(DESC_SEPARATORLINE,true);
		if (!description->SetParameter(DescLevel(SC_LINE_ADD+i, DTYPE_SEPARATOR, 0),bc1,DescLevel(SC_ID_TARGET))) return false;

		BaseContainer subgroup1 = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup1.SetLong(DESC_COLUMNS, 2);
		if(!description->SetParameter(DescLevel(SC_LINK_GROUP+i, DTYPE_GROUP, 0), subgroup1, DescLevel(SC_ID_TARGET))) return true;
		
		BaseContainer bc2 = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
		bc2.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
		bc2.SetString(DESC_NAME,GeLoadString(IDS_TARGET)+"."+CDLongToString(i));
		bc2.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
		bc2.SetBool(DESC_REMOVEABLE,true);
		if (!description->SetParameter(DescLevel(SC_TARGET+i,DTYPE_BASELISTLINK,0),bc2,DescLevel(SC_LINK_GROUP+i))) return false;
		
		BaseContainer bc3 = GetCustomDataTypeDefault(DTYPE_REAL);
		bc3.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_REAL);
		bc3.SetString(DESC_NAME, GeLoadString(IDS_MIX)+"."+CDLongToString(i));
		bc3.SetLong(DESC_UNIT, DESC_UNIT_PERCENT);
		bc3.SetReal(DESC_MIN,0.0);
		bc3.SetReal(DESC_MAX,1.0);
		bc3.SetReal(DESC_STEP,0.01);
		bc3.SetReal(DESC_DEFAULT,1.0);
		if (!description->SetParameter(DescLevel(SC_SCA_MIX+i, DTYPE_REAL, 0), bc3, DescLevel(SC_LINK_GROUP+i))) return true;
	}

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDSConstraintPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	BaseDocument *doc = node->GetDocument(); if(!doc) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	LONG i, scCnt = tData->GetLong(SC_TARGET_COUNT);
	switch (id[0].id)
	{
		case SC_AXIS_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SC_AXIS_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SC_AXIS_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SC_OFFSET_X:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SC_AXIS_X);
		case SC_OFFSET_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SC_AXIS_Y);
		case SC_OFFSET_Z:
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(SC_AXIS_Z);
		case SC_USE_AB_MIX:
		{
			if(!tData->GetBool(T_REG)) return false;
			else
			{
				Bool ABMixAvailable = true;
				if(scCnt < 2) ABMixAvailable = false;
				for(i=0; i<scCnt; i++)
				{
					BaseObject *goal = tData->GetObjectLink(SC_TARGET+i,doc);
					if(!goal) ABMixAvailable = false;
				}
				return ABMixAvailable;
			}
		}
		case SC_AB_MIX:
			return tData->GetBool(SC_USE_AB_MIX);
		case SC_ADD_SCA:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case SC_SUB_SCA:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	for(i=0; i<scCnt; i++)
	{
		if(id[0].id == SC_TARGET+i)
		{
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		}
		else if(id[0].id == SC_SCA_MIX+i)
		{
			BaseObject *goal = tData->GetObjectLink(SC_TARGET+i,doc);
			if(!goal) return false;
			else if(tData->GetBool(SC_USE_AB_MIX)) return false;
			else return true;
		}
	}
	return true;
}

Bool RegisterCDSConstraintPlugin(void)
{
	if(CDFindPlugin(ID_CDSCONSTRAINTPLUGIN,CD_TAG_PLUGIN)) return true;
	
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
	String name=GeLoadString(IDS_CDSCONSTRAINT); if (!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDSCONSTRAINTPLUGIN,name,info,CDSConstraintPlugin::Alloc,"tCDSConstraint","CDSConstraint.tif",1);
}
