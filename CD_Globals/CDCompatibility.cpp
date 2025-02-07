// R12 Compatiblity

#include "c4d.h"

#include "CDCompatibility.h"
#include "c4d_string.h"

// BaseObject compatiblity functions
Vector CDGetPos(BaseObject *op)
{
	Vector pos;
#if API_VERSION < 12000
	if(op) pos = op->GetPos();
#else
	if(op) pos = op->GetAbsPos();
#endif
	return pos;
}

void CDSetPos(BaseObject *op, Vector pos)
{
	if(!op) return;
#if API_VERSION < 12000
	op->SetPos(pos);
#else
	op->SetAbsPos(pos);
#endif
}

Vector CDGetScale(BaseObject *op)
{
	Vector sca;
#if API_VERSION < 12000
	if(op) sca = op->GetScale();
#else
	if(op) sca = op->GetAbsScale();
#endif
	return sca;
}

void CDSetScale(BaseObject *op, Vector sca)
{
	if(!op) return;
#if API_VERSION < 12000
	op->SetScale(sca);
#else
	op->SetAbsScale(sca);
#endif
}

Vector CDGetRot(BaseObject *op)
{
	Vector rot;
#if API_VERSION < 12000
	if(op) rot = op->GetRot();
#else
	if(op) rot = op->GetAbsRot();
#endif
	return rot;
}

void CDSetRot(BaseObject *op, Vector rot)
{
	if(!op) return;
#if API_VERSION < 12000
	op->SetRot(rot);
#else
	op->SetAbsRot(rot);
#endif
}

Bool CDIsBone(BaseObject *op)
{
#if API_VERSION < 12000
	if(op && op->GetType() == Obone) return true;
#endif
	return false;
}

Bool CDIsDirty(BaseObject *op, LONG dirtyID)
{
	if(!op) return false;
	
#if API_VERSION < 12000
	return op->IsDirty(dirtyID);
#else
	return op->IsDirty((DIRTYFLAGS)dirtyID);
#endif
}


// BaseDraw compatiblity functions
void CDDrawHandle2D(BaseDraw *bd, const Vector &p, LONG type)
{
	if(!bd) return;
#if API_VERSION < 12000
	bd->Handle2D(p, type);
#else
	bd->DrawHandle2D(p, (DRAWHANDLE)type);
#endif
}

void CDDrawHandle3D(BaseDraw *bd, const Vector &vp, LONG type, LONG flags)
{
	if(!bd) return;
#if API_VERSION < 12000
	bd->Handle3D(vp, type);
#else
	bd->DrawHandle(vp, (DRAWHANDLE)type, flags);
#endif
}

void CDDrawLine2D(BaseDraw *bd, Vector a, Vector b)
{
	if(!bd) return;
#if API_VERSION < 12000
	bd->Line2D(a, b);
#else
	bd->DrawLine2D(a, b);
#endif
}

void CDDrawLine(BaseDraw *bd, Vector a, Vector b)
{
	if(!bd) return;
#if API_VERSION < 12000
	bd->Line3D(a, b);
#else
	bd->DrawLine(a, b, 0);
#endif
}

void CDDrawCircle2D(BaseDraw *bd, LONG mx, LONG my, Real rad)
{
	if(!bd) return;
#if API_VERSION < 12000
	bd->Circle2D(mx, my, rad);
#else
	bd->DrawCircle2D(mx, my, rad);
#endif
}

void CDDrawCircle(BaseDraw *bd, Matrix m)
{
	if(!bd) return;
#if API_VERSION < 12000
	bd->Circle3D(m);
#else
	bd->DrawCircle(m);
#endif
}

void CDDrawPolygon(BaseDraw *bd, Vector *p, Vector *f, Bool quad)
{
	if(!bd) return;
#if API_VERSION < 12000
	bd->Polygon3D(p, f, quad);
#else
	bd->DrawPolygon(p, f, quad);
#endif
}

