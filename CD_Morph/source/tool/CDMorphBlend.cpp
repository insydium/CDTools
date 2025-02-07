//	Cactus Dan's Morph
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"

#include "CDMorph.h"

enum
{
	POINT_COUNT				= 1002,
	
	MT_DEST_LINK			= 1010,
	SET_SELECTION			= 1011,
	
	EDIT_SELECTION			= 1031,
	
	BS_POINT_COUNT			= 2000
};

class CDMBlendDialog: public SubDialog
{
	private:
		
	public:
		virtual Bool CreateLayout(void);
		virtual Bool InitValues(void);
		//virtual Bool CoreMessage(LONG id,const BaseContainer &msg);
		virtual Bool InitDialog(void);
		virtual Bool Command(LONG id,const BaseContainer &msg);
};

Bool CDMBlendDialog::InitDialog(void)
{
	//GePrint("CDMBlendDialog::InitDialog");
	BaseContainer *bc = GetToolData(GetActiveDocument(),ID_CDMORPHBLEND);
	if(!bc) return FALSE;
	
	return TRUE;
}

Bool CDMBlendDialog::CreateLayout(void)
{
	//GePrint("CDMBlendDialog::CreateLayout");
	
	GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
	{
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,0,0,0,GeLoadString(D_MB_RADIUS),0);
			AddEditNumberArrows(D_MB_RADIUS,BFH_LEFT,70,0);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupSpace(4,1);
			AddCheckbox(D_MB_VISIBLE_ONLY,BFH_LEFT,0,0,GeLoadString(D_MB_VISIBLE_ONLY));
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,0,0,0,GeLoadString(D_MB_STRENGTH),0);
			AddEditSlider(D_MB_STRENGTH,BFH_SCALEFIT);
		}
		GroupEnd();
	}
	GroupEnd();
	
	return TRUE;
}

Bool CDMBlendDialog::InitValues(void)
{
	//GePrint("CDMBlendDialog::InitValues");
	
	// first call the parent instance
	if(!GeDialog::InitValues()) return FALSE;
	
	BaseContainer *wpData = GetWorldPluginData(ID_CDMORPHBLEND);
	if(!wpData)
	{
		SetReal(D_MB_RADIUS,20,1,100,1,FORMAT_LONG);
		SetBool(D_MB_VISIBLE_ONLY, TRUE);
		SetReal(D_MB_STRENGTH,0.1,0,1,0.01,FORMAT_PERCENT);
	}
	else
	{
		SetReal(D_MB_RADIUS,wpData->GetReal(D_MB_RADIUS),1,100,1,FORMAT_LONG);
		SetBool(D_MB_VISIBLE_ONLY, wpData->GetBool(D_MB_VISIBLE_ONLY));
		SetReal(D_MB_STRENGTH,wpData->GetReal(D_MB_STRENGTH),0,1,0.01,FORMAT_PERCENT);
	}
	
	return InitDialog();
}

Bool CDMBlendDialog::Command(LONG id,const BaseContainer &msg)
{
	Real r,s;
	Bool vo;
	
	GetReal(D_MB_RADIUS,r);
	GetBool(D_MB_VISIBLE_ONLY,vo);
	GetReal(D_MB_STRENGTH,s);
	
	BaseDocument *doc = GetActiveDocument(); if(!doc) return FALSE;
	BaseContainer *bc = GetToolData(doc,ID_CDMORPHBLEND);
	if(!bc) return FALSE;
	
	bc->SetReal(D_MB_RADIUS,r);
	bc->SetBool(D_MB_VISIBLE_ONLY,vo);
	bc->SetReal(D_MB_STRENGTH,s);
	
	BaseContainer wpData;
	wpData.SetReal(D_MB_RADIUS,r);
	wpData.SetBool(D_MB_VISIBLE_ONLY,vo);
	wpData.SetReal(D_MB_STRENGTH,s);
	SetWorldPluginData(ID_CDMORPHSPLIT,wpData,FALSE);

	return TRUE;
}

class CDMorphBlend : public ToolData
{
	private:
		Real			mx, my;// , *tWeight, *paintWeight
		Bool			mDrag;// , *mPass
		
		Vector GetProjectionRayDelta(LONG projection);
		
	public:
			CDMBlendDialog		*dlg;
		
