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
	bool result = InitializeSdkObjects(l_FbxManager, l_Scene);
	if (!result)
	{
		return false;
	}
	result = LoadScene(l_FbxManager, l_Scene, modelFile);
	if (!result)
	{
		return false;
	}
	result = ConvertToStandardScene(l_FbxManager, l_Scene);
	if (!result)
	{
		return false;
	}
	//开始准备解析模型资源



	DestroySdkObjects(l_FbxManager);
	return true;
}

bool FBXModel::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		return false;
	}
	FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

#ifndef FBXSDK_ENV_WINSTORE
	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());
#endif

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		return false;
	}
	return true;
}

bool FBXModel::DestroySdkObjects(FbxManager* pManager)
{
	//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
	if (pManager) pManager->Destroy();
	return true;
}

bool FBXModel::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* modelFile)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	int lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(modelFile, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", modelFile, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", modelFile, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		FBXSDK_printf("\n");

		for (int i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			FBXSDK_printf("    Animation Stack %d\n", i);
			FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
			FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should be imported 
			// under a different name.
			FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false if the animation stack should be not
			// be imported. 
			FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
			FBXSDK_printf("\n");
		}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_MATERIAL, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_TEXTURE, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_LINK, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_SHAPE, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_GOBO, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_ANIMATION, true);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);
	if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		(*(pManager->GetIOSettings())).SetStringProp(IMP_FBX_PASSWORD, lString);
		(*(pManager->GetIOSettings())).SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	if (!lStatus || (lImporter->GetStatus() != FbxStatus::eSuccess))
	{
		FBXSDK_printf("********************************************************************************\n");
		if (lStatus)
		{
			FBXSDK_printf("WARNING:\n");
			FBXSDK_printf("   The importer was able to read the file but with errors.\n");
			FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
		}
		else
		{
			FBXSDK_printf("Importer failed to load the file!\n\n");
		}

		if (lImporter->GetStatus() != FbxStatus::eSuccess)
			FBXSDK_printf("   Last error message: %s\n", lImporter->GetStatus().GetErrorString());

		FbxArray<FbxString*> history;
		lImporter->GetStatus().GetErrorStringHistory(history);
		if (history.GetCount() > 1)
		{
			FBXSDK_printf("   Error history stack:\n");
			for (int i = 0; i < history.GetCount(); i++)
			{
				FBXSDK_printf("      %s\n", history[i]->Buffer());
			}
		}
		FbxArrayDelete<FbxString*>(history);
		FBXSDK_printf("********************************************************************************\n");
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;


}

bool FBXModel::ConvertToStandardScene(FbxManager* pManager, FbxScene* pScene)
{
	// Convert Axis System to what is used in this example, if needed
	FbxAxisSystem SceneAxisSystem = pScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
	if (SceneAxisSystem != OurAxisSystem)
	{
		OurAxisSystem.ConvertScene(pScene);
	}

	// Convert Unit System to what is used in this example, if needed
	FbxSystemUnit SceneSystemUnit = pScene->GetGlobalSettings().GetSystemUnit();
	if (SceneSystemUnit.GetScaleFactor() != 1.0)
	{
		//The unit in this example is centimeter.
		FbxSystemUnit::cm.ConvertScene(pScene);
	}
	// Convert mesh, NURBS and patch into triangle mesh
	FbxGeometryConverter lGeomConverter(pManager);
	try {
		lGeomConverter.Triangulate(pScene, /*replace*/true);
	}
	catch (std::runtime_error) {
		FBXSDK_printf("Scene integrity verification failed.\n");
		return false;
	}
	return true;
}

bool FBXModel::FetchScene(FbxScene* pScene)
{
	int i;
	FbxNode* lNode = pScene->GetRootNode();

	if (lNode)
	{
		for (i = 0; i < lNode->GetChildCount(); i++)
		{
			FbxNode* pNode = lNode->GetChild(i);
			FbxNodeAttribute* pNodeAttribute = pNode->GetNodeAttribute();
			if (pNodeAttribute != NULL)
			{
				FbxNodeAttribute::EType lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());
				switch (lAttributeType)
				{
				case FbxNodeAttribute::eSkeleton:
					FetchSkeleton(pNode, pNodeAttribute);
					break;

				case FbxNodeAttribute::eMesh:
					FetchMesh(pNode, pNodeAttribute);
					break;

					//case FbxNodeAttribute::eMarker:
					//	DisplayMarker(pNode);
					//	break;

					//case FbxNodeAttribute::eNurbs:
					//	DisplayNurb(pNode);
					//	break;

					//case FbxNodeAttribute::ePatch:
					//	DisplayPatch(pNode);
					//	break;

					//case FbxNodeAttribute::eCamera:
					//	DisplayCamera(pNode);
					//	break;

					//case FbxNodeAttribute::eLight:
					//	DisplayLight(pNode);
					//	break;

					//case FbxNodeAttribute::eLODGroup:
					//	DisplayLodGroup(pNode);
					//	break;
				default:
					break;
				}
			}
		}
	}
	return true;
}
MATRIX FbxMatrixToD3DXMatrix(const FbxAMatrix& fbxMat) {
	MATRIX d3dMat;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			// FBX 列优先 → D3DX 行优先：d3dMat._rc = fbxMat.m[col][row]
			d3dMat.m[row][col] = (float)fbxMat.Get(col, row);
		}
	}
	return d3dMat;
}
bool FBXModel::FetchSkeleton(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute)
{

	//LPFRAME frame = FetchSkeletons(pNode, pNodeAttribute, -1);

	return false;
}

FRAME* FBXModel::FetchSkeletons(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute, int parentIndex)
{
	LPFRAME pFrame = NULL, pParentFrame=NULL, pTempFrame = NULL, FrameRoot = NULL;
	FbxSkeleton* lSkeleton = (FbxSkeleton*)pNode->GetNodeAttribute();
	const char* lName = pNode->GetName();

	FbxAMatrix lGlobal, lLocal;
	lGlobal = pNode->EvaluateGlobalTransform();
	lLocal = pNode->EvaluateLocalTransform();

	pFrame = new FRAME;
	memset(pFrame, 0, sizeof(FRAME));
	pFrame->Name = lName;
	pFrame->TransformationMatrix = FbxMatrixToD3DXMatrix(lLocal);

	if (pParentFrame == NULL)
	{
		pTempFrame = pFrame;
		FrameRoot = pFrame;
		pFrame->pFrameSibling = pTempFrame;
		pParentFrame = pFrame;
	}
	else
	{
		pTempFrame = pParentFrame->pFrameFirstChild;
		pParentFrame->pFrameFirstChild = pFrame;
		pFrame->pFrameSibling = pTempFrame;
	}
	parentIndex += 1;
	if (pNode->GetChildCount() > 0)
	{


	}
	for (int i = 0; i < pNode->GetChildCount(); i++)
	{
		FbxNode* pChildNode = pNode->GetChild(i);
		pFrame = FetchSkeletons(pChildNode, pChildNode->GetNodeAttribute(), parentIndex);
		pTempFrame = pParentFrame->pFrameFirstChild;
		pParentFrame->pFrameFirstChild = pFrame;
		pFrame->pFrameSibling = pTempFrame;
	}
	return pFrame;
}

bool FBXModel::FetchMesh(FbxNode* pNode, FbxNodeAttribute* pNodeAttribute)
{
	return false;
}
