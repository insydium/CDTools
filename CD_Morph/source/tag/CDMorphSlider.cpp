//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_description.h"
#include "customgui_priority.h"

#include "tCDMorphSlider.h"
#include "CDMorph.h"
#include "CDTagData.h"

enum
{
	//MS_BOX_POSITION				= 1000,
	//MS_BOX_SIZE_X					= 1001,
	//MS_BOX_SIZE_Y					= 1002,
	//MS_BOX_COLOR					= 1003,
	//MS_SHOW_BOX					= 1004,
	//MS_SHOW_DIAGONAL				= 1005,
	
	//MS_MORPH_CONTROL				= 1010,
	//MS_HOME_POSITION				= 1011,
	//MS_POS_X_LINK				= 1012,
	//MS_NEG_X_LINK				= 1013,
	//MS_POS_Y_LINK				= 1014,
	//MS_NEG_Y_LINK				= 1015,
	
	//MS_OUTPUT_VALUES				= 1020,
	//MS_OUTPUT_X					= 1021,
	//MS_OUTPUT_Y					= 1022,
	//MS_POSITIVE_X					= 1023,
	//MS_POSITIVE_Y					= 1024,
	
	MS_INITIAL_POSITION			= 1025,
	
	//MS_NEGATIVE_X					= 1026,
	//MS_NEGATIVE_Y					= 1027,
	
	MS_INITIAL_SIZE				= 1028,
	MS_OP_SIZE						= 1029,
	MS_OP_ID						= 1030,

	MS_REF_MATRIX				= 1031,
	MS_LOCAL_MATRIX				= 1032,
	
	//MS_MIX_X_Y						= 1033,
	//MS_XPOS_YNEG_LINK			= 1034,
	//MS_XNEG_YNEG_LINK			= 1035,
	//MS_YPOS_XNEG_LINK			= 1036,
	//MS_YNEG_XNEG_LINK			= 1037,
	
	MS_PARENT_ID					= 1038,
	MS_BOX_ROTATION				= 1039,
	MS_LINK_COUNT					= 1040
};

class CDMorphSliderPlugin : public CDTagData
{
private:
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
	Matrix GetBoxMg(BaseContainer *bc, BaseObject *op);
	Matrix GetParentMatrix(BaseObject *op, BaseContainer *tData);

public:
	Matrix sliderMatrix;
		
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	virtual Bool CDGetDDescription(GeListNode *node, Description *description, LONG &flags);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDMorphSliderPlugin); }
};

Bool CDMorphSliderPlugin::Init(GeListNode *node)
{
	BaseTag			*tag  = (BaseTag*)node;
	BaseContainer	*tData = tag->GetDataInstance();
		
	tData->SetBool(MS_SHOW_BOX,true);
	tData->SetBool(MS_SHOW_DIAGONAL,true);
	tData->SetReal(MS_BOX_SIZE_X,50.0);
	tData->SetReal(MS_BOX_SIZE_Y,50.0);
	tData->SetVector(MS_BOX_COLOR, Vector(0.75,0.75,0.75));
	
	tData->SetBool(T_REG,false);
	tData->SetBool(T_SET,false);

	GeData d;
	if(CDGetParameter(node,DescLevel(EXPRESSION_PRIORITY),d))
	{
		PriorityData *pd = (PriorityData*)d.GetCustomDataType(CUSTOMGUI_PRIORITY_DATA);
		{
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT,false);
		}
		CDSetParameter(node,DescLevel(EXPRESSION_PRIORITY),d);
	}

	return true;
}

