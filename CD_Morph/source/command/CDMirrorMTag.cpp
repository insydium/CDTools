//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch
	
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_colors.h"

#include "CDCompatibility.h"
#include "CDMorph.h"
#include "tCDMorphTag.h"

class CDMirrorMorphTagDialog : public GeDialog
{
	private:
		CDMOptionsUA 			ua;
		
		Bool DoMirror(Real t, LONG a);
		Bool MirrorTag(BaseDocument *doc, BaseObject *op, BaseTag *tag, Real t, LONG a);

	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};


Bool CDMirrorMorphTagDialog::CreateLayout(void)
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if(res)
	{
		SetTitle(GeLoadString(IDS_CDMIRRORMTAG));
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,0);
			
			AddUserArea(IDC_CDM_OPTIONS_IMAGE,BFH_SCALEFIT);
			AttachUserArea(ua,IDC_CDM_OPTIONS_IMAGE);
		}
		GroupEnd();

		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBorderNoTitle(BORDER_NONE);
			GroupBorderSpace(10,0,10,10);
			
			GroupBegin(0,BFH_SCALEFIT,1,0,GeLoadString(D_MIRROR_AXIS),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(8,8,8,8);
				
				AddRadioGroup(IDC_MIRROR_AXIS,BFH_CENTER,3,1);
					AddChild(IDC_MIRROR_AXIS, 0, GeLoadString(M_X));
					AddChild(IDC_MIRROR_AXIS, 1, GeLoadString(M_Y));
					AddChild(IDC_MIRROR_AXIS, 2, GeLoadString(M_Z));
			}
			GroupEnd();
			GroupBegin(0,BFH_SCALEFIT,3,0,"",0);
			{
				
				AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_TOLERANCE),0);
				AddEditNumberArrows(IDC_TOLERANCE,BFH_LEFT);
				AddButton(GADGET_MIRROR,BFH_RIGHT,0,0,GeLoadString(D_MIRROR));
			}
			GroupEnd();
		}
		GroupEnd();
	}

	return res;
}

Bool CDMirrorMorphTagDialog::InitValues(void)
{
	// first call the parent instance
	if (!GeDialog::InitValues()) return false;
	
	SetLong(IDC_MIRROR_AXIS,0);
	SetReal(IDC_TOLERANCE,0.005,0.001,100,0.001,FORMAT_REAL);


	return true;
}

Bool CDMirrorMorphTagDialog::Command(LONG id,const BaseContainer &msg)
{
	Real t; LONG a;
	
	GetReal(IDC_TOLERANCE,t);
	GetLong(IDC_MIRROR_AXIS,a);
	switch (id)
	{
		case GADGET_MIRROR:
			StopAllThreads(); // so the document can be safely modified
			DoMirror(t, a);
			break;
	}

	return true;
}

