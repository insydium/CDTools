//	Cactus Dan's Joints & Skin
//	Copyright 2008 by Cactus Dan Libisch

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "lib_clipmap.h"

#include "CDCompatibility.h"
#include "CDPaintSkin.h"
#include "CDJointSkin.h"
#include "CDClusterTag.h"
#include "CDSkinTag.h"

#include "CDClothFuncLib.h"

#include "tCDCluster.h"

Bool CDPaintSkinWeight::InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt)
{
	BaseContainer *bc = GetWorldPluginData(ID_SKINWEIGHTTOOL);
	if(!bc)
	{
		data.SetReal(D_WP_RADIUS,20);
		data.SetBool(D_WP_SOFT_EDGES,false);
		data.SetReal(D_WP_SOFTNESS,1.0);
		data.SetBool(D_WP_VISIBLE_ONLY,true);
		data.SetReal(D_WP_STRENGTH,1.0);
		data.SetReal(D_WP_CONTRAST,0);
		data.SetLong(D_WP_MODE,D_WP_MODE_SET);
		data.SetBool(D_WP_JCOLORS,false);
		data.SetBool(D_WP_NORMALIZED,true);
	}
	else
	{
		data.SetReal(D_WP_RADIUS,bc->GetReal(D_WP_RADIUS));
		data.SetBool(D_WP_SOFT_EDGES, bc->GetBool(D_WP_SOFT_EDGES));
		data.SetReal(D_WP_SOFTNESS,bc->GetReal(D_WP_SOFTNESS));
		data.SetBool(D_WP_VISIBLE_ONLY, bc->GetBool(D_WP_VISIBLE_ONLY));
		data.SetReal(D_WP_CONTRAST,bc->GetReal(D_WP_CONTRAST));
		data.SetLong(D_WP_MODE,bc->GetLong(D_WP_MODE));
		data.SetReal(D_WP_STRENGTH,bc->GetReal(D_WP_STRENGTH));
		data.SetBool(D_WP_JCOLORS, bc->GetBool(D_WP_JCOLORS));
		data.SetBool(D_WP_NORMALIZED, bc->GetBool(D_WP_NORMALIZED));
	}
	wHilight = false;
	mWtSmpl = 0.0;

	UpdatePaintData(doc,data);
	
	vw1 = CDGetVertexStartColor();
	vw2 = CDGetVertexEndColor();

#if API_VERSION > 11999
	displayBD = doc->GetActiveBaseDraw();
	StoreDisplayFilters(displayBD);
	SetDisplayFilters(displayBD);
#endif
	
	return true;
}


void CDPaintSkinWeight::FreeTool(BaseDocument* doc, BaseContainer& data)
{
#if API_VERSION > 11999
	if(doc)
	{
		displayBD = doc->GetActiveBaseDraw();
		RestoreDisplayFilters(displayBD);
	}
#endif
	
	StatusClear();
}

Bool CDPaintSkinWeight::SetDisplayFilters(BaseDraw* bd)
{
	GeData off = GeData(false);
	CDSetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_SDS),off);
	CDSetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_HYPERNURBS),off);
	CDSetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_DEFORMER),off);
	
	return true;
}

Bool CDPaintSkinWeight::StoreDisplayFilters(BaseDraw* bd)
{
	if(!CDGetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_SDS),dispFilterSDS)) return false;
	if(!CDGetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_HYPERNURBS),dispFilterHN)) return false;
	if(!CDGetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_DEFORMER),dispFilterDF)) return false;
	
	return true;
}

Bool CDPaintSkinWeight::RestoreDisplayFilters(BaseDraw* bd)
{
	CDSetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_SDS),dispFilterSDS);
	CDSetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_HYPERNURBS),dispFilterHN);
	CDSetParameter(bd,DescLevel(BASEDRAW_DISPLAYFILTER_DEFORMER),dispFilterDF);
	
	return true;
}

void CDPaintSkinWeight::UpdatePaintData(BaseDocument *doc, BaseContainer &data)
{
	BaseObject *mesh = NULL;
	BaseTag *tagActive = doc->GetActiveTag(), *theTag=NULL;
	BaseList2D *tag = data.GetLink(WP_PREV_TAG_LINK,doc);
	if(tagActive)
	{
		if(tagActive != tag)
		{
			if(tagActive->GetType() == ID_CDSKINPLUGIN || tagActive->GetType() == ID_CDCLOTHTAG || tagActive->GetType() == ID_CDROPETAG)
			{
				data.SetLink(WP_PREV_TAG_LINK,tagActive);
				theTag = tagActive;
				mesh = theTag->GetObject();
			}
			else if(tag && (tag->GetType() == ID_CDSKINPLUGIN || tag->GetType() == ID_CDCLOTHTAG))
			{
				theTag = static_cast<BaseTag*>(tag);
				mesh = theTag->GetObject();
			}
			
			SpecialEventAdd(ID_SKINWEIGHTTOOL); // needed to update tool dialog layout // if(dlg) dlg->ReLayout();// 
		}
		
		if(tagActive->GetType() == ID_CDCLUSTERTAGPLUGIN)
		{
			BaseContainer *tData = tagActive->GetDataInstance();
			if(tData) mesh = tData->GetObjectLink(CLST_DEST_LINK,doc);
		}
	}
	else if(tag)
	{
		if(tag->GetType() == ID_CDSKINPLUGIN || tag->GetType() == ID_CDCLOTHTAG || tag->GetType() == ID_CDROPETAG)
		{
			theTag = static_cast<BaseTag*>(tag);
		}
	}
	
	BaseObject *opActive = doc->GetActiveObject();
	if(opActive && theTag)
	{
		if(theTag->GetType() == ID_CDSKINPLUGIN)
		{
			DescriptionCommand dc;
			BaseContainer	*tData = theTag->GetDataInstance();
			LONG i, jCount = tData->GetLong(SKN_J_COUNT);
			for(i=0; i<jCount; i++)
			{
				if(opActive == tData->GetObjectLink(SKN_J_LINK+i,doc))
				{
					tData->SetLong(SKN_J_LINK_ID,i);
					theTag->Message(MSG_UPDATE);
				}
			}
		}
	}
	
	if(mesh)
	{
		BaseObject *prvMesh = (BaseObject*)data.GetLink(WP_PREV_MESH_LINK, doc);
		if(mesh != prvMesh)
		{
			data.SetLink(WP_PREV_MESH_LINK, mesh);
		}
	}	
}

Bool CDPaintSkinWeight::Message(BaseDocument *doc, BaseContainer &data, LONG type, void *t_data)
{
	switch(type)
	{
		case CD_MSG_UPDATE:
		{
			UpdatePaintData(doc, data);
			break;
		}
	}
	
	return true;
}

LONG CDPaintSkinWeight::GetState(BaseDocument *doc)
{
	if(doc->GetMode() == Mpaint) return 0;

	return CMD_ENABLED;
}

