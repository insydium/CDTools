//	Cactus Dan's HPB View 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"

#include "CDFBX.h"
#include "tCDNDisplay.h"
#include "CDGeneral.h"
#include "CDCompatibility.h"
#include "CDTagData.h"

enum
{
	//ND_NORMAL_SHOW			= 1000,
	//ND_NORMAL_SIZE,
};

class CDNormalsDisplay : public CDTagData
{
private:
	void CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	Bool CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData);
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, LONG type, void *data);
	
	virtual Bool CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh);
	virtual LONG CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags);
	virtual Bool CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc);
	
	static NodeData *Alloc(void) { return CDDataAllocator(CDNormalsDisplay); }
};

Bool CDNormalsDisplay::Init(GeListNode *node)
{
	BaseTag				*tag  = (BaseTag*)node;
	BaseContainer *tData = tag->GetDataInstance();
	
	tData->SetBool(ND_NORMAL_SHOW, true);
	tData->SetReal(ND_NORMAL_SIZE, 20);
	
	return true;
}

Bool CDNormalsDisplay::CDDraw(BaseTag *tag, BaseObject *op, BaseDraw *bd, BaseDrawHelp *bh)
{
	if(!IsValidPolygonObject(op)) return true;
	
	if(op->GetBit(BIT_ACTIVE) && op->GetTag(Tphong))
	{

		BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
		if(!tData->GetBool(ND_NORMAL_SHOW)) return true;
		
		Real size = tData->GetReal(ND_NORMAL_SIZE);
		
		Vector *padr = GetPointArray(op); if(!padr) return true;
		CPolygon *vadr = GetPolygonArray(op); if(!vadr) return true;
		LONG plyCnt = ToPoly(op)->GetPolygonCount();
		
		CDSetBaseDrawMatrix(bd, NULL, Matrix());
		
	#if API_VERSION < 12000
		Vector *phN = ToPoly(op)->CreatePhongNormals();
	#else
		SVector *phN = ToPoly(op)->CreatePhongNormals();
	#endif
		
		Matrix opM = op->GetMg();
		Matrix nRotM = opM;
		nRotM.off = Vector(0,0,0);
		
		bd->SetPen(Vector(1.0,1.0,0.0));
		
		for(LONG i=0; i<plyCnt; i++)
		{
			CPolygon ply = vadr[i];
			Vector start, end;
			
		#if API_VERSION < 12000
			Vector norm = nRotM * VNorm(phN[i*4]);
		#else
            #if API_VERSION < 15000
                Vector norm = nRotM * VNorm(phN[i*4].ToLV());
            #else
                Vector norm = nRotM * VNorm((Vector)phN[i*4]);
            #endif
		#endif
			start = opM * padr[ply.a];
			end = start + VNorm(norm) * size;
			CDDrawLine(bd,start,end);
			
		#if API_VERSION < 12000
			norm = nRotM * VNorm(phN[i*4+1]);
		#else
            #if API_VERSION < 15000
                norm = nRotM * VNorm(phN[i*4+1].ToLV());
            #else
                norm = nRotM * VNorm((Vector)phN[i*4+1]);
            #endif
		#endif
			start = opM * padr[ply.b];
			end = start + VNorm(norm) * size;
			CDDrawLine(bd,start,end);
			
		#if API_VERSION < 12000
			norm = nRotM * VNorm(phN[i*4+2]);
		#else
            #if API_VERSION < 15000
                norm = nRotM * VNorm(phN[i*4+2].ToLV());
            #else
                norm = nRotM * VNorm((Vector)phN[i*4+2]);
            #endif
		#endif
			start = opM * padr[ply.c];
			end = start + VNorm(norm) * size;
			CDDrawLine(bd,start,end);
			
			if(ply.c != ply.d)
			{
			#if API_VERSION < 12000
				norm = nRotM * VNorm(phN[i*4+3]);
			#else
                #if API_VERSION < 15000
                    norm = nRotM * VNorm(phN[i*4+3].ToLV());
               #else
                    norm = nRotM * VNorm((Vector)phN[i*4+3]);
                #endif
			#endif
				start = opM * padr[ply.d];
				end = start + VNorm(norm) * size;
				CDDrawLine(bd,start,end);
			}
		}
		if(phN) CDFree(phN);
	}
	
	return true;
}

