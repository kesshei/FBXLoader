#ifndef _FBXModel_h_
#define _FBXModel_h_
#include <fbxsdk.h>

//FBX Ä£ÐÍ
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
	bool FetchSkeleton(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute);
	bool FetchMesh(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute);
};

#endif //_FBXModel_h_