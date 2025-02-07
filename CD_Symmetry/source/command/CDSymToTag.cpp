//	Cactus Dan's Symmetry Tools 1.0 plugin
//	Copyright 2009 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"
#include "osymmetry.h"

#include "tCDSymTag.h"
#include "CDSymmetry.h"

class CDSymmetryToTag : public CommandData
{
	private:
		void ConvertChildren(BaseDocument *doc, BaseObject *ch, Matrix prM, LONG axis);
		Bool DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, LONG axis);
		LONG GetSymmetryAxis(BaseObject *op);
		
	public:
		virtual Bool Execute(BaseDocument *doc);
};

Bool CDSymmetryToTag::DeleteSymmetryPlanePolys(BaseDocument *doc, BaseObject *op, LONG axis)
{
	if(!IsValidPolygonObject(op)) return false;
	
	CPolygon *vadr = GetPolygonArray(op);
	Vector *padr = GetPointArray(op);
	LONG i, vCnt = ToPoly(op)->GetPolygonCount();
	
	BaseSelect *bs = ToPoly(op)->GetPolygonS();
	bs->DeselectAll();
	
	for(i=0; i<vCnt; i++)
	{
		CPolygon ply = vadr[i];
		Vector ptA = padr[ply.a];
		Vector ptB = padr[ply.b];
		Vector ptC = padr[ply.c];
		Vector ptD = padr[ply.d];
		
		switch(axis)
		{
			case 0: // x axis
			{
				if(ptA.x == 0 && ptB.x == 0 && ptC.x == 0 && ptD.x == 0) bs->Select(i);
				break;
			}
			case 1: // y axis
			{
				if(ptA.y == 0 && ptB.y == 0 && ptC.y == 0 && ptD.y == 0) bs->Select(i);
				break;
			}
			case 2: // z axis
			{
				if(ptA.z == 0 && ptB.z == 0 && ptC.z == 0 && ptD.z == 0) bs->Select(i);
				break;
			}
		}
	}
	
	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = op;
#if API_VERSION < 12000
	mcd.mode = MODIFY_POLYGONSELECTION;
#else
	mcd.mode = MODELINGCOMMANDMODE_POLYGONSELECTION;
#endif
	if(!SendModelingCommand(MCOMMAND_DELETE, mcd)) return false;
	
	return true;
}

void CDSymmetryToTag::ConvertChildren(BaseDocument *doc, BaseObject *ch, Matrix prM, LONG axis)
{
	while(ch)
	{
		Matrix chM = ch->GetMg();
		
		RepositionChildren(doc,ch->GetDown(),prM,chM,true);
		
		CDAddUndo(doc,CD_UNDO_CHANGE,ch);
		if(IsValidPointObject(ch))
		{
			RecalculatePoints(ch,prM,chM);
			ch->SetMg(prM);
			
			BaseTag *mpTag = NULL;
			if(ch->GetTag(ID_CDSYMMETRYTAG))
			{
				mpTag = ch->GetTag(ID_CDSYMMETRYTAG);
				BaseTag::Free(mpTag);
			}
			
			// check for symmetry plane polys
			if(DeleteSymmetryPlanePolys(doc,ch,axis)) CDOptimize(doc,ch,0.01,false,false,true,CD_MODELINGCOMMANDMODE_ALL);
			
			mpTag = BaseTag::Alloc(ID_CDSYMMETRYTAG);
			ch->InsertTag(mpTag,NULL);
			mpTag->Message(MSG_MENUPREPARE);
			CDAddUndo(doc,CD_UNDO_NEW,mpTag);
			
			CDAddUndo(doc,CD_UNDO_CHANGE_SMALL,mpTag);
			mpTag->Message(MSG_MENUPREPARE);
			
			BaseContainer *tData = mpTag->GetDataInstance();
			switch(axis)
			{
				case 0: // x axis
					tData->SetLong(SYM_SYMMETRY_AXIS,SYM_MX);
					break;
				case 1: // y axis
					tData->SetLong(SYM_SYMMETRY_AXIS,SYM_MY);
					break;
				case 2: // z axis
					tData->SetLong(SYM_SYMMETRY_AXIS,SYM_MZ);
					break;
			}
			mpTag->Message(MSG_UPDATE);
			
			DescriptionCommand dc;
			dc.id = DescID(SYM_SET_SYMMETRY);
			mpTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
		}
		else ch->SetMg(prM);
		
		ConvertChildren(doc,ch->GetDown(),prM,axis);
		
		ch = ch->GetNext();
	}
}

