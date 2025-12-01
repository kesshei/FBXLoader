#include "FBXModel.h"

FBXModel::FBXModel()
{
}

FBXModel::~FBXModel()
{
}

bool FBXModel::Load(const char* modelFile)
{
	FbxManager* l_FbxManager = NULL;
	FbxScene* l_Scene = NULL;
	bool result = InitFBXSDK(l_FbxManager, l_Scene);
	if (!result)
	{
		return false;
	}
	result = LoadScene(l_FbxManager, l_Scene, modelFile);
	if (!result)
	{
		return false;
	}

	l_FbxManager->Destroy();
	return true;
}

bool FBXModel::InitFBXSDK(FbxManager*& pManager, FbxScene*& pScene)
{
	// Initialize the SDK manager. This object handles all our memory management.
	pManager = FbxManager::Create();

	// Create the IO settings object.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());

	// Create a new scene so that it can be populated by the imported file.
	pScene = FbxScene::Create(pManager, "myScene");

	return true;
}

bool FBXModel::LoadScene(FbxManager* pManager, FbxScene* pScene, const char* modelFile)
{
	// Create an importer using the SDK manager.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Use the first argument as the filename for the importer.
	bool result = lImporter->Initialize(modelFile, -1, pManager->GetIOSettings());
	if (result)
	{
		// Import the contents of the file into the scene.
		lImporter->Import(pScene);
		result = true;
	}
	// The file is imported; so get rid of the importer.
	lImporter->Destroy();
	return result;
}