void CDNormalsDisplay::CheckTagReg(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b;
	String kb, cdknr = tData->GetString(T_STR);
	SerialInfo si;
	
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
			tData->SetLink(T_PID,op->GetUp());
			
			tData->SetBool(ND_NORMAL_SHOW+T_LST,tData->GetBool(ND_NORMAL_SHOW));
			tData->SetReal(ND_NORMAL_SIZE+T_LST,tData->GetReal(ND_NORMAL_SIZE));
			
			tData->SetBool(T_SET,true);
		}
	}
}

Bool CDNormalsDisplay::CheckTagAssign(BaseDocument *doc, BaseObject *op, BaseContainer *tData)
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
		else
		{
			if(op->GetUp() != tData->GetObjectLink(T_PID,doc))
			{
				BaseObject *pOp = tData->GetObjectLink(T_PID,doc);
				if(pOp)
				{
					if(pOp->GetDocument())
					{
						tagMoved = true;
						tData->SetBool(T_MOV,true);
					}
				}
				if(!tagMoved && !tData->GetBool(T_MOV))  tData->SetLink(T_PID,op->GetUp());
			}
			else tData->SetBool(T_MOV,false);
		}
		if(tagMoved || tData->GetBool(T_MOV)) enable = false;
				
		tData->SetBool(ND_NORMAL_SHOW,tData->GetBool(ND_NORMAL_SHOW+T_LST));
		tData->SetReal(ND_NORMAL_SIZE,tData->GetReal(ND_NORMAL_SIZE+T_LST));
	}
	else
	{
		tData->SetBool(T_SET,false);
		tData->SetBool(T_MOV,false);
	}
	
	return enable;
}


Bool CDNormalsDisplay::Message(GeListNode *node, LONG type, void *data)
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
			CHAR snData[CDFBX_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDFBXPLUGIN,snData,CDFBX_SERIAL_SIZE);
			cdknr.SetCString(snData,CDFBX_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
		case MSG_MENUPREPARE:
		{
			tData->SetBool(T_SET,false);
			CHAR snData[CDFBX_SERIAL_SIZE];
			String cdknr;
			
			CDReadPluginInfo(ID_CDFBXPLUGIN,snData,CDFBX_SERIAL_SIZE);
			cdknr.SetCString(snData,CDFBX_SERIAL_SIZE-1);
			tData->SetString(T_STR,cdknr);
			break;
		}
	}
	if(!doc) return true;
	CheckTagReg(doc,op,tData);
	
	return true;
}

LONG CDNormalsDisplay::CDExecute(BaseTag *tag, BaseDocument *doc, BaseObject *op, BaseThread *bt, LONG priority, LONG flags)
{
	BaseContainer *tData = tag->GetDataInstance();
	if(!CheckTagAssign(doc,op,tData)) return false;
	
	return CD_EXECUTION_RESULT_OK;
}

Bool CDNormalsDisplay::CDGetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, LONG flags, const BaseContainer *itemdesc)
{
	BaseTag *tag = (BaseTag*)node; if(!tag) return false;
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return false;
	if(!tData->GetBool(T_REG) && tData->GetBool(T_MOV)) return false;
	
	return true;
}

Bool RegisterCDNormalsDisplay(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDFBX_SERIAL_SIZE];
	String cdknr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDFBXPLUGIN,data,CDFBX_SERIAL_SIZE)) reg = false;
	
	cdknr.SetCString(data,CDFBX_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdknr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdknr.FindFirst("-",&pos);
	while(h)
	{
		cdknr.Delete(pos,1);
		h = cdknr.FindFirst("-",&pos);
	}
	cdknr.ToUpper();
	
	kb = cdknr.SubStr(pK,2);
	
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
	
	// decide by name if the plugin shall be registered - just for user convenience  TAG_EXPRESSION|
	String name=GeLoadString(IDS_CDDISPLAYN); if (!name.Content()) return true;

	return CDRegisterTagPlugin(ID_CDDISPLAYNORMALS,name,info,CDNormalsDisplay::Alloc,"tCDNDisplay","tCDNDisplay.tif",0);
}
