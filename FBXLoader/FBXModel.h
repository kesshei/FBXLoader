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
private:
	
};

#endif //_FBXModel_h_