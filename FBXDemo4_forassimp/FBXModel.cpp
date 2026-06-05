#include "FBXModel.h"

FBXModel::FBXModel()
{
	m_modelData = NULL;
}

FBXModel::~FBXModel()
{
	m_modelData = NULL;
}
// 导入模型使用的标志
// aiProcess_ConvertToLeftHanded: Assimp 导入的模型是以 OpenGL 的右手坐标系为基础的，将模型转换成 DirectX 的左手坐标系
// aiProcess_Triangulate：模型设计师可能使用多边形对模型进行建模的，对于用多边形建模的模型，将它们都转换成基于三角形建模
// aiProcess_FixInfacingNormals：建模软件都是双面显示的，所以设计师不会在意顶点绕序方向，部分面会被剔除无法正常显示，需要翻转过来
// aiProcess_LimitBoneWeights: 限制顶点的骨骼权重最多为 4 个，其余权重无需处理
// aiProcess_GenBoundBoxes: 对每个网格，都生成一个 AABB 体积盒
// aiProcess_JoinIdenticalVertices: 将位置相同的顶点合并为一个顶点，从而减少模型的顶点数量，优化内存使用和提升渲染效率。
//aiProcess_PopulateArmatureData | // 强制填充骨骼/蒙皮数据（关键！）
//aiProcess_ValidateDataStructure | // 验证骨骼层级结构
//aiProcess_Triangulate | aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices| aiProcess_PopulateArmatureData| aiProcess_ValidateDataStructure

// 导入文件时预处理的标志 define in "postprocess.h"
// 注意这里的Post Porcess的意思是相对于Assimp来说，导入文件中数据以后的后处理，跟渲染的后处理没有半毛钱关系
// aiProcess_LimitBoneWeights
// aiProcess_OptimizeMeshes
// aiProcess_MakeLeftHanded
// aiProcess_ConvertToLeftHanded
// aiProcess_MakeLeftHanded
#define ASSIMP_LOAD_FLAGS aiProcess_Triangulate\
 | aiProcess_GenSmoothNormals\
 | aiProcess_JoinIdenticalVertices\
 | aiProcess_ConvertToLeftHanded\
 | aiProcess_GenBoundingBoxes\
 | aiProcess_LimitBoneWeights 

//#define ASSIMP_LOAD_FLAGS 0



	// 导入模型使用的标志
	// aiProcess_ConvertToLeftHanded: Assimp 导入的模型是以 OpenGL 的右手坐标系为基础的，将模型转换成 DirectX 的左手坐标系
	// aiProcess_Triangulate：模型设计师可能使用多边形对模型进行建模的，对于用多边形建模的模型，将它们都转换成基于三角形建模
	// aiProcess_FixInfacingNormals：建模软件都是双面显示的，所以设计师不会在意顶点绕序方向，部分面会被剔除无法正常显示，需要翻转过来
	// aiProcess_LimitBoneWeights: 限制网格的骨骼权重最多为 4 个，其余权重无需处理
	// aiProcess_GenBoundBoxes: 对每个网格，都生成一个 AABB 体积盒
	// aiProcess_JoinIdenticalVertices: 将位置相同的顶点合并为一个顶点，从而减少模型的顶点数量，优化内存使用和提升渲染效率。
uint32_t ModelImportFlag = aiProcess_ConvertToLeftHanded | aiProcess_Triangulate |
aiProcess_FixInfacingNormals | aiProcess_LimitBoneWeights |
aiProcess_GenBoundingBoxes | aiProcess_JoinIdenticalVertices;


