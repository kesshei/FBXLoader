#include "FBXModel.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
	const unsigned int ModelImportFlag =
		aiProcess_ConvertToLeftHanded |
		aiProcess_Triangulate |
		aiProcess_FixInfacingNormals |
		aiProcess_LimitBoneWeights |
		aiProcess_GenBoundingBoxes |
		aiProcess_JoinIdenticalVertices;

	MATRIX MatrixToD3D(const aiMatrix4x4& mxAI)
	{
		return MATRIX(
			mxAI.a1, mxAI.a2, mxAI.a3, mxAI.a4,
			mxAI.b1, mxAI.b2, mxAI.b3, mxAI.b4,
			mxAI.c1, mxAI.c2, mxAI.c3, mxAI.c4,
			mxAI.d1, mxAI.d2, mxAI.d3, mxAI.d4);
	}

	void ReleaseBoneHierarchy(LPBoneNode pNode)
	{
		if (pNode == NULL)
		{
			return;
		}

		ReleaseBoneHierarchy(pNode->pFrameFirstChild);
		ReleaseBoneHierarchy(pNode->pFrameSibling);
		delete pNode;
	}

	LPBoneNode AppendBoneNodeChain(LPBoneNode pHead, LPBoneNode pAppend)
	{
		if (pAppend == NULL)
		{
			return pHead;
		}

		if (pHead == NULL)
		{
			return pAppend;
		}

		LPBoneNode pTail = pHead;
		while (pTail->pFrameSibling != NULL)
		{
			pTail = pTail->pFrameSibling;
		}

		pTail->pFrameSibling = pAppend;
		return pHead;
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
			const aiNode* pFoundNode = FindNodeByName(pNode->mChildren[i], nodeName);
			if (pFoundNode != NULL)
			{
				return pFoundNode;
			}
		}

		return NULL;
	}

	void CollectAncestorChain(const aiNode* pNode, std::map<std::string, int>& boneNameToIndex)
	{
		if (pNode == NULL)
		{
			return;
		}

		std::vector<const aiNode*> chain;
		const aiNode* pCurrent = pNode;
		while (pCurrent != NULL)
		{
			chain.push_back(pCurrent);
			pCurrent = pCurrent->mParent;
		}

		for (int i = static_cast<int>(chain.size()) - 1; i >= 0; --i)
		{
			const std::string nodeName = chain[i]->mName.C_Str();
			if (boneNameToIndex.find(nodeName) == boneNameToIndex.end())
			{
				boneNameToIndex[nodeName] = static_cast<int>(boneNameToIndex.size());
			}
		}
	}

	void ExpandBoneNameMap(const aiScene* pScene, std::map<std::string, int>& boneNameToIndex)
	{
		if (pScene == NULL || pScene->mRootNode == NULL)
		{
			return;
		}

		std::vector<std::string> seedNames;
		for (std::map<std::string, int>::const_iterator it = boneNameToIndex.begin();it != boneNameToIndex.end();it++)
		{
			seedNames.push_back(it->first);
		}

		for (std::size_t i = 0; i < seedNames.size(); ++i)
		{
			const aiNode* pNode = FindNodeByName(pScene->mRootNode, seedNames[i]);
			CollectAncestorChain(pNode, boneNameToIndex);
		}

		if (pScene->HasAnimations())
		{
			for (unsigned int i = 0; i < pScene->mNumAnimations; ++i)
			{
				const aiAnimation* pAnimation = pScene->mAnimations[i];
				for (unsigned int j = 0; j < pAnimation->mNumChannels; ++j)
				{
					const aiNodeAnim* pChannel = pAnimation->mChannels[j];
					const aiNode* pNode = FindNodeByName(pScene->mRootNode, pChannel->mNodeName.C_Str());
					CollectAncestorChain(pNode, boneNameToIndex);
				}
			}
		}
	}

	void NormalizeWeights(Vertex& vertex)
	{
		float total = 0.0f;
		for (std::size_t i = 0; i < vertex.Weights.size(); ++i)
		{
			total += vertex.Weights[i];
		}

		if (total <= 0.0f)
		{
			return;
		}

		for (std::size_t i = 0; i < vertex.Weights.size(); ++i)
		{
			vertex.Weights[i] /= total;
		}
	}

	LPAnimationKeyFrame EnsureKeyFrame(
		std::map<double, LPAnimationKeyFrame>& keyFramesMap,
		double keyTime,
		const aiVector3D& defaultPosition,
		const aiQuaternion& defaultRotation,
		const aiVector3D& defaultScale)
	{
		std::map<double, LPAnimationKeyFrame>::iterator it = keyFramesMap.find(keyTime);
		if (it != keyFramesMap.end())
		{
			return it->second;
		}

		LPAnimationKeyFrame pKeyFrame = new AnimationKeyFrame();
		pKeyFrame->Time = static_cast<float>(keyTime);

		pKeyFrame->Translation.x = defaultPosition.x;
		pKeyFrame->Translation.y = defaultPosition.y;
		pKeyFrame->Translation.z = defaultPosition.z;

		pKeyFrame->Scale.x = defaultScale.x;
		pKeyFrame->Scale.y = defaultScale.y;
		pKeyFrame->Scale.z = defaultScale.z;

		pKeyFrame->Rotation.x = defaultRotation.x;
		pKeyFrame->Rotation.y = defaultRotation.y;
		pKeyFrame->Rotation.z = defaultRotation.z;
		pKeyFrame->Rotation.w = defaultRotation.w;

		keyFramesMap[keyTime] = pKeyFrame;
		return pKeyFrame;
	}
}