void CDSetBaseDrawMatrix(BaseDraw *bd, BaseObject *op, const Matrix &mg)
{
	if(!bd) return;
#if API_VERSION > 11999
	bd->SetMatrix_Matrix(op, mg);
#endif
}

Bool CDDrawViews(LONG flags, BaseDraw *bd)
{
#if API_VERSION < 12000
	#if API_VERSION < 9800
		DrawViews(flags);
	#else
		DrawViews(flags, bd);
	#endif
#else
	DrawViews((DRAWFLAGS)flags, bd);
#endif
	
	return true;
}

Vector CDGetViewColor(LONG colid)
{
#if API_VERSION < 9800
	return GetWorldColor(colid);
#else
	return GetViewColor(colid);
#endif
}


// SplineObject compatiblity functions
Real CDUniformToNatural(SplineObject *spl, Real val)
{
	Real t = 0.0;
	
#if API_VERSION < 12000
	if(spl && spl->InitLength())
	{
		t = spl->UniformToNatural(val);
		spl->FreeLength();
	}
#else
	AutoAlloc<SplineLengthData> sld;
	if(sld->Init(spl))
	{
		t = sld->UniformToNatural(val);
	}
#endif
	
	return t;
}

Real CDGetSplineLength(SplineObject *spl)
{
	Real sLen = 0.0;
	
#if API_VERSION < 12000
	if(spl && spl->InitLength())
	{
		sLen = spl->GetLength();
		spl->FreeLength();
	}
#else
	AutoAlloc<SplineLengthData> sld;
	if(sld->Init(spl))
	{
		sLen = sld->GetLength();
	}
#endif
	
	return sLen;
}

Vector CDGetSplinePoint(SplineObject *spl, Real t, LONG segment, const Vector *padr)
{
	Vector pt;
	
#if API_VERSION < 12000
	if(spl && spl->InitLength())
	{
		#if API_VERSION < 9800
			Vector adr = *padr;
			pt = spl->GetSplinePoint(t, segment, &adr);
		#else
			pt = spl->GetSplinePoint(t, segment, padr);
		#endif
		spl->FreeLength();
	}
#else
	if(spl) pt = spl->GetSplinePoint(t, segment, padr);
#endif
	
	return pt;
}

SplineObject* CDAllocateSplineObject(LONG pcnt, LONG type)
{
#if API_VERSION < 12000
	return SplineObject::Alloc(pcnt, type);
#else
	return SplineObject::Alloc(pcnt, (SPLINETYPE)type);
#endif
}


// UVWTag compatiblity functions
void CDGetUVWStruct(UVWTag *uvTag, UVWStruct &uv, LONG ind)
{
	if(!uvTag) return;
	
#if API_VERSION < 12000
	uv = uvTag->Get(ind);
#else
	#if API_VERSION < 15000
		const UVWHandle dataptr = uvTag->GetDataAddressR();
	#else
		ConstUVWHandle dataptr = uvTag->GetDataAddressR();
	#endif
	UVWTag::Get(dataptr,ind,uv);
#endif
}

void CDSetUVWStruct(UVWTag *uvTag, UVWStruct &uv, LONG ind)
{
	if(!uvTag) return;

#if API_VERSION < 12000
	uvTag->Set(ind, uv);
#else
	UVWHandle dataptr = uvTag->GetDataAddressW();
	UVWTag::Set(dataptr, ind, uv);
#endif
}


// GeListNode compatiblity functions
Bool CDGetParameter(GeListNode *node, const DescID& id, GeData& t_data, LONG flags)
{
	if(!node) return false;
	
#if API_VERSION < 12000
	return node->GetParameter(id, t_data, flags);
#else
	return node->GetParameter(id, t_data, (DESCFLAGS_GET)flags);
#endif
}

