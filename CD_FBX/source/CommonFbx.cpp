//	Cactus Dan's FBX Import/Export plugin
//	Copyright 2011 by Cactus Dan Libisch

#include "CommonFbx.h"
#include "c4d.h"

#include "CDCompatibility.h"

// common fbx functions
void ConvertNurbsAndPatchRecursive(KFbxSdkManager* pSdkManager, KFbxNode* kNode, int lod)
{
    KFbxNodeAttribute* lNodeAttribute = kNode->GetNodeAttribute();
	
    if(lNodeAttribute)
    {
		KFbxGeometryConverter lConverter(pSdkManager);
        if(lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::ePATCH)
        {
			KFbxPatch *pPatch = (KFbxPatch*)kNode->GetNodeAttribute();
			if(pPatch) pPatch->SetStep(lod,lod);
			
            lConverter.TriangulateInPlace(kNode);
        }
		else if(lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eNURB)
		{
			KFbxNurb *pNurb = (KFbxNurb*)kNode->GetNodeAttribute();
			if(pNurb) pNurb->SetStep(lod,lod);
			
            lConverter.TriangulateInPlace(kNode);
		}
		else if(lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eNURBS_SURFACE)
		{
			KFbxNurbsSurface *pNurbSurface = (KFbxNurbsSurface*)kNode->GetNodeAttribute();
			if(pNurbSurface) pNurbSurface->SetStep(lod,lod);
			
			if(lConverter.ConvertNurbsSurfaceToNurbInPlace(kNode))
			{
				lConverter.TriangulateInPlace(kNode);
			}
		}
    }
	
    int i, lCount = kNode->GetChildCount();
	
    for (i = 0; i < lCount; i++)
    {
        ConvertNurbsAndPatchRecursive(pSdkManager, kNode->GetChild(i), lod);
    }
}

void ConvertNurbsAndPatch(KFbxSdkManager* pSdkManager, KFbxScene* pScene, int lod)
{
    ConvertNurbsAndPatchRecursive(pSdkManager, pScene->GetRootNode(), lod);
}


KFbxXMatrix GetGlobalPosition(KFbxNode* kNode, KTime& pTime, KFbxXMatrix* pParentGlobalPosition)
{
    return kNode->GetGlobalFromCurrentTake(pTime);
}


// Get the global position of the node for the current pose.
// If the specified node is not part of the pose, get its
// global position at the current time.
KFbxXMatrix GetGlobalPosition(KFbxNode* kNode, KTime& pTime, KFbxPose* pPose, KFbxXMatrix* pParentGlobalPosition)
{
    KFbxXMatrix lGlobalPosition;
    bool        lPositionFound = false;

    if(pPose)
    {
        int lNodeIndex = pPose->Find(kNode);

        if(lNodeIndex > -1)
        {
            // The bind pose is always a global matrix.
            // If we have a rest pose, we need to check if it is
            // stored in global or local space.
            if(pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex))
            {
                lGlobalPosition = GetPoseMatrix(pPose, lNodeIndex);
            }
            else
            {
                // We have a local matrix, we need to convert it to
                // a global space matrix.
                KFbxXMatrix lParentGlobalPosition;

                if(pParentGlobalPosition)
                {
                    lParentGlobalPosition = *pParentGlobalPosition;
                }
                else
                {
                    if(kNode->GetParent())
                    {
                        lParentGlobalPosition = GetGlobalPosition(kNode->GetParent(), pTime, pPose);
                    }
                }

                KFbxXMatrix lLocalPosition = GetPoseMatrix(pPose, lNodeIndex);
                lGlobalPosition = lParentGlobalPosition * lLocalPosition;
            }

            lPositionFound = true;
        }
    }

    if(!lPositionFound)
    {
        // There is no pose entry for that node, get the current global position instead
        lGlobalPosition = GetGlobalPosition(kNode, pTime, pParentGlobalPosition);
    }

    return lGlobalPosition;
}