Bool CDMirrorMorphTagDialog::MirrorTag(BaseDocument *doc, BaseObject *op, BaseTag *tag, Real t, LONG a)
{
	BaseContainer	*mTData = tag->GetDataInstance();
	BaseObject 		*destObject = mTData->GetObjectLink(1010,doc); if(!destObject) return false;
	BaseTag 		*refTag = destObject->GetTag(ID_CDMORPHREFPLUGIN); if (!refTag) return true;
	BaseContainer	*rTData = refTag->GetDataInstance();
	if (!IsValidPointObject(destObject)) return true;
	
	LONG rPtCount = rTData->GetLong(1002), ptCount = ToPoint(destObject)->GetPointCount();
	if(rPtCount != ptCount) return true;
	
	CDMRefPoint *refadr = CDGetMorphReference(refTag); if(!refadr) return true;
	
	Vector	*padr = GetPointArray(destObject);
	CDMPoint *mpadr = CDGetMorphPoints(tag); if(!mpadr) return false;
	
	LONG i, j, transCount=0, mPtCount = mTData->GetLong(2000);
	Real difference;
	Vector rPoint, refPt;
	CDMPoint mPoint;
	CDArray<CDMPoint> tempPoints;
	tempPoints.Alloc(mPtCount);
	
	CDMSpPoint sPoint, *sphPts = NULL;
	CDArray<CDMSpPoint> tempSphPts;
	if(mTData->GetBool(MT_MIDPOINT_EXISTS))
	{
		sphPts = CDGetMorphSphericalPoints(tag);
		tempSphPts.Alloc(mPtCount);
	}
	
	for(j=0; j<mPtCount; j++)
	{
		tempPoints[j].ind = -1;
	}
	for(i=0; i<rPtCount; i++)
	{
		refPt = refadr[i].GetVector();
		for(j=0; j<mPtCount; j++)
		{
			rPoint = refadr[mpadr[j].ind].GetVector();
			mPoint = mpadr[j];
			switch (a)
			{
				case 0:
					rPoint.x *= -1;
					mPoint.x *= -1;
					break;
				case 1:
					rPoint.y *= -1;
					mPoint.y *= -1;
					break;
				case 2:
					rPoint.z *= -1;
					mPoint.z *= -1;
					break;
				default:
				break;
			}
			difference = Len(refPt - rPoint);
			if(t > difference)
			{
				mPoint.ind = i;
				tempPoints[j] = mPoint;
				transCount++;
				
				// check for spherical morph
				if(mTData->GetBool(MT_MIDPOINT_EXISTS))
				{
					sPoint = sphPts[j];
					switch (a)
					{
						case 0:
							sPoint.normal.x *= -1;
							sPoint.center.x *= -1;
							break;
						case 1:
							sPoint.normal.y *= -1;
							sPoint.center.y *= -1;
							break;
						case 2:
							sPoint.normal.z *= -1;
							sPoint.center.z *= -1;
							break;
						default:
						break;
					}
					tempSphPts[j] = sPoint;
				}
			}
		}
		if(transCount == mPtCount) break;
	}
	if(transCount != mPtCount)
	{
		MessageDialog(GeLoadString(NOT_ALL));
	}
	BaseSelect *bs = ToPoint(destObject)->GetPointS();
	if (bs) bs->DeselectAll();
	for(j=0; j<mPtCount; j++)
	{
		if(tempPoints[j].ind > -1)
		{
			bs->Select(tempPoints[j].ind);
		}
	}
	BaseTag *prev = NULL;
	if(op == destObject) prev = refTag;
	
	BaseTag *cdmTag = NULL;
	cdmTag = BaseTag::Alloc(ID_CDMORPHTAGPLUGIN);
	op->InsertTag(cdmTag,prev);
	cdmTag->Message(MSG_MENUPREPARE);
	CDAddUndo(doc,CD_UNDO_NEW,cdmTag);
	
	CDAddUndo(doc,CD_UNDO_CHANGE,cdmTag);
	BaseContainer *tagData = cdmTag->GetDataInstance();
	tagData->SetLink(1010,destObject);
	cdmTag->Message(MSG_MENUPREPARE);
	
	DescriptionCommand dc;
	dc.id = DescID(MT_SET_SELECTION);
	cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	Vector offsetPt;
	for(j=0; j<mPtCount; j++)
	{
		if(tempPoints[j].ind > -1)
		{
			refPt = refadr[tempPoints[j].ind].GetVector();
			offsetPt = tempPoints[j].GetDelta(refPt);
			padr[tempPoints[j].ind] = offsetPt;
		}
	}
	dc.id = DescID(MT_SET_OFFSET);
	cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
	cdmTag->Message(MSG_UPDATE);

	if(mTData->GetBool(MT_MIDPOINT_EXISTS))
	{
		tagData->SetBool(MT_USE_MIDPOINT,true);
		for(j=0; j<mPtCount; j++)
		{
			if(tempPoints[j].ind > -1)
			{
				Vector refPt = refadr[tempPoints[j].ind].GetVector();
				Vector mpoint = mpadr[j].GetDelta(refPt);
				if(!VEqual(refPt,mpoint,0.001))
				{
					Vector cPt = tempSphPts[j].center.GetDelta(refPt);
					offsetPt = cPt + tempSphPts[j].normal.GetVector() * Len(refPt-cPt);
				}
				else offsetPt = refPt;
				padr[tempPoints[j].ind] = offsetPt;
			}
		}
		dc.id = DescID(MT_SET_MIDPOINT);
		cdmTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
		cdmTag->Message(MSG_UPDATE);
	}
	
	doc->SetActiveTag(cdmTag);
	
	tempPoints.Free();
	tempSphPts.Free();
	
	return true;
}

