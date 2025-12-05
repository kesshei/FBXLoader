#include "FBXModel.h"

FBXModel::FBXModel()
{
	m_modelData = NULL;
}

FBXModel::~FBXModel()
{
	m_modelData = NULL;
}

bool FBXModel::Load(const char* modelFile)
{
	FbxManager* l_FbxManager = NULL;
	FbxScene* l_Scene = NULL;
	bool result = InitializeSdkObjects(l_FbxManager, l_Scene);
	if (!result)
	{
		return false;
	}
	result = LoadScene(l_FbxManager, l_Scene, modelFile);
	if (!result)
	{
		return false;
	}
	//result = ConvertToStandardScene(l_FbxManager, l_Scene);
	//if (!result)
	//{
	//	return false;
	//}

	//开始准备解析模型资源
	LPModelData modelData = FetchScene(l_Scene);
	if (modelData != NULL)
	{
		m_modelData = modelData;
	}

	DestroySdkObjects(l_FbxManager);
	return true;
}

bool FBXModel::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		return false;
	}
	FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

#ifndef FBXSDK_ENV_WINSTORE
	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());
#endif

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		return false;
	}
	return true;
}

bool FBXModel::DestroySdkObjects(FbxManager* pManager)
{
	//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
	if (pManager) pManager->Destroy();
	return true;
}

bool FBXModel::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* modelFile)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	int lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(modelFile, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", modelFile, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", modelFile, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		FBXSDK_printf("\n");

		for (int i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			FBXSDK_printf("    Animation Stack %d\n", i);
			FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
			FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should be imported 
			// under a different name.
			FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false if the animation stack should be not
			// be imported. 
			FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
			FBXSDK_printf("\n");
		}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_MATERIAL, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_TEXTURE, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_LINK, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_SHAPE, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_GOBO, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_ANIMATION, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);
	if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		(*(pManager->GetIOSettings())).SetStringProp(IMP_FBX_PASSWORD, lString);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	if (!lStatus || (lImporter->GetStatus() != FbxStatus::eSuccess))
	{
		FBXSDK_printf("********************************************************************************\n");
		if (lStatus)
		{
			FBXSDK_printf("WARNING:\n");
			FBXSDK_printf("   The importer was able to read the file but with errors.\n");
			FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
		}
		else
		{
			FBXSDK_printf("Importer failed to load the file!\n\n");
		}

		if (lImporter->GetStatus() != FbxStatus::eSuccess)
			FBXSDK_printf("   Last error message: %s\n", lImporter->GetStatus().GetErrorString());

		FbxArray<FbxString*> history;
		lImporter->GetStatus().GetErrorStringHistory(history);
		if (history.GetCount() > 1)
		{
			FBXSDK_printf("   Error history stack:\n");
			for (int i = 0; i < history.GetCount(); i++)
			{
				FBXSDK_printf("      %s\n", history[i]->Buffer());
			}
		}
		FbxArrayDelete<FbxString*>(history);
		FBXSDK_printf("********************************************************************************\n");
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;


}

bool FBXModel::ConvertToStandardScene(FbxManager* pManager, FbxScene* pScene)
{
	// Convert Axis System to what is used in this example, if needed
	FbxAxisSystem SceneAxisSystem = pScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
	if (SceneAxisSystem != OurAxisSystem)
	{
		OurAxisSystem.ConvertScene(pScene);
	}

	// Convert Unit System to what is used in this example, if needed
	FbxSystemUnit SceneSystemUnit = pScene->GetGlobalSettings().GetSystemUnit();
	if (SceneSystemUnit.GetScaleFactor() != 1.0)
	{
		//The unit in this example is centimeter.
		FbxSystemUnit::cm.ConvertScene(pScene);
	}
	// Convert mesh, NURBS and patch into triangle mesh
	FbxGeometryConverter lGeomConverter(pManager);
	try {
		lGeomConverter.Triangulate(pScene, /*replace*/true);
	}
	catch (std::runtime_error) {
		FBXSDK_printf("Scene integrity verification failed.\n");
		return false;
	}
	return true;
}