// Get the matrix of the given pose
KFbxXMatrix GetPoseMatrix(KFbxPose* pPose, int pNodeIndex)
{
    KFbxXMatrix lPoseMatrix;
    KFbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

    memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

    return lPoseMatrix;
}

// Get the geometry deformation local to a node. It is never inherited by the
// children.
KFbxXMatrix GetGeometry(KFbxNode* kNode) {
    KFbxVector4 lT, lR, lS;
    KFbxXMatrix lGeometry;

    lT = kNode->GetGeometricTranslation(KFbxNode::eSOURCE_SET);
    lR = kNode->GetGeometricRotation(KFbxNode::eSOURCE_SET);
    lS = kNode->GetGeometricScaling(KFbxNode::eSOURCE_SET);

    lGeometry.SetT(lT);
    lGeometry.SetR(lR);
    lGeometry.SetS(lS);

    return lGeometry;
}

// Get the local position from the default take node.
KFbxXMatrix GetDefaultLocalPosition(KFbxNode* kNode)
{
	
	KFbxNode* pParent = kNode->GetParent();
	
	KFbxXMatrix lLocalPosition;
	
	
	if(pParent)
	{
		KFbxXMatrix lParentGlobalPosition = pParent->GetGlobalFromDefaultTake();
		KFbxXMatrix lGlobalPosition = kNode->GetGlobalFromDefaultTake();
		lLocalPosition = lParentGlobalPosition.Inverse() * lGlobalPosition;
	}
	else
	{
		lLocalPosition = kNode->GetGlobalFromDefaultTake();
		
	}
	return lLocalPosition;
}

void GetNodePSR(Vector *pos, Vector *sca, Vector *rot, KFbxNode *kNode)
{
	KFbxVector4 pPos, pRot, pSca;
	
	kNode->GetDefaultT(pPos);
	*pos = Vector(Real(pPos[0]),Real(pPos[1]),Real(pPos[2]));
	
	kNode->GetDefaultS(pSca);
	*sca = Vector(Real(pSca[0]),Real(pSca[1]),Real(pSca[2]));
	
	kNode->GetDefaultR(pRot);
	*rot = Vector(Rad(Real(pRot[0])),Rad(Real(pRot[1])),Rad(Real(pRot[2])));
}

KFbxVector4 GetOptimalRotation(KFbxVector4 oldR, KFbxVector4 newR)
{
	Vector oRot, nRot;
	
	oRot.x = Rad(oldR.GetAt(0));
	oRot.y = Rad(oldR.GetAt(1));
	oRot.z = Rad(oldR.GetAt(2));
	
	nRot.x = Rad(newR.GetAt(0));
	nRot.y = Rad(newR.GetAt(1));
	nRot.z = Rad(newR.GetAt(2));
	
	Vector ret = CDGetOptimalAngle(oRot,nRot);
	
	return KFbxVector4(double(Deg(ret.x)),double(Deg(ret.y)),double(Deg(ret.z)));
}

KFbxVector4 VectorToKFbxVector4(Vector v)
{
	return KFbxVector4(double(v.x),double(v.y),double(v.z));
}

Vector KFbxVector4ToVector(KFbxVector4 kV)
{
	return Vector(Real(kV.GetAt(0)),Real(kV.GetAt(1)),Real(kV.GetAt(2)));
}

Matrix KFbxXMatrixToMatrix(KFbxXMatrix kM, LONG handed)
{
	Matrix m;
	Matrix rotM = Matrix(), transM = Matrix();
	
	m.v1.x = kM.Get(0, 0);
	m.v1.y = kM.Get(0, 1);
	m.v1.z = kM.Get(0, 2);
	
	m.v2.x = kM.Get(1, 0);
	m.v2.y = kM.Get(1, 1);
	m.v2.z = kM.Get(1, 2);
	
	m.v3.x = kM.Get(2, 0);
	m.v3.y = kM.Get(2, 1);
	m.v3.z = kM.Get(2, 2);
	
	m.off.x = kM.Get(3, 0);
	m.off.y = kM.Get(3, 1);
	m.off.z = kM.Get(3, 2);
	
	if(handed == 1)
	{
		transM = Matrix(Vector(0,0,0), Vector(1,0,0), Vector(0,1,0), Vector(0,0,-1));
		m.v3 *= -1;
	}
	m = transM * m;
	
	return m;
}