FBXModel::FBXModel()
{
	m_modelData = NULL;
}

FBXModel::~FBXModel()
{
	ReleaseModelData();
}

void FBXModel::ReleaseModelData()
{
	if (m_modelData == NULL)
	{
		return;
	}

	for (std::size_t i = 0; i < m_modelData->Materials.size(); ++i)
	{
		delete m_modelData->Materials[i];
	}

	for (std::size_t i = 0; i < m_modelData->Meshs.size(); ++i)
	{
		delete m_modelData->Meshs[i];
	}

	for (std::size_t i = 0; i < m_modelData->Bones.size(); ++i)
	{
		delete m_modelData->Bones[i];
	}

	for (std::size_t i = 0; i < m_modelData->Animations.size(); ++i)
	{
		LPAnimationClip pClip = m_modelData->Animations[i];
		for (std::map<std::string, std::vector<LPAnimationKeyFrame>>::iterator it = pClip->boneKeyFrames.begin();
			it != pClip->boneKeyFrames.end();
			++it)
		{
			for (std::size_t k = 0; k < it->second.size(); ++k)
			{
				delete it->second[k];
			}
		}
		delete pClip;
	}

	ReleaseBoneHierarchy(m_modelData->BoneHierarchyRoot.pFrameFirstChild);
	ReleaseBoneHierarchy(m_modelData->BoneHierarchyRoot.pFrameSibling);

	delete m_modelData;
	m_modelData = NULL;
}

bool FBXModel::Load(const std::string modelFile)
{
	ReleaseModelData();

	Assimp::Importer importer;
	//importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* pScene = importer.ReadFile(modelFile, ModelImportFlag);
	if (pScene == NULL || (pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || pScene->mRootNode == NULL)
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return false;
	}

	m_modelData = FetchModelData(pScene);
	return m_modelData != NULL;
}

LPModelData FBXModel::FetchModelData(const aiScene* pScene)
{
	if (pScene == NULL)
	{
		return NULL;
	}

	LPModelData modelData = new ModelData();
	std::map<std::string, int> boneNameToIndex;
	std::map<std::string, MATRIX> boneOffsetByName;

	FetchMaterials(pScene, modelData);
	FetchMeshs(pScene, modelData, boneNameToIndex, boneOffsetByName);
	FetchBones(pScene, modelData, boneNameToIndex, boneOffsetByName);
	FetchAnimations(pScene, modelData, boneNameToIndex);

	return modelData;
}

void FBXModel::FetchMaterials(const aiScene* pScene, LPModelData modelData)
{
	modelData->Materials.clear();

	if (pScene != NULL && pScene->HasMaterials())
	{
		for (unsigned int i = 0; i < pScene->mNumMaterials; i++)
		{
			modelData->Materials.push_back(FetchMaterial(pScene->mMaterials[i]));
		}
	}

	if (modelData->Materials.empty())
	{
		modelData->Materials.push_back(new Material());
	}
}