Bool CDMorphSliderPlugin::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	BaseContainer *tData = tag->GetDataInstance();

	if(!tData->GetBool(MS_SHOW_BOX)) return true;
	
	CDSetBaseDrawMatrix(bd, NULL, Matrix());
	
	BaseContainer *opData = op->GetDataInstance();

	Vector pPosition, bxPos, upLt, lowLt, upRt, lowRt;
	Matrix pM, bxM, opM = op->GetMg(), posM, transM;
	
	Vector bAxisX = Vector(1,0,0), bAxisY = Vector(0,1,0), bAxisZ = Vector(0,0,1);
	
	// Check for scale
	Real opSize, opRatio;
	if(op->GetType() != Onull)
	{
		opSize = op->GetRad().x;
		
		if(opSize == 0) opSize = 1.0;   //check for division by 0
		opRatio = op->GetRad().y / opSize;
	}
	else
	{
		opSize = opData->GetReal(NULLOBJECT_RADIUS);
		opRatio = opData->GetReal(NULLOBJECT_ASPECTRATIO);
	}

	pM = GetParentMatrix(op,tData);
	transM = MInv(pM) * opM;
	transM.off = tData->GetVector(MS_BOX_POSITION);
	
	Real boxSizeX = tData->GetReal(MS_BOX_SIZE_X)/2 + opSize;
	Real boxSizeY = tData->GetReal(MS_BOX_SIZE_Y)/2 + opSize*opRatio;
	
	// Get the upper left corner position
	upLt.x = transM.off.x - boxSizeX;
	upLt.y = transM.off.y + boxSizeY;
	upLt.z = transM.off.z;
	//Convert to Parent's coordinates
	bxM = Matrix(upLt, bAxisX, bAxisY, bAxisZ);
	posM = pM * bxM;
	upLt = posM.off;
	
	// Get the lower left corner position
	lowLt.x = transM.off.x - boxSizeX;
	lowLt.y = transM.off.y - boxSizeY;
	lowLt.z = transM.off.z;
	//Convert to Parent's coordinates
	bxM = Matrix(lowLt, bAxisX, bAxisY, bAxisZ);
	posM = pM * bxM;
	lowLt = posM.off;
	
	// Get the upper right corner position
	upRt.x = transM.off.x + boxSizeX;
	upRt.y = transM.off.y + boxSizeY;
	upRt.z = transM.off.z;
	//Convert to Parent's coordinates
	bxM = Matrix(upRt, bAxisX, bAxisY, bAxisZ);
	posM = pM * bxM;
	upRt = posM.off;
	
	// Get the lower right corner position
	lowRt.x = transM.off.x + boxSizeX;
	lowRt.y = transM.off.y - boxSizeY;
	lowRt.z = transM.off.z;
	//Convert to Parent's coordinates
	bxM = Matrix(lowRt, bAxisX, bAxisY, bAxisZ);
	posM = pM * bxM;
	lowRt = posM.off;
	

	bd->SetPen(tData->GetVector(MS_BOX_COLOR));
	CDDrawLine(bd,upLt, upRt);
	CDDrawLine(bd,upRt, lowRt);
	CDDrawLine(bd,lowRt, lowLt);
	CDDrawLine(bd,lowLt, upLt);
	
	Vector midPt = transM.off;
	if(tData->GetBool(MS_SHOW_DIAGONAL))
	{
		if(tData->GetReal(MS_BOX_SIZE_X)!=0 && tData->GetReal(MS_BOX_SIZE_Y)!=0)
		{
			if(tData->GetBool(MS_POSITIVE_X) && !tData->GetBool(MS_POSITIVE_Y))
			{
				if(!tData->GetBool(MS_NEGATIVE_X))
				{
					//Move midPt to the left
					midPt.x = transM.off.x - boxSizeX;
					//Convert to Parent's coordinates;
					bxM = Matrix(midPt, bAxisX, bAxisY, bAxisZ);
					posM = pM * bxM;
					midPt = posM.off;
					// Draw diagonals
					CDDrawLine(bd,midPt, upRt);
					CDDrawLine(bd,midPt, lowRt);
				}
				else
				{
					//Move midPt to the right
					midPt.x = transM.off.x + boxSizeX;
					//Convert to Parent's coordinates;
					bxM = Matrix(midPt, bAxisX, bAxisY, bAxisZ);
					posM = pM * bxM;
					midPt = posM.off;
					// Draw diagonals
					CDDrawLine(bd,midPt, upLt);
					CDDrawLine(bd,midPt, lowLt);
				}
			}
			else if(!tData->GetBool(MS_POSITIVE_X) && tData->GetBool(MS_POSITIVE_Y))
			{
				if(!tData->GetBool(MS_NEGATIVE_Y))
				{
					//Move midPt to the bottom
					midPt.y = transM.off.y - boxSizeY;
					//Convert to Parent's coordinates;
					bxM = Matrix(midPt, bAxisX, bAxisY, bAxisZ);
					posM = pM * bxM;
					midPt = posM.off;
					// Draw diagonals
					CDDrawLine(bd,midPt, upRt);
					CDDrawLine(bd,midPt, upLt);
				}
				else
				{
					//Move midPt to the top
					midPt.y = transM.off.y + boxSizeY;
					//Convert to Parent's coordinates;
					bxM = Matrix(midPt, bAxisX, bAxisY, bAxisZ);
					posM = pM * bxM;
					midPt = posM.off;
					// Draw diagonals
					CDDrawLine(bd,midPt, lowRt);
					CDDrawLine(bd,midPt, lowLt);
				}
			}
			else if(tData->GetBool(MS_POSITIVE_X) && tData->GetBool(MS_POSITIVE_Y))
			{
				if(!tData->GetBool(MS_NEGATIVE_X) && tData->GetBool(MS_NEGATIVE_Y)) CDDrawLine(bd,lowRt, upLt);
				else if(tData->GetBool(MS_NEGATIVE_X) && !tData->GetBool(MS_NEGATIVE_Y)) CDDrawLine(bd,lowRt, upLt);
				else CDDrawLine(bd,lowLt, upRt);
			}
			else
			{
				CDDrawLine(bd,lowLt, upRt);
				CDDrawLine(bd,upLt, lowRt);
			}
		}
	}


	return true;
}