Bool CDSetParameter(GeListNode *node, const DescID& id, const GeData& t_data, LONG flags)
{
	if(!node) return false;
	
#if API_VERSION < 12000
	return node->SetParameter(id, t_data, flags);
#else
	return node->SetParameter(id, t_data, (DESCFLAGS_SET)flags);
#endif
}

Bool CDGetDescription(GeListNode *node, Description *desc, LONG flags)
{
	if(!node) return false;
	
#if API_VERSION < 12000
	return node->GetDescription(desc, flags);
#else
	return node->GetDescription(desc, (DESCFLAGS_DESC)flags);
#endif
}


// C4DAtom compatiblity functions
C4DAtom* CDGetClone(C4DAtom* atom, LONG flags, AliasTrans *trn)
{
	if(!atom) return NULL;
	
#if API_VERSION < 12000
	return atom->GetClone(flags, NULL);
#else
	return atom->GetClone((COPYFLAGS)flags, NULL);
#endif
}


// BaseDocument compatiblity functions
void CDAddUndo(BaseDocument *doc, LONG type, void *data)
{
	if(!doc) return;
	
#if API_VERSION < 12000
	doc->AddUndo(type, data);
#else
	doc->AddUndo((UNDOTYPE)type, data);
#endif
}

void CDAnimateObject(BaseDocument *doc, BaseList2D *op, const BaseTime &time, LONG flags)
{
	if(!doc) return;
#if API_VERSION < 12000
	LONG aFlags;

	if(flags & CD_ANIMATEFLAGS_NO_PARTICLES) aFlags |= ANIMATE_NO_PARTICLES;
	if(flags & CD_ANIMATEFLAGS_QUICK) aFlags |= ANIMATE_QUICK;
	if(flags & CD_ANIMATEFLAGS_NO_CHILDREN) aFlags |= ANIMATE_NO_CHILDS;
	#if API_VERSION > 9600
		if(flags & CD_ANIMATEFLAGS_INRENDER) aFlags |= ANIMATE_INRENDER;
	#endif
#else
	ANIMATEFLAGS aFlags;
	
	if(flags & CD_ANIMATEFLAGS_NO_PARTICLES) aFlags |= ANIMATEFLAGS_NO_PARTICLES;
	if(flags & CD_ANIMATEFLAGS_NO_CHILDREN) aFlags |= ANIMATEFLAGS_NO_CHILDREN;
	if(flags & CD_ANIMATEFLAGS_INRENDER) aFlags |= ANIMATEFLAGS_INRENDER;
	if(flags & CD_ANIMATEFLAGS_NO_MINMAX) aFlags |= ANIMATEFLAGS_NO_MINMAX; // private
	if(flags & CD_ANIMATEFLAGS_NO_NLA) aFlags |= ANIMATEFLAGS_NO_NLA; // private
	if(flags & CD_ANIMATEFLAGS_NLA_SUM) aFlags |= ANIMATEFLAGS_NLA_SUM; // private
#endif
	
	doc->AnimateObject(op, time, aFlags);
}


// Misc. compatiblity functions
LONG CDGeGetCurrentOS(void)
{
	switch(GeGetCurrentOS())
	{
	#if API_VERSION < 12000
		case GE_MAC:
			return CD_MAC;
		case GE_WIN:
			return CD_WIN;
	#else
		case OPERATINGSYSTEM_OSX:
			return CD_MAC;
		case OPERATINGSYSTEM_WIN:
			return CD_WIN;
	#endif
	}
	
	return CD_UNIX;
}

Bool CDMinVector(Vector v, Real val)
{
	return ((v.x < val) && (v.y < val) && (v.z < val));
}

