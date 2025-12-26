#ifndef _FBXModel_h_
#define _FBXModel_h_
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include "ModelData.h"
#include <iostream>
#include <string>
#include <iomanip> // 用于格式化浮点数输出（保留 2 位小数）


//FBX 模型
class FBXModel
{
public:
	FBXModel();
	~FBXModel();
	bool Load(std::string modelFile);
	LPModelData m_modelData;
private:
	//bool InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
	//bool DestroySdkObjects(FbxManager* pManager);
	//bool LoadScene(FbxManager* pManager, FbxDocument* pScene, std::string modelFile);
	//bool ConvertToStandardScene(FbxManager* pManager, FbxScene* pScene);
private:
	LPModelData FetchScene(const aiScene* pScene);
	LPFRAME FetchSkeleton(const aiScene* pScene, LPModelData modelData);
	LPFRAME FetchSkeletons(const aiNode* pNode, int parentIndex, int& boneIndex);
	LPMESH FetchMesh(const aiMesh* paiSubMesh, const aiScene* pScene);
	std::vector<LPAnimationClip> FetchAnimations(const aiScene* pScene, LPModelData modelData);
};

#endif //_FBXModel_h_