//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "stdlib.h"
#include "c4d_symbols.h"

//#include "CDCompatibility.h"
#include "CDJointSkin.h"
#include "CDSkinTag.h"
#include "CDSkinRefTag.h"

class CDMergeSkin : public CommandData
{
private:
	Bool JointInList(BaseObject *op, AtomArray *jList);
	Bool MergeSkinWeights(BaseDocument *doc, BaseTag *src, BaseTag *dst, LONG sCnt, LONG dCnt, LONG &vCnt, AtomArray *jList);
		
public:		
	virtual Bool Execute(BaseDocument* doc);
};

Bool CDMergeSkin::JointInList(BaseObject *op, AtomArray *jList)
{
	if(!op || !jList) return false;
	
	LONG i, jCnt = jList->GetCount();
	for(i=0; i<jCnt; i++)
	{
		BaseObject *jnt = static_cast<BaseObject*>(jList->GetIndex(i));
		if(jnt == op) return true;
	}
	
	return false;
}

Bool CDMergeSkin::MergeSkinWeights(BaseDocument *doc, BaseTag *src, BaseTag *dst, LONG sCnt, LONG dCnt, LONG &vCnt, AtomArray *jList)
{
	BaseContainer *sData = src->GetDataInstance(); if(!sData) return false;
	BaseContainer *dData = dst->GetDataInstance(); if(!dData) return false;
	
	CDSkinVertex *sWt = CDGetSkinWeights(src); if(!sWt) return false;
	CDSkinVertex *dWt = CDGetSkinWeights(dst); if(!dWt) return false;
	
	LONG i, j;
	for(i=0; i<sCnt; i++)
	{
		if(vCnt < dCnt)
		{
			if(sWt[i].taw > 0)
			{
				LONG djn = 0, jCnt = sWt[i].jn;
				for(j=0; j<jCnt; j++)
				{
					LONG indPlus = sWt[i].jPlus[j];
					BaseObject *jnt = sData->GetObjectLink(SKN_J_LINK+indPlus,doc);
					if(jnt)
					{
						LONG ind = jList->Find(jnt);
						if(ind > NOTOK)
						{
							dWt[vCnt].jPlus[j] = ind;
							dWt[vCnt].jw[ind] = sWt[i].jw[indPlus];
							djn++;
						}
					}
				}
				dWt[vCnt].jn = djn;
			}
			vCnt++;
		}
	}	
	
	return true;
}