DescID CDGetPSRTrackDescriptionID(LONG trkID)
{
	DescID dscID;
	
	switch(trkID)
	{
		case CD_POS_X:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
			break;
		case CD_POS_Y:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
			break;
		case CD_POS_Z:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_POSITION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
			break;
		case CD_SCA_X:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
			break;
		case CD_SCA_Y:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
			break;
		case CD_SCA_Z:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_SCALE,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
			break;
		case CD_ROT_H:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_X,DTYPE_REAL,0));
			break;
		case CD_ROT_P:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Y,DTYPE_REAL,0));
			break;
		case CD_ROT_B:
			dscID = DescID(DescLevel(CD_ID_BASEOBJECT_ROTATION,DTYPE_VECTOR,0),DescLevel(VECTOR_Z,DTYPE_REAL,0));
			break;
	}
	
	return dscID;
}

void CDPriorityListAdd(PriorityList* list, GeListNode* op, LONG priority, LONG flags)
{
	if(!list || !op) return;

#if API_VERSION < 12000
	list->Add(op, priority, flags);
#else
	list->Add(op, priority, (EXECUTIONFLAGS)flags);
#endif
}

BasePlugin* CDFindPlugin(LONG id, LONG type)
{
	switch(type)
	{
	#if API_VERSION < 12000
		case CD_COMMAND_PLUGIN:
			return FindPlugin(id,C4DPL_COMMAND);
		case CD_TOOL_PLUGIN:
			return FindPlugin(id,C4DPL_TOOL);
		case CD_OBJECT_PLUGIN:
			return FindPlugin(id,C4DPL_OBJECT);
		case CD_TAG_PLUGIN:
			return FindPlugin(id,C4DPL_TAG);
		case CD_MESSAGE_PLUGIN:
			return FindPlugin(id,C4DPL_COREMESSAGE);
		case CD_SCENEHOOK_PLUGIN:
			return FindPlugin(id,C4DPL_SCENEHOOK);
	#else
		case CD_COMMAND_PLUGIN:
			return FindPlugin(id,PLUGINTYPE_COMMAND);
		case CD_TOOL_PLUGIN:
			return FindPlugin(id,PLUGINTYPE_TOOL);
		case CD_OBJECT_PLUGIN:
			return FindPlugin(id,PLUGINTYPE_OBJECT);
		case CD_TAG_PLUGIN:
			return FindPlugin(id,PLUGINTYPE_TAG);
		case CD_MESSAGE_PLUGIN:
			return FindPlugin(id,PLUGINTYPE_COREMESSAGE);
		case CD_SCENEHOOK_PLUGIN:
			return FindPlugin(id,PLUGINTYPE_SCENEHOOK);
		case CD_PREFS_PLUGIN:
			return FindPlugin(id,PLUGINTYPE_PREFS);
	#endif
	}
	
	return NULL;
}

Real CDGetNumerator(BaseTime *t)
{
	if(!t) return 0;
	
#if API_VERSION < 12000
	return t->GetNominator();
#else
	return t->GetNumerator();
#endif
}

void CDSetNumerator(BaseTime *t, Real r)
{
	if(!t) return;
	
#if API_VERSION < 12000
	return t->SetNominator(r);
#else
	return t->SetNumerator(r);
#endif
}



// R13 specific
Bool CDGetRange(BaseSelect *bs, LONG seg, LONG *a, LONG *b, LONG maxelem)
{
	if(!bs || !a || !b) return false;
#if API_VERSION < 14000
	return bs->GetRange(seg, a, b);
#else
	return bs->GetRange(seg, maxelem, a, b);
#endif
}

Vector CDGetOptimalAngle(Vector oldRot, Vector newRot, BaseObject *op)
{
#if API_VERSION < 13000
	return GetOptimalAngle(oldRot, newRot);
#else
	if(!op) return GetOptimalAngle(oldRot, newRot, ROTATIONORDER_DEFAULT);
	else return GetOptimalAngle(oldRot, newRot, op->GetRotationOrder());
#endif
}

