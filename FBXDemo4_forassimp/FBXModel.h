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

	void FetchMeshs(const aiScene* pScene,LPModelData modelData,std::map<std::string, int>& boneNameToIndex,std::map<std::string, MATRIX>& boneOffsetByName);
	LPMESH FetchMesh(const aiMesh* pMesh, const aiScene* pScene, std::map<std::string, int>& boneNameToIndex, std::map<std::string, MATRIX>& boneOffsetByName);

	void FetchAnimations(const aiScene* pScene,LPModelData modelData,const std::map<std::string, int>& boneNameToIndex);
	LPAnimationClip FetchAnimation(const aiAnimation* pAnimation,const aiScene* pScene,const std::map<std::string, int>& boneNameToIndex);

	//有问题部分
	void FetchBones(const aiScene* pScene, LPModelData modelData, std::map<std::string, int>& boneNameToIndex, const std::map<std::string, MATRIX>& boneOffsetByName);
	LPBoneNode BuildBoneHierarchy(const aiNode* pNode, const std::map<std::string, int>& boneNameToIndex, LPModelData modelData);
};

#endif //_FBXModel_h_