Bool CDPaintSkinWeight::DrawText(LONG xpos, LONG ypos, BaseDraw *bd, Bool total)
{
	AutoAlloc<GeClipMap> cm;
	if(!cm)
	{
		return false;
	}

#if API_VERSION < 12000
	if(!cm->Init(0, 0, 32))
#else
	if(cm->Init(0, 0, 32) != IMAGERESULT_OK)
#endif
	{
		return false;
	}
	cm->BeginDraw();
	LONG width, height;
	
#if API_VERSION < 13000
	if(total)
	{
		if(cm->TextWidth(mWt) > cm->TextWidth(mWtT)) width = cm->TextWidth(mWt) + 4;
		else width = cm->TextWidth(mWtT) + 4;
		height = 2 * cm->TextHeight() + 2;
	}
	else
	{
		width = cm->TextWidth(mWt) + 4;
		height = cm->TextHeight() + 2;
	}
#else
	if(total)
	{
		if(cm->GetTextWidth(mWt) > cm->GetTextWidth(mWtT)) width = cm->GetTextWidth(mWt) + 4;
		else width = cm->GetTextWidth(mWtT) + 4;
		height = 2 * cm->GetTextHeight() + 2;
	}
	else
	{
		width = cm->GetTextWidth(mWt) + 4;
		height = cm->GetTextHeight() + 2;
	}
#endif
	
	cm->EndDraw();
	cm->Destroy();

#if API_VERSION < 12000
	if(!cm->Init(width, height, 32))
#else
	if(cm->Init(width, height, 32) != IMAGERESULT_OK)
#endif
	{
		return false;
	}
	cm->BeginDraw();
	
	// Get HUD color and opacity parameters
	Vector bkClr = (bd->GetParameterData(BASEDRAW_HUD_BACKCOLOR)).GetVector();
	Vector txClr = (bd->GetParameterData(BASEDRAW_HUD_TEXTCOLOR)).GetVector();
	Real bkOpc = (bd->GetParameterData(BASEDRAW_HUD_BACKOPACITY)).GetReal();
	Real txOpc = (bd->GetParameterData(BASEDRAW_HUD_TEXTOPACITY)).GetReal();
	
	// draw rectangle
	cm->SetColor(bkClr.x*130,bkClr.y*130,bkClr.z*130,bkOpc*400);
	cm->Rect(0,0,width-1,height-1);
	
	// draw filled rectangle
	cm->SetColor(bkClr.x*255,bkClr.y*255,bkClr.z*255,bkOpc*255);
	cm->FillRect(1,1,width-2,height-2);
	
	// draw corners
	cm->SetColor(0,0,0,0);
	cm->SetPixel(0,0);
	cm->SetPixel(0,height-1);
	cm->SetPixel(width-1,0);
	cm->SetPixel(width-1,height-1);
	
	// draw text
	cm->SetColor(txClr.x*255,txClr.y*255,txClr.z*255,txOpc*255);
	cm->TextAt(2,1,mWt);
	if(total) cm->TextAt(2,height*0.5,mWtT);
	cm->EndDraw();

	bd->SetMatrix_Screen();
	bd->SetLightList(BDRAW_SETLIGHTLIST_NOLIGHTS);

	Vector padr[4];
	Vector cadr[4];
	Vector vnadr[4];
	Vector uvadr[4];
	
	padr[0] = Vector(xpos,ypos,0);
	padr[1] = Vector(xpos+width,ypos,0);
	padr[2] = Vector(xpos+width,ypos+height,0);
	padr[3] = Vector(xpos,ypos+height,0);

	cadr[0] = Vector(1,1,1);
	cadr[1] = Vector(1,1,1);
	cadr[2] = Vector(1,1,1);
	cadr[3] = Vector(1,1,1);

	vnadr[0] = Vector(0,0,1);
	vnadr[1] = Vector(0,0,1);
	vnadr[2] = Vector(0,0,1);
	vnadr[3] = Vector(0,0,1);

	uvadr[0] = Vector(0,0,0);
	uvadr[1] = Vector(1,0,0);
	uvadr[2] = Vector(1,1,0);
	uvadr[3] = Vector(0,1,0);

	BaseBitmap *cmbmp = NULL;
	cmbmp = cm->GetBitmap();
	if(!cmbmp)
	{
		return false;
	}

	BaseBitmap *bmp = NULL;
	bmp = cmbmp->GetClone();
	if(!bmp)
	{
		return false;
	}

#if API_VERSION < 12000
	bd->DrawTexture(bmp, padr, cadr, vnadr, uvadr, 4, false, DRAW_ALPHA_NORMAL);
#else
	bd->DrawTexture(bmp, padr, cadr, vnadr, uvadr, 4, DRAW_ALPHA_NORMAL, DRAW_TEXTUREFLAGS_0);
#endif

	BaseBitmap::Free(bmp);

	return true;
}

Bool CDPaintSkinWeight::DoToolDrawing(BaseDocument *doc, BaseContainer &data, BaseDraw *bd)
{
	Real  rad=data.GetReal(D_WP_RADIUS);
	
	BaseTag* tagActive = doc->GetActiveTag();
	if(!tagActive)
	{
		BaseList2D *tag = data.GetLink(WP_PREV_TAG_LINK,doc);
		if(tag)
		{
			tagActive = static_cast<BaseTag*>(tag);
		}
		else return false;
	}
	
	BaseContainer *tData = tagActive->GetDataInstance(); if(!tData) return false;
	BaseObject *op = NULL;
	
	LONG tType = tagActive->GetType();
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
			if(!tagActive->GetBit(BIT_ACTIVE)) return false;
			data.SetLink(WP_CURRENT_TAG_LINK,tagActive);
			op = tData->GetObjectLink(CLST_DEST_LINK,doc); if(!op) return false;
			break;		
		case ID_CDSKINPLUGIN:
			op = tagActive->GetObject(); if(!op) return false;
			break;
		case ID_CDCLOTHTAG:
			op = tagActive->GetObject(); if(!op) return false;
			break;
		case ID_CDROPETAG:
			op = tagActive->GetObject(); if(!op) return false;
			break;
		default:
			return false;
	}
	if(!IsValidPointObject(op)) return false;
	
	BaseObject *dest = op;
	if(op->GetDeformCache())
	{
		dest = op->GetDeformCache();
	}
	
	Real contrast = data.GetReal(D_WP_CONTRAST);
	Vector	*padr = GetPointArray(dest);
	if(dest->IsInstanceOf(Offd) || dest->IsInstanceOf(Ospline))
	{
		Real brightness = contrast + 1.0;
		Matrix opM = dest->GetMg();
		
		LONG i, pCnt = ToPoint(dest)->GetPointCount();
		
		CDArray<Real> tWeight; if(!tWeight.Alloc(pCnt)) return false;
		
		switch(tType)
		{
			case ID_CDCLUSTERTAGPLUGIN:
			{
				if(!CDGetClusterWeight(tagActive, tData, tWeight.Array(), pCnt))
				{
					tWeight.Free();
					return false;
				}
				
				CDSetBaseDrawMatrix(bd, dest, Matrix());
				
				for(i=0; i<pCnt; i++)
				{
					Vector pt = opM * padr[i];
					
					bd->SetPen(CDBlend(vw1/brightness,vw2*brightness,tWeight[i]));
					CDDrawHandle3D(bd,pt,CD_DRAWHANDLE_MIDDLE,0);
				}
				tWeight.Free();
				break;
			}
			case ID_CDSKINPLUGIN:
			{
				if(!IsValidPointObject(dest))
				{
					tWeight.Free();
					return false;
				}
				
				LONG opCount = tData->GetLong(SKN_J_COUNT);
				Bool opInList = false;
				BaseObject *opActive = doc->GetActiveObject();
				if(opActive)
				{
					for(i=0; i<opCount; i++)
					{
						if(opActive == tData->GetObjectLink(SKN_J_LINK+i,doc))
						{
							opInList = true;
							tData->SetLong(SKN_J_LINK_ID,i);
						}
					}
				}
				if(!opInList)
				{
					tWeight.Free();
					return false;
				}
				
				if(!CDGetJointWeight(tagActive, tData, tWeight.Array(), pCnt))
				{
					tWeight.Free();
					return false;
				}
				
				CDArray<Vector> jColor;
				if(!jColor.Alloc(pCnt))
				{
					tWeight.Free();
					return false;
				}
				CDGetSkinJointColors(tagActive, doc, tData, jColor.Array(), pCnt);  

				if(data.GetBool(D_WP_JCOLORS))
				{
					ObjectColorProperties prop;
					opActive->GetColorProperties(&prop);
					if(prop.usecolor > 0)
					{
						if(CDMinVector(prop.color,0.1))  vw2 = Vector(0.5,0.5,0.5);
						else  vw2 = prop.color;
					}
					else vw2 = Vector(0,0,0.75);
				}
				
				CDSetBaseDrawMatrix(bd, dest, Matrix());
				
				for(i=0; i<pCnt; i++)
				{
					Vector pt = opM * padr[i];
					
					if(data.GetBool(D_WP_JCOLORS))
					{
						vw1 = jColor[i]*.75;
					}

					bd->SetPen(CDBlend(vw1/brightness,vw2*brightness,tWeight[i]));
					CDDrawHandle3D(bd,pt,CD_DRAWHANDLE_MIDDLE,0);
				}
				jColor.Free();
				break;
			}
			case ID_CDROPETAG:
			{
				if(!CDRopeGetWeight(tagActive, tData, tWeight.Array(), pCnt))
				{
					tWeight.Free();
					return false;
				}
			 
				CDSetBaseDrawMatrix(bd, dest, Matrix());
			 
				for(i=0; i<pCnt; i++)
				{
					Vector pt = opM * padr[i];
			 
					bd->SetPen(CDBlend(vw1/brightness,vw2*brightness,tWeight[i]));
					CDDrawHandle3D(bd,pt,CD_DRAWHANDLE_MIDDLE,0);
				}
				tWeight.Free();
				break;
			}
		}
		tWeight.Free();
	}
	
	if(mDrag)
	{
		bd->SetPen(Vector(0,0,0));
		CDDrawCircle2D(bd,LONG(mx),LONG(my),rad);
		if(contrast > 1.0)
		{
			bd->SetPen(Vector(1,1,1));
			bd->SetTransparency(-150);
			CDDrawCircle2D(bd,LONG(mx),LONG(my),rad);
			bd->SetTransparency(0);
		}
	}
	
	if(!mDrag && wHilight)
	{
		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
		
		switch (tType)
		{
			case ID_CDCLUSTERTAGPLUGIN:
			{
				Vector scrPt = bd->WS(dest->GetMg() * padr[weightDisplayIndex]);
				bd->SetPen(Vector(0,1,0));
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),5);
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),4);
				
				if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT)
				{
					DrawText(scrPt.x+12, scrPt.y-12, bd, false);
				}
				break;
			}
			case ID_CDSKINPLUGIN:
			{
				Vector scrPt = bd->WS(dest->GetMg() * padr[weightDisplayIndex]);
				bd->SetPen(Vector(0,1,0));
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),5);
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),4);
				
				if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT)
				{
					DrawText(scrPt.x+12, scrPt.y-20, bd, true);
				}
				break;
			}
			case ID_CDCLOTHTAG:
			{
				Vector scrPt = bd->WS(dest->GetMg() * padr[weightDisplayIndex]);
				bd->SetPen(Vector(0,1,0));
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),5);
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),4);
				
				if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT)
				{
					DrawText(scrPt.x+12, scrPt.y-12, bd, false);
				}
				break;
			}
			case ID_CDROPETAG:
			{
				Vector scrPt = bd->WS(dest->GetMg() * padr[weightDisplayIndex]);
				bd->SetPen(Vector(0,1,0));
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),5);
				CDDrawCircle2D(bd,LONG(scrPt.x),LONG(scrPt.y),4);
				
				if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT)
				{
					DrawText(scrPt.x+12, scrPt.y-12, bd, false);
				}
				break;
			}
		}
	}

	return true;
}