void CDGetActiveObjects(BaseDocument *doc, AtomArray &selection, LONG flags)
{
	if(!doc) return;
	
#if API_VERSION < 13000
	switch(flags)
	{
		case CD_GETACTIVEOBJECTFLAGS_0:
			doc->GetActiveObjects(selection,false);
			break;
		case CD_GETACTIVEOBJECTFLAGS_CHILDREN:
			doc->GetActiveObjects(selection,true);
			break;
	}
#else
	doc->GetActiveObjects(selection,(GETACTIVEOBJECTFLAGS)flags);
#endif
}

Vector CDMatrixToHPB(const Matrix &m, BaseObject *op)
{
#if API_VERSION < 13000
	return MatrixToHPB(m);
#else
	if(!op) return MatrixToHPB(m, ROTATIONORDER_DEFAULT);
	else return MatrixToHPB(m, op->GetRotationOrder());
#endif
}

Matrix CDHPBToMatrix(const Vector &hpb, BaseObject *op)
{
#if API_VERSION < 13000
	return HPBToMatrix(hpb);
#else
	if(!op) return HPBToMatrix(hpb, ROTATIONORDER_DEFAULT);
	else return HPBToMatrix(hpb, op->GetRotationOrder());
#endif
}


// R14 specific
Bool CDAddGlobalSymbol(Coffee *cof,String str,VALUE *val)
{
	if(!cof || !val) return false;
	
#if API_VERSION < 14000
    return cof->AddGlobalSymbol(str, val, ST_DATA);
#else
    return cof->AddGlobalSymbol(str, val, COFFEE_STYPE_DATA);
#endif
}

// coffee types compatibility
#if API_VERSION < 14000
VaType GetCoffeeType(LONG t) { return (VaType)t; }
#else
COFFEE_VTYPE GetCoffeeType(LONG t) { return (COFFEE_VTYPE)t; }
#endif




// R15 specific
String CDLongToString(LONG l)
{
#if API_VERSION < 15000
    return LongToString(l);
#else
    return String::IntToString(l);
#endif
}

String CDRealToString(Real v, LONG vvk, LONG nnk, Bool e, UWORD xchar)
{
#if API_VERSION < 15000
    return RealToString(v, vvk, nnk, e, xchar);
#else
    return String::FloatToString(v, vvk, nnk, e, xchar);
#endif
}

Real CDClamp(Real min, Real max, Real t)
{
#if API_VERSION < 15000
    return Clamp(min, max, t);
#else
    return ClampValue(t, min, max);
#endif
}

Real CDBlend(Real a, Real b, Real mix)
{
#if API_VERSION < 15000
    return Mix(a, b, mix);
#else
    return Blend(a, b, mix);
#endif
}

Vector CDBlend(Vector a, Vector b, Real mix)
{
#if API_VERSION < 15000
    return Mix(a, b, mix);
#else
    return Blend(a, b, mix);
#endif
}



// R15 vector & matrix compatiblity
Vector VNorm(Vector v)
{
#if API_VERSION < 15000
    return !v;
#else
    return v.GetNormalized();
#endif
}

Vector VCross(Vector a, Vector b)
{
#if API_VERSION < 15000
    return a % b;
#else
    return Cross(a, b);
#endif
}

Real VDot(Vector a, Vector b)
{
#if API_VERSION < 15000
    return a * b;
#else
    return Dot(a, b);
#endif
}

Bool VEqual(Vector a, Vector b, Real t)
{
#if API_VERSION < 15000
    return VectorEqual(a, b, t);
#else
    return a.IsEqual(b, t);
#endif
}

Vector CDTransformVector(Vector v, Matrix m)
{
#if API_VERSION < 15000
    return v ^ m;
#else
    return m.TransformVector(v);
#endif
}

Matrix MInv(Matrix m)
{
#if API_VERSION < 15000
    return !m;
#else
    return ~m;
#endif
}

//R15 File compatibility
Bool CDHFReadLong(HyperFile *hf, CDLong *l)
{
#if API_VERSION < 15000
	return hf->ReadLong(l);
#else
	return hf->ReadInt32(l);
#endif
}