Bool CDMergeSkin::Execute(BaseDocument* doc)
{
	StopAllThreads(); // so the document can be safely modified
	
	AtomArray *opSelectionList = GetSelectionLog(doc); if(!opSelectionList) return false;
	opSelectionList->FilterObjectChildren();
	
	LONG i, selCnt = opSelectionList->GetCount();
	if(selCnt > 1)
	{
		BaseObject *mrg = NULL;
		BaseObject *op = static_cast<BaseObject*>(opSelectionList->GetIndex(0));
		if(op)
		{
			Matrix opM = op->GetMg();
			BaseTag *skinTag = op->GetTag(ID_CDSKINPLUGIN);
			if(!skinTag)
			{
				MessageDialog(GeLoadString(A_SEL_OBJ_SKIN)); // object is not skinned
				return false;
			}
			
			LONG type = 0;
			if(IsValidPolygonObject(op)) type = 1;
			
			// remove incompatible objects
			for(i=1; i<selCnt; i++)
			{
				mrg = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
				if(type == 1)// polygon object
				{
					if(!IsValidPolygonObject(mrg)) opSelectionList->Remove(mrg);
				}
				else // point object
				{
					if(!IsValidPointObject(mrg)) opSelectionList->Remove(mrg);
				}
			}
			selCnt = opSelectionList->GetCount();
			
			// check for bound skinned objects
			for(i=0; i<selCnt; i++)
			{
				mrg = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
				if(mrg)
				{
					BaseTag *skTag = mrg->GetTag(ID_CDSKINPLUGIN);
					if(skTag)
					{
						BaseContainer *skData = skinTag->GetDataInstance();
						if(skData && skData->GetBool(SKN_BOUND))
						{
							MessageDialog(GeLoadString(MD_MERGE_UNBIND_MESH)); // unbind skin first
							return false;
						}
					}
				}
			}
			
			BaseContainer *tData = skinTag->GetDataInstance();
			if(tData)
			{
				// clone objects to be joined
				AutoAlloc<AtomArray> joinOps; if (!joinOps) return false;
				
				doc->SetActiveObject(NULL);
				
				BaseObject *pred = NULL;
				for(i=0; i<selCnt; i++)
				{
					mrg = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
					if(mrg)
					{
						BaseObject *cln = (BaseObject*)CDGetClone(mrg,CD_COPYFLAGS_NO_HIERARCHY,NULL);
						if(cln)
						{
							BaseTag *sknT = cln->GetTag(ID_CDSKINPLUGIN);
							if(sknT) BaseTag::Free(sknT); // remove skin tag
							
							BaseTag *skinRef = cln->GetTag(ID_CDSKINREFPLUGIN);
							if(skinRef) BaseTag::Free(skinRef); // remove skin ref tag
							
							BaseTag *mRef = cln->GetTag(ID_CDMORPHREFPLUGIN);
							if(mRef) BaseTag::Free(mRef); // remove morph ref tag
							
							BaseTag *mTag = cln->GetTag(ID_CDMORPHTAGPLUGIN);
							while(mTag)
							{
								BaseTag::Free(mTag);// remove morph tags
								mTag = cln->GetTag(ID_CDMORPHTAGPLUGIN);
							}
							
							doc->InsertObject(cln,NULL,pred);
							cln->SetMg(mrg->GetMg());
							doc->SetActiveObject(cln,SELECTION_ADD);
							joinOps->Append(cln);
							pred = cln;
						}
					}
				}
			
				CallCommand(12144);
				
				BaseObject *newOp = doc->GetActiveObject();
				newOp->SetName("Merged Skinned Meshes");
				
				// add skin ref tag
				BaseTag *skinRef = skinRef = BaseTag::Alloc(ID_CDSKINREFPLUGIN);
				if(skinRef)
				{
					newOp->InsertTag(skinRef,NULL);
					
					DescriptionCommand skrdc;
					skrdc.id = DescID(SKR_SET_REFERENCE);
					skinRef->Message(MSG_DESCRIPTION_COMMAND,&skrdc);
					
					// build merged joint list
					AutoAlloc<AtomArray> jList;
					if(jList)
					{
						for(i=0; i<selCnt; i++)
						{
							mrg = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
							if(mrg)
							{
								BaseTag *skTag = mrg->GetTag(ID_CDSKINPLUGIN);
								if(skTag)
								{
									BaseContainer *sData = skTag->GetDataInstance();
									if(sData)
									{
										LONG j, jCnt = sData->GetLong(SKN_J_COUNT);
										for(j=0; j<jCnt; j++)
										{
											BaseObject *jnt = sData->GetObjectLink(SKN_J_LINK+j,doc);
											if(jnt)
											{
												if(!JointInList(jnt,jList)) jList->Append(jnt);
											}
										}
									}
								}
							}
						}
					}
					
					// add skin tag
					BaseTag *dstTag = BaseTag::Alloc(ID_CDSKINPLUGIN);
					if(dstTag)
					{	
						newOp->InsertTag(dstTag,skinRef);
						
						BaseContainer *sData = dstTag->GetDataInstance();
						if(sData)
						{
							sData->SetLink(SKN_DEST_ID,newOp);
							sData->SetLong(SKN_J_COUNT,0);
							sData->SetLong(SKN_J_LINK_ID,0);
							
							LONG jCnt = 0, cnt = jList->GetCount();
							for(i=0; i<cnt; i++)
							{
								BaseObject *jnt = static_cast<BaseObject*>(jList->GetIndex(i));
								if(jnt)
								{
									sData->SetLink(SKN_J_LINK+jCnt,jnt);
									jCnt++;
								}
							}
							sData->SetLong(SKN_J_COUNT,jCnt);
							
							// Initialize skin tag's weight array
							dstTag->Message(MSG_MENUPREPARE);
							
							LONG vCnt = 0, dCnt = ToPoint(newOp)->GetPointCount();
							for(i=0; i<selCnt; i++)
							{
								mrg = static_cast<BaseObject*>(opSelectionList->GetIndex(i));
								if(mrg)
								{
									BaseTag *srcTag = mrg->GetTag(ID_CDSKINPLUGIN);
									if(srcTag)
									{
										LONG sCnt = ToPoint(mrg)->GetPointCount();
										MergeSkinWeights(doc,srcTag,dstTag,sCnt,dCnt,vCnt,jList);
									}
								}
							}
							
							// Set accumulated weights
							DescriptionCommand dc;
							dc.id = DescID(SKN_ACCUMULATE);
							dstTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							
							//Bind skin
							dc.id = DescID(SKN_AUTO_BIND_SKIN);
							dstTag->Message(MSG_DESCRIPTION_COMMAND,&dc);
							dstTag->Message(MSG_UPDATE);
						}
					}
				}
				
				// delete temporary objects
				LONG dCnt = joinOps->GetCount();
				for(i=0; i<dCnt; i++)
				{
					BaseObject *delOp = static_cast<BaseObject*>(joinOps->GetIndex(i));
					if(delOp) BaseObject::Free(delOp);
				}
				
				EventAdd(EVENT_FORCEREDRAW);
			}			
		}
	}

	return true;
}

class CDMergeSkinR : public CommandData
{
	public:		
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDMergeSkin(void)
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
	
	// decide by name ifthe plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDMERGE); if (!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_CDMERGESKIN,name,PLUGINFLAG_HIDE,"CDMergeSkin.TIF","CD Merge Skin",CDDataAllocator(CDMergeSkinR));
	else return CDRegisterCommandPlugin(ID_CDMERGESKIN,name,0,"CDMergeSkin.TIF","CD Merge Skin",CDDataAllocator(CDMergeSkin));
}