LONG CDPaintSkinWeight::CDDraw(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, BaseDrawHelp *bh, BaseThread *bt, LONG flags)
{
	if(!DoToolDrawing(doc,data,bd)) return CD_TOOLDRAW_0;
	
	return CD_TOOLDRAW_HANDLES|CD_TOOLDRAW_AXIS;
}

Vector CDPaintSkinWeight::GetProjectionRayDelta(LONG projection)
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

Bool CDPaintSkinWeight::MouseInput(BaseDocument *doc, BaseContainer &data, BaseDraw *bd,EditorWindow *win, const BaseContainer &msg)
{
	BaseTag* tagActive = doc->GetActiveTag();
	if(!tagActive)
	{
		BaseList2D *tag = data.GetLink(WP_PREV_TAG_LINK,doc);
		if(tag)
		{
			tagActive = static_cast<BaseTag*>(tag);
		}
		else return true;
	}
	
	
	LONG tType = tagActive->GetType();
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
			if(!tagActive->GetBit(BIT_ACTIVE)) return true;
			break;
		case ID_CDSKINPLUGIN:
			data.SetLink(WP_PREV_TAG_LINK,tagActive);
			break;
		case ID_CDCLOTHTAG:
			data.SetLink(WP_PREV_TAG_LINK,tagActive);
			break;
		case ID_CDROPETAG:
			data.SetLink(WP_PREV_TAG_LINK,tagActive);
			break;
		default:
			return true;
	}
	data.SetLink(WP_CURRENT_TAG_LINK,tagActive);
	
	BaseContainer	*tData = tagActive->GetDataInstance();
	BaseObject		*op = NULL, *joint = NULL;
	
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
			op = tData->GetObjectLink(CLST_DEST_LINK,doc);
			break;
		case ID_CDSKINPLUGIN:
			op = tagActive->GetObject();
			joint = tData->GetObjectLink(SKN_J_LINK+tData->GetLong(SKN_J_LINK_ID),doc);
			if(!joint) return true;
			if(!joint->GetBit(BIT_ACTIVE)) return true;
			break;
		case ID_CDCLOTHTAG:
			op = tagActive->GetObject();
			break;
		case ID_CDROPETAG:
			op = tagActive->GetObject();
			break;
	}
	if(!op)  return true;
	if(!IsValidPointObject(op)) return true;
	
	BaseObject *dest = op;
	if(op->GetDeformCache())
	{
		dest = op->GetDeformCache();
	}
	Bool polyOp = IsValidPolygonObject(dest);
	Bool splineOp = IsValidSplineObject(dest);
	
	Matrix opM = dest->GetMg();
	Vector	rayEnd, rayStart, rayDir, rayPt, opt, mpt;
	Vector	*padr = NULL;
	LONG	i, pCnt = 0, vCnt = 0;
	Real	rayLen;

	padr = GetPointArray(dest);
	pCnt = ToPoint(dest)->GetPointCount();
	
	AutoAlloc<GeRayCollider> rc; if(!rc) return true;
	CPolygon *vadr = NULL;
	if(polyOp)
	{
		vadr = GetPolygonArray(dest);
		if(vadr) vCnt = ToPoly(dest)->GetPolygonCount();
		if(!rc->Init(dest, false)) return true;
	}
	
	mx = msg.GetReal(BFM_INPUT_X);
	my = msg.GetReal(BFM_INPUT_Y);
	LONG button;
	
	switch (msg.GetLong(BFM_INPUT_CHANNEL))
	{
		case BFM_INPUT_MOUSELEFT : button=KEY_MLEFT; break;
		case BFM_INPUT_MOUSERIGHT: button=KEY_MRIGHT; break;
		default: return true;
	}
	wHilight = false;

	Real	dx, dy, rad=data.GetReal(D_WP_RADIUS);
	mDrag = false;
	BaseContainer bc, device;
	
	
#if API_VERSION < 12000
	win->MouseDragStart(button,mx,my,MOUSEDRAG_DONTHIDEMOUSE|MOUSEDRAG_NOMOVE);
#else
	win->MouseDragStart(button,mx,my,MOUSEDRAGFLAGS_DONTHIDEMOUSE|MOUSEDRAGFLAGS_NOMOVE);
