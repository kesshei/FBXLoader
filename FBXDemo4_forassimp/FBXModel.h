#ifndef _FBXModel_h_
#define _FBXModel_h_

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <map>
#include <string>
#include "ModelData.h"

class FBXModel
{
public:
	FBXModel();
	~FBXModel();

	bool Load(const std::string modelFile);

	LPModelData m_modelData;

private:
	void ReleaseModelData();
	LPModelData FetchModelData(const aiScene* pScene);

	void FetchMaterials(const aiScene* pScene, LPModelData modelData);
	LPMaterial FetchMaterial(const aiMaterial* pMaterial);

	void FetchMeshs(const aiScene* pScene,LPModelData modelData);
	LPMESH FetchMesh(const aiMesh* pMesh, const aiScene* pScene,LPModelData modelData);

	void FetchAnimations(const aiScene* pScene,LPModelData modelData);
	LPAnimationClip FetchAnimation(const aiAnimation* pAnimation,const aiScene* pScene);

	//有问题部分
	void FetchNodeHierarchy(const aiNode* pNode, LPModelData modelData, int parentIndex = -1);
};

#endif //_FBXModel_h_