Bool CDHFReadFloat(HyperFile *hf, CDFloat *f)
{
#if API_VERSION < 15000
	return hf->ReadReal(f);
#else
	return hf->ReadFloat(f);
#endif
}

Bool CDHFWriteLong(HyperFile *hf, CDLong l)
{
#if API_VERSION < 15000
	return hf->WriteLong(l);
#else
	return hf->WriteInt32(l);
#endif
}

Bool CDHFWriteFloat(HyperFile *hf, CDFloat f)
{
#if API_VERSION < 15000
	return hf->WriteReal(f);
#else
	return hf->WriteFloat(f);
#endif
}


Bool CDBFReadLong(BaseFile *bf, CDLong *l)
{
#if API_VERSION < 15000
	return bf->ReadLong(l);
#else
	return bf->ReadInt32(l);
#endif
}

Bool CDBFReadFloat(BaseFile *bf, CDFloat *f)
{
#if API_VERSION < 15000
	#if API_VERSION < 12000
		return bf->ReadReal(f);
	#else
		return bf->ReadLReal(f);
	#endif
#else
	return bf->ReadFloat(f);
#endif
}

Bool CDBFReadDouble(BaseFile *bf, CDDouble *d)
{
#if API_VERSION < 15000
	return bf->ReadLReal(d);
#else
	return bf->ReadFloat(d);
#endif
}

Bool CDBFWriteLong(BaseFile *bf, CDLong l)
{
#if API_VERSION < 15000
	return bf->WriteLong(l);
#else
	return bf->WriteInt32(l);
#endif
}

Bool CDBFWriteFloat(BaseFile *bf, CDFloat f)
{
#if API_VERSION < 15000
	#if API_VERSION < 12000
		return bf->WriteReal(f);
	#else
		return bf->WriteLReal(f);
	#endif
#else
	return bf->WriteFloat(f);
#endif
}

Bool CDBFWriteDouble(BaseFile *bf, CDDouble d)
{
#if API_VERSION < 15000
	return bf->WriteLReal(d);
#else
	return bf->WriteFloat(d);
#endif
}




// Register Functions
Bool CDRegisterTagPlugin(LONG id, const String &str, LONG info, DataAllocator *g, const String &description, const String &icon, LONG disklevel)
{
#if API_VERSION < 12000
	return RegisterTagPlugin(id, str, info, g, description, icon, disklevel);
#else
	return RegisterTagPlugin(id, str, info, g, description, AutoBitmap(icon), disklevel);
#endif
}

Bool CDRegisterObjectPlugin(LONG id, const String &str, LONG info, DataAllocator *g, const String &description, const String &icon, String icon_small, LONG disklevel)
{
#if API_VERSION < 12000
	#if API_VERSION < 9800
		return RegisterObjectPlugin(id, str, info, g, description, icon, icon_small, disklevel);
	#else
		return RegisterObjectPlugin(id, str, info, g, description, icon, disklevel);
	#endif
#else
	return RegisterObjectPlugin(id, str, info, g, description, AutoBitmap(icon), disklevel);
#endif
}

Bool CDRegisterToolPlugin(LONG id, const String& str, LONG info, String icon, const String& help, ToolData* dat)
{
#if API_VERSION < 12000
	return RegisterToolPlugin(id, str, info, icon, help, dat);
#else
	return RegisterToolPlugin(id, str, info, AutoBitmap(icon), help, dat);
#endif
	
}

Bool CDRegisterCommandPlugin(LONG id, const String& str, LONG info, String icon, const String& help, CommandData* dat)
{
	if(icon == "") return RegisterCommandPlugin(id, str, info, NULL, help, dat);
	
#if API_VERSION < 12000
	return RegisterCommandPlugin(id, str, info, icon, help, dat);
#else
	return RegisterCommandPlugin(id, str, info, AutoBitmap(icon), help, dat);
#endif
}