// 核心递归函数：通过引用修改映射，返回引用方便调用
std::map<int, std::string>& TraverseFrameTree(LPFRAME pFrame, std::map<int, std::string>& boneMap)
{
	if (pFrame == nullptr)
		return boneMap; // 空节点直接返回原映射

	// 1. 处理当前节点：收集有效骨骼
	if (pFrame->BoneIndex >= 0 && pFrame->Name != nullptr && pFrame->Name[0] != '\0')
	{
		boneMap[pFrame->BoneIndex] = pFrame->Name;
	}

	// 2. 递归遍历子节点（传递映射引用）
	if (pFrame->pFrameFirstChild != nullptr)
	{
		TraverseFrameTree(pFrame->pFrameFirstChild, boneMap);
	}

	// 3. 遍历兄弟节点（传递映射引用）
	if (pFrame->pFrameSibling != nullptr)
	{
		TraverseFrameTree(pFrame->pFrameSibling, boneMap);
	}

	return boneMap; // 返回映射引用，支持直接接收结果
}

LPModelData FBXModel::FetchScene(FbxScene* pScene)
{
	LPModelData modelData = NULL;
	FbxNode* lNode = pScene->GetRootNode();
	FbxAnimEvaluator* FbxAnim = pScene->GetAnimationEvaluator();
	if (lNode)
	{
		modelData = new ModelData();
		//提取各种属性信息
		for (int i = 0; i < lNode->GetChildCount(); i++)
		{
			FbxNode* pNode = lNode->GetChild(i);
			FbxNodeAttribute* pNodeAttribute = pNode->GetNodeAttribute();
			if (pNodeAttribute != NULL)
			{
				FbxNodeAttribute::EType lAttributeType = (pNodeAttribute->GetAttributeType());
				switch (lAttributeType)
				{
				case FbxNodeAttribute::eSkeleton: {
					LPFRAME frame = FetchSkeleton(pNode, pNodeAttribute, FbxAnim);
					modelData->Bones.push_back(frame);
					TraverseFrameTree(frame, modelData->BoneNameToIndex);
				}break;
				case FbxNodeAttribute::eMesh:
				{
					LPMESH mesh = FetchMesh(pNode, pNodeAttribute);
					modelData->Meshs.push_back(mesh);
				}
				break;

				//case FbxNodeAttribute::eMarker:
				//	DisplayMarker(pNode);
				//	break;

				//case FbxNodeAttribute::eNurbs:
				//	DisplayNurb(pNode);
				//	break;

				//case FbxNodeAttribute::ePatch:
				//	DisplayPatch(pNode);
				//	break;

				//case FbxNodeAttribute::eCamera:
				//	DisplayCamera(pNode);
				//	break;

				//case FbxNodeAttribute::eLight:
				//	DisplayLight(pNode);
				//	break;

				//case FbxNodeAttribute::eLODGroup:
				//	DisplayLodGroup(pNode);
				//	break;
				default:
					break;
				}
			}
		}
		//提取动画信息
		FetchAnimation(pScene, modelData);
	}
	return modelData;
}

MATRIX _fbxToMatrix(const FbxAMatrix& fbxMatrix)
{
	MATRIX matrix = MATRIX();
	for (unsigned long i = 0; i < 4; ++i)
	{
		matrix(i, 0) = (float)fbxMatrix.Get(i, 0);
		matrix(i, 1) = (float)fbxMatrix.Get(i, 1);
		matrix(i, 2) = (float)fbxMatrix.Get(i, 2);
		matrix(i, 3) = (float)fbxMatrix.Get(i, 3);
	}
	return matrix;
}
MATRIX FbxMatrixToD3DXMatrix(const FbxAMatrix& fbxMat) {
	MATRIX d3dMat;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			// FBX 列优先 → D3DX 行优先：d3dMat._rc = fbxMat.m[col][row]
			d3dMat.m[row][col] = (float)fbxMat.Get(col, row);
		}
	}
	return d3dMat;
}
// 递归打印骨骼树形结构（核心函数）
// 参数说明：
// pFrame：当前要打印的骨骼帧
// prefix：当前层级的缩进前缀（控制树形格式）
// isLastSibling：当前骨骼是否是同级最后一个兄弟（决定用 └─ 还是 ├─）
void PrintBoneTree(FRAME* pFrame, const std::string& prefix, bool isLastSibling) {
	if (!pFrame) return;

	// 1. 只打印实际骨骼（bIsBone = true），跳过虚拟根帧（若有）
	if (pFrame) {
		// 打印当前骨骼：前缀 + 分支符号（├─ 或 └─） + 骨骼名称 + 索引
		std::cout << prefix;
		if (isLastSibling) {
			std::cout << "└─ "; // 最后一个兄弟，用 └─ 结尾
		}
		else {
			std::cout << "├─ "; // 非最后一个兄弟，用 ├─ 结尾
		}
		std::cout << pFrame->Name << " (索引：" << pFrame->BoneIndex << ")" << " (X:" << pFrame->TransformationMatrix._41 << ", Y:" << pFrame->TransformationMatrix._42 << ", Z:" << pFrame->TransformationMatrix._43 << ")" << std::endl;
	}

	// 2. 处理子骨骼（pFrameFirstChild）：递归深入下一层
	FRAME* pChild = (FRAME*)pFrame->pFrameFirstChild;
	if (pChild) {
		// 构建子骨骼的前缀：
		// - 若当前是最后一个兄弟：前缀 + "   "（不显示竖线）
		// - 若不是最后一个兄弟：前缀 + "│  "（显示竖线，保持层级对齐）
		std::string childPrefix = prefix + (isLastSibling ? "   " : "│  ");
		// 递归打印子骨骼，先判断子骨骼是否有兄弟（这里先处理第一个子骨骼）
		PrintBoneTree(pChild, childPrefix, !pChild->pFrameSibling);
	}

	// 3. 处理兄弟骨骼（pFrameSibling）：递归遍历同一层
	FRAME* pSibling = (FRAME*)pFrame->pFrameSibling;
	if (pSibling) {
		// 兄弟骨骼的前缀和当前骨骼一致（同一层级）
		PrintBoneTree(pSibling, prefix, !pSibling->pFrameSibling);
	}
}