		virtual Bool	InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt);
		virtual Bool	MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg);
		virtual LONG	GetState(BaseDocument *doc);
		virtual Bool    GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x, Real y, BaseContainer &bc);
		virtual LONG    Draw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt,LONG flags);
		
		virtual SubDialog*	AllocSubDialog(BaseContainer* bc) { dlg = gNew CDMBlendDialog; return dlg; }
};

Bool CDMorphBlend::InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt)
{
	//GePrint("CDMorphBlend::InitTool");
	BaseContainer *wpData = GetWorldPluginData(ID_CDMORPHBLEND);
	if(!wpData)
	{
		data.SetReal(D_MB_RADIUS,20);
		data.SetBool(D_MB_VISIBLE_ONLY,TRUE);
		data.SetReal(D_MB_STRENGTH,0.01);
	}
	else
	{
		data.SetReal(D_MB_RADIUS,wpData->GetReal(D_MB_RADIUS));
		data.SetBool(D_MB_VISIBLE_ONLY, wpData->GetBool(D_MB_VISIBLE_ONLY));
		data.SetReal(D_MB_STRENGTH,wpData->GetReal(D_MB_STRENGTH));
	}
	
	return TRUE;
}

LONG CDMorphBlend::GetState(BaseDocument *doc)
{
	//GePrint("CDMorphBlend::GetState");
	if(doc->GetMode()==Mpaint) return 0;
	
	return CMD_ENABLED;
}