LPMaterial FBXModel::FetchMaterial(const aiMaterial* pMaterial)
{
	LPMaterial material = new Material();

	if (pMaterial == NULL)
	{
		return material;
	}

	aiColor3D color3(0.0f, 0.0f, 0.0f);
	float value = 0.0f;
	aiString texturePath;

	if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color3) == aiReturn_SUCCESS)
	{
		material->MatProps.Ambient = VECTOR4(color3.r, color3.g, color3.b, 1.0f);
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color3) == aiReturn_SUCCESS)
	{
		material->MatProps.Diffuse = VECTOR4(color3.r, color3.g, color3.b, material->MatProps.Diffuse.w);
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color3) == aiReturn_SUCCESS)
	{
		material->MatProps.Specular = VECTOR4(color3.r, color3.g, color3.b, 1.0f);
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color3) == aiReturn_SUCCESS)
	{
		material->MatProps.Emissive = VECTOR4(color3.r, color3.g, color3.b, 1.0f);
	}

	if (pMaterial->Get(AI_MATKEY_OPACITY, value) == aiReturn_SUCCESS)
	{
		material->MatProps.Opacity = value;
		material->MatProps.Diffuse.w = value;
	}

	if (pMaterial->Get(AI_MATKEY_SHININESS, value) == aiReturn_SUCCESS)
	{
		material->MatProps.Power = value;
	}

	if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
	{
		material->TexturePath = texturePath.C_Str();
	}
	else if (pMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath) == aiReturn_SUCCESS)
	{
		material->TexturePath = texturePath.C_Str();
	}

	return material;
}

void FBXModel::FetchMeshs(const aiScene* pScene,LPModelData modelData,std::map<std::string, int>& boneNameToIndex,std::map<std::string, MATRIX>& boneOffsetByName)
{
	modelData->Meshs.clear();

	if (pScene == NULL || !pScene->HasMeshes())
	{
		return;
	}

	for (unsigned int i = 0; i < pScene->mNumMeshes;i++)
	{
		LPMESH pMesh = FetchMesh(pScene->mMeshes[i], pScene, boneNameToIndex, boneOffsetByName);
		if (pMesh != NULL)
		{
			modelData->Meshs.push_back(pMesh);
		}
	}
}