#endif

	SplineObject *falloffspline = CDAllocateSplineObject(3,CD_SPLINETYPE_BSPLINE);
	if(!falloffspline) return false;
	Real falloff = 100.0 - data.GetReal(D_WP_SOFTNESS)*100.0, softness, softWeight;
	Vector *fsPts = GetPointArray(falloffspline);
	fsPts[0] = Vector(0.0,1.0,0.0);
	fsPts[1] = Vector(falloff, 1.0,0.0);
	fsPts[2] = Vector(100.0,0.0,0.0);
	
	CDSkinVertex *sknW = NULL;
	CDArray<Real> tWeight; if(!tWeight.Alloc(pCnt)) return true;
	
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
			if(!CDGetClusterWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			break;
		case ID_CDSKINPLUGIN:
			sknW = CDGetSkinWeights(tagActive);
			if(!sknW)
			{
				tWeight.Free();
				return true;
			}
			if(!CDGetJointWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			break;
		case ID_CDCLOTHTAG:
			if(!CDClothGetWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			break;
		case ID_CDROPETAG:
			if(!CDRopeGetWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			break;
	}

	CDArray<Bool> mPass; mPass.Alloc(pCnt); mPass.Fill(false);
	CDArray<Bool> ptHdn; ptHdn.Alloc(pCnt); ptHdn.Fill(false);
	CDArray<Bool> plyHdn; plyHdn.Alloc(vCnt); plyHdn.Fill(false);
	
	CDArray<Real> paintWeight;
	paintWeight.Alloc(pCnt);
	for(i=0; i<pCnt; i++)
	{
		paintWeight[i] = tWeight[i];
	}

	Vector softPt;
	Real strength;
	Bool clicked = false, softEdge = data.GetBool(D_WP_SOFT_EDGES);
	
	//Get the hidden points
	BaseSelect *bsPt = ToPoint(op)->GetPointH();
	LONG bsPtCount = bsPt->GetCount();
	if(bsPtCount > 0)
	{
		LONG seg=0,a,b;
		while (CDGetRange(bsPt,seg++,&a,&b))
		{
			for (i=a; i<=b; ++i)
			{
				ptHdn[i] = true;
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
			while (CDGetRange(bsPly,seg++,&a,&b))
			{
				for (i=a; i<=b; ++i)
				{
					plyHdn[i] = true;
				}
			}
		}
		if(!n.Init(pCnt,vadr,vCnt,NULL))
		{
			tWeight.Free();
			mPass.Free();
			ptHdn.Free();
			plyHdn.Free();
			paintWeight.Free();
			return false;
		}
	}	
				
	LONG projection = bd->GetProjection();
	Vector rayDelta = GetProjectionRayDelta(projection);

	Bool getSample = false;
#if API_VERSION < 12000
	while (win->MouseDrag(&dx,&dy,&device)==MOUSEDRAG_CONTINUE)
#else
	while (win->MouseDrag(&dx,&dy,&device)==MOUSEDRAGRESULT_CONTINUE)
#endif
	{
		Bool ctrl = (device.GetLong(BFM_INPUT_QUALIFIER) & QCTRL);
		Bool shft = (device.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT);

		if(ctrl && shft)
		{
			if(mWtSmpl > 0.0 && data.GetLong(D_WP_MODE) == D_WP_MODE_SET)
			{
				getSample = true;
				break;
			}
		}
		if(!clicked)
		{
			clicked = true;
			doc->StartUndo();
			CDAddUndo(doc,CD_UNDO_CHANGE,tagActive);
		}
		else if(dx==0.0 && dy==0.0) continue;

		mx+=dx;
		my+=dy;
		mDrag = true;
		for(i=0; i<pCnt; i++)
		{
			Bool ptHidden = false;
			
			if(polyOp)
			{
				if(!ptHdn[i])
				{
					ptHidden = true;
					LONG *dadr = NULL, dcnt = 0, p, poly;
					n.GetPointPolys(i, &dadr, &dcnt);
					for (p=0; p<dcnt; p++)
					{
						poly = dadr[p];
						if(!plyHdn[poly])
						{
							ptHidden = false;
						}
					}
				}
			}
			
			if(!ptHidden)
			{
				opt = bd->WS(opM * padr[i]);
				opt.z = 0.0;
				mpt.x = mx;
				mpt.y = my;
				mpt.z = 0.0;
				if(Len(opt - mpt) < rad)
				{
					softPt = falloffspline->GetSplinePoint(CDUniformToNatural(falloffspline,Len(opt - mpt)/rad));
					softness = softPt.y;
					
					Bool ptVisible = true;
					if(data.GetBool(D_WP_VISIBLE_ONLY))
					{
						if(polyOp)
						{
							ptVisible = false;
							rayEnd = bd->SW(opt);
							rayStart = opM * padr[i];
							if(projection > 1 && projection < 8)
							{
								rayEnd = rayStart + rayDelta;
							}
							rayLen = Len(rayEnd - rayStart);
							rayDir = CDTransformVector((rayEnd - rayStart), MInv(opM));
							rayPt = padr[i] + VNorm(rayDir) * 0.1;
							if(!rc->Intersect(rayPt, VNorm(rayDir), rayLen))  ptVisible = true;
							else
							{
								ptVisible = true;
								LONG intInd, intCnt = rc->GetIntersectionCount();
								for(intInd=0; intInd<intCnt; intInd++)
								{
									GeRayColResult res;
									rc->GetIntersection(intInd, &res);
									if(!plyHdn[res.face_id])  ptVisible = false;
								}
							}
						}
					}

					if(ptVisible)
					{
						strength = data.GetReal(D_WP_STRENGTH);
						
						switch(data.GetLong(D_WP_MODE))
						{
							case D_WP_MODE_SET:
								if(ctrl)
								{
									if(softEdge)
									{
										softWeight = 1.0 - strength*softness;
										if(!mPass[i])
										{
											if(tWeight[i] > softWeight)
											{
												tWeight[i] = softWeight;
											}
											if(tWeight[i] < 0.0)
											{
												tWeight[i] = 0.0;
												mPass[i] = true;
											}
										}
									}
									else tWeight[i] = 0.0;
								}
								else
								{
									if(softEdge)
									{
										softWeight = strength*softness;
										if(!mPass[i])
										{
											if(tWeight[i] < softWeight)
											{
												tWeight[i] = softWeight;
											}
											if(tWeight[i] > 1.0)
											{
												tWeight[i] = 1.0;
												mPass[i] = true;
											}
										}
									}
									else tWeight[i] = strength;
									
								}
								break;
							case D_WP_MODE_ADD:
								if(ctrl)
								{
									if(softEdge)
									{
										softWeight = strength*softness;
										if(!mPass[i])
										{
											if(tWeight[i] > paintWeight[i] - softWeight)
											{
												tWeight[i] = paintWeight[i] - softWeight;
											}
											if(tWeight[i] < 0.0)
											{
												tWeight[i] = 0.0;
												mPass[i] = true;
											}
										}
									}
									else
									{
										if(!mPass[i])
										{
											tWeight[i]-=strength;
											if(tWeight[i] < 0) tWeight[i] = 0;
											mPass[i] = true;
										}
									}
								}
								else
								{
									if(softEdge)
									{
										softWeight = strength*softness;
										if(!mPass[i])
										{
											if(tWeight[i] < paintWeight[i] + softWeight)
											{
												tWeight[i] = paintWeight[i] + softWeight;
											}
											if(tWeight[i] > 1.0)
											{
												tWeight[i] = 1.0;
												mPass[i] = true;
											}
										}
									}
									else
									{
										if(!mPass[i])
										{
											tWeight[i] += strength;
											if(tWeight[i] > 1.0) tWeight[i] = 1.0;
											mPass[i] = true;
										}
									}
								}
								break;
							case D_WP_MODE_BLEND:
								switch (tType)
								{
									case ID_CDCLUSTERTAGPLUGIN:
										if(!mPass[i] && tWeight[i] > 0.01)
										{
											Real tw = 0.0;
											LONG wcnt = 0;
											if(polyOp)
											{
												LONG *dadr = NULL, dcnt = 0, p;
												n.GetPointPolys(i, &dadr, &dcnt);
												for (p=0; p<dcnt; p++)
												{
													PolyInfo *plyi = n.GetPolyInfo(dadr[p]);
													LONG edg,a,b;
													for(edg=0; edg<4; edg++)
													{
														if(!plyi->mark[edg])
														{
															switch (edg)
															{
																case 0:
																	a = vadr[dadr[p]].a;
																	b = vadr[dadr[p]].b;
																	break;
																case 1:
																	a = vadr[dadr[p]].b;
																	b = vadr[dadr[p]].c;
																	break;
																case 2:
																	a = vadr[dadr[p]].c;
																	b = vadr[dadr[p]].d;
																	break;
																case 3:
																	a = vadr[dadr[p]].d;
																	b = vadr[dadr[p]].a;
																	break;
															}
															if(a == i && b < pCnt)
															{
																tw += tWeight[b];
																wcnt++;
															}
															else if(b == i && a < pCnt)
															{
																tw += tWeight[a];
																wcnt++;
															}
														}
													}
												}
											}
											else if(splineOp)
											{
												if(i > 0 && i < pCnt-1)
												{
													tw += tWeight[i-1];
													wcnt++;
													tw += tWeight[i+1];
													wcnt++;
												}
												else
												{
													if(i == 0)
													{
														tw += tWeight[i+1];
														wcnt++;
													}
													else
													{
														tw += tWeight[i-1];
														wcnt++;
													}
												}
											}
											if(tw > 0.0 && wcnt > 0)
											{
												Real smW = ((tw/(Real)wcnt) - tWeight[i]) * strength;
												tWeight[i] += smW;
												if(tWeight[i] > 1.0) tWeight[i] = 1.0;
												else if(tWeight[i] < 0.01) tWeight[i] = 0.0;
											}
											mPass[i] = true;
										}
										break;
									case ID_CDSKINPLUGIN:
										if(!mPass[i])
										{
											Real tw = 0.0;
											LONG wcnt = 0;
											if(polyOp)
											{
												LONG *dadr = NULL, dcnt = 0, p;
												n.GetPointPolys(i, &dadr, &dcnt);
												for (p=0; p<dcnt; p++)
												{
													PolyInfo *plyi = n.GetPolyInfo(dadr[p]);
													LONG edg,a,b;
													for(edg=0; edg<4; edg++)
													{
														if(!plyi->mark[edg])
														{
															switch (edg)
															{
																case 0:
																	a = vadr[dadr[p]].a;
																	b = vadr[dadr[p]].b;
																	break;
																case 1:
																	a = vadr[dadr[p]].b;
																	b = vadr[dadr[p]].c;
																	break;
																case 2:
																	a = vadr[dadr[p]].c;
																	b = vadr[dadr[p]].d;
																	break;
																case 3:
																	a = vadr[dadr[p]].d;
																	b = vadr[dadr[p]].a;
																	break;
															}
															if(a == i && b < pCnt)
															{
																tw += tWeight[b];
																wcnt++;
															}
															else if(b == i && a < pCnt)
															{
																tw += tWeight[a];
																wcnt++;
															}
														}
													}
												}
											}
											else if(splineOp)
											{
												if(i > 0 && i < pCnt-1)
												{
													tw += tWeight[i-1];
													wcnt++;
													tw += tWeight[i+1];
													wcnt++;
												}
												else
												{
													if(i == 0)
													{
														tw += tWeight[i+1];
														wcnt++;
													}
													else
													{
														tw += tWeight[i-1];
														wcnt++;
													}
												}
											}
											if(tw > 0.0 && wcnt > 0)
											{
												Real smW = ((tw/(Real)wcnt) - tWeight[i]) * strength;
												if(!tData->GetBool(SKN_NORMALIZED_WEIGHT))
												{
													tWeight[i] += smW;
												}
												else
												{
													if(sknW[i].jn > 0)
													{
														if(sknW[i].jn == 1)
														{
															if(tWeight[i] < 1.0) tWeight[i] += smW;
														}
														else tWeight[i] += smW;
													}
												}
												
												if(tWeight[i] > 1.0) tWeight[i] = 1.0;
												else if(tWeight[i] < 0.01) tWeight[i] = 0.0;
											}
											mPass[i] = true;
										}
										break;
									case ID_CDCLOTHTAG:
										if(!mPass[i] && tWeight[i] > 0.01)
										{
											Real tw = 0.0;
											LONG wcnt = 0;
											LONG *dadr = NULL, dcnt = 0, p;
											n.GetPointPolys(i, &dadr, &dcnt);
											for (p=0; p<dcnt; p++)
											{
												PolyInfo *plyi = n.GetPolyInfo(dadr[p]);
												LONG edg,a,b;
												for(edg=0; edg<4; edg++)
												{
													if(!plyi->mark[edg])
													{
														switch (edg)
														{
															case 0:
																a = vadr[dadr[p]].a;
																b = vadr[dadr[p]].b;
																break;
															case 1:
																a = vadr[dadr[p]].b;
																b = vadr[dadr[p]].c;
																break;
															case 2:
																a = vadr[dadr[p]].c;
																b = vadr[dadr[p]].d;
																break;
															case 3:
																a = vadr[dadr[p]].d;
																b = vadr[dadr[p]].a;
																break;
														}
														if(a == i && b < pCnt)
														{
															tw += tWeight[b];
															wcnt++;
														}
														else if(b == i && a < pCnt)
														{
															tw += tWeight[a];
															wcnt++;
														}
													}
												}
											}
											if(tw > 0.0 && wcnt > 0)
											{
												Real smW = ((tw/(Real)wcnt) - tWeight[i]) * strength;
												tWeight[i] += smW;
												if(tWeight[i] > 1.0) tWeight[i] = 1.0;
												else if(tWeight[i] < 0.01) tWeight[i] = 0.0;
											}
											mPass[i] = true;
										}
										break;
									case ID_CDROPETAG:
										if(!mPass[i] && tWeight[i] > 0.01)
										{
											Real tw = 0.0;
											LONG wcnt = 0;
											if(i > 0 && i < pCnt-1)
											{
												tw += tWeight[i-1];
												wcnt++;
												tw += tWeight[i+1];
												wcnt++;
											}
											else
											{
												if(i == 0)
												{
													tw += tWeight[i+1];
													wcnt++;
												}
												else
												{
													tw += tWeight[i-1];
													wcnt++;
												}
											}
											if(tw > 0.0 && wcnt > 0)
											{
												Real smW = ((tw/(Real)wcnt) - tWeight[i]) * strength;
												tWeight[i] += smW;
												if(tWeight[i] > 1.0) tWeight[i] = 1.0;
												else if(tWeight[i] < 0.01) tWeight[i] = 0.0;
											}
											mPass[i] = true;
										}
										break;
								}
								break;
							default:
								break;
						}
					}
				}
			}
		}
		switch(tType)
		{
			case ID_CDCLUSTERTAGPLUGIN:
				CDSetClusterWeight(tagActive, tData, tWeight.Array(), pCnt);
				break;
			case ID_CDSKINPLUGIN:
				CDSetJointWeight(tagActive, doc, tData, tWeight.Array(), pCnt);
				break;
			case ID_CDCLOTHTAG:
				CDClothSetWeight(tagActive, op, tData, tWeight.Array(), pCnt);
				break;
			case ID_CDROPETAG:
				CDRopeSetWeight(tagActive, op, tData, tWeight.Array(), pCnt);
				break;
		}
		op->Message(MSG_UPDATE);
		
		CDDrawViews(CD_DRAWFLAGS_INDRAG|CD_DRAWFLAGS_NO_REDUCTION|CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	}
	if(clicked)
	{
		if(tType == ID_CDCLOTHTAG || tType == ID_CDROPETAG) tagActive->Message(CD_MSG_UPDATE);
		else tagActive->Message(MSG_UPDATE);
		clicked = false;
		doc->EndUndo();
		CDDrawViews(CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	}
	if(getSample)
	{
		data.SetReal(D_WP_STRENGTH,mWtSmpl);
		
		BaseContainer wpData;
		wpData.SetReal(D_WP_STRENGTH,mWtSmpl);
		wpData.SetReal(WP_SET_STRENGTH,mWtSmpl);
		SetWorldPluginData(ID_SKINWEIGHTTOOL,wpData,true);
		
		SpecialEventAdd(ID_SKINWEIGHTTOOL); // needed to update tool dialog layout // if(dlg) dlg->ReLayout();// 
	}
	
	tWeight.Free();
	mPass.Free();
	ptHdn.Free();
	plyHdn.Free();
	paintWeight.Free();
	
	mDrag = false;
	
	SplineObject::Free(falloffspline);
	
	EventAdd(EVENT_FORCEREDRAW);
	
	return true;
}

Bool CDPaintSkinWeight::GetCursorInfo(BaseDocument *doc, BaseContainer &data, BaseDraw *bd, Real x,Real y, BaseContainer &bc)
{
	bc.SetString(RESULT_BUBBLEHELP,GeLoadString(IDS_SKINWTTOOL));
	
	Bool shft = false, cntr = false;
	BaseContainer state;
	GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_CHANNEL, state);
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL) cntr = true;
	if(state.GetLong(BFM_INPUT_QUALIFIER) & QSHIFT) shft = true;
	
	if(cntr && shft) bc.SetLong(RESULT_CURSOR,MOUSE_PAINTPICK);
	else if(!cntr && shft) bc.SetLong(RESULT_CURSOR,MOUSE_POINT_HAND);
	else bc.SetLong(RESULT_CURSOR,MOUSE_PAINTBRUSH);
	
	mx=x;
	my=y;
	wHilight = false;
	Real wDis = 0.0, twDis = 0.0;
	BaseTag* tagActive = doc->GetActiveTag();
	if(!tagActive)
	{
		BaseList2D *tag = data.GetLink(WP_PREV_TAG_LINK,doc);
		if(tag)
		{
			tagActive = static_cast<BaseTag*>(tag);
		}
		else return true;
	}
	BaseContainer *tData = tagActive->GetDataInstance();
	BaseObject *op = NULL;
	
	LONG tType = tagActive->GetType();
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
			if(!tagActive->GetBit(BIT_ACTIVE)) return true;
			op = tData->GetObjectLink(CLST_DEST_LINK,doc);
			break;
		case ID_CDSKINPLUGIN:
			op = tagActive->GetObject();
			break;
		case ID_CDCLOTHTAG:
			op = tagActive->GetObject();
			break;
		case ID_CDROPETAG:
			op = tagActive->GetObject();
			break;
		default:
			return true;
	}
	if(!op) return true;
	 
	if(!IsValidPointObject(op)) return true;
	
	BaseObject *dest = op;
	if(op->GetDeformCache())
	{
		dest = op->GetDeformCache();
	}

	Matrix opM = op->GetMg();
	Vector	rayEnd, rayStart, rayDir, rayPt, opt, mpt;
	Vector	*padr = NULL;
	LONG	i, pCnt = 0;
	Real	rayLen;

	padr = GetPointArray(dest);
	pCnt = ToPoint(dest)->GetPointCount();

	AutoAlloc<GeRayCollider> rc; if(!rc) return true;
	Bool polyOp = IsValidPolygonObject(dest);
	if(polyOp)
	{
		if(!rc->Init(dest, false)) return true;
	}
				
	CDSkinVertex *sknW = NULL;
	CDArray<Real> tWeight; if(!tWeight.Alloc(pCnt)) return true;
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
		{
			if(!CDGetClusterWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			break;
		}
		case ID_CDSKINPLUGIN:
		{
			if(!CDGetJointWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			sknW = CDGetSkinWeights(tagActive);
			if(!sknW)
			{
				tWeight.Free();
				return false;
			}
			break;
		}
		case ID_CDCLOTHTAG:
		{
			if(!CDClothGetWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			break;
		}
		case ID_CDROPETAG:
		{
			if(!CDRopeGetWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				return true;
			}
			break;
		}
	}
	
	for(i=0; i<pCnt; i++)
	{
		opt = bd->WS(opM * padr[i]);
		opt.z = 0.0;
		mpt.x = x;
		mpt.y = y;
		mpt.z = 0.0;
		if(Len(opt - mpt) < 5)
		{
			if(data.GetBool(D_WP_VISIBLE_ONLY) && polyOp)
			{
				rayEnd = bd->SW(opt);
				rayStart = opM * padr[i];
				rayLen = Len(rayEnd - rayStart);
				rayDir = CDTransformVector((rayEnd - rayStart), MInv(opM));
				rayPt = padr[i] + VNorm(rayDir) * 0.1;
				if(!rc->Intersect(rayPt, VNorm(rayDir), rayLen))
				{
					mWtSmpl = tWeight[i];
					wDis = tWeight[i] * 100;
					if(sknW) twDis = sknW[i].taw * 100;
					weightDisplayIndex = i;
					wHilight = true;
				}
			}
			else
			{
				mWtSmpl = tWeight[i];
				wDis = tWeight[i] * 100;
				if(sknW) twDis = sknW[i].taw * 100;
				weightDisplayIndex = i;
				wHilight = true;
			}
		}
	}
	tWeight.Free();
	
	String totalweight = "";
	if(sknW) totalweight = "% : Total weight = "+CDRealToString(twDis)+"%";
	StatusSetText("Weight = "+CDRealToString(wDis)+totalweight);
	
	// set display weight variables
	mWtT = "Total = "+CDRealToString(twDis)+"%";
	mWt = "Weight = "+CDRealToString(wDis)+"%";
	
	bc.SetString(RESULT_BUBBLEHELP,GeLoadString(IDS_SKINWTTOOL)+GeLoadString(IDS_HLP_SKINWTTOOL));
	
	CDDrawViews(CD_DRAWFLAGS_NO_REDUCTION|CD_DRAWFLAGS_NO_EXPRESSIONS|CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);

	return true;
}

Bool CDPaintSkinWeight::CDInitDisplayControl(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, const AtomArray* active)
{
	displayBD = bd;

#if API_VERSION < 12000
	if(!StoreDisplayFilters(bd)) return false;
#endif
	
	vw1 = CDGetVertexStartColor();
	vw2 = CDGetVertexEndColor();
	
	BaseTag* tagActive = doc->GetActiveTag();
	if(!tagActive)
	{
		BaseList2D *tag = data.GetLink(WP_PREV_TAG_LINK,doc);
		if(tag)
		{
			tagActive = static_cast<BaseTag*>(tag);
		}
		else return true;
	}
	else data.SetLink(WP_PREV_TAG_LINK,tagActive);
	
	BaseContainer *tData = tagActive->GetDataInstance(); if(!tData) return false;
	BaseObject *dest = NULL;
	
	LONG tType = tagActive->GetType();
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
			if(!tagActive->GetBit(BIT_ACTIVE)) return false;
			dest = tData->GetObjectLink(CLST_DEST_LINK,doc);
			break;
		case ID_CDSKINPLUGIN:
			if(!tagActive) return false;
			dest = tagActive->GetObject();
			break;
		case ID_CDCLOTHTAG:
			if(!tagActive) return false;
			dest = tagActive->GetObject();
			break;
		case ID_CDROPETAG:
			if(!tagActive) return false;
			dest = tagActive->GetObject();
			break;
		default:
			return false;
	}
	if(!dest) return false;
	
	if(!IsValidPointObject(dest)) return false;

	LONG	pCnt = ToPoint(dest)->GetPointCount();
	CDArray<Real> tWeight; if(!tWeight.Alloc(pCnt)) return false;
	
#if API_VERSION < 12000
	ptColor = (Vector*)CDAlloc<Vector>(pCnt);
#else
	ptColor = (SVector*)CDAlloc<SVector>(pCnt);
#endif
	if(!ptColor)
	{
		tWeight.Free();
		return false;
	}
	
	LONG i;
	Real brightness = data.GetReal(D_WP_CONTRAST)+1.0;
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
		{
			if(!CDGetClusterWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				CDFree(ptColor);
				return false;
			}

		#if API_VERSION < 12000
			SetDisplayFilters(bd);
		#endif
			
			for(i=0; i<pCnt; i++)
			{
				Vector clrMix = CDBlend(vw1/brightness,vw2*brightness,tWeight[i]);
			#if API_VERSION < 12000
				ptColor[i] =  clrMix;
			#else
				#if API_VERSION < 15000
                    ptColor[i] =  clrMix.ToSV();
                #else
                    ptColor[i] =  (Vector32)clrMix;
                #endif
			#endif
			}
			break;
		}
		case ID_CDSKINPLUGIN:
		{
			LONG opCount = tData->GetLong(SKN_J_COUNT);
			Bool opInList = false;
			BaseObject *opActive = doc->GetActiveObject();
			if(opActive)
			{
				for(i=0; i<opCount; i++)
				{
					if(opActive == tData->GetObjectLink(SKN_J_LINK+i,doc))
					{
						opInList = true;
						tData->SetLong(SKN_J_LINK_ID,i);
					}
				}
			}
			if(!opInList)
			{
				tWeight.Free();
				CDFree(ptColor);
				return false;	
			}

			if(!CDGetJointWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				CDFree(ptColor);
				return false;	
			}
			
			CDArray<Vector> jColor;
			if(!jColor.Alloc(pCnt))
			{
				tWeight.Free();
				CDFree(ptColor);
				return false;	
			}
			CDGetSkinJointColors(tagActive, doc, tData, jColor.Array(), pCnt);  

		#if API_VERSION < 12000
			SetDisplayFilters(bd);
		#endif
			
			if(data.GetBool(D_WP_JCOLORS))
			{
				ObjectColorProperties prop;
				opActive->GetColorProperties(&prop);
				if(prop.usecolor > 0)
				{
					if(CDMinVector(prop.color,0.1))  vw2 = Vector(0.5,0.5,0.5);
					else  vw2 = prop.color;
				}
				else vw2 = Vector(0,0,0.75);
			}
			
			for(i=0; i<pCnt; i++)
			{
				if(data.GetBool(D_WP_JCOLORS))
				{
					vw1 = jColor[i] * 0.75;
				}
				Vector clrMix = CDBlend(vw1/brightness, vw2 * brightness, tWeight[i]);
			#if API_VERSION < 12000
				ptColor[i] =  clrMix;
			#else
                #if API_VERSION < 15000
                    ptColor[i] =  clrMix.ToSV();
                #else
                    ptColor[i] =  (Vector32)clrMix;
                #endif
			#endif
			}
			jColor.Free();
			break;
		}
		case ID_CDCLOTHTAG:
		{
			if(!CDClothGetWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				CDFree(ptColor);
				return false;
			}
			
		#if API_VERSION < 12000
			SetDisplayFilters(bd);
		#endif
			
			for(i=0; i<pCnt; i++)
			{
				Vector clrMix = CDBlend(vw1/brightness, vw2 * brightness, tWeight[i]);
			#if API_VERSION < 12000
				ptColor[i] =  clrMix;
			#else
                #if API_VERSION < 15000
                    ptColor[i] =  clrMix.ToSV();
                #else
                    ptColor[i] =  (Vector32)clrMix;
                #endif
			#endif
			}
			break;
		}
		case ID_CDROPETAG:
		{
			if(!CDRopeGetWeight(tagActive, tData, tWeight.Array(), pCnt))
			{
				tWeight.Free();
				CDFree(ptColor);
				return false;
			}
			
		#if API_VERSION < 12000
			SetDisplayFilters(bd);
		#endif
			
			for(i=0; i<pCnt; i++)
			{
				Vector clrMix = CDBlend(vw1/brightness,vw2*brightness,tWeight[i]);
			#if API_VERSION < 12000
				ptColor[i] =  clrMix;
			#else
                #if API_VERSION < 15000
                    ptColor[i] =  clrMix.ToSV();
                #else
                    ptColor[i] =  (Vector32)clrMix;
                #endif
			#endif
			}
			break;
		}
		default:
		#if API_VERSION < 12000
			RestoreDisplayFilters(bd);
		#endif
			break;
	}
	tWeight.Free();
	
	return true;
}
 
void CDPaintSkinWeight::FreeDisplayControl(void)
{
#if API_VERSION < 12000
	RestoreDisplayFilters(displayBD);
#endif
}
 
Bool CDPaintSkinWeight::DisplayControl(BaseDocument* doc, BaseObject* op, BaseObject* chainstart, BaseDraw* bd, BaseDrawHelp* bh, ControlDisplayStruct& cds)
{
	GeData dispSData = bd->GetParameterData(BASEDRAW_DATA_SDISPLAYACTIVE);
	GeData dispBData = bd->GetParameterData(BASEDRAW_DATA_BACKCULL);
	
	BaseTag* tagActive = doc->GetActiveTag();
	if(!tagActive)
	{
		BaseContainer *bc=GetToolData(GetActiveDocument(),ID_SKINWEIGHTTOOL);
		if(!bc) return true;
		
		BaseList2D *tag = bc->GetLink(WP_PREV_TAG_LINK,doc);
		if(tag)
		{
			tagActive = static_cast<BaseTag*>(tag);
		}
		else return true;
	}
	
	BaseContainer *tData = tagActive->GetDataInstance();
	BaseObject *dest = NULL;
			
	LONG tType = tagActive->GetType();
	switch (tType)
	{
		case ID_CDCLUSTERTAGPLUGIN:
			if(!tagActive->GetBit(BIT_ACTIVE)) return true;
			dest = tData->GetObjectLink(CLST_DEST_LINK,doc); if(!dest) return true;
			break;
		case ID_CDSKINPLUGIN:
			if(!tagActive) return true;
			dest = tagActive->GetObject(); 
			break;
		case ID_CDCLOTHTAG:
			if(!tagActive) return true;
			dest = tagActive->GetObject(); 
			break;
		case ID_CDROPETAG:
			if(!tagActive) return true;
			dest = tagActive->GetObject(); 
			break;
		default:
			return true;
	}
	if(!dest) return true;
	
	if(!IsValidPointObject(dest)) return true;
	if(!ptColor) return false;
	
	BaseObject *opDeform = NULL;
	if(dest->GetDeformCache())
	{
		opDeform = dest->GetDeformCache();
		if(opDeform)
		{
			if(op == opDeform)
			{
			#if API_VERSION < 12000
				if(dispSData == BASEDRAW_SDISPLAY_NOSHADING) cds.displaymode = DISPLAYCONTROLMODE_WIRE;
				else cds.displaymode = DISPLAYCONTROLMODE_FLATWIRE;
			#else
				if(dispSData == BASEDRAW_SDISPLAY_NOSHADING) cds.displaymode = DISPLAYMODE_WIRE;
				else cds.displaymode = DISPLAYMODE_FLATWIRE;
			#endif
				
				if(dispBData == true) cds.backface_culling = true;
				else cds.backface_culling = false;
				
				cds.vertex_color = ptColor;
				cds.editmode = true;
			}
		}
	}
	if(op == dest)
	{
		if(!opDeform)
		{
		#if API_VERSION < 12000
			if(dispSData == BASEDRAW_SDISPLAY_NOSHADING) cds.displaymode = DISPLAYCONTROLMODE_WIRE;
			else cds.displaymode = DISPLAYCONTROLMODE_FLATWIRE;
		#else
			if(dispSData == BASEDRAW_SDISPLAY_NOSHADING) cds.displaymode = DISPLAYMODE_WIRE;
			else cds.displaymode = DISPLAYMODE_FLATWIRE;
		#endif
			
			if(dispBData == true) cds.backface_culling = true;
			else cds.backface_culling = false;
			
			cds.vertex_color = ptColor;
			cds.editmode = true;
		}
	}

	return true;
}

Bool CDPSWDialog::CreateLayout(void)
{
	GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
	{
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,0,0,0,GeLoadString(D_WP_RADIUS),0);
			AddEditNumberArrows(D_WP_RADIUS,BFH_LEFT,70,0);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupSpace(4,1);
			AddCheckbox(D_WP_VISIBLE_ONLY,BFH_LEFT,0,0,GeLoadString(D_WP_VISIBLE_ONLY));
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,0,0,0,GeLoadString(D_WP_CONTRAST),0);
			AddEditSlider(D_WP_CONTRAST,BFH_SCALEFIT);
		}
		GroupEnd();
		
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,BFH_LEFT,0,0,GeLoadString(D_WP_MODE),0);
			AddComboBox(D_WP_MODE, BFH_LEFT,100,0);
				AddChild(D_WP_MODE, D_WP_MODE_SET, GeLoadString(D_WP_MODE_SET));
				AddChild(D_WP_MODE, D_WP_MODE_ADD, GeLoadString(D_WP_MODE_ADD));
				AddChild(D_WP_MODE, D_WP_MODE_BLEND, GeLoadString(D_WP_MODE_BLEND));
		}
		GroupEnd();
		
		CreateDynamicStrength();
		
		CreateDynamicGroup();
	}
	GroupEnd();
	
	return true;
}

