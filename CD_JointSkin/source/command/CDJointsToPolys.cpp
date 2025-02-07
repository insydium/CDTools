//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

#include "oCDJoint.h"
#include "CDJointSkin.h"

class CDJointsToPolygons : public CommandData
{
	public:
		Bool ChildJointsToPolygons(BaseDocument* doc, BaseObject *op, BaseObject *bn);
		BaseObject* CreatePolygonObject(BaseDocument *doc, BaseObject *op, BaseObject *par, BaseObject *sib);
		
		virtual Bool Execute(BaseDocument* doc);

};

BaseObject* CDJointsToPolygons::CreatePolygonObject(BaseDocument *doc, BaseObject *op, BaseObject *par, BaseObject *sib)
{
	
	Matrix m = op->GetMg();
	
	Vector ePts[4] = {
			Vector(1,1,0),
			Vector(-1,1,0),
			Vector(-1,-1,0),
			Vector(1,-1,0),
			};

	Matrix rootM = m;
	Matrix tipM = rootM;
	BaseObject *ch = op->GetDown();
	if(ch)
	{
		Vector opPos = op->GetMg().off, cPos = ch->GetMg().off;
		Real bnLen = Len(cPos - opPos);
		tipM.off = cPos;
			
		BaseContainer *jData = op->GetDataInstance();
		if(jData->GetBool(JNT_SHOW_PROXY))
		{
			Vector rSk, tSk, rOff=jData->GetVector(JNT_ROOT_OFFSET), tOff=jData->GetVector(JNT_TIP_OFFSET);
			rSk.x = jData->GetReal(JNT_SKEW_RX);
			rSk.y = jData->GetReal(JNT_SKEW_RY);
			tSk.x = jData->GetReal(JNT_SKEW_TX);
			tSk.y = jData->GetReal(JNT_SKEW_TY);
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
		}
		else
		{
			Real rRad = jData->GetReal(JNT_JOINT_RADIUS);
			rootM.v1*=rRad;
			rootM.v2*=rRad;
			if(ch->GetType() == ID_CDJOINTOBJECT)
			{
				BaseContainer *chData = ch->GetDataInstance();
				if(chData)
				{
					Real tRad = chData->GetReal(JNT_JOINT_RADIUS);
					tipM.v1*=tRad;
					tipM.v2*=tRad;
				}
			}
			else
			{
				tipM.v1*=rRad;
				tipM.v2*=rRad;
			}
		}
	}
	else
	{
		tipM.off = rootM.off + VNorm(rootM.v3) * 6;
		rootM.v1*=3;
		rootM.v2*=3;
		tipM.v1*=3;
		tipM.v2*=3;
	}
	
	BaseObject *poly = BaseObject::Alloc(Opolygon); if(!poly) return NULL;
	doc->InsertObject(poly, par, sib, false);
	CDAddUndo(doc,CD_UNDO_NEW,poly);
	
	ToPoly(poly)->ResizeObject(8,6);
	
	Vector *padr = GetPointArray(poly);
	padr[0] = MInv(m) * rootM * ePts[0];
	padr[1] = MInv(m) * rootM * ePts[1];
	padr[2] = MInv(m) * rootM * ePts[2];
	padr[3] = MInv(m) * rootM * ePts[3];
	padr[4] = MInv(m) * tipM * ePts[0];
	padr[5] = MInv(m) * tipM * ePts[1];
	padr[6] = MInv(m) * tipM * ePts[2];
	padr[7] = MInv(m) * tipM * ePts[3];
	
	CPolygon *vadr = GetPolygonArray(poly);
	vadr[0] = CPolygon(0,3,2,1);
	vadr[1] = CPolygon(4,5,6,7);
	vadr[2] = CPolygon(0,4,7,3);
	vadr[3] = CPolygon(1,5,4,0);
	vadr[4] = CPolygon(2,6,5,1);
	vadr[5] = CPolygon(3,7,6,2);
	
	return poly;
}