// 入口函数：从根骨骼开始打印整个树
void PrintBoneTreeRoot(FRAME* pRootFrame) {
	// 关键：设置输出格式为「固定小数位 + 保留 2 位」
	std::cout << std::fixed << std::setprecision(2);
	if (!pRootFrame) {
		std::cerr << "根骨骼帧为空！" << std::endl;
		return;
	}

	std::cout << "=== BIP 骨骼树形结构 ===" << std::endl;
	// 根骨骼的前缀为空，且根骨骼无兄弟（isLastSibling = true）
	PrintBoneTree(pRootFrame, "", true);
	// （可选）恢复默认输出格式（避免影响后续 cout）
	std::cout.unsetf(std::ios::fixed);
	std::cout.precision(6); // C++ 默认精度
}



LPFRAME FBXModel::FetchSkeleton(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute, FbxAnimEvaluator* FbxAnim)
{
	int number = -1;
	LPFRAME frame = FetchSkeletons(pNode, pNodeAttribute, FbxAnim, -1, number);
	PrintBoneTreeRoot(frame);
	return frame;
}

LPFRAME FBXModel::FetchSkeletons(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute, FbxAnimEvaluator* FbxAnim, int parentIndex, int& boneIndex)
{
	LPFRAME pFrame = NULL, pParentFrame = NULL, pTempFrame = NULL, FrameRoot = NULL;
	FbxSkeleton* lSkeleton = (FbxSkeleton*)pNode->GetNodeAttribute();
	const char* lName = pNode->GetName();

	FbxAMatrix lGlobal, lLocal;
	FbxAMatrix fbxMatrix = FbxAnim->GetNodeLocalTransform(pNode);
	lGlobal = pNode->EvaluateGlobalTransform();
	lLocal = pNode->EvaluateLocalTransform();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();

	boneIndex += 1;
	pFrame = new FRAME;
	memset(pFrame, 0, sizeof(FRAME));
	pFrame->Name = lName;
	pFrame->TransformationMatrix = _fbxToMatrix(lLocal);
	pFrame->ParentBoneIndex = parentIndex;
	pFrame->BoneIndex = boneIndex;

	if (pNode->GetChildCount() > 0)
	{
		parentIndex += 1;
		for (int i = 0; i < pNode->GetChildCount(); i++)
		{
			FbxNode* pChildNode = pNode->GetChild(i);
			LPFRAME frameChild = FetchSkeletons(pChildNode, pChildNode->GetNodeAttribute(), FbxAnim, parentIndex, boneIndex);
			if (pFrame->pFrameFirstChild == NULL)
			{
				pFrame->pFrameFirstChild = frameChild;
				pParentFrame = frameChild;
			}
			else
			{
				pTempFrame = pParentFrame;
				pTempFrame->pFrameSibling = frameChild;
				pParentFrame = frameChild;
			}
		}
	}
	return pFrame;
}