LONG CDSymmetryToTag::GetSymmetryAxis(BaseObject *op)
{
	LONG axis = 0;
	BaseContainer *oData = op->GetDataInstance();
	
	if(op->IsInstanceOf(Osymmetry))
	{
		switch(oData->GetLong(SYMMETRYOBJECT_PLANE))
		{
			case SYMMETRYOBJECT_PLANE_YZ:
				axis = 0;
				break;
			case SYMMETRYOBJECT_PLANE_XZ:
				axis = 1;
				break;
			case SYMMETRYOBJECT_PLANE_XY:
				axis = 2;
				break; 
		}
	}
	else if(op->IsInstanceOf(ID_CDDYNSYMMETRY))
	{
		switch(oData->GetLong(SYM_SYMMETRY_AXIS))
		{
			case SYM_MX:
				axis = 0;
				break;
			case SYM_MY:
				axis = 1;
				break;
			case SYM_MZ:
				axis = 2;
				break;
		}
	}
	
	return axis;
}

Bool CDSymmetryToTag::Execute(BaseDocument *doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AutoAlloc<AtomArray> objects; if(!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_CHILDREN);
	
	LONG  i, opCnt = objects->GetCount();
	if(opCnt > 0)
	{
		for(i=0; i<opCnt; i++)
		{
			BaseObject *op = static_cast<BaseObject*>(objects->GetIndex(i));
			if(op)
			{
				if(op->IsInstanceOf(Osymmetry) || op->IsInstanceOf(ID_CDDYNSYMMETRY))
				{
					Matrix opM = op->GetMg();
					LONG axis = GetSymmetryAxis(op);
					String name = op->GetName();
					
					Bool dynSym = false;
					if(op->IsInstanceOf(ID_CDDYNSYMMETRY))
					{
						dynSym = true;
						BaseObject *ch = op->GetDown();
						if(ch) name = ch->GetName();
					}
					
					ModelingCommandData mcd;
					mcd.doc = doc;
					mcd.op = op;
				#if API_VERSION < 12000
					mcd.flags = MODELINGCOMMANDFLAG_CREATEUNDO;
				#else
					mcd.flags = MODELINGCOMMANDFLAGS_CREATEUNDO;
				#endif

					if(!SendModelingCommand(MCOMMAND_MAKEEDITABLE, mcd)) return false;
					BaseObject *newOp = static_cast<BaseObject*>(mcd.result->GetIndex(0));
					newOp->SetName(name);
					
					ConvertChildren(doc,newOp->GetDown(),opM,axis);
				}
			}
		}
		EventAdd(EVENT_FORCEREDRAW);
	}
	
	return true;
}

class CDSymmetryToTagR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDSymmetryToTag(void)
{
	Bool reg = true;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDSY_SERIAL_SIZE];
	String cdsnr, kb;
	SerialInfo si;
	
	if(!CDReadPluginInfo(ID_CDSYMMETRYTOOLS,data,CDSY_SERIAL_SIZE)) reg = false;
	
	cdsnr.SetCString(data,CDSY_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdsnr)) reg = false;
	
	CDGeGetSerialInfo(CD_SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	LONG pos;
	Bool h = cdsnr.FindFirst("-",&pos);
	while(h)
	{
		cdsnr.Delete(pos,1);
		h = cdsnr.FindFirst("-",&pos);
	}
	cdsnr.ToUpper();
	
	kb = cdsnr.SubStr(pK,2);
	
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
	String name=GeLoadString(IDS_CDSYMTOTAG); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDSYMMETRYTOTAG,name,PLUGINFLAG_HIDE,"CDSymToTag.tif","CD Symmetry To Tag",CDDataAllocator(CDSymmetryToTagR));
	else return CDRegisterCommandPlugin(ID_CDSYMMETRYTOTAG,name,0,"CDSymToTag.tif","CD Symmetry To Tag",CDDataAllocator(CDSymmetryToTag));
}