void CDMorphSliderPlugin::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdmnr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdmnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdmnr.FindFirst("-",&pos);
	while(h)
	{
		cdmnr.Delete(pos,1);
		h = cdmnr.FindFirst("-",&pos);
	}
	cdmnr.ToUpper();
	kb = cdmnr.SubStr(pK,2);
	
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
			if(op) tData->SetLink(T_PID,op->GetUp());
			
			tData->SetVector(MS_BOX_POSITION+T_LST,tData->GetVector(MS_BOX_POSITION));
			tData->SetReal(MS_BOX_SIZE_X+T_LST,tData->GetReal(MS_BOX_SIZE_X));
			tData->SetReal(MS_BOX_SIZE_Y+T_LST,tData->GetReal(MS_BOX_SIZE_Y));
			
			tData->SetBool(MS_MIX_X_Y+T_LST,tData->GetBool(MS_MIX_X_Y));
			tData->SetBool(MS_POSITIVE_X+T_LST,tData->GetBool(MS_POSITIVE_X));
			tData->SetBool(MS_POSITIVE_Y+T_LST,tData->GetBool(MS_POSITIVE_Y));
			tData->SetBool(MS_NEGATIVE_X+T_LST,tData->GetBool(MS_NEGATIVE_X));
			tData->SetBool(MS_NEGATIVE_Y+T_LST,tData->GetBool(MS_NEGATIVE_Y));
			
			tData->SetLink(MS_POS_X_LINK+T_LST,tData->GetLink(MS_POS_X_LINK,doc));
			tData->SetLink(MS_NEG_X_LINK+T_LST,tData->GetLink(MS_NEG_X_LINK,doc));
			tData->SetLink(MS_POS_Y_LINK+T_LST,tData->GetLink(MS_POS_Y_LINK,doc));
			tData->SetLink(MS_NEG_Y_LINK+T_LST,tData->GetLink(MS_NEG_Y_LINK,doc));

			tData->SetLink(MS_XPOS_YNEG_LINK+T_LST,tData->GetLink(MS_XPOS_YNEG_LINK,doc));
			tData->SetLink(MS_XNEG_YNEG_LINK+T_LST,tData->GetLink(MS_XNEG_YNEG_LINK,doc));
			tData->SetLink(MS_YPOS_XNEG_LINK+T_LST,tData->GetLink(MS_YPOS_XNEG_LINK,doc));
			tData->SetLink(MS_YNEG_XNEG_LINK+T_LST,tData->GetLink(MS_YNEG_XNEG_LINK,doc));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDMorphSliderPlugin::Message(GeListNode *node, LONG type, void *data)
{
	BaseTag *tag  = (BaseTag*)node; if(!tag) return true;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	BaseObject *op = tag->GetObject(); if(!op) return true;

	Matrix pM, bxM, transM, targM, opM = op->GetMg(), posM;
					
	BaseDocument *doc = node->GetDocument();
	switch (type)
	{
		case MSG_MULTI_DOCUMENTIMPORTED:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdmnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdmnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdmnr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDM_SERIAL_SIZE];
			String cdmnr;
			
			CDReadPluginInfo(ID_CDMORPH,snData,CDM_SERIAL_SIZE);
			cdmnr.SetCString(snData,CDM_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdmnr);

			BaseObject *parent = op->GetUp();
			if(parent)
			{
				pM = parent->GetMg();
			}
			else pM = Matrix();
			
			pM.v1 = opM.v1;
			pM.v2 = opM.v2;
			pM.v3 = opM.v3;
			targM = MInv(pM) * opM;
			
			tData->SetMatrix(MS_LOCAL_MATRIX,pM);
			tData->SetVector(MS_BOX_POSITION, targM.off);
			tData->SetBool(MS_INITIAL_POSITION, true);
			tData->SetLink(MS_OP_ID, op);
			break;
		}
	}
	if(!doc) return true;
	
	CheckTagReg(doc,op,tData);
	
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand *dc = (DescriptionCommand*) data;
			if(dc->id[0].id==MS_PURCHASE)
			{
				GeOpenHTML(GeLoadString(IDS_WEB_SITE));
			}
			else if(dc->id[0].id==MS_HOME_POSITION)
			{
				Real boxSizeX = tData->GetReal(MS_BOX_SIZE_X)/2;
				Real boxSizeY = tData->GetReal(MS_BOX_SIZE_Y)/2;
				
				Vector bxPos = tData->GetVector(MS_BOX_POSITION);
				Vector midPt = bxPos;
				
				pM = GetParentMatrix(op,tData);
				transM = MInv(pM) * opM;
				transM.off.z = bxPos.z;
				
				if(tData->GetBool(MS_POSITIVE_X) && !tData->GetBool(MS_POSITIVE_Y))
				{
					if(!tData->GetBool(MS_NEGATIVE_X))
					{
						//Move midPt to the left
						midPt.x = bxPos.x - boxSizeX;
					}
					else
					{
						//Move midPt to the right
						midPt.x = bxPos.x + boxSizeX;
					}
				}
				else if(!tData->GetBool(MS_POSITIVE_X) && tData->GetBool(MS_POSITIVE_Y))
				{
					if(!tData->GetBool(MS_NEGATIVE_Y))
					{
						//Move midPt to the bottom
						midPt.y = bxPos.y - boxSizeY;
					}
					else
					{
						//Move midPt to the top
						midPt.y = bxPos.y + boxSizeY;
					}
				}
				else if(tData->GetBool(MS_POSITIVE_X) && tData->GetBool(MS_POSITIVE_Y))
				{
					if(tData->GetBool(MS_NEGATIVE_X) && tData->GetBool(MS_NEGATIVE_Y))
					{
						midPt.x = bxPos.x + boxSizeX;
						midPt.y = bxPos.y + boxSizeY;
					}
					else if(!tData->GetBool(MS_NEGATIVE_X) && tData->GetBool(MS_NEGATIVE_Y))
					{
						midPt.x = bxPos.x - boxSizeX;
						midPt.y = bxPos.y + boxSizeY;
					}
					else if(tData->GetBool(MS_NEGATIVE_X) && !tData->GetBool(MS_NEGATIVE_Y))
					{
						midPt.x = bxPos.x + boxSizeX;
						midPt.y = bxPos.y - boxSizeY;
					}
					else
					{
						midPt.x = bxPos.x - boxSizeX;
						midPt.y = bxPos.y - boxSizeY;
					}
				}
				transM.off.x = midPt.x;
				transM.off.y = midPt.y;
				transM.off.z = bxPos.z;
				targM = pM * transM;
				
				Vector theScale = CDGetScale(op);
				op->SetMg(targM);
				CDSetScale(op,theScale);
				tData->SetVector(MS_BOX_ROTATION,CDGetRot(op));
			}
			break;
		}
	}

	return true;
}