KFbxXMatrix MatrixToKFbxXMatrix(Matrix m, LONG handed, Real scale)
{
	KFbxMatrix kM;
	KFbxXMatrix retM;
	
	Matrix transM = Matrix(), rotM = Matrix();
	m.off *= scale;
	
	if(handed == 1)
	{
		transM = Matrix(Vector(0,0,0), Vector(1,0,0), Vector(0,1,0), Vector(0,0,-1));
		m.v3 *= -1;
	}
	m = transM * m;
	
	kM.Set(0, 0, double(m.v1.x));
	kM.Set(0, 1, double(m.v1.y));
	kM.Set(0, 2, double(m.v1.z));
	
	kM.Set(1, 0, double(m.v2.x));
	kM.Set(1, 1, double(m.v2.y));
	kM.Set(1, 2, double(m.v2.z));
	
	kM.Set(2, 0, double(m.v3.x));
	kM.Set(2, 1, double(m.v3.y));
	kM.Set(2, 2, double(m.v3.z));
	
	kM.Set(3, 0, double(m.off.x));
	kM.Set(3, 1, double(m.off.y));
	kM.Set(3, 2, double(m.off.z));
	
	retM = *(KFbxXMatrix*)(double*)&kM;
	
	return retM;
}

Matrix KFbxMatrixToMatrix(KFbxMatrix kM, LONG handed)
{
	Matrix m;
	Matrix rotM = Matrix(), transM = Matrix();
	
	m.v1.x = kM.Get(0, 0);
	m.v1.y = kM.Get(0, 1);
	m.v1.z = kM.Get(0, 2);
	
	m.v2.x = kM.Get(1, 0);
	m.v2.y = kM.Get(1, 1);
	m.v2.z = kM.Get(1, 2);
	
	m.v3.x = kM.Get(2, 0);
	m.v3.y = kM.Get(2, 1);
	m.v3.z = kM.Get(2, 2);
	
	m.off.x = kM.Get(3, 0);
	m.off.y = kM.Get(3, 1);
	m.off.z = kM.Get(3, 2);
	
	if(handed == 1)
	{
		transM = Matrix(Vector(0,0,0), Vector(1,0,0), Vector(0,1,0), Vector(0,0,-1));
		m.v3 *= -1;
	}
	m = transM * m;
	
	return m;
}

KFbxMatrix MatrixToKFbxMatrix(Matrix m, LONG handed, Real scale)
{
	KFbxMatrix kM;
	
	Matrix transM = Matrix(), rotM = Matrix();
	m.off *= scale;
	
	if(handed == 1)
	{
		transM = Matrix(Vector(0,0,0), Vector(1,0,0), Vector(0,1,0), Vector(0,0,-1));
		m.v3 *= -1;
	}
	m = transM * m;
	
	kM.Set(0, 0, double(m.v1.x));
	kM.Set(0, 1, double(m.v1.y));
	kM.Set(0, 2, double(m.v1.z));
	
	kM.Set(1, 0, double(m.v2.x));
	kM.Set(1, 1, double(m.v2.y));
	kM.Set(1, 2, double(m.v2.z));
	
	kM.Set(2, 0, double(m.v3.x));
	kM.Set(2, 1, double(m.v3.y));
	kM.Set(2, 2, double(m.v3.z));
	
	kM.Set(3, 0, double(m.off.x));
	kM.Set(3, 1, double(m.off.y));
	kM.Set(3, 2, double(m.off.z));
	
	return kM;	
}

