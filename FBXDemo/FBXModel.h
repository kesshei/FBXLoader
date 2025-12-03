#ifndef _FBXModel_h_
#define _FBXModel_h_
#include <fbxsdk.h>
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
	bool Load(const char* modelFile);
private:
	bool InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
	bool DestroySdkObjects(FbxManager* pManager);
	bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* modelFile);
	bool ConvertToStandardScene(FbxManager* pManager, FbxScene* pScene);
private:
	LPModelData FetchScene(FbxScene* pScene);
	LPFRAME FetchSkeleton(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute, FbxAnimEvaluator* FbxAnim);
	LPFRAME FetchSkeletons(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute, FbxAnimEvaluator* FbxAnim, int parentIndex,int& boneIndex);
	LPMESH FetchMesh(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute);
	LPModelData FetchAnimation(FbxScene* pScene, LPModelData modelData);
private:
	LPModelData m_modelData;
};

#endif //_FBXModel_h_