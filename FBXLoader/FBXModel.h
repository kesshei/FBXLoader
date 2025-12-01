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
	bool InitFBXSDK(FbxManager*& pManager, FbxScene*& pScene);
	bool LoadScene(FbxManager* pManager, FbxScene* pScene, const char* modelFile);
private:
	
};

#endif //_FBXModel_h_