Bool CDMorphSliderPlugin::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		
		tData->SetVector(MS_BOX_POSITION,tData->GetVector(MS_BOX_POSITION+T_LST));
		tData->SetReal(MS_BOX_SIZE_X,tData->GetReal(MS_BOX_SIZE_X+T_LST));
		tData->SetReal(MS_BOX_SIZE_Y,tData->GetReal(MS_BOX_SIZE_Y+T_LST));
		
		tData->SetBool(MS_MIX_X_Y,tData->GetBool(MS_MIX_X_Y+T_LST));
		tData->SetBool(MS_POSITIVE_X,tData->GetBool(MS_POSITIVE_X+T_LST));
		tData->SetBool(MS_POSITIVE_Y,tData->GetBool(MS_POSITIVE_Y+T_LST));
		tData->SetBool(MS_NEGATIVE_X,tData->GetBool(MS_NEGATIVE_X+T_LST));
		tData->SetBool(MS_NEGATIVE_Y,tData->GetBool(MS_NEGATIVE_Y+T_LST));
		
		tData->SetLink(MS_POS_X_LINK,tData->GetLink(MS_POS_X_LINK+T_LST,doc));
		tData->SetLink(MS_NEG_X_LINK,tData->GetLink(MS_NEG_X_LINK+T_LST,doc));
		tData->SetLink(MS_POS_Y_LINK,tData->GetLink(MS_POS_Y_LINK+T_LST,doc));
		tData->SetLink(MS_NEG_Y_LINK,tData->GetLink(MS_NEG_Y_LINK+T_LST,doc));

		tData->SetLink(MS_XPOS_YNEG_LINK,tData->GetLink(MS_XPOS_YNEG_LINK+T_LST,doc));
		tData->SetLink(MS_XNEG_YNEG_LINK,tData->GetLink(MS_XNEG_YNEG_LINK+T_LST,doc));
		tData->SetLink(MS_YPOS_XNEG_LINK,tData->GetLink(MS_YPOS_XNEG_LINK+T_LST,doc));
		tData->SetLink(MS_YNEG_XNEG_LINK,tData->GetLink(MS_YNEG_XNEG_LINK+T_LST,doc));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}

Matrix CDMorphSliderPlugin::GetBoxMg(BaseContainer *bc, BaseObject *op)
{
	Matrix opM = op->GetMg(), bxM = Matrix(), targM;
	Real boxSizeX = bc->GetReal(MS_BOX_SIZE_X), boxSizeY = bc->GetReal(MS_BOX_SIZE_Y);
	Real ouputX = bc->GetReal(MS_OUTPUT_X), ouputY = bc->GetReal(MS_OUTPUT_Y);
	
	if(!bc->GetBool(MS_POSITIVE_X) && !bc->GetBool(MS_POSITIVE_Y))
	{
		bxM.off.x = (boxSizeX/2) * ouputX * -1;
		bxM.off.y = (boxSizeY/2) * ouputY * -1;
		targM = opM * bxM;
	}
	else if(bc->GetBool(MS_POSITIVE_X) && !bc->GetBool(MS_POSITIVE_Y))
	{
		if(!bc->GetBool(MS_NEGATIVE_X))
		{
			bxM.off.x = ((boxSizeX*ouputX) - (boxSizeX/2)) * -1;
			bxM.off.y = (boxSizeY/2) * ouputY * -1;
			targM = opM * bxM;
		}
		else
		{
			bxM.off.x = (boxSizeX*ouputX) - (boxSizeX/2);
			bxM.off.y = (boxSizeY/2) * ouputY * -1;
			targM = opM * bxM;
		}
	}
	else if(!bc->GetBool(MS_POSITIVE_X) && bc->GetBool(MS_POSITIVE_Y))
	{
		if(!bc->GetBool(MS_NEGATIVE_Y))
		{
			bxM.off.x = (boxSizeX/2) * ouputX * -1;
			bxM.off.y = ((boxSizeY*ouputY) - (boxSizeY/2)) * -1;
			targM = opM * bxM;
		}
		else
		{
			bxM.off.x = (boxSizeX/2) * ouputX * -1;
			bxM.off.y = (boxSizeY*ouputY) - (boxSizeY/2);
			targM = opM * bxM;
		}
	}
	else if(bc->GetBool(MS_POSITIVE_X) && bc->GetBool(MS_POSITIVE_Y))
	{
		if(bc->GetBool(MS_NEGATIVE_X) && bc->GetBool(MS_NEGATIVE_Y))
		{
			bxM.off.x = (boxSizeX*ouputX) - (boxSizeX/2);
			bxM.off.y = (boxSizeY*ouputY) - (boxSizeY/2);
			targM = opM * bxM;
		}
		else if(!bc->GetBool(MS_NEGATIVE_X) && bc->GetBool(MS_NEGATIVE_Y))
		{
			bxM.off.x = ((boxSizeX*ouputX) - (boxSizeX/2)) * -1;
			bxM.off.y = (boxSizeY*ouputY) - (boxSizeY/2);
			targM = opM * bxM;
		}
		else if(bc->GetBool(MS_NEGATIVE_X) && !bc->GetBool(MS_NEGATIVE_Y))
		{
			bxM.off.x = (boxSizeX*ouputX) - (boxSizeX/2);
			bxM.off.y = ((boxSizeY*ouputY) - (boxSizeY/2)) * -1;
			targM = opM * bxM;
		}
		else
		{
			bxM.off.x = ((boxSizeX*ouputX) - (boxSizeX/2)) * -1;
			bxM.off.y = ((boxSizeY*ouputY) - (boxSizeY/2)) * -1;
			targM = opM * bxM;
		}
	}
	
	return targM;
}

Matrix CDMorphSliderPlugin::GetParentMatrix(BaseObject *op, BaseContainer *tData)
{
	Matrix pM = Matrix(), opM = op->GetMg();
	
	BaseObject *pr = op->GetUp();
	if(pr)
	{
		Bool sameRot = true;
		pM = pr->GetMg() * tData->GetMatrix(MS_REF_MATRIX);
		
		if(!VEqual(pM.v1,opM.v1,0.001)) sameRot = false;
		if(!VEqual(pM.v2,opM.v2,0.001)) sameRot = false;
		if(!VEqual(pM.v3,opM.v3,0.001)) sameRot = false;
		if(!sameRot)
		{
			pM.v1 = opM.v1;
			pM.v2 = opM.v2;
			pM.v3 = opM.v3;
			tData->SetMatrix(MS_LOCAL_MATRIX, pM);
		}
	}
	else
	{
		Bool sameRot = true;
		pM = tData->GetMatrix(MS_LOCAL_MATRIX);
		if(!VEqual(pM.v1,opM.v1,0.001)) sameRot = false;
		if(!VEqual(pM.v2,opM.v2,0.001)) sameRot = false;
		if(!VEqual(pM.v3,opM.v3,0.001)) sameRot = false;
		if(!sameRot)
		{
			pM.v1 = opM.v1;
			pM.v2 = opM.v2;
			pM.v3 = opM.v3;
			tData->SetMatrix(MS_LOCAL_MATRIX, pM);
		}
	}
	
	return pM;
}