LPMESH FBXModel::FetchMesh(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute)
{
	LPMESH pMesh = new MESH();
	const char* nodeName = pNode->GetName();
	pMesh->Name = nodeName;
	FbxMesh* pFbxMesh = (FbxMesh*)pNode->GetNodeAttribute();
	if (!pFbxMesh->IsTriangleMesh()) {
		FbxGeometryConverter converter(pFbxMesh->GetFbxManager());
		converter.Triangulate(pFbxMesh, true); // 强制三角化
	}

	// 1. 获取 FBX Mesh 的基础信息
	const int numVertices = pFbxMesh->GetControlPointsCount(); // 控制点数量（顶点数）
	const int numPolygons = pFbxMesh->GetPolygonCount();       // 面数量（每个面默认是三角形，需确保 FBX 已三角化）
	pMesh->VertexCount = numVertices;
	pMesh->FaceCount = numPolygons;

	const FbxVector4* controlPoints = pFbxMesh->GetControlPoints();
	const FbxGeometryElementNormal* lNormalElement = pFbxMesh->GetElementNormal(0);;
	const FbxGeometryElementUV* lUVElement = pFbxMesh->GetElementUV(0);


	//获取所有顶点，法线，UV信息
	for (int index = 0; index < numVertices; index++)
	{
		FbxVector4 currentVertex = controlPoints[index];
		VECTOR3 dxVertex = VECTOR3(
			static_cast<float>(currentVertex[0]),
			static_cast<float>(currentVertex[1]),
			static_cast<float>(currentVertex[2]));

		long lNormalIndex = index;

		if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
		{
			lNormalIndex = lNormalElement->GetIndexArray().GetAt(index);
		}

		FbxVector4 currentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
		VECTOR3 dxNormal = VECTOR3(
			static_cast<float>(currentNormal[0]),
			static_cast<float>(currentNormal[1]),
			static_cast<float>(currentNormal[2]));

		long uvIndex = index;

		if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
		{
			uvIndex = lUVElement->GetIndexArray().GetAt(index);
		}

		FbxVector2 currentUV = lUVElement->GetDirectArray().GetAt(uvIndex);
		float u = static_cast<float>(currentUV[0]);
		float v = 1 - static_cast<float>(currentUV[1]);
		pMesh->Vertices.push_back(Vertex{
			dxVertex.x, dxVertex.y, dxVertex.z,
			u, v,
			dxNormal.x,  dxNormal.y,  dxNormal.z
			});
	}

	//获取所有索引缓存信息
	std::vector<DWORD> indices;
	// 遍历每个面，提取 3 个顶点索引
	for (int polyIdx = 0; polyIdx < numPolygons; polyIdx++) {

		// 提取当前面的 3 个顶点索引
		DWORD idx0 = (DWORD)pFbxMesh->GetPolygonVertex(polyIdx, 0);
		DWORD idx1 = (DWORD)pFbxMesh->GetPolygonVertex(polyIdx, 1);
		DWORD idx2 = (DWORD)pFbxMesh->GetPolygonVertex(polyIdx, 2);

		FbxVector4 v0 = pFbxMesh->GetControlPointAt(0);
		FbxVector4 v1 = pFbxMesh->GetControlPointAt(1);
		FbxVector4 v2 = pFbxMesh->GetControlPointAt(2);

		FbxVector4 edge1 = v1 - v0;
		FbxVector4 edge2 = v2 - v0;
		FbxVector4 normal = edge1.CrossProduct(edge2);

		// 判断绕序（此为YZ平面举例）
		//if (normal[2] > 0) {
		//	printf("逆时针 CCW\n");
		//}
		//else {
		//	printf("顺时针 CW\n");
		//}

		// 5. 存入修正后的索引
		indices.push_back(idx0);
		indices.push_back(idx2);
		indices.push_back(idx1);

		pMesh->Indices.push_back(idx0);
		pMesh->Indices.push_back(idx2);
		pMesh->Indices.push_back(idx1);
	}

	// 遍历网格的所有蒙皮控制器（FbxSkin）
	int count = pFbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int skinIdx = 0; skinIdx < pFbxMesh->GetDeformerCount(FbxDeformer::eSkin); skinIdx++)
	{

		FbxSkin* pSkin = FbxCast<FbxSkin>(pFbxMesh->GetDeformer(skinIdx, FbxDeformer::eSkin));
		if (!pSkin) continue;

		std::cout << "找到蒙皮控制器，包含骨骼簇数：" << pSkin->GetClusterCount() << std::endl;

		// 遍历每个骨骼簇（FbxCluster = 一根骨骼 + 受影响顶点 + 权重）
		for (int clusterIdx = 0; clusterIdx < pSkin->GetClusterCount(); clusterIdx++)
		{
			Influence influence;
			FbxCluster* pCluster = pSkin->GetCluster(clusterIdx);
			const char* pBoneName = pCluster->GetLink()->GetName();
			const int lIndexCount = pCluster->GetControlPointIndicesCount();
			const int* lIndices = pCluster->GetControlPointIndices();
			const double* lWeights = pCluster->GetControlPointWeights();

			influence.count = lIndexCount;

			for (int i = 0; i < lIndexCount; i++)
			{
				int vertexIdx = lIndices[i];
				float weight = static_cast<float>(lWeights[i]);

				influence.Vertices.push_back(vertexIdx);
				influence.Weights.push_back(weight);
				/*		std::cout << "vertex：" << vertexIdx << "weight：" << weight << std::endl;*/
			}
			pMesh->Influences[pBoneName] = influence;
		}
		break;//默认只获取一个
	}
	//获取材质和贴图信息
	FbxNode* node = pFbxMesh->GetNode();
	int materialCount = node->GetMaterialCount();

	for (int m = 0; m < materialCount; ++m) {
		Material mat;
		FbxSurfaceMaterial* material = node->GetMaterial(m);

		// 常见的标准材质类型
		if (material->GetClassId().Is(FbxSurfacePhong::ClassId)) {
			FbxSurfacePhong* phong = (FbxSurfacePhong*)material;
			// 可访问 Diffuse、Specular 等属性
			// 漫反射颜色 Diffuse
			FbxDouble3 diffuseColor = phong->Diffuse.Get();
			// 高光颜色 Specular
			FbxDouble3 specularColor = phong->Specular.Get();
			// 环境色 Ambient
			FbxDouble3 ambientColor = phong->Ambient.Get();
			// 发光色 Emissive
			FbxDouble3 emissiveColor = phong->Emissive.Get();
			// 高光指数 Shininess
			double shininess = phong->Shininess.Get();
			// 透明度获取
			float opacity = 1.0f - phong->TransparencyFactor.Get(); // 透明度是反向的


			mat.MatD3D.Diffuse = COLORVALUE(diffuseColor[0], diffuseColor[1], diffuseColor[2]);
			mat.MatD3D.Specular = COLORVALUE(specularColor[0], specularColor[1], specularColor[2]);
			mat.MatD3D.Ambient = COLORVALUE(ambientColor[0], ambientColor[1], ambientColor[2]);
			mat.MatD3D.Emissive = COLORVALUE(emissiveColor[0], emissiveColor[1], emissiveColor[2]);
			mat.MatD3D.Power = (float)shininess;
			mat.MatD3D.Opacity = opacity;

			printf("Diffuse: %.2f %.2f %.2f\n", diffuseColor[0], diffuseColor[1], diffuseColor[2]);
			printf("Specular: %.2f %.2f %.2f\n", specularColor[0], specularColor[1], specularColor[2]);
			printf("Shininess: %.2f\n", shininess);

			FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
			int textureCount = prop.GetSrcObjectCount<FbxTexture>();
			for (int t = 0; t < textureCount; ++t) {
				FbxTexture* texture = prop.GetSrcObject<FbxTexture>(t);
				if (texture && texture->GetClassId().Is(FbxFileTexture::ClassId)) {
					FbxFileTexture* fileTex = (FbxFileTexture*)texture;
					// 获取贴图文件路径
					const char* texPath = fileTex->GetFileName();
					printf("Diffuse Texture: %s\n", texPath);
					mat.pTexture = texPath;
					break;//默认只取第一张贴图
				}
			}
			pMesh->MatD3Ds.push_back(mat);
		}
	}

	return pMesh;
}

