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
	bool FetchScene(FbxScene* pScene);
	FRAME* FetchSkeleton(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute, FbxAnimEvaluator* FbxAnim);
	FRAME* FetchSkeletons(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute, FbxAnimEvaluator* FbxAnim, int parentIndex);
	bool FetchMesh(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute);
};

#endif //_FBXModel_h_