void CDPSWDialog::CreateDynamicStrength(void)
{
	GroupBegin(D_WP_DYN_STRENGTH,BFH_SCALEFIT,1,0,"",0);
	{
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,0,0,0,GeLoadString(D_WP_STRENGTH),0);
			AddEditSlider(D_WP_STRENGTH,BFH_SCALEFIT);
		}
		GroupEnd();

		GroupBegin(0,BFH_SCALEFIT,1,0,"",0);
		{
			GroupSpace(4,1);
			AddCheckbox(D_WP_SOFT_EDGES,BFH_LEFT,0,0,GeLoadString(D_WP_SOFT_EDGES));
		}
		GroupEnd();
	
		GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
		{
			GroupSpace(4,1);
			AddStaticText(0,0,0,0,GeLoadString(D_WP_SOFTNESS),0);
			AddEditSlider(D_WP_SOFTNESS,BFH_SCALEFIT);
		}
		GroupEnd();
	}
	GroupEnd();
}

void CDPSWDialog::CreateDynamicGroup(void)
{
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *bc = NULL;
	BaseTag* tagActive = doc->GetActiveTag();
	if(!tagActive)
	{
		bc = GetToolData(GetActiveDocument(),ID_SKINWEIGHTTOOL);
		if(bc)
		{
			BaseList2D *tag = bc->GetLink(WP_PREV_TAG_LINK,doc);
			if(tag)
			{
				tagActive = static_cast<BaseTag*>(tag);
			}
		}
	}
	
	if(tagActive && tagActive->GetType() == ID_CDSKINPLUGIN)
	{
		LONG i, items = listview.GetItemCount();
		for(i=0; i<items; i++)
		{
			listview.RemoveItem(i);
		}

		BaseContainer layout;
		layout.SetLong('name',LV_COLUMN_TEXT);
		listview.SetLayout(2,layout);

		BaseContainer data;

		selection->DeselectAll();
		BaseContainer	*tData = tagActive->GetDataInstance();
		
		Bool inList = false;
		LONG lcount=0, jcount = tData->GetLong(SKN_J_COUNT);
		for(i=0; i<jcount; i++)
		{
			BaseObject *joint = tData->GetObjectLink(SKN_J_LINK+i,doc);
			if(joint)
			{
				if(joint->GetBit(BIT_ACTIVE))
				{
					selection->Select(i);
					scrlPosY = i;
					tData->SetLong(SKN_J_LINK_ID,i);
					tagActive->Message(MSG_UPDATE);
					inList = true;
				}
				data.SetString('name',joint->GetName());
				data.SetLong('used',lcount);
				listview.SetItem(lcount,data);
				lcount++;
			}
			else
			{
				data.SetString('name',"");
				data.SetLong('used',lcount);
				listview.SetItem(lcount,data);
				lcount++;
			}
		}
		listview.SetSelection(selection);
		listview.DataChanged();
		
		GroupBegin(D_WP_DYN_GROUP,BFH_SCALEFIT,1,0,"",0);
		{
			GroupBegin(D_WP_DYN_TITLE,BFV_SCALEFIT|BFH_SCALEFIT,0,1,GeLoadString(D_WP_DYN_TITLE),0);
			{
				GroupBorder(BORDER_GROUP_IN|BORDER_WITH_TITLE);
				GroupBorderSpace(4,4,4,4);
				AddListView(D_WP_JLIST, BFH_SCALEFIT|BFV_SCALEFIT, 0, 96);
				listview.AttachListView(this, D_WP_JLIST);
			}
			GroupEnd();

			GroupBegin(0,BFH_SCALEFIT,2,0,"",0);
			{
				GroupSpace(20,1);
				AddCheckbox (D_WP_JCOLORS,BFH_LEFT,0,0,GeLoadString(D_WP_JCOLORS));
				AddCheckbox (D_WP_NORMALIZED,BFH_RIGHT,0,0,GeLoadString(D_WP_NORMALIZED));
			}
			GroupEnd();
		}
		GroupEnd();
	}
	
}