LPMESH FBXModel::FetchMesh(const aiMesh* pMesh,const aiScene* pScene,std::map<std::string, int>& boneNameToIndex,std::map<std::string, MATRIX>& boneOffsetByName)
{
	if (pMesh == NULL || pMesh->mNumVertices == 0)
	{
		return NULL;
	}

	LPMESH mesh = new MESH();
	mesh->Name = pMesh->mName.C_Str();
	mesh->Vertices.resize(pMesh->mNumVertices);

	for (unsigned int i = 0; i < pMesh->mNumVertices; i++)
	{
		Vertex& vertex = mesh->Vertices[i];

		if (pMesh->HasPositions())
		{
			vertex.position.x = pMesh->mVertices[i].x;
			vertex.position.y = pMesh->mVertices[i].y;
			vertex.position.z = pMesh->mVertices[i].z;
		}

		if (pMesh->HasNormals())
		{
			vertex.normal.x = pMesh->mNormals[i].x;
			vertex.normal.y = pMesh->mNormals[i].y;
			vertex.normal.z = pMesh->mNormals[i].z;
		}

		if (pMesh->HasTextureCoords(0))
		{
			vertex.texCoord.x = pMesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = pMesh->mTextureCoords[0][i].y;
		}

		if (pMesh->HasVertexColors(0))
		{
			vertex.color.x = pMesh->mColors[0][i].r;
			vertex.color.y = pMesh->mColors[0][i].g;
			vertex.color.z = pMesh->mColors[0][i].b;
			vertex.color.w = pMesh->mColors[0][i].a;
		}
		else
		{
			vertex.color = VECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	mesh->Faces.resize(pMesh->mNumFaces * Model_INDICES_PER_FACE);

	for (unsigned int i = 0; i < pMesh->mNumFaces; i++)
	{
		const aiFace& face = pMesh->mFaces[i];
		if (face.mNumIndices != Model_INDICES_PER_FACE)
		{
			continue;
		}

		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			mesh->Faces.push_back(static_cast<UINT>(face.mIndices[j]));
		}
	}

	mesh->Material_index = -1;
	if (pScene != NULL && pScene->HasMaterials() && pMesh->mMaterialIndex < pScene->mNumMaterials)
	{
		mesh->Material_index = static_cast<int>(pMesh->mMaterialIndex);
	}

	if (pMesh->HasBones())
	{
		for (unsigned int i = 0; i < pMesh->mNumBones; i++)
		{
			const aiBone* pBone = pMesh->mBones[i];
			const std::string boneName = pBone->mName.C_Str();

			std::map<std::string, int>::iterator boneIt = boneNameToIndex.find(boneName);
			if (boneIt == boneNameToIndex.end())
			{
				boneNameToIndex[boneName] = static_cast<int>(boneNameToIndex.size());
				boneIt = boneNameToIndex.find(boneName);
			}

			if (boneOffsetByName.find(boneName) == boneOffsetByName.end())
			{
				boneOffsetByName[boneName] = MatrixToD3D(pBone->mOffsetMatrix);
			}

			const unsigned long boneIndex = static_cast<unsigned long>(boneIt->second);

			for (unsigned int w = 0; w < pBone->mNumWeights;w++)
			{
				const unsigned int vertexId = pBone->mWeights[w].mVertexId;
				if (vertexId >= mesh->Vertices.size())
				{
					continue;
				}
				mesh->Vertices[vertexId].Bones.push_back(boneIndex);
				mesh->Vertices[vertexId].Weights.push_back(pBone->mWeights[w].mWeight);
			}
		}
	}

	return mesh;
}

void FBXModel::FetchBones(const aiScene* pScene,LPModelData modelData,std::map<std::string, int>& boneNameToIndex,const std::map<std::string, MATRIX>& boneOffsetByName)
{
	modelData->Bones.clear();
	modelData->BoneHierarchyRoot = BoneNode();

	if (pScene == NULL || pScene->mRootNode == NULL)
	{
		return;
	}

	ExpandBoneNameMap(pScene, boneNameToIndex);

	if (boneNameToIndex.empty())
	{
		return;
	}

	modelData->Bones.resize(boneNameToIndex.size(), NULL);

	for (std::map<std::string, int>::const_iterator it = boneNameToIndex.begin();it != boneNameToIndex.end();it++)
	{
		LPBone bone = new Bone();
		bone->Name = it->first;

		const aiNode* pNode =  FindNodeByName(pScene->mRootNode, it->first);
		if (pNode != NULL)
		{
			bone->LocalBindPose = MatrixToD3D(pNode->mTransformation);

			const aiNode* pParent = pNode->mParent;
			while (pParent != NULL)
			{
				std::map<std::string, int>::const_iterator parentIt = boneNameToIndex.find(pParent->mName.C_Str());
				if (parentIt != boneNameToIndex.end())
				{
					bone->ParentBoneIndex = parentIt->second;
					break;
				}
				pParent = pParent->mParent;
			}
		}

		std::map<std::string, MATRIX>::const_iterator offsetIt = boneOffsetByName.find(it->first);
		if (offsetIt != boneOffsetByName.end())
		{
			bone->OffsetMatrix = offsetIt->second;
		}

		modelData->Bones[it->second] = bone;
	}

	LPBoneNode pRoot = BuildBoneHierarchy(pScene->mRootNode, boneNameToIndex, modelData);
	if (pRoot != NULL)
	{
		modelData->BoneHierarchyRoot = *pRoot;
		delete pRoot;
	}
}

LPBoneNode FBXModel::BuildBoneHierarchy(const aiNode* pNode,const std::map<std::string, int>& boneNameToIndex,LPModelData modelData)
{
	if (pNode == NULL)
	{
		return NULL;
	}

	LPBoneNode pChildren = NULL;
	for (unsigned int i = 0; i < pNode->mNumChildren; i++)
	{
		pChildren = AppendBoneNodeChain(pChildren, BuildBoneHierarchy(pNode->mChildren[i], boneNameToIndex, modelData));
	}

	std::map<std::string, int>::const_iterator it = boneNameToIndex.find(pNode->mName.C_Str());
	if (it == boneNameToIndex.end())
	{
		return pChildren;
	}

	LPBoneNode pCurrent = new BoneNode();
	pCurrent->pBone = modelData->Bones[it->second];
	pCurrent->pFrameFirstChild = pChildren;
	return pCurrent;
}

void FBXModel::FetchAnimations(const aiScene* pScene,LPModelData modelData,const std::map<std::string, int>& boneNameToIndex)
{
	modelData->Animations.clear();

	if (pScene == NULL || !pScene->HasAnimations())
	{
		return;
	}

	for (unsigned int i = 0; i < pScene->mNumAnimations; i++)
	{
		LPAnimationClip pClip = FetchAnimation(pScene->mAnimations[i], pScene, boneNameToIndex);
		if (pClip != NULL)
		{
			modelData->Animations.push_back(pClip);
		}
	}
}

LPAnimationClip FBXModel::FetchAnimation(const aiAnimation* pAnimation,const aiScene* pScene,const std::map<std::string, int>& boneNameToIndex)
{
	if (pAnimation == NULL)
	{
		return NULL;
	}

	const double ticksPerSecond = (pAnimation->mTicksPerSecond > 0.0) ? pAnimation->mTicksPerSecond : 30.0;

	LPAnimationClip pClip = new AnimationClip();
	pClip->Name = pAnimation->mName.C_Str();
	pClip->duration = static_cast<float>(pAnimation->mDuration / ticksPerSecond);

	for (unsigned int i = 0; i < pAnimation->mNumChannels; i++)
	{
		const aiNodeAnim* pChannel = pAnimation->mChannels[i];
		const std::string boneName = pChannel->mNodeName.C_Str();

		std::map<double, LPAnimationKeyFrame> keyFramesMap;

		aiVector3D defaultPosition;
		aiQuaternion defaultRotation;
		aiVector3D defaultScale;

		for (unsigned int k = 0; k < pChannel->mNumPositionKeys; k++)
		{
			double keyTime = pChannel->mPositionKeys[k].mTime;
			LPAnimationKeyFrame keyFrame = NULL;
			if (keyFramesMap.count(keyTime))
			{
				keyFrame = keyFramesMap[keyTime];
			}
			else
			{
				keyFrame = new AnimationKeyFrame();
			}
			keyFrame->Time = keyTime;
			keyFrame->Translation.x = pChannel->mPositionKeys[k].mValue.x;
			keyFrame->Translation.y = pChannel->mPositionKeys[k].mValue.y;
			keyFrame->Translation.z = pChannel->mPositionKeys[k].mValue.z;
			keyFramesMap[keyTime] = keyFrame;
		}

		for (unsigned int k = 0; k < pChannel->mNumRotationKeys; ++k)
		{
			const double keyTime = pChannel->mRotationKeys[k].mTime / ticksPerSecond;
			LPAnimationKeyFrame keyFrame = NULL;
			if (keyFramesMap.count(keyTime))
			{
				keyFrame = keyFramesMap[keyTime];
			}
			else
			{
				keyFrame = new AnimationKeyFrame();
			}
			keyFrame->Time = keyTime;
			keyFrame->Rotation.x = pChannel->mRotationKeys[k].mValue.x;
			keyFrame->Rotation.y = pChannel->mRotationKeys[k].mValue.y;
			keyFrame->Rotation.z = pChannel->mRotationKeys[k].mValue.z;
			keyFrame->Rotation.w = pChannel->mRotationKeys[k].mValue.w;
			keyFramesMap[keyTime] = keyFrame;
		}

		for (unsigned int k = 0; k < pChannel->mNumScalingKeys; ++k)
		{
			const double keyTime = pChannel->mScalingKeys[k].mTime / ticksPerSecond;
			LPAnimationKeyFrame keyFrame = NULL;
			if (keyFramesMap.count(keyTime))
			{
				keyFrame = keyFramesMap[keyTime];
			}
			else
			{
				keyFrame = new AnimationKeyFrame();
			}
			keyFrame->Time = keyTime;
			keyFrame->Scale.x = pChannel->mScalingKeys[k].mValue.x;
			keyFrame->Scale.y = pChannel->mScalingKeys[k].mValue.y;
			keyFrame->Scale.z = pChannel->mScalingKeys[k].mValue.z;
			keyFramesMap[keyTime] = keyFrame;
		}

		std::vector<LPAnimationKeyFrame> keyFrames;
		for (std::map<double, LPAnimationKeyFrame>::iterator it = keyFramesMap.begin();it != keyFramesMap.end();it++)
		{
			keyFrames.push_back(it->second);
		}

		if (!keyFrames.empty())
		{
			pClip->boneKeyFrames[boneName] = keyFrames;
		}
	}

	if (pClip->boneKeyFrames.empty())
	{
		delete pClip;
		return NULL;
	}

	return pClip;
}