LPModelData FBXModel::FetchAnimation(FbxScene* pScene, LPModelData modelData)
{
	//正常3dMax的fbx里，只会有一个动画，所以，我们默认只取第一个动画（各个动画如何分割呢，目前只能通过配置的方式了）
	FbxAnimStack* pAnimStack = pScene->GetSrcObject<FbxAnimStack>(0);
	if (pAnimStack == NULL)
	{
		return NULL;
	}
	LPAnimationClip pAnimClip = new AnimationClip();
	const char* name = pAnimStack->GetName();
	pAnimClip->Name = name;

	FbxAnimLayer* animLayer = pAnimStack->GetSrcObject<FbxAnimLayer>(0);
	// 遍历每个骨骼节点，提取平移/旋转/缩放的关键帧
	for (int boneIdx = 0; boneIdx < modelData->BoneNameToIndex.size(); boneIdx++) {
		const char* boneName = modelData->BoneNameToIndex[boneIdx].c_str();
		FbxNode* boneNode = pScene->FindNodeByName(boneName);
		if (!boneNode) continue;

		std::vector<AnimationKeyFrame> keyFrames;

		// 提取平移关键帧
		FbxAnimCurve* txCurve = boneNode->LclTranslation.GetCurve(animLayer, "X");
		FbxAnimCurve* tyCurve = boneNode->LclTranslation.GetCurve(animLayer, "Y");
		FbxAnimCurve* tzCurve = boneNode->LclTranslation.GetCurve(animLayer, "Z");

		// 提取旋转关键帧（FBX默认是Euler角，转换为四元数）
		FbxAnimCurve* rxCurve = boneNode->LclRotation.GetCurve(animLayer, "X");
		FbxAnimCurve* ryCurve = boneNode->LclRotation.GetCurve(animLayer, "Y");
		FbxAnimCurve* rzCurve = boneNode->LclRotation.GetCurve(animLayer, "Z");

		// 提取缩放关键帧
		FbxAnimCurve* sxCurve = boneNode->LclScaling.GetCurve(animLayer, "X");
		FbxAnimCurve* syCurve = boneNode->LclScaling.GetCurve(animLayer, "Y");
		FbxAnimCurve* szCurve = boneNode->LclScaling.GetCurve(animLayer, "Z");

		// 假设所有通道关键帧数量相同，取最大关键帧数量
		int keyCount = std::max({
			txCurve ? txCurve->KeyGetCount() : 0,
			rxCurve ? rxCurve->KeyGetCount() : 0,
			sxCurve ? sxCurve->KeyGetCount() : 0
			});


		//遍历关键帧，插值计算每个时间点的变换
		for (int k = 0; k < keyCount; k++) {
			AnimationKeyFrame keyFrame;
			FbxTime time;

			// 提取时间（FBX时间单位转换为秒）
			if (txCurve) time = txCurve->KeyGetTime(k);
			else if (rxCurve) time = rxCurve->KeyGetTime(k);
			else if (sxCurve) time = sxCurve->KeyGetTime(k);
			keyFrame.Time = (float)time.GetSecondDouble();

			// 平移插值
			keyFrame.Translation.x = txCurve ? txCurve->Evaluate(time) : 0.0f;
			keyFrame.Translation.y = tyCurve ? tyCurve->Evaluate(time) : 0.0f;
			keyFrame.Translation.z = tzCurve ? tzCurve->Evaluate(time) : 0.0f;

			// 旋转插值（Euler角→四元数）
			float rx = rxCurve ? rxCurve->Evaluate(time) * PI / 180.0f : 0.0f;
			float ry = ryCurve ? ryCurve->Evaluate(time) * PI / 180.0f : 0.0f;
			float rz = rzCurve ? rzCurve->Evaluate(time) * PI / 180.0f : 0.0f;
			//FbxQuaternion fbxQuat;
			//fbxQuat.eul(yaw, pitch, roll);
			//D3DXQuaternionRotationYawPitchRoll(&frame.rotate, ry, rx, rz);
			keyFrame.Rotation.x = rx;
			keyFrame.Rotation.y = ry;
			keyFrame.Rotation.z = rz;
			keyFrame.Rotation.w = 1;

			//// 缩放置信
			//keyFrame.Scale.x = sxCurve ? sxCurve->Evaluate(time) : 1.0f;
			//keyFrame.Scale.y = syCurve ? syCurve->Evaluate(time) : 1.0f;
			//keyFrame.Scale.z = szCurve ? szCurve->Evaluate(time) : 1.0f;

			keyFrames.push_back(keyFrame);
		}
		pAnimClip->boneKeyFrames[boneName] = keyFrames;
	}
	modelData->Animations.push_back(pAnimClip);
	return modelData;
}