Bool CDJointsToPolygons::ChildJointsToPolygons(BaseDocument* doc, BaseObject *op, BaseObject *bn)
{
	BaseObject *prev=NULL, *poly=NULL, *prevCH=NULL;
	BaseTag *opTag=NULL, *tagClone=NULL;
	String opName;
	Matrix opMatrix;

	
	StatusSetSpin();
	while(op && bn)
	{
		opName = op->GetName();
		opMatrix = op->GetMg();

		prevCH = bn->GetDown();
		while(prevCH)
		{
			prev = prevCH;
			prevCH = prevCH->GetNext();
		}
		if(op->GetType() == ID_CDJOINTOBJECT)
		{
			poly = CreatePolygonObject(doc,op,bn,prev);
			if(!poly) return false;
			
			poly->SetName(opName);
			poly->SetMg(opMatrix);
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDTransferGoals(op,poly);
			for (opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
			{
				tagClone = NULL;
				tagClone = (BaseTag*)CDGetClone(opTag,CD_COPYFLAGS_0,NULL);
				if (tagClone)
				{
					poly->InsertTag(tagClone,NULL);
					CDAddUndo(doc,CD_UNDO_NEW,tagClone);
					CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
					CDTransferGoals(opTag,tagClone);
				}
			}
			poly->Message(MSG_UPDATE);
		}
		else
		{
			poly = (BaseObject*)CDGetClone(op,CD_COPYFLAGS_NO_HIERARCHY|CD_COPYFLAGS_NO_BRANCHES|CD_COPYFLAGS_PRIVATE_NO_INTERNALS,NULL);
			if (poly)
			{
				doc->InsertObject(poly,bn,prev,false);
				poly->SetMg(opMatrix);
				CDAddUndo(doc,CD_UNDO_NEW,poly);
				CDAddUndo(doc,CD_UNDO_CHANGE,op);
				CDTransferGoals(op,poly);
				for (BaseTag* opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
				{
					tagClone = NULL;
					tagClone = (BaseTag*)CDGetClone(opTag,CD_COPYFLAGS_0,NULL);
					if (tagClone)
					{
						poly->InsertTag(tagClone,NULL);
						CDAddUndo(doc,CD_UNDO_NEW,tagClone);
						CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
						CDTransferGoals(opTag,tagClone);
					}
				}
			}
		}
		prev = poly;
		ChildJointsToPolygons(doc,op->GetDown(),poly);
		op = op->GetNext();
	}
	return true;
}

Bool CDJointsToPolygons::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	BaseObject *op=NULL, *poly=NULL, *ch=NULL;
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	if(objects->GetCount() == 0)
	{
		MessageDialog(GeLoadString(A_SEL_OBJ_JOINT));
		return false;
	}
	LONG i, opCount = objects->GetCount();
	Matrix opMatrix;
	String opName;
	BaseTag *tagClone=NULL;
	
	StatusSetSpin();
	doc->StartUndo();
	for(i=0; i<opCount; i++)
	{
		op = static_cast<BaseObject*>(objects->GetIndex(i));
		if(op && op->GetType() == ID_CDJOINTOBJECT)
		{
			opMatrix = op->GetMg();
			opName = op->GetName();
			
			poly = CreatePolygonObject(doc,op,op->GetUp(),op);
			if(!poly)
			{
				//GePrint("no poly object");
				doc->EndUndo();
				StatusClear();
				return true;
			}
			poly->SetName(opName);
			poly->SetMg(opMatrix);
			CDAddUndo(doc,CD_UNDO_CHANGE,op);
			CDTransferGoals(op,poly);
			for (BaseTag* opTag = op->GetFirstTag(); opTag; opTag = opTag->GetNext())
			{
				tagClone = (BaseTag*)CDGetClone(opTag,CD_COPYFLAGS_0,NULL);
				
				if (tagClone)
				{
					poly->InsertTag(tagClone,NULL);
					CDAddUndo(doc,CD_UNDO_NEW,tagClone);
					CDAddUndo(doc,CD_UNDO_CHANGE,opTag);
					CDTransferGoals(opTag,tagClone);
				}
			}
			poly->Message(MSG_UPDATE);
			
			ch = op->GetDown();
			if(ch)
			{
				if(!ChildJointsToPolygons(doc,op->GetDown(),poly))
				{
					doc->EndUndo();
					StatusClear();
					return true;
				}
			}
		}
	}
	doc->EndUndo();
	StatusClear();

	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDJointsToPolygonsR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDJointsToPolygons(void)
{
	Bool reg = true;

	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDJS_SERIAL_SIZE];
	String cdjnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDJOINTSANDSKIN,data,CDJS_SERIAL_SIZE)) reg = false;
	
	cdjnr.SetCString(data,CDJS_SERIAL_SIZE-1);
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
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDJTOP); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDJOINTSTOPOLYGONS,name,PLUGINFLAG_HIDE,"CDJoints-Polys.tif","CD Joints to Polygons",CDDataAllocator(CDJointsToPolygonsR));
	else return CDRegisterCommandPlugin(ID_CDJOINTSTOPOLYGONS,name,0,"CDJoints-Polys.tif","CD Joints to Polygons",CDDataAllocator(CDJointsToPolygons));
}