LONG CDMorphBlend::Draw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags)
{
	BaseTag *tag = doc->GetActiveTag();
	if(tag && tag->GetType() == ID_CDMORPHTAGPLUGIN)
	{
		if(flags == DRAWFLAGS_HIGHLIGHT)
		{
			if(mDrag)
			{
				Real  rad = data.GetReal(D_MB_RADIUS);
				bd->SetPen(Vector(0,0,0));
				bd->Circle2D(LONG(mx),LONG(my),rad);
			}
			
			BaseContainer *tData = tag->GetDataInstance();
			if(tData)
			{
				BaseObject *op = tData->GetObjectLink(MT_DEST_LINK,doc);
				if(op)
				{
					Matrix opM = op->GetMg();
					
					BaseObject *dest = op;
					if(op->GetDeformCache()) dest = op->GetDeformCache();
					
					Vector *padr = GetPointArray(dest);
					if(padr)
					{
						CDMorphTagPlugin *mTag = static_cast<CDMorphTagPlugin*>(tag->GetNodeData());
						CDMPoint *mpadr = mTag->GetMorphPoints();
						if(mpadr)
						{
							LONG i, pInd, mCnt = tData->GetLong(BS_POINT_COUNT), pCnt = tData->GetLong(POINT_COUNT);
							
							AutoAlloc<ViewportSelect> vps;
							
							LONG left, top, right, bottom, width, height;
							bd->GetFrame(&left, &top, &right, &bottom);
							width = right - left + 1;
							height = bottom - top + 1;
							
							if(vps && vps->Init(width,height,bd,op,Mpoints,TRUE,VIEWPORT_USE_HN|VIEWPORT_USE_DEFORMERS))
							{
								Vector color = CDGetVertexStartColor();
								bd->SetPen(color);
								
								for(i=0; i<mCnt; i++)
								{
									pInd = mpadr[i].ind;
									if(pInd < pCnt)
									{
										Vector pt = padr[pInd];
										Vector scrPt = bd->WS(opM*pt);
										LONG x = scrPt.x;
										LONG y = scrPt.y;
										scrPt.z = 0.0;
										
										ViewportPixel *vp = vps->GetNearestPoint(op,x,y,10.0);
										if(vp)
										{
											if(vp->i == pInd) bd->Handle2D(scrPt,HANDLE_MIDDLE);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return DRAW_HANDLES|DRAW_AXIS;
}

Vector CDMorphBlend::GetProjectionRayDelta(LONG projection)
{
	Vector delta;
	
	if(projection > 1 && projection < 8)
	{
		switch(projection)
		{
			case 2:
				delta = Vector(-1000000,0,0);
				break;
			case 3:
				delta = Vector(1000000,0,0);
				break;
			case 4:
				delta = Vector(0,0,-1000000);
				break;
			case 5:
				delta = Vector(0,0,1000000);
				break;
			case 6:
				delta = Vector(0,1000000,0);
				break;
			case 7:
				delta = Vector(0,-1000000,0);
				break;
		}
	}
	
	return delta;
}

Bool CDMorphBlend::MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, EditorWindow *win, const BaseContainer &msg)
{
	//GePrint("CDMorphBlend::MouseInput");
	BaseTag *tag = doc->GetActiveTag(); if(!tag) return TRUE;
	if(tag->GetType() != ID_CDMORPHTAGPLUGIN) return TRUE;
	
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return TRUE;
	
	BaseObject *op = tData->GetObjectLink(MT_DEST_LINK,doc); if(!op) return TRUE;
	if(!IsValidPointObject(op)) return TRUE;
	
	CDMorphTagPlugin *mTag = static_cast<CDMorphTagPlugin*>(tag->GetNodeData());
	CDMPoint *mpadr = mTag->GetMorphPoints(); if(!mpadr) return TRUE;

	BaseObject *dest = op;
	//if(op->GetDeformCache()) dest = op->GetDeformCache();
	
	Matrix destM = dest->GetMg();
	Vector	*padr = NULL;
	LONG	i, mCnt = tData->GetLong(BS_POINT_COUNT), pCnt = tData->GetLong(POINT_COUNT);;

	padr = GetPointArray(dest);
	pCnt = ToPoint(dest)->GetPointCount();
	
	mx = msg.GetReal(BFM_INPUT_X);
	my = msg.GetReal(BFM_INPUT_Y);
	LONG button;
	
	switch (msg.GetLong(BFM_INPUT_CHANNEL))
	{
		case BFM_INPUT_MOUSELEFT : button=KEY_MLEFT; break;
		case BFM_INPUT_MOUSERIGHT: button=KEY_MRIGHT; break;
		default: return TRUE;
	}

	Real dx, dy, rad = data.GetReal(D_MB_RADIUS);
	Bool vOnly = data.GetBool(D_MB_VISIBLE_ONLY);
	
	AutoAlloc<ViewportSelect> vps; if(!vps) return TRUE;
	
	LONG left, top, right, bottom, width, height;
	bd->GetFrame(&left, &top, &right, &bottom);
	width = right - left + 1;
	height = bottom - top + 1;
	
	if(!vps->Init(width,height,bd,op,Mpoints,vOnly,VIEWPORT_USE_HN|VIEWPORT_USE_DEFORMERS)) return TRUE;
	vps->SetBrushRadius(rad);
	
	mDrag = FALSE;
	BaseContainer device;
	win->MouseDragStart(button,mx,my,MOUSEDRAG_DONTHIDEMOUSE|MOUSEDRAG_NOMOVE);

	Bool *mPass = (Bool*)GeAlloc(sizeof(Bool)*mCnt); if(!mPass) return TRUE;
	for(i=0; i<mCnt; i++)
	{
		mPass[i] = FALSE;
	}
	
	/*CPolygon *vadr = GetPolygonArray(dest);
	LONG vcnt = ToPoly(dest)->GetPolygonCount();
	
	AutoAlloc<GeRayCollider> rc; if(!rc) return TRUE;
	rc->Init(dest, TRUE);
	Vector	rayEnd, rayStart, rayDir, rayPt, opt, mpt;
	Real	rayLen;
	
	Bool *ptHdn = (Bool*)GeAlloc(sizeof(Bool)*pCnt);
	if(!ptHdn)
	{
		GeFree(mPass);
		return TRUE;
	}
	Bool *plyHdn = (Bool*)GeAlloc(sizeof(Bool)*vcnt);
	if(!ptHdn)
	{
		GeFree(mPass);
		GeFree(mPass);
		return TRUE;
	}
	for(i=0; i<pCnt; i++)
	{
		ptHdn[i] = FALSE;
	}
	for(i=0; i<vcnt; i++)
	{
		plyHdn[i] = FALSE;
	}

	//Get the hidden points
	BaseSelect *bsPt = ToPoint(dest)->GetPointH();
	LONG bsPtCount = bsPt->GetCount();
	if(bsPtCount > 0)
	{
		LONG seg=0,a,b;
		while (bsPt->GetRange(seg++,&a,&b))
		{
			for (i=a; i<=b; ++i)
			{
				ptHdn[i] = TRUE;
			}
		}
	}
	
	Neighbor n;
	if(IsValidPolygonObject(op))
	{
		//Get the hidden polygons
		BaseSelect *bsPly = ToPoly(op)->GetPolygonH();
		LONG bsPlyCount = bsPly->GetCount();
		if(bsPlyCount > 0)
		{
			LONG seg=0,a,b;
			while (bsPly->GetRange(seg++,&a,&b))
			{
				for (i=a; i<=b; ++i)
				{
					plyHdn[i] = TRUE;
				}
			}
		}
		if(!n.Init(pCnt,vadr,vcnt,NULL))
		{
			GeFree(mPass);
			GeFree(ptHdn);
			GeFree(plyHdn);
			return TRUE;
		}
	}	
				
	LONG projection = bd->GetProjection();
	Vector rayDelta = GetProjectionRayDelta(projection);*/

	Real scale, strength = data.GetReal(D_MB_STRENGTH);
	Bool clicked = FALSE;
	
	while (win->MouseDrag(&dx,&dy,&device)==MOUSEDRAG_CONTINUE)
	{
		Bool ctrl = (device.GetLong(BFM_INPUT_QUALIFIER) & QCTRL);

		if(!clicked)
		{
			clicked = TRUE;
			doc->StartUndo();
			doc->AddUndo(UNDO_CHANGE,tag);
		}
		else if(dx==0.0 && dy==0.0) continue;

		mx+=dx;
		my+=dy;
		mDrag = TRUE;
		
		LONG x = mx, y = my;
		Vector mpt = Vector(mx,my,0.0);
		ViewportPixel *vp = vps->GetNearestPoint(op,x,y);
		if(vp)
		{
			Vector pxl = Vector(x,y,0.0);
			if(Len(pxl - mpt) < rad)
			{
				for(i=0; i<mCnt; i++)
				{
					LONG mInd = mpadr[i].ind;
					if(mInd < pCnt && mInd == vp->i)
					{
						Vector pt = mpadr[i].GetVector();
						if(ctrl)
						{
							if(!mPass[i])
							{
								scale = 1 - strength;
								mpadr[i].SetVector(pt * scale);
								mPass[i] = TRUE;
							}
						}
						else
						{
							if(!mPass[i])
							{
								scale = 1 + strength;
								mpadr[i].SetVector(pt * scale);
								mPass[i] = TRUE;
							}
						}
					}
				}
			}
		}
		
		/*for(i=0; i<mCnt; i++)
		{
			LONG mInd = mpadr[i].ind;
			if(mInd < pCnt)
			{
				Bool ptHidden = FALSE;
				
				if(IsValidPolygonObject(op))
				{
					if(!ptHdn[mInd])
					{
						ptHidden = TRUE;
						LONG *dadr = NULL, dcnt = 0, p, poly;
						n.GetPointPolys(mInd, &dadr, &dcnt);
						for (p=0; p<dcnt; p++)
						{
							poly = dadr[p];
							if(!plyHdn[poly])
							{
								ptHidden = FALSE;
							}
						}
					}
				}
				
				if(!ptHidden)
				{
					opt = bd->WS(destM * padr[mInd]);
					opt.z = 0.0;
					mpt.x = mx;
					mpt.y = my;
					mpt.z = 0.0;
					if(Len(opt - mpt) < rad)
					{
						Bool ptVisible = TRUE;
						if(data.GetBool(D_MB_VISIBLE_ONLY))
						{
							//if polygon object check for ray collider
							if(IsValidPolygonObject(dest))
							{
								ptVisible = FALSE;
								rayEnd = bd->SW(opt);
								rayStart = destM * padr[mInd];
								if(projection > 1 && projection < 8)
								{
									rayEnd = rayStart + rayDelta;
								}
								rayLen = Len(rayEnd - rayStart);
								rayDir = (rayEnd - rayStart) ^ !destM;
								rayPt = padr[mInd] + !rayDir*0.1;
								if(!rc->Intersect(rayPt, !rayDir, rayLen))  ptVisible = TRUE;
								else
								{
									ptVisible = TRUE;
									LONG intInd, intCnt = rc->GetIntersectionCount();
									for(intInd=0; intInd<intCnt; intInd++)
									{
										GeRayColResult res;
										rc->GetIntersection(intInd, &res);
										if(!plyHdn[res.face_id])  ptVisible = FALSE;
									}
								}
							}
						}

						if(ptVisible)
						{
							Vector pt = mpadr[i].GetVector();
							if(ctrl)
							{
								if(!mPass[i])
								{
									scale = 1 - strength;
									mpadr[i].SetVector(pt * scale);
									mPass[i] = TRUE;
								}
							}
							else
							{
								if(!mPass[i])
								{
									scale = 1 + strength;
									mpadr[i].SetVector(pt * scale);
									mPass[i] = TRUE;
								}
							}
						}
					}
				}
			}
			
		}*/
		op->Message(MSG_UPDATE);
		
		//DrawViews(DA_INDRAG|DA_NO_REDUCTION|DA_NO_EXPRESSIONS|DA_ONLY_ACTIVE_VIEW|DA_NO_THREAD|DA_NO_ANIMATION);
		DrawViews(DA_INDRAG|DA_NO_REDUCTION|DA_ONLY_ACTIVE_VIEW|DA_NO_THREAD|DA_NO_ANIMATION|DA_ONLY_BASEDRAW,bd);
	}
	if(clicked)
	{
		tag->Message(MSG_UPDATE);
		clicked = FALSE;
		doc->EndUndo();
		DrawViews(DA_ONLY_ACTIVE_VIEW|DA_NO_THREAD|DA_NO_ANIMATION);
	}
	
	if(mPass) GeFree(mPass);
	//GeFree(ptHdn);
	//GeFree(plyHdn);
	
	mDrag = FALSE;
	
	EventAdd(EVENT_FORCEREDRAW);

	return TRUE;
}

Bool CDMorphBlend::GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x,Real y, BaseContainer &bc)
{
	//GePrint("CDMorphBlend::GetCursorInfo");
	bc.SetString(RESULT_BUBBLEHELP,GeLoadString(ID_CDMORPHBLEND));
	
	mx=x;
	my=y;
	
	DrawViews(DA_NO_REDUCTION|DA_NO_EXPRESSIONS|DA_ONLY_ACTIVE_VIEW|DA_NO_THREAD|DA_NO_ANIMATION);
	return TRUE;
}

class CDMorphBlendR : public CommandData
{
public:
	
	virtual Bool Execute(BaseDocument *doc)
	{
		//MessageDialog(GeLoadString(MD_PLEASE_REGISTER));
		return TRUE;
	}
};

Bool RegisterCDMorphBlendPlugin(void)
{
	Bool reg = TRUE;
	
	LONG pK;
	CHAR aK, bK, cK;
	SetRValues(&pK,&aK,&bK,&cK);
	
	CHAR b, data[CDM_SERIAL_SIZE];
	String cdmnr, kb;
	SerialInfo si;
	
	if(!ReadPluginInfo(ID_CDMORPH,data,CDM_SERIAL_SIZE)) reg = FALSE;
	
	cdmnr.SetCString(data,CDM_SERIAL_SIZE-1);
	if(!CheckKeyChecksum(cdmnr)) reg = FALSE;
	
	GeGetSerialInfo(SERIAL_CINEMA4D,&si);
	LONG seed = GetSeed(si.nr);
	
	kb = GetKeyByte(cdmnr,pK,2);
	b = GetKeyByte(seed,aK,bK,cK);
	if(kb != LongToHexString(b,2)) reg = FALSE;
	
	if(GeGetVersionType() & VERSION_DEMO) reg = TRUE;
	if(GetC4DVersion() >= 11000)
	{
		if(GeGetVersionType() & R11_VERSION_SAVABLEDEMO) reg = TRUE;
		if(GeGetVersionType() & R11_VERSION_SAVABLEDEMO_ACTIVE) reg = FALSE;
	}
	
	// decide by name if the plugin shall be registered - just for user convenience
	String name=GeLoadString(IDS_CDMBLEND); if (!name.Content()) return TRUE;
	if(!reg) return RegisterCommandPlugin(ID_CDMORPHBLEND,name,PLUGINFLAG_HIDE,"CDMorphBlend.tif","CD Morph Blend",gNew CDMorphBlendR);
	else return RegisterToolPlugin(ID_CDMORPHBLEND,name,PLUGINFLAG_TOOL_HIGHLIGHT|PLUGINFLAG_TOOL_EDITSTATES|PLUGINFLAG_TOOL_TWEAK,"CDMorphBlend.tif","CD Morph Blend",gNew CDMorphBlend);
}