bool FBXModel::Load(std::string modelFile)
{
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* scene = importer.ReadFile(modelFile, ModelImportFlag);
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
		bool hasSkeletons = false;
		for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
		{
			const aiMesh* paiSubMesh = pScene->mMeshes[i];
			if (paiSubMesh->mNumVertices == 0)
			{
				continue;
			}
			LPMESH mesh = FetchMesh(paiSubMesh, pScene);
			if (paiSubMesh->HasBones())
			{
				hasSkeletons = true;
			}
			modelData->Meshs.push_back(mesh);
		}
		if (hasSkeletons)
		{
			LPFRAME frame = FetchSkeleton(pScene, modelData);
			modelData->Bone = frame;
			TraverseFrameTree(frame, modelData->BoneNameToIndex);
		}
		if (pScene->HasAnimations())
		{
			modelData->Animations = FetchAnimations(pScene, modelData);
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

MATRIX MatrixToD3D(const aiMatrix4x4& mxAI)
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


const aiNode* GetSkeletonNode(const aiNode* pNode, const std::map<std::string, Influence>& Influences)
{
	if (Influences.count(pNode->mName.C_Str()))
	{
		return pNode;
	}
	for (unsigned int i = 0; i < pNode->mNumChildren; i++)
	{
		const aiNode* pFoundNode = GetSkeletonNode(pNode->mChildren[i], Influences);
		if (pFoundNode != NULL)
		{
			return pFoundNode;
		}
	}
	return NULL;
}

const aiNode* FindNodeByName(const aiNode* pNode, const std::string& nodeName)
{
	if (pNode == NULL)
	{
		return NULL;
	}

	if (nodeName == pNode->mName.C_Str())
	{
		return pNode;
	}

	for (unsigned int i = 0; i < pNode->mNumChildren; ++i)
	{
		const aiNode* foundNode = FindNodeByName(pNode->mChildren[i], nodeName);
		if (foundNode != NULL)
		{
			return foundNode;
		}
	}

	return NULL;
}

LPAnimationKeyFrame CreateDefaultAnimationKeyFrame(float keyTime, const aiVector3D& defaultPosition, const aiQuaternion& defaultRotation, const aiVector3D& defaultScale)
{
	LPAnimationKeyFrame keyFrame = new AnimationKeyFrame();
	keyFrame->Time = keyTime;
	keyFrame->Translation.x = defaultPosition.x;
	keyFrame->Translation.y = defaultPosition.y;
	keyFrame->Translation.z = defaultPosition.z;
	keyFrame->Scale.x = defaultScale.x;
	keyFrame->Scale.y = defaultScale.y;
	keyFrame->Scale.z = defaultScale.z;
	keyFrame->Rotation.x = defaultRotation.x;
	keyFrame->Rotation.y = defaultRotation.y;
	keyFrame->Rotation.z = defaultRotation.z;
	keyFrame->Rotation.w = defaultRotation.w;
	return keyFrame;
}

void GetDefaultNodeTransform(const aiScene* pScene, const std::string& nodeName, aiVector3D& defaultPosition, aiQuaternion& defaultRotation, aiVector3D& defaultScale)
{
	defaultPosition = aiVector3D(0.0f, 0.0f, 0.0f);
	defaultRotation = aiQuaternion();
	defaultScale = aiVector3D(1.0f, 1.0f, 1.0f);

	if (pScene == NULL || pScene->mRootNode == NULL)
	{
		return;
	}

	const aiNode* node = FindNodeByName(pScene->mRootNode, nodeName);
	if (node == NULL)
	{
		return;
	}

	node->mTransformation.Decompose(defaultScale, defaultRotation, defaultPosition);
}

LPFRAME FBXModel::FetchSkeleton(const aiScene* pScene, LPModelData modelData)
{
	int number = -1;
	if (modelData->Meshs.size() > 0)
	{
		const aiNode* ainode = NULL;
		for (int i = 0; i < pScene->mRootNode->mNumChildren; i++)
		{
			const aiNode* temp = GetSkeletonNode(pScene->mRootNode->mChildren[i], modelData->Meshs[0]->Influences);
			if (temp != NULL)
			{
				ainode = pScene->mRootNode->mChildren[i];
				break;
			}
		}
		if (ainode)
		{
			LPFRAME frame = FetchSkeletons(ainode, -1, number);
			PrintBoneTreeRoot(frame);
			return frame;
		}
	}
	return NULL;
}


LPFRAME FBXModel::FetchSkeletons(const aiNode* pNode, int parentIndex, int& boneIndex)
{
	LPFRAME pFrame = NULL, pParentFrame = NULL, pTempFrame = NULL, FrameRoot = NULL;
	const char* lName = pNode->mName.C_Str();

	boneIndex += 1;
	pFrame = new FRAME;
	pFrame->Name = lName;
	pFrame->TransformationMatrix = MatrixToD3D(pNode->mTransformation);
	pFrame->ParentBoneIndex = parentIndex;
	pFrame->BoneIndex = boneIndex;

	if (pNode->mNumChildren > 0)
	{
		parentIndex += 1;
		for (int i = 0; i < pNode->mNumChildren; i++)
		{
			const aiNode* pChildNode = pNode->mChildren[i];
			LPFRAME frameChild = FetchSkeletons(pChildNode, parentIndex, boneIndex);
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
			vertex.position.x = paiSubMesh->mVertices[i].x;
			vertex.position.y = paiSubMesh->mVertices[i].y;
			vertex.position.z = paiSubMesh->mVertices[i].z;
		}

		if (paiSubMesh->HasNormals())
		{
			vertex.normal.x = paiSubMesh->mNormals[i].x;
			vertex.normal.y = paiSubMesh->mNormals[i].y;
			vertex.normal.z = paiSubMesh->mNormals[i].z;
		}

		// 注意这个地方只考虑一个纹理的情况，其实最多可以有八个，可以再做个循环进行加载
		if (paiSubMesh->HasTextureCoords(0))
		{
			vertex.texCoord.x = paiSubMesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = paiSubMesh->mTextureCoords[0][i].y;
		}
		if (paiSubMesh->HasVertexColors(0))
		{
			vertex.color.x	 = paiSubMesh->mColors[0][i].r;
			vertex.color.y = paiSubMesh->mColors[0][i].g;
			vertex.color.z = paiSubMesh->mColors[0][i].b;
			vertex.color.w = paiSubMesh->mColors[0][i].a;
		}
		pMesh->Vertices.push_back(vertex);
	}
	// 加载索引数据
	pMesh->FaceCount = paiSubMesh->mNumFaces;
	for (unsigned int i = 0; i < paiSubMesh->mNumFaces; i++)
	{
		pMesh->Indices.push_back(paiSubMesh->mFaces[i].mIndices[0]);
		pMesh->Indices.push_back(paiSubMesh->mFaces[i].mIndices[1]);
		pMesh->Indices.push_back(paiSubMesh->mFaces[i].mIndices[2]);
	}
	if (paiSubMesh->HasBones())
	{
		pMesh->HasBones = true;
		// 加载骨骼数据
		for (unsigned int i = 0; i < paiSubMesh->mNumBones; i++)
		{
			aiBone* pBone = paiSubMesh->mBones[i];
			std::string pBoneName = pBone->mName.C_Str();
			Influence influence;
			influence.BoneSpaceToModelSpace_BoneOffset = MatrixToD3D(pBone->mOffsetMatrix);
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
	}
	// 获取材质
	const aiMaterial* pMaterial = pScene->mMaterials[paiSubMesh->mMaterialIndex];
	LPMaterial material = new Material();
	aiColor3D outColor;

	//if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, outColor) == aiReturn_SUCCESS)
	//{
	//	material->MatD3D.Diffuse = COLORVALUE(outColor.r, outColor.g, outColor.b, 1.0f);
	//}
	//if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, outColor) == aiReturn_SUCCESS)
	//{
	//	material->MatD3D.Ambient = COLORVALUE(outColor.r, outColor.g, outColor.b, 1.0f);
	//}
	//if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, outColor) == aiReturn_SUCCESS)
	//{
	//	material->MatD3D.Specular = COLORVALUE(outColor.r, outColor.g, outColor.b, 1.0f);
	//}
	//if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, outColor) == aiReturn_SUCCESS)
	//{
	//	material->MatD3D.Emissive = COLORVALUE(outColor.r, outColor.g, outColor.b, 1.0f);
	//}

	//float out = 0.0f;
	//if (pMaterial->Get(AI_MATKEY_OPACITY, out) == aiReturn_SUCCESS)
	//{
	//	material->MatD3D.Opacity = out;
	//}

	//float outShininess = 0.0f;
	//if (pMaterial->Get(AI_MATKEY_SHININESS, outShininess) == aiReturn_SUCCESS)
	//{
	//	material->MatD3D.Power = outShininess;
	//}

	// 模型文件携带的材质/纹理文件路径
	aiString materialPath;
	if (pMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &materialPath) == aiReturn_SUCCESS)
	{
		material->TexturePath = materialPath.C_Str();
	}
	else if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &materialPath) == aiReturn_SUCCESS)
	{
		material->TexturePath = materialPath.C_Str();
	}

	pMesh->Material = material;
	return pMesh;
}

