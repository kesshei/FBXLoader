#include "FBXModel.h"

FBXModel::FBXModel()
{
	m_modelData = NULL;
}

FBXModel::~FBXModel()
{
	m_modelData = NULL;
}

bool FBXModel::Load(std::string modelFile)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelFile, 0);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return false;
	}
	//开始准备解析模型资源
	LPModelData modelData = FetchScene(scene);
	if (modelData != NULL)
	{
		m_modelData = modelData;
	}
	return true;
}

// 核心递归函数：通过引用修改映射，返回引用方便调用
std::map<int, std::string>& TraverseFrameTree(LPFRAME pFrame, std::map<int, std::string>& boneMap)
{
	if (pFrame == nullptr)
		return boneMap; // 空节点直接返回原映射

	// 1. 处理当前节点：收集有效骨骼
	if (pFrame->BoneIndex >= 0 && !pFrame->Name.empty() && pFrame->Name[0] != '\0')
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

LPModelData FBXModel::FetchScene(const aiScene* pScene)
{
	LPModelData modelData = NULL;
	if (pScene->HasMeshes())
	{
		modelData = new ModelData();
		for (int i = 0; i < pScene->mNumMeshes; i++)
		{
			const aiMesh* paiSubMesh = pScene->mMeshes[i];
			LPMESH mesh = FetchMesh(paiSubMesh, pScene);
			modelData->Meshs.push_back(mesh);
		}
	}
	return modelData;
}

MATRIX _ToMatrix(const aiMatrix4x4& mxAI)
{
	return MATRIX(
		mxAI.a1, mxAI.a2, mxAI.a3, mxAI.a4,
		mxAI.b1, mxAI.b2, mxAI.b3, mxAI.b4,
		mxAI.c1, mxAI.c2, mxAI.c3, mxAI.c4,
		mxAI.d1, mxAI.d2, mxAI.d3, mxAI.d4);
}

MATRIX MatrixTranspose(const aiMatrix4x4& mxAI)
{
	MATRIX d3dMat;
	d3dMat._11 = mxAI.a1;  d3dMat._12 = mxAI.b1;  d3dMat._13 = mxAI.c1;  d3dMat._14 = mxAI.d1;
	d3dMat._21 = mxAI.a2;  d3dMat._22 = mxAI.b2;  d3dMat._23 = mxAI.c2;  d3dMat._24 = mxAI.d2;
	d3dMat._31 = mxAI.a3;  d3dMat._32 = mxAI.b3;  d3dMat._33 = mxAI.c3;  d3dMat._34 = mxAI.d3;
	d3dMat._41 = mxAI.a4;  d3dMat._42 = mxAI.b4;  d3dMat._43 = mxAI.c4;  d3dMat._44 = mxAI.d4;
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
	pFrame->Name = lName;
	pFrame->TransformationMatrix = _fbxToMatrix(lGlobal);
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


LPMESH FBXModel::FetchMesh(const aiMesh* paiSubMesh, const aiScene* pScene)
{
	LPMESH pMesh = new MESH();
	pMesh->Name = paiSubMesh->mName.C_Str();


	int vertexCounter = 0;
	std::map<int, Vertex> Vertexs;

	// 加载顶点常规数据
	pMesh->VertexCount = paiSubMesh->mNumVertices;
	for (unsigned int i = 0; i < paiSubMesh->mNumVertices; i++) {
		Vertex vertex;
		if (paiSubMesh->HasPositions())
		{
			vertex.x = paiSubMesh->mVertices[i].x;
			vertex.y = paiSubMesh->mVertices[i].y;
			vertex.z = paiSubMesh->mVertices[i].z;
		}

		if (paiSubMesh->HasNormals())
		{
			vertex.nx = paiSubMesh->mNormals[i].x;
			vertex.ny = paiSubMesh->mNormals[i].y;
			vertex.nz = paiSubMesh->mNormals[i].z;
		}

		// 注意这个地方只考虑一个纹理的情况，其实最多可以有八个，可以再做个循环进行加载
		if (paiSubMesh->HasTextureCoords(0))
		{
			vertex.u = paiSubMesh->mTextureCoords[0][i].x;
			vertex.v = paiSubMesh->mTextureCoords[0][i].y;
		}
	}
	// 加载索引数据
	pMesh->FaceCount = paiSubMesh->mNumFaces;
	for (unsigned int i = 0; i < paiSubMesh->mNumFaces; i++)
	{
		const aiFace& Face = paiSubMesh->mFaces[i];

		for (unsigned int k = 0; k < Face.mNumIndices; k++)
		{
			pMesh->Indices.push_back(Face.mIndices[k]);
		}
	}
	// 加载骨骼数据
	for (unsigned int i = 0; i < paiSubMesh->mNumBones; i++)
	{
		aiBone* pBone = paiSubMesh->mBones[i];
		std::string pBoneName = pBone->mName.C_Str();
		Influence influence;
		influence.BoneSpaceToModelSpace_BoneOffset = MatrixTranspose(pBone->mOffsetMatrix);
		influence.count = pBone->mNumWeights;
		for (unsigned int k = 0; k < pBone->mNumWeights; k++)
		{
			unsigned int VertexID = pBone->mWeights[k].mVertexId;
			float Weight = pBone->mWeights[k].mWeight;

			influence.Vertices.push_back(VertexID);
			influence.Weights.push_back(Weight);

			pMesh->VerticeInfluences[VertexID].push_back(Weight);
		}
		pMesh->Influences[pBoneName] = influence;
	}
	// 获取材质
	const aiMaterial* pMaterial = pScene->mMaterials[paiSubMesh->mMaterialIndex];
	aiColor3D outColor;
	if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, outColor) == aiReturn_SUCCESS)
	{
		pMesh->Material.MatD3D.Diffuse = COLORVALUE(outColor.r * 255, outColor.g * 255, outColor.b * 255);
	}
	if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, outColor) == aiReturn_SUCCESS)
	{
		pMesh->Material.MatD3D.Ambient = COLORVALUE(outColor.r * 255, outColor.g * 255, outColor.b * 255);
	}
	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, outColor) == aiReturn_SUCCESS)
	{
		pMesh->Material.MatD3D.Specular = COLORVALUE(outColor.r * 255, outColor.g * 255, outColor.b * 255);
	}
	if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, outColor) == aiReturn_SUCCESS)
	{
		pMesh->Material.MatD3D.Emissive = COLORVALUE(outColor.r * 255, outColor.g * 255, outColor.b * 255);
	}
	float outOpacity;
	if (pMaterial->Get(AI_MATKEY_OPACITY, outOpacity) == aiReturn_SUCCESS)
	{
		pMesh->Material.MatD3D.Opacity = outOpacity * 255;
	}
	float outOpacity;
	if (pMaterial->Get(AI_MATKEY_SHININESS, outOpacity) == aiReturn_SUCCESS)
	{
		pMesh->Material.MatD3D.Opacity = outOpacity * 255;
	}
	float outShininess;
	if (pMaterial->Get(AI_MATKEY_SHININESS, outShininess) == aiReturn_SUCCESS)
	{
		pMesh->Material.MatD3D.Power = outShininess;
	}
	if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		aiString aistrPath;
		if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aistrPath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
		{
			std::string TextureFileName = aistrPath.C_Str();
			pMesh->Material.pTexture = TextureFileName;
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
	pScene->SetCurrentAnimationStack(pAnimStack);
	LPAnimationClip pAnimClip = new AnimationClip();
	const char* name = pAnimStack->GetName();
	pAnimClip->Name = name;

	FbxTimeSpan kTimeSpan = pAnimStack->GetLocalTimeSpan();
	FbxTime kAnimDuration = kTimeSpan.GetDuration();
	const float fAnimLength = (float)kAnimDuration.GetSecondDouble();
	const int nFrameCount = (int)kAnimDuration.GetFrameCount(FbxTime::eFrames30);
	const int nFrameCount2 = (int)kAnimDuration.GetFrameCount();
	pAnimClip->duration = fAnimLength;
	pAnimClip->keyframes = nFrameCount;


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

		int a = txCurve ? txCurve->KeyGetCount() : -1;
		int b = rxCurve ? rxCurve->KeyGetCount() : -1;
		int c = sxCurve ? sxCurve->KeyGetCount() : -1;
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
			keyFrame.keyframe = k;

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
			//keyFrame.Rotation.x = rxCurve ? rxCurve->Evaluate(time) : 0.0f;
			//keyFrame.Rotation.y = rxCurve ? rxCurve->Evaluate(time) : 0.0f;
			//keyFrame.Rotation.z = rxCurve ? rxCurve->Evaluate(time) : 0.0f;
			//keyFrame.Rotation.w = 1;
			keyFrame.Rotation.x = rx;
			keyFrame.Rotation.y = ry;
			keyFrame.Rotation.z = rz;
			keyFrame.Rotation.w = 1;

			//// 缩放置信
			//keyFrame.Scale.x = sxCurve ? sxCurve->Evaluate(time) : 1.0f;
			//keyFrame.Scale.y = syCurve ? syCurve->Evaluate(time) : 1.0f;
			//keyFrame.Scale.z = szCurve ? szCurve->Evaluate(time) : 1.0f;
			const FbxAMatrix& kGlobalTran = boneNode->EvaluateLocalTransform(time);
			const FbxVector4 T = kGlobalTran.GetT();
			const FbxVector4 R = kGlobalTran.GetR();
			keyFrame.Translation.x = (float)T[0];
			keyFrame.Translation.x = (float)T[1];
			keyFrame.Translation.x = (float)T[2];

			keyFrame.Rotation.x = (float)R[0];
			keyFrame.Rotation.y = (float)R[1];
			keyFrame.Rotation.z = (float)R[2];
			keyFrame.Rotation.w = (float)R[3];

			keyFrames.push_back(keyFrame);
		}
		pAnimClip->boneKeyFrames[boneName] = keyFrames;
	}
	modelData->Animation = pAnimClip;
	return modelData;
}