KFbxXMatrix GetGeometricPivotMatrix(KFbxNode *kNode)
{
	KFbxVector4 tOffset = kNode->GetGeometricTranslation(KFbxNode::eSOURCE_SET);
	KFbxVector4 rOffset = kNode->GetGeometricRotation(KFbxNode::eSOURCE_SET);
	KFbxVector4 sOffset = kNode->GetGeometricScaling(KFbxNode::eSOURCE_SET);
	
	return KFbxXMatrix(tOffset,rOffset,sOffset);
}


String CharToString(const char* chrs)
{
	String str;
	str.SetCString(chrs);
	return str;
}

Bool IsUsableCharacter(const String s)
{
	if(s >= "a" && s <= "z") return true;
	if(s >= "A" && s <= "Z") return true;
	if(s >= "0" && s <= "9") return true;
	if(s == "_") return true;
	
	return false;
}

String FixNameCharacters(const String s)
{
	String ret = s;
	
	LONG i, cnt = ret.GetLength();
	for(i=0; i<cnt; i++)
	{
		String subS = ret.SubStr(i,1);
		if(!IsUsableCharacter(subS))
		{
			ret.Delete(i,1);
			ret.Insert(i,"_");
		}
	}
	
	return ret;
}

char* StringToChar(String str)
{
	LONG strLen = str.GetCStringLen()+1;
	
	char *chrs = (char*)CDAlloc<char>(strLen);// must be freed
	if(chrs)
	{
	#if API_VERSION < 12000
		str.GetCString(chrs,strLen,St8bit);
	#else
		str.GetCString(chrs,strLen,STRINGENCODING_8BIT);
	#endif
	}
	
	return chrs;
}




// debug
void PrintKFbxXMatrix(KFbxXMatrix kM)
{
	GePrint("{ "+CDRealToString(Real(kM.Get(0,0)))+", "+CDRealToString(Real(kM.Get(0,1)))+", "+CDRealToString(Real(kM.Get(0,2)))+", "+CDRealToString(Real(kM.Get(0,3)))+" }");
	GePrint("{ "+CDRealToString(Real(kM.Get(1,0)))+", "+CDRealToString(Real(kM.Get(1,1)))+", "+CDRealToString(Real(kM.Get(1,2)))+", "+CDRealToString(Real(kM.Get(1,3)))+" }");
	GePrint("{ "+CDRealToString(Real(kM.Get(2,0)))+", "+CDRealToString(Real(kM.Get(2,1)))+", "+CDRealToString(Real(kM.Get(2,2)))+", "+CDRealToString(Real(kM.Get(2,3)))+" }");
	GePrint("{ "+CDRealToString(Real(kM.Get(3,0)))+", "+CDRealToString(Real(kM.Get(3,1)))+", "+CDRealToString(Real(kM.Get(3,2)))+", "+CDRealToString(Real(kM.Get(3,3)))+" }");
}

void PrintCoordinateSystem(KFbxAxisSystem &SceneAxisSystem)
{
	String hand, upV;
	
	switch(SceneAxisSystem.GetCoorSystem())
	{
		case KFbxAxisSystem::LeftHanded:
		{
			hand = "LeftHanded";
			break;
		}
		case KFbxAxisSystem::RightHanded:
		{
			hand = "RightHanded";
			break;
		}
	}
	
	int kSign;
	switch(SceneAxisSystem.GetUpVector(kSign))
	{
		case KFbxAxisSystem::XAxis:
		{
			upV = "XAxis";
			break;
		}
		case KFbxAxisSystem::YAxis:
		{
			upV = "YAxis";
			break;
		}
		case KFbxAxisSystem::ZAxis:
		{
			upV = "ZAxis";
			break;
		}
	}
	
	GePrint(hand+" - "+upV+" - sign = "+CDLongToString(kSign));
}