void CDPSWDialog::ReLayout(void)
{
	LayoutFlushGroup(D_WP_DYN_STRENGTH);
	CreateDynamicStrength();
	LayoutChanged(D_WP_DYN_STRENGTH);
	
	LayoutFlushGroup(D_WP_DYN_GROUP);
	CreateDynamicGroup();
	LayoutChanged(D_WP_DYN_GROUP);
	
	InitValues();
}


Bool CDPSWDialog::InitDialog(void)
{
	BaseContainer *bc=GetToolData(GetActiveDocument(),ID_SKINWEIGHTTOOL);
	if(!bc) return false;

	return true;
}

Bool CDPSWDialog::JointInList(BaseDocument *doc, BaseObject *jnt, BaseContainer *tData)
{
	LONG i, jCount = tData->GetLong(SKN_J_COUNT);
	for(i=0; i<jCount; i++)
	{
		if(jnt == tData->GetObjectLink(SKN_J_LINK+i,doc))
		{
			tData->SetLong(SKN_J_LINK_ID,i);
			return true;
		}
	}
	
	return false;
}

Bool CDPSWDialog::CoreMessage(LONG id,const BaseContainer &msg)
{
	BaseDocument *doc = GetActiveDocument();
	BaseTag* tagActive = doc->GetActiveTag(), *prevTag=NULL;
	BaseObject *opActive = doc->GetActiveObject();
	switch (id)
	{
		case EVMSG_CHANGE:
		{
			BaseContainer *bc = GetToolData(doc,ID_SKINWEIGHTTOOL);
			if(!bc) return GeDialog::CoreMessage(id,msg);
			BaseList2D *tag = bc->GetLink(WP_PREV_TAG_LINK,doc);
			if(tag)  prevTag = static_cast<BaseTag*>(tag);
			if(tagActive)
			{
				if(tagActive != prevTag)
				{
					tag = tagActive;
					bc->SetLink(WP_PREV_TAG_LINK,tagActive);
					ReLayout();
				}
			}
			
			if(opActive && tag)
			{
				if(tag->GetType() == ID_CDSKINPLUGIN)
				{
					BaseContainer	*tData = tag->GetDataInstance();
					if(!JointInList(doc, opActive, tData))
					{
						selection->DeselectAll();
						listview.SetSelection(selection);
						listview.Redraw();
					}
					else
					{
						LONG ind = tData->GetLong(SKN_J_LINK_ID);
						selection->DeselectAll();
						selection->Select(ind);
						listview.SetSelection(selection);
						listview.Redraw();
					}
					
					LONG jCount = tData->GetLong(SKN_J_COUNT), wpjCount = bc->GetLong(WP_SKIN_JOINT_COUNT);
					if(wpjCount != jCount)
					{
						bc->SetLong(WP_SKIN_JOINT_COUNT,jCount);
						ReLayout();
					}
				}
			}
			else ReLayout();
			break;
		}
		case ID_SKINWEIGHTTOOL:
		{
			ReLayout();
			break;
		}
	}
	
	return GeDialog::CoreMessage(id,msg);
}