std::vector<LPAnimationClip> FBXModel::FetchAnimations(const aiScene* pScene, LPModelData modelData)
{
	std::vector<LPAnimationClip> animations;
	for (unsigned int i = 0; i < pScene->mNumAnimations; i++)
	{
		aiAnimation* animation = pScene->mAnimations[i];
		double durationInTicks = animation->mDuration;
		double ticksPerSecond = animation->mTicksPerSecond;

		if (ticksPerSecond <= 0.0)
		{
			ticksPerSecond = 25.0;
		}
		float ticksPerSecond = animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f;
		float timeInTicks = 1.0f / ticksPerSecond;
		LPAnimationClip pAnimClip = new AnimationClip();
		pAnimClip->Name = animation->mName.C_Str();
		pAnimClip->duration = static_cast<float>(durationInTicks / ticksPerSecond);
		pAnimClip->keyframes = static_cast<int>(ticksPerSecond);

		for (unsigned int j = 0; j < animation->mNumChannels; j++)
		{
			aiNodeAnim* pChannle = animation->mChannels[j];
			std::string boneName = pChannle->mNodeName.C_Str();
			std::map<double, LPAnimationKeyFrame> keyFramesMap;
			aiVector3D defaultPosition;
			aiQuaternion defaultRotation;
			aiVector3D defaultScale;
			GetDefaultNodeTransform(pScene, boneName, defaultPosition, defaultRotation, defaultScale);

			for (unsigned int b = 0; b < pChannle->mNumPositionKeys; b++)
			{
				double keyTime = pChannle->mPositionKeys[b].mTime * timeInTicks;
				const aiVector3D& position = pChannle->mPositionKeys[b].mValue;

				if (keyFramesMap.count(keyTime))
				{
					keyFramesMap[keyTime]->Translation.x = position.x;
					keyFramesMap[keyTime]->Translation.y = position.y;
					keyFramesMap[keyTime]->Translation.z = position.z;
				}
				else
				{
					LPAnimationKeyFrame keyFrame = CreateDefaultAnimationKeyFrame(static_cast<float>(keyTime), defaultPosition, defaultRotation, defaultScale);
					keyFrame->Translation.x = position.x;
					keyFrame->Translation.y = position.y;
					keyFrame->Translation.z = position.z;
					keyFramesMap[keyTime] = keyFrame;
				}
			}

			for (unsigned int a = 0; a < pChannle->mNumRotationKeys; a++)
			{
				double keyTime = pChannle->mRotationKeys[a].mTime * timeInTicks;
				const aiQuaternion& rotation = pChannle->mRotationKeys[a].mValue;

				if (keyFramesMap.count(keyTime))
				{
					keyFramesMap[keyTime]->Rotation.x = rotation.x;
					keyFramesMap[keyTime]->Rotation.y = rotation.y;
					keyFramesMap[keyTime]->Rotation.z = rotation.z;
					keyFramesMap[keyTime]->Rotation.w = rotation.w;
				}
				else
				{
					LPAnimationKeyFrame keyFrame = CreateDefaultAnimationKeyFrame(static_cast<float>(keyTime), defaultPosition, defaultRotation, defaultScale);
					keyFrame->Rotation.x = rotation.x;
					keyFrame->Rotation.y = rotation.y;
					keyFrame->Rotation.z = rotation.z;
					keyFrame->Rotation.w = rotation.w;
					keyFramesMap[keyTime] = keyFrame;
				}
			}

			for (unsigned int c = 0; c < pChannle->mNumScalingKeys; c++)
			{
				double keyTime = pChannle->mScalingKeys[c].mTime * timeInTicks;
				const aiVector3D& scale = pChannle->mScalingKeys[c].mValue;

				if (keyFramesMap.count(keyTime))
				{
					keyFramesMap[keyTime]->Scale.x = scale.x;
					keyFramesMap[keyTime]->Scale.y = scale.y;
					keyFramesMap[keyTime]->Scale.z = scale.z;
				}
				else
				{
					LPAnimationKeyFrame keyFrame = CreateDefaultAnimationKeyFrame(static_cast<float>(keyTime), defaultPosition, defaultRotation, defaultScale);
					keyFrame->Scale.x = scale.x;
					keyFrame->Scale.y = scale.y;
					keyFrame->Scale.z = scale.z;
					keyFramesMap[keyTime] = keyFrame;
				}
			}

			std::vector<AnimationKeyFrame> keyFrames;
			for (const auto& pair : keyFramesMap)
			{
				keyFrames.push_back(*(pair.second));
			}
			pAnimClip->boneKeyFrames[boneName] = keyFrames;
		}
		animations.push_back(pAnimClip);
	}

	return animations;
}