void PrintPivotOffsets(KFbxNode *kNode, String name)
{
    KFbxVector4 lTmpVector;
	
    GePrint(name+" Pivot Information\n");
	
    KFbxNode::EPivotState lPivotState;
    kNode->GetPivotState(KFbxNode::eSOURCE_SET, lPivotState);
	String pvState = (lPivotState == KFbxNode::ePIVOT_STATE_ACTIVE ? "Active" : "Reference");
    GePrint("        Pivot State: "+pvState);
	
    lTmpVector = kNode->GetPreRotation(KFbxNode::eSOURCE_SET);// eSOURCE_SET  eDESTINATION_SET
    GePrint("        Pre-Rotation: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetPostRotation(KFbxNode::eSOURCE_SET);
    GePrint("        Post-Rotation: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetRotationPivot(KFbxNode::eSOURCE_SET);
    GePrint("        Rotation Pivot: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetRotationOffset(KFbxNode::eSOURCE_SET);
    GePrint("        Rotation Offset: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetScalingPivot(KFbxNode::eSOURCE_SET);
    GePrint("        Scaling Pivot: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetScalingOffset(KFbxNode::eSOURCE_SET);
    GePrint("        Scaling Offset: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetGeometricTranslation(KFbxNode::eSOURCE_SET);
    GePrint("        Geometric Translation: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetGeometricRotation(KFbxNode::eSOURCE_SET);
    GePrint("        Geometric Rotation: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
	
    lTmpVector = kNode->GetGeometricScaling(KFbxNode::eSOURCE_SET);
    GePrint("        Geometric Scaling: "+CDRealToString(lTmpVector[0])+"; "+CDRealToString(lTmpVector[1])+"; "+CDRealToString(lTmpVector[2]));
}

void PrintNodeAttributeType(KFbxNode *kNode, KFbxNodeAttribute::EAttributeType lAttributeType)
{
	String oName = CharToString(kNode->GetName());
	
	switch(lAttributeType)
	{
        case KFbxNodeAttribute::eUNIDENTIFIED:
			GePrint(oName+" type = eUNIDENTIFIED");
			break;
        case KFbxNodeAttribute::eNULL:
			GePrint(oName+" type = eNULL");
			break;
		case KFbxNodeAttribute::eMARKER:
			GePrint(oName+" type = eMARKER");
			break;
        case KFbxNodeAttribute::eSKELETON: 
			GePrint(oName+" type = eSKELETON");
			break;
        case KFbxNodeAttribute::eMESH: 
			GePrint(oName+" type = eMESH");
			break;
        case KFbxNodeAttribute::eNURB: 
			GePrint(oName+" type = eNURB");
			break;
        case KFbxNodeAttribute::ePATCH:
			GePrint(oName+" type = ePATCH");
			break;
        case KFbxNodeAttribute::eCAMERA: 
			GePrint(oName+" type = eCAMERA");
			break;
        case KFbxNodeAttribute::eCAMERA_SWITCHER:
			GePrint(oName+" type = eCAMERA_SWITCHER");
			break;
        case KFbxNodeAttribute::eLIGHT:
			GePrint(oName+" type = eLIGHT");
			break;
        case KFbxNodeAttribute::eOPTICAL_REFERENCE:
			GePrint(oName+" type = eOPTICAL_REFERENCE");
			break;
        case KFbxNodeAttribute::eOPTICAL_MARKER:
			GePrint(oName+" type = eOPTICAL_MARKER");
			break;
        case KFbxNodeAttribute::eNURBS_CURVE:
			GePrint(oName+" type = eNURBS_CURVE");
			break;
        case KFbxNodeAttribute::eTRIM_NURBS_SURFACE:
			GePrint(oName+" type = eTRIM_NURBS_SURFACE");
			break;
        case KFbxNodeAttribute::eBOUNDARY:
			GePrint(oName+" type = eBOUNDARY");
			break;
        case KFbxNodeAttribute::eNURBS_SURFACE:
			GePrint(oName+" type = eNURBS_SURFACE");
			break;
        case KFbxNodeAttribute::eSHAPE:
			GePrint(oName+" type = eSHAPE");
			break;
        case KFbxNodeAttribute::eLODGROUP:
			GePrint(oName+" type = eLODGROUP");
			break;
        case KFbxNodeAttribute::eSUBDIV:
			break;
	}
}