Bool CDPSWDialog::InitValues(void)
{
	// first call the parent instance
	if(!GeDialog::InitValues()) return false;

	BaseContainer *bc = GetWorldPluginData(ID_SKINWEIGHTTOOL);
	if(!bc)
	{
		SetReal(D_WP_RADIUS,20,1,100,1,FORMAT_LONG);
		SetBool(D_WP_SOFT_EDGES, false);
		SetReal(D_WP_SOFTNESS,1.0,0,1,0.01,FORMAT_PERCENT);
		SetBool(D_WP_VISIBLE_ONLY, true);
		SetReal(D_WP_STRENGTH,1.0,0,1,0.01,FORMAT_PERCENT);
		SetReal(D_WP_CONTRAST,0.5,0.0,1.0,0.01,FORMAT_PERCENT,0.0,CDMAXREAL);
		SetLong(D_WP_MODE,D_WP_MODE_SET);
		SetBool(D_WP_JCOLORS, false);
		SetBool(D_WP_NORMALIZED, true);
	}
	else
	{
		SetReal(D_WP_RADIUS,bc->GetReal(D_WP_RADIUS),1,100,1,FORMAT_LONG);
		SetBool(D_WP_SOFT_EDGES, bc->GetBool(D_WP_SOFT_EDGES));
		SetReal(D_WP_SOFTNESS,bc->GetReal(D_WP_SOFTNESS),0,1,0.01,FORMAT_PERCENT);
		SetBool(D_WP_VISIBLE_ONLY, bc->GetBool(D_WP_VISIBLE_ONLY));
		SetReal(D_WP_CONTRAST,bc->GetReal(D_WP_CONTRAST),0,1,0.01,FORMAT_PERCENT,0.0,CDMAXREAL);
		SetLong(D_WP_MODE,bc->GetLong(D_WP_MODE));
		SetReal(D_WP_STRENGTH,bc->GetReal(D_WP_STRENGTH),0,1,0.01,FORMAT_PERCENT);
		SetBool(D_WP_JCOLORS, bc->GetBool(D_WP_JCOLORS));
		SetBool(D_WP_NORMALIZED, bc->GetBool(D_WP_NORMALIZED));
	}

	DoEnable();

	return InitDialog();
}