LONG CDMorphSliderPlugin::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer	*tData = tag->GetDataInstance(); if(!tData) return false;
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	BaseContainer	*opData = op->GetDataInstance();
	Matrix opM = op->GetMg(), bxM, transM, targM, pM = Matrix();
	Vector bxPos = tData->GetVector(MS_BOX_POSITION);
	Vector oldRot, newRot, rotSet;
	Real boxSizeX = tData->GetReal(MS_BOX_SIZE_X)/2;
	Real boxSizeY = tData->GetReal(MS_BOX_SIZE_Y)/2;

	// Check if tag has moved to another object
	if(op != tData->GetObjectLink(MS_OP_ID,doc))
	{
		tData->SetBool(MS_INITIAL_SIZE, false);
		tData->SetBool(MS_INITIAL_POSITION, false);
		tData->SetLink(MS_OP_ID, op);
	}
	
	// Check if parent has changed
	BaseObject *parent = op->GetUp();
	if(parent != tData->GetObjectLink(MS_PARENT_ID,doc))
	{
		bxM = GetBoxMg(tData,op);
		if(parent)
		{
			pM = parent->GetMg();
		}
		pM.v1 = opM.v1;
		pM.v2 = opM.v2;
		pM.v3 = opM.v3;
		targM = MInv(pM) * bxM;
		tData->SetMatrix(MS_LOCAL_MATRIX,pM);
		tData->SetVector(MS_BOX_POSITION, targM.off);
		
		tData->SetLink(MS_PARENT_ID, parent);
	}

	if(!tData->GetBool(MS_INITIAL_POSITION))
	{
		if(parent)
		{
			pM = parent->GetMg();
		}
		pM.v1 = opM.v1;
		pM.v2 = opM.v2;
		pM.v3 = opM.v3;
		targM = MInv(pM) * opM;
		tData->SetMatrix(MS_LOCAL_MATRIX,pM);
		tData->SetVector(MS_BOX_POSITION, targM.off);
		tData->SetBool(MS_INITIAL_POSITION, true);
	}
	
	// Check ifX/Y mix is on
	if(tData->GetBool(MS_MIX_X_Y)) tData->SetBool(MS_POSITIVE_X, false);

	// Check for scaling
	Vector opSize, sizeOriginal, sizeCurrent, sizeScale;
	if(!tData->GetBool(MS_INITIAL_SIZE))
	{
		if(op->GetType() != Onull)
		{
			opSize = op->GetRad();
		}
		else
		{
			opSize.x = opData->GetReal(NULLOBJECT_RADIUS);
			opSize.y = opData->GetReal(NULLOBJECT_RADIUS) * opData->GetReal(NULLOBJECT_ASPECTRATIO);
			opSize.z = opSize.x;
		}
		if((opSize.x != 0) && (opSize.y != 0))    // Check for division by 0
		{
			tData->SetVector(MS_OP_SIZE, opSize);
		}
		else if((opSize.x == 0) && (opSize.y != 0))
		{
			opSize.x = 1.0;
			tData->SetVector(MS_OP_SIZE, opSize);
		}
		else if((opSize.x != 0) && (opSize.y == 0))
		{
			opSize.y = 1.0;
			tData->SetVector(MS_OP_SIZE, opSize);
		}
		else tData->SetVector(MS_OP_SIZE, Vector(1,1,1));
		tData->SetBool(MS_INITIAL_SIZE, true);
	}
	
	sizeOriginal = tData->GetVector(MS_OP_SIZE);
	if(op->GetType() != Onull)
	{
		sizeCurrent = op->GetRad();
	}
	else
	{
		sizeCurrent.x = opData->GetReal(NULLOBJECT_RADIUS);
		sizeCurrent.y = opData->GetReal(NULLOBJECT_RADIUS) * opData->GetReal(NULLOBJECT_ASPECTRATIO);
		sizeCurrent.z = sizeCurrent.x;
	}

	if(sizeCurrent != sizeOriginal)
	{
		if((sizeOriginal.x != 0) && (sizeOriginal.y != 0))   // Check for division by 0
		{
			sizeScale.x = sizeCurrent.x / sizeOriginal.x;
			sizeScale.y = sizeCurrent.y / sizeOriginal.y;
		}
		else
		{
			sizeScale.x = sizeCurrent.x;
			sizeScale.y = sizeCurrent.y;
		}
		bxPos.x = bxPos.x * sizeScale.x;
		bxPos.y = bxPos.y * sizeScale.y;
		bxPos.z = bxPos.z * sizeScale.x;
		boxSizeX = boxSizeX * sizeScale.x;
		boxSizeY = boxSizeY * sizeScale.y;
		tData->SetVector(MS_BOX_POSITION, bxPos);
		tData->SetReal(MS_BOX_SIZE_X, boxSizeX*2);
		tData->SetReal(MS_BOX_SIZE_Y, boxSizeY*2);
		tData->SetVector(MS_OP_SIZE, sizeCurrent);
	}
	
	// Check for op rotating
	if(!CheckIsRunning(CHECKISRUNNING_ANIMATIONRUNNING))
	{
		Vector opRot = CDGetRot(op), boxRot = tData->GetVector(MS_BOX_ROTATION);
		if(doc->GetAction() == ID_MODELING_ROTATE)
		{
			if(!VEqual(opRot,boxRot,0.001))
			{
				bxM = GetBoxMg(tData,op);
				if(parent)
				{
					pM = parent->GetMg();
				}
				pM.v1 = opM.v1;
				pM.v2 = opM.v2;
				pM.v3 = opM.v3;
				targM = MInv(pM) * bxM;
				tData->SetVector(MS_BOX_POSITION, targM.off);
				tData->SetVector(MS_BOX_ROTATION,CDGetRot(op));
				tData->SetMatrix(MS_LOCAL_MATRIX,pM);
				return true;
			}
		}
		if(!VEqual(opRot,boxRot,0.005))
		{
			bxM = GetBoxMg(tData,op);
			if(parent)
			{
				pM = parent->GetMg();
			}
			pM.v1 = opM.v1;
			pM.v2 = opM.v2;
			pM.v3 = opM.v3;
			targM = MInv(pM) * bxM;
			tData->SetVector(MS_BOX_POSITION, targM.off);
			tData->SetVector(MS_BOX_ROTATION,CDGetRot(op));
			tData->SetMatrix(MS_LOCAL_MATRIX,pM);
		}
	}
	
	if(parent)
	{
		bxM = GetBoxMg(tData,op);
		pM = parent->GetMg();
		pM.v1 = opM.v1;
		pM.v2 = opM.v2;
		pM.v3 = opM.v3;
		if(pM != tData->GetMatrix(MS_LOCAL_MATRIX))
		{
			tData->SetMatrix(MS_LOCAL_MATRIX,pM);
		}
	}
	// Restrict op position to box
	if(parent)
	{
		pM = tData->GetMatrix(MS_LOCAL_MATRIX);
		pM.off = parent->GetMg().off;
	}
	else
	{
		pM = tData->GetMatrix(MS_LOCAL_MATRIX);
	}
	bxPos = tData->GetVector(MS_BOX_POSITION);
	transM = MInv(pM) * opM;
	
	Bool clamp = false;
	if(transM.off.x < bxPos.x-boxSizeX)
	{
		transM.off.x = bxPos.x-boxSizeX;
		clamp = true;
	}
	if(transM.off.y < bxPos.y-boxSizeY)
	{
		transM.off.y = bxPos.y-boxSizeY;
		clamp = true;
	}
	if(transM.off.x > bxPos.x+boxSizeX)
	{
		transM.off.x = bxPos.x+boxSizeX;
		clamp = true;
	}
	if(transM.off.y > bxPos.y+boxSizeY)
	{
		transM.off.y = bxPos.y+boxSizeY;
		clamp = true;
	}
	if(transM.off.z > bxPos.z || transM.off.z < bxPos.z)
	{
		transM.off.z = bxPos.z;
		clamp = true;
	}
	
	if(clamp)
	{
		targM = pM * transM;
		Vector theScale = CDGetScale(op);
		op->SetMg(targM);
		CDSetScale(op,theScale);
		op->Message(MSG_UPDATE);
		
		sliderMatrix = op->GetMg();
		tData->SetVector(MS_BOX_ROTATION,CDGetRot(op));
	}
	
	// Output to X/Y
	Real xOutput, yOutput, xValue, yValue;
	if((bxPos.x+boxSizeX)-(bxPos.x-boxSizeX) != 0) //Check for division by zero
	{
		xValue = (transM.off.x-(bxPos.x-boxSizeX))/((bxPos.x+boxSizeX)-(bxPos.x-boxSizeX));
		if(!tData->GetBool(MS_POSITIVE_X)) 
		{
			xOutput = (xValue*2)-1;
			tData->SetBool(MS_NEGATIVE_X,false);
		}
		else
		{
			xOutput = xValue;
			if(tData->GetBool(MS_NEGATIVE_X)) xOutput = -xOutput + 1;
		}
	}
	else
	{
		xOutput = 0;
	}
	if((bxPos.y+boxSizeY)-(bxPos.y-boxSizeY) != 0) //Check for division by zero
	{
		yValue = (transM.off.y-(bxPos.y-boxSizeY))/((bxPos.y+boxSizeY)-(bxPos.y-boxSizeY));
		if(!tData->GetBool(MS_POSITIVE_Y)) 
		{
			yOutput = (yValue*2)-1;
			tData->SetBool(MS_NEGATIVE_Y,false);
		}
		else
		{
			yOutput = yValue;
			if(tData->GetBool(MS_NEGATIVE_Y)) yOutput = -yOutput + 1;
		}
	}
	else
	{
		yOutput = 0;
	}
	tData->SetReal(MS_OUTPUT_X, xOutput);
	tData->SetReal(MS_OUTPUT_Y, yOutput);
	
	//Output to links
	Real mixOutput;
	if(!tData->GetBool(MS_MIX_X_Y))
	{
		if(tData->GetReal(MS_BOX_SIZE_X) != 0)
		{
			BaseList2D *linkXPos = tData->GetLink(MS_POS_X_LINK,doc,ID_CDMORPHTAGPLUGIN); // Get object from link
			if(linkXPos)
			{
				BaseContainer *dataXPos = linkXPos->GetDataInstance();
				if(!dataXPos->GetBool(1020))// Check ifnot using bone rotation
				{
					if(xOutput > 0) dataXPos->SetReal(1018,xOutput);
					else dataXPos->SetReal(1018,0);
				}
			}
			if(!tData->GetBool(MS_POSITIVE_X))
			{
				BaseList2D *linkXNeg = tData->GetLink(MS_NEG_X_LINK,doc,ID_CDMORPHTAGPLUGIN);
				if(linkXNeg)
				{
					BaseContainer *dataXNeg = linkXNeg->GetDataInstance();
					if(!dataXNeg->GetBool(1020))
					{
						if(xOutput < 0) dataXNeg->SetReal(1018,-xOutput);
						else dataXNeg->SetReal(1018,0);
					}
				}
			}
		}
		if(tData->GetReal(MS_BOX_SIZE_Y) != 0)
		{
			if(!tData->GetBool(MS_MIX_X_Y))
			{
				BaseList2D *linkYPos = tData->GetLink(MS_POS_Y_LINK,doc,ID_CDMORPHTAGPLUGIN);
				if(linkYPos)
				{
					BaseContainer *dataYPos = linkYPos->GetDataInstance();
					if(!dataYPos->GetBool(1020))
					{
						if(yOutput > 0) dataYPos->SetReal(1018,yOutput);
						else dataYPos->SetReal(1018,0);
					}
				}
				if(!tData->GetBool(MS_POSITIVE_Y))
				{
					BaseList2D *linkYNeg = tData->GetLink(MS_NEG_Y_LINK,doc,ID_CDMORPHTAGPLUGIN);
					if(linkYNeg)
					{
						BaseContainer *dataYNeg = linkYNeg->GetDataInstance();
						if(!dataYNeg->GetBool(1020))
						{
							if(yOutput < 0) dataYNeg->SetReal(1018,-yOutput);
							else dataYNeg->SetReal(1018,0);
						}
					}
				}
			}
		}
	}
	else
	{
		if(tData->GetReal(MS_BOX_SIZE_X) != 0)
		{
			BaseList2D *linkXpYp = tData->GetLink(MS_POS_X_LINK,doc,ID_CDMORPHTAGPLUGIN); // Get object from link
			if(linkXpYp)
			{
				BaseContainer *dataXpYp = linkXpYp->GetDataInstance();
				if(!dataXpYp->GetBool(1020))// Check ifnot using bone rotation
				{
					if(tData->GetReal(MS_BOX_SIZE_Y) != 0)
					{
						mixOutput = yOutput;
						if(xOutput < 0) mixOutput += xOutput;
					}
					else mixOutput = xOutput;
					
					if(mixOutput < 0) mixOutput = 0;
					dataXpYp->SetReal(1018,mixOutput);
				}
			}
			BaseList2D *linkXnYp = tData->GetLink(MS_NEG_X_LINK,doc,ID_CDMORPHTAGPLUGIN);
			if(linkXnYp)
			{
				BaseContainer *dataXnYp = linkXnYp->GetDataInstance();
				if(!dataXnYp->GetBool(1020))
				{
					if(tData->GetReal(MS_BOX_SIZE_Y) != 0)
					{
						mixOutput = yOutput;
						if(xOutput > 0) mixOutput -= xOutput;
					}
					else mixOutput = -xOutput;
					
					if(mixOutput < 0) mixOutput = 0;
					dataXnYp->SetReal(1018,mixOutput);
				}
			}
			if(!tData->GetBool(MS_POSITIVE_Y))
			{
				BaseList2D *linkXpYn = tData->GetLink(MS_XPOS_YNEG_LINK,doc,ID_CDMORPHTAGPLUGIN); // Get object from link
				if(linkXpYn)
				{
					BaseContainer *dataXpYn = linkXpYn->GetDataInstance();
					if(!dataXpYn->GetBool(1020))// Check ifnot using bone rotation
					{
						if(tData->GetReal(MS_BOX_SIZE_Y) != 0)
						{
							mixOutput = -yOutput;
							if(xOutput < 0) mixOutput += xOutput;
						}
						else mixOutput = xOutput;
						
						if(mixOutput < 0) mixOutput = 0;
						dataXpYn->SetReal(1018,mixOutput);
					}
				}
				BaseList2D *linkXnYn = tData->GetLink(MS_XNEG_YNEG_LINK,doc,ID_CDMORPHTAGPLUGIN);
				if(linkXnYn)
				{
					BaseContainer *dataXnYn = linkXnYn->GetDataInstance();
					if(!dataXnYn->GetBool(1020))
					{
						if(tData->GetReal(MS_BOX_SIZE_Y) != 0)
						{
							mixOutput = yOutput * -1;
							if(xOutput > 0) mixOutput -= xOutput;
						}
						else mixOutput = xOutput;
						
						if(mixOutput < 0) mixOutput = 0;
						dataXnYn->SetReal(1018,mixOutput);
					}
				}
			}
		}
	}

	return CD_EXECUTION_RESULT_OK;
}