Bool CDMirrorMorphTagDialog::DoMirror(Real t, LONG a)
{
	BaseDocument *doc = GetActiveDocument();
	
	AutoAlloc<AtomArray> tags; if (!tags) return false;
	doc->GetActiveTags(tags);
	LONG ti, tagCount = tags->GetCount();
	BaseTag *tagActive = static_cast<BaseTag*>(tags->GetIndex(0));
	if(!tagActive)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return true;
	}
	Bool isMTag = false;
	for(ti=0; ti<tagCount; ti++)
	{
		tagActive = static_cast<BaseTag*>(tags->GetIndex(ti));
		if(tagActive->GetType() == ID_CDMORPHTAGPLUGIN)
		{
			isMTag = true;
		}
	}
	if(!isMTag)
	{
		MessageDialog(GeLoadString(A_NO_TAG));
		return true;
	}
	
	AutoAlloc<AtomArray> objects; if (!objects) return false;
	CDGetActiveObjects(doc,objects,CD_GETACTIVEOBJECTFLAGS_0);
	BaseObject *tagOp = static_cast<BaseObject*>(objects->GetIndex(0));
	if(!tagOp)
	{
		MessageDialog(GeLoadString(A_SELECT_OBJECT));
		return false;	
	}
	
	doc->StartUndo();
	
	for(ti=0; ti<tagCount; ti++)
	{
		StatusSetBar(LONG(Real(ti)/Real(tagCount)*100.0));
		tagActive = static_cast<BaseTag*>(tags->GetIndex(ti));
		if(tagActive->GetType() == ID_CDMORPHTAGPLUGIN)
		{
			MirrorTag(doc,tagOp,tagActive,t,a);
		}
	}
	StatusClear();
	
	doc->EndUndo();
	
	EventAdd(EVENT_FORCEREDRAW);

	return true;
}

class CDMirrorMorphTagPlugin : public CommandData
{
	private:
		CDMirrorMorphTagDialog dlg;

	public:

		virtual Bool Execute(BaseDocument *doc)
		{
		#if API_VERSION < 12000
			 return dlg.Open(true,ID_CDMIRRORMTAGPLUGIN,-1,-1);
		#else
			return dlg.Open(DLG_TYPE_ASYNC,ID_CDMIRRORMTAGPLUGIN,-1,-1);
		#endif
		}

		virtual Bool RestoreLayout(void *secret)
		{
			return dlg.RestoreLayout(ID_CDMIRRORMTAGPLUGIN,0,secret);
		}
};

class CDMirrorMorphTagPluginR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMirrorMorphTagPlugin(void)
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
#else
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO) reg = true;
	if(GeGetSystemInfo() & SYSTEMINFO_SAVABLEDEMO_ACTIVE) reg = false;
#endif
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDMIRRORMTAG); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMIRRORMTAGPLUGIN,name,PLUGINFLAG_HIDE,"CDMirrorMorph.tif","CD Mirror Morph Tag",CDDataAllocator(CDMirrorMorphTagPluginR));
	else return CDRegisterCommandPlugin(ID_CDMIRRORMTAGPLUGIN,name,0,"CDMirrorMorph.tif","CD Mirror Morph Tag",CDDataAllocator(CDMirrorMorphTagPlugin));
}