Bool CDPSWDialog::Command(LONG id,const BaseContainer &msg)
{
	Real sSet = 1.0, sAdd = 0.1, sBlend = 1.0;
	Real r,s,br,fo;
	LONG md;
	Bool vo,se,jc,nw;
	Bool redraw = false;
	
	GetReal(D_WP_RADIUS,r);
	GetBool(D_WP_SOFT_EDGES,se);
	GetBool(D_WP_VISIBLE_ONLY,vo);
	GetReal(D_WP_STRENGTH,s);
	GetReal(D_WP_SOFTNESS,fo);
	GetReal(D_WP_CONTRAST,br);
	GetLong(D_WP_MODE,md);
	GetBool(D_WP_JCOLORS,jc);
	GetBool(D_WP_NORMALIZED,nw);
	
	BaseContainer wpData;
	BaseContainer *wpd = GetWorldPluginData(ID_SKINWEIGHTTOOL);
	if(wpd)
	{
		sSet = wpd->GetReal(WP_SET_STRENGTH);
		sAdd = wpd->GetReal(WP_ADD_STRENGTH);
		sBlend = wpd->GetReal(WP_BLEND_STRENGTH);
		switch (id)
		{
			case D_WP_STRENGTH:
			{
				switch(md)
				{
					case D_WP_MODE_SET:
						sSet = s;
						break;
					case D_WP_MODE_ADD:
						sAdd = s;
						break;
					case D_WP_MODE_BLEND:
						sBlend = s;
						break;
				}
				break;
			}
			case D_WP_MODE:
			{
				switch(md)
				{
					case D_WP_MODE_SET:
						s = sSet;
						break;
					case D_WP_MODE_ADD:
						s = sAdd;
						break;
					case D_WP_MODE_BLEND:
						s = sBlend;
						break;
				}
				SpecialEventAdd(ID_SKINWEIGHTTOOL); // needed to update tool dialog dynamic layout // redraw = true;// 
				break;
			}
		}
	}
	wpData.SetReal(WP_SET_STRENGTH,sSet);
	wpData.SetReal(WP_ADD_STRENGTH,sAdd);
	wpData.SetReal(WP_BLEND_STRENGTH,sBlend);
	wpData.SetReal(D_WP_RADIUS,r);
	wpData.SetBool(D_WP_SOFT_EDGES,se);
	wpData.SetBool(D_WP_VISIBLE_ONLY,vo);
	wpData.SetReal(D_WP_STRENGTH,s);
	wpData.SetReal(D_WP_SOFTNESS,fo);
	wpData.SetReal(D_WP_CONTRAST,br);
	wpData.SetLong(D_WP_MODE,md);
	wpData.SetBool(D_WP_JCOLORS,jc);
	wpData.SetBool(D_WP_NORMALIZED,nw);
	SetWorldPluginData(ID_SKINWEIGHTTOOL,wpData,false);
		
	BaseDocument *doc = GetActiveDocument();
	BaseContainer *bc = GetToolData(doc,ID_SKINWEIGHTTOOL);
	if(!bc) return false;
	
	bc->SetReal(D_WP_RADIUS,r);
	bc->SetBool(D_WP_VISIBLE_ONLY,vo);
	bc->SetReal(D_WP_SOFTNESS,fo);
	bc->SetReal(D_WP_STRENGTH,s);
	bc->SetLong(D_WP_MODE,md);
	bc->SetBool(D_WP_NORMALIZED,nw);
	
	if(md == D_WP_MODE_BLEND) bc->SetBool(D_WP_SOFT_EDGES,false);
	else bc->SetBool(D_WP_SOFT_EDGES,se);
	
	if(jc != bc->GetReal(D_WP_JCOLORS))
	{
		bc->SetBool(D_WP_JCOLORS,jc);
		CDDrawViews(CD_DRAWFLAGS_INDRAG|CD_DRAWFLAGS_NO_REDUCTION|CD_DRAWFLAGS_NO_EXPRESSIONS|CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	}
	
	if(br != bc->GetReal(D_WP_CONTRAST))
	{
		bc->SetReal(D_WP_CONTRAST,br);
		CDDrawViews(CD_DRAWFLAGS_INDRAG|CD_DRAWFLAGS_NO_REDUCTION|CD_DRAWFLAGS_NO_EXPRESSIONS|CD_DRAWFLAGS_ONLY_ACTIVE_VIEW|CD_DRAWFLAGS_NO_THREAD|CD_DRAWFLAGS_NO_ANIMATION);
	}

	BaseTag *tag = doc->GetActiveTag();
	if(!tag)
	{
		BaseList2D *theTag = bc->GetLink(WP_PREV_TAG_LINK,doc);
		if(theTag)
		{
			tag = static_cast<BaseTag*>(theTag);
		}
		else return true;
	}
	BaseContainer *tData = tag->GetDataInstance(); if(!tData) return true;
	
	if(tag->GetType() == ID_CDSKINPLUGIN) tData->SetBool(SKN_NORMALIZED_WEIGHT,nw);
	
	switch (id)
	{
		case D_WP_JLIST:
		{
			switch (msg.GetLong(BFM_ACTION_VALUE))
			{
				case LV_SIMPLE_SELECTIONCHANGED:
				{
					LONG listID = msg.GetLong(LV_SIMPLE_ITEM_ID);
					BaseContainer bcList;
					listview.GetItem(listID,&bcList);
					tData->SetLong(SKN_J_LINK_ID,bcList.GetLong('used'));
					BaseObject *joint = tData->GetObjectLink(SKN_J_LINK+tData->GetLong(SKN_J_LINK_ID),doc);
					if(joint)
					{
						doc->SetActiveObject(joint,SELECTION_NEW);
						doc->SetActiveTag(tag);
						doc->SetAction(ID_SKINWEIGHTTOOL);
						EventAdd(EVENT_FORCEREDRAW);
					}
					EventAdd();
					break;
				}
			}
			break;
		}
	}
	DoEnable();
	
	return true;
}

void CDPSWDialog::DoEnable(void)
{
	Bool se;
	LONG mode;

	GetBool(D_WP_SOFT_EDGES,se);
	if(!se)  Enable(D_WP_SOFTNESS,false);
	else  Enable(D_WP_SOFTNESS,true);
	
	GetLong(D_WP_MODE,mode);
	if(mode == D_WP_MODE_BLEND)
	{
		SetBool(D_WP_SOFT_EDGES,false);
		Enable(D_WP_SOFT_EDGES,false);
	}
	else  Enable(D_WP_SOFT_EDGES,true);
}

class CDPaintSkinWeightR : public CommandData
{
	public:
	
		virtual Bool Execute(BaseDocument *doc)
		{
			return true;
		}
};

Bool RegisterCDPaintSkinWeight(void)
{
	if(CDFindPlugin(ID_SKINWEIGHTTOOL,CD_TOOL_PLUGIN)) return true;

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
	
	// decide by name ifthe plugin shall be registered - just for user convenience  PLUGINFLAG_TOOL_EDITSTATES|PLUGINFLAG_TOOL_SINGLECLICK
	String name=GeLoadString(IDS_SKINWTTOOL); if(!name.Content()) return true;

	if(!reg) return CDRegisterCommandPlugin(ID_SKINWEIGHTTOOL,name,PLUGINFLAG_HIDE,"CDSkinWeight.tif","CD Paint Skin Weight",CDDataAllocator(CDPaintSkinWeightR));
	else return CDRegisterToolPlugin(ID_SKINWEIGHTTOOL,name,0,"CDSkinWeight.tif","CD Paint Skin Weight",CDDataAllocator(CDPaintSkinWeight));
}