Bool CDMorphSliderPlugin::CDGetDDescription(GeListNode *node, Description *description, LONG &flags)
{
	BaseTag 		*tag = (BaseTag*)node;
	BaseContainer	*tData = tag->GetDataInstance();

	if(!description->LoadDescription(node->GetType())) return false;
	
	AutoAlloc<AtomArray> ar; if(!ar) return false;
	ar->Append(static_cast<C4DAtom*>(node));
	
	BaseContainer *bc = description->GetParameterI(DescLevel(MS_PURCHASE), ar);
	if(bc)
	{
		if(!tData->GetBool(T_REG)) bc->SetBool(DESC_HIDE, false);
		else bc->SetBool(DESC_HIDE, true);
	}

	LONG linkCnt = 0;
	if(tData->GetReal(MS_BOX_SIZE_X) != 0)
	{
		if(!tData->GetBool(MS_MIX_X_Y))
		{
			BaseContainer bcXPos = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
			bcXPos.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
			bcXPos.SetString(DESC_NAME,GeLoadString(XPOS_NAME));
			bcXPos.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
			bcXPos.SetBool(DESC_REMOVEABLE,true);
			if(!description->SetParameter(DescLevel(MS_POS_X_LINK,DTYPE_BASELISTLINK,0),bcXPos,DescLevel(MS_MORPH_CONTROL))) return false;
			linkCnt++;
			
			if(!tData->GetBool(MS_POSITIVE_X))
			{
				BaseContainer bcXNeg = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
				bcXNeg.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
				bcXNeg.SetString(DESC_NAME,GeLoadString(XNEG_NAME));
				bcXNeg.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
				bcXNeg.SetBool(DESC_REMOVEABLE,true);
				if(!description->SetParameter(DescLevel(MS_NEG_X_LINK,DTYPE_BASELISTLINK,0),bcXNeg,DescLevel(MS_MORPH_CONTROL))) return false;
				linkCnt++;
			}
		}
		else
		{
			BaseContainer bcXPosYPos = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
			bcXPosYPos.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
			bcXPosYPos.SetString(DESC_NAME,GeLoadString(XPOS_YPOS_NAME));
			bcXPosYPos.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
			bcXPosYPos.SetBool(DESC_REMOVEABLE,true);
			if(!description->SetParameter(DescLevel(MS_POS_X_LINK,DTYPE_BASELISTLINK,0),bcXPosYPos,DescLevel(MS_MORPH_CONTROL))) return false;
			linkCnt++;
			
			if(!tData->GetBool(MS_POSITIVE_X))
			{
				BaseContainer bcXNegYPos = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
				bcXNegYPos.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
				bcXNegYPos.SetString(DESC_NAME,GeLoadString(XNEG_YPOS_NAME));
				bcXNegYPos.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
				bcXNegYPos.SetBool(DESC_REMOVEABLE,true);
				if(!description->SetParameter(DescLevel(MS_NEG_X_LINK,DTYPE_BASELISTLINK,0),bcXNegYPos,DescLevel(MS_MORPH_CONTROL))) return false;
				linkCnt++;
			}
			if(tData->GetReal(MS_BOX_SIZE_Y) != 0 && !tData->GetBool(MS_POSITIVE_Y))
			{
				BaseContainer bcXPosYNeg = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
				bcXPosYNeg.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
				bcXPosYNeg.SetString(DESC_NAME,GeLoadString(XPOS_YNEG_NAME));
				bcXPosYNeg.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
				bcXPosYNeg.SetBool(DESC_REMOVEABLE,true);
				if(!description->SetParameter(DescLevel(MS_XPOS_YNEG_LINK,DTYPE_BASELISTLINK,0),bcXPosYNeg,DescLevel(MS_MORPH_CONTROL))) return false;
				linkCnt++;
				
				if(!tData->GetBool(MS_POSITIVE_X))
				{
					BaseContainer bcXNegYNeg = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
					bcXNegYNeg.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
					bcXNegYNeg.SetString(DESC_NAME,GeLoadString(XNEG_YNEG_NAME));
					bcXNegYNeg.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
					bcXNegYNeg.SetBool(DESC_REMOVEABLE,true);
					if(!description->SetParameter(DescLevel(MS_XNEG_YNEG_LINK,DTYPE_BASELISTLINK,0),bcXNegYNeg,DescLevel(MS_MORPH_CONTROL))) return false;
					linkCnt++;
				}
			}
		}
	}

	if(tData->GetReal(MS_BOX_SIZE_Y) != 0)
	{
		if(!tData->GetBool(MS_MIX_X_Y))
		{
			BaseContainer bcYPos = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
			bcYPos.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
			bcYPos.SetString(DESC_NAME,GeLoadString(YPOS_NAME));
			bcYPos.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
			bcYPos.SetBool(DESC_REMOVEABLE,true);
			if(!description->SetParameter(DescLevel(MS_POS_Y_LINK,DTYPE_BASELISTLINK,0),bcYPos,DescLevel(MS_MORPH_CONTROL))) return false;
			linkCnt++;
			
			if(!tData->GetBool(MS_POSITIVE_Y))
			{
				BaseContainer bcYNeg = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
				bcYNeg.SetLong(DESC_CUSTOMGUI,CUSTOMGUI_LINKBOX);
				bcYNeg.SetString(DESC_NAME,GeLoadString(YNEG_NAME));
				bcYNeg.SetLong(DESC_ANIMATE,DESC_ANIMATE_ON);
				bcYNeg.SetBool(DESC_REMOVEABLE,true);
				if(!description->SetParameter(DescLevel(MS_NEG_Y_LINK,DTYPE_BASELISTLINK,0),bcYNeg,DescLevel(MS_MORPH_CONTROL))) return false;
				linkCnt++;
			}
		}
	}
	tData->SetLong(MS_LINK_COUNT,linkCnt);

	flags |= CD_DESCFLAGS_DESC_LOADED;
	return CDSuperGetDDescriptionReturn(node,description,flags);
}

Bool CDMorphSliderPlugin::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;

	switch (id[0].id)
	{
		case MS_SHOW_DIAGONAL:	
			return tData->GetBool(MS_SHOW_BOX);
		case MS_BOX_POSITION:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_BOX_SIZE_X:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_BOX_SIZE_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_MIX_X_Y:
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_POSITIVE_X:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(MS_MIX_X_Y)) return true;
			else return false;
		case MS_POSITIVE_Y:
			if(!tData->GetBool(T_REG)) return false;
			else if(!tData->GetBool(MS_MIX_X_Y)) return true;
			else return false;
		case MS_NEGATIVE_X:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MS_POSITIVE_X);
		case MS_NEGATIVE_Y:	
			if(!tData->GetBool(T_REG)) return false;
			else return tData->GetBool(MS_POSITIVE_Y);
		case MS_POS_X_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_NEG_X_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_POS_Y_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_NEG_Y_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_XPOS_YNEG_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_XNEG_YNEG_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_YPOS_XNEG_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
		case MS_YNEG_XNEG_LINK:	
			if(!tData->GetBool(T_REG)) return false;
			else return true;
	}
	return true;
}

Bool RegisterCDMorphSliderPlugin(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDM_SERIAL_SIZE];
	String cdmnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDMORPH,data,CDM_SERIAL_SIZE)) reg = false;
	
	cdmnr.SetCString(data,CDM_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdmnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdmnr.FindFirst("-",&pos);
	while(h)
	{
		cdmnr.Delete(pos,1);
		h = cdmnr.FindFirst("-",&pos);
	}
	cdmnr.ToUpper();
	kb = cdmnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDMORPHSLIDER); if(!name.Content()) return true;
	return CDRegisterTagPlugin(ID_CDMORPHSLIDERPLUGIN,name,info,CDMorphSliderPlugin::Alloc,"tCDMorphSlider","CDMorphSlider.tif",0);
}
