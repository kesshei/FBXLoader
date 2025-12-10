#include "D3DXLoadMeshHierarchyFromFBX.h"
#include "FBXModel.h"

//--------------------------------------------------------------------------------------
// Name: AllocateName()
// Desc: 为骨骼或网格名称的字符串分配内存
//--------------------------------------------------------------------------------------
HRESULT AllocateName(LPCSTR Name, LPSTR* pNewName)
{
	UINT cbLength;

	if (Name != NULL)
	{
		cbLength = (UINT)strlen(Name) + 1;
		*pNewName = new CHAR[cbLength];
		memcpy(*pNewName, Name, cbLength * sizeof(CHAR));
	}
	else
	{
		*pNewName = NULL;
	}

	return S_OK;
}
HRESULT CreateFrame(LPCSTR Name, LPD3DXFRAME* ppNewFrame)
{
	HRESULT hr = S_OK;
	D3DXFRAME_DERIVED* pFrame;

	*ppNewFrame = NULL;

	//为框架指定名称
	pFrame = new D3DXFRAME_DERIVED;  //创建框架结构对象
	if (FAILED(AllocateName(Name, (LPSTR*)&pFrame->Name)))
	{
		delete pFrame;
		return hr;
	}

	//初始化D3DXFRAME_DERIVED结构其它成员变量
	D3DXMatrixIdentity(&pFrame->TransformationMatrix);
	D3DXMatrixIdentity(&pFrame->CombinedTransformationMatrix);

	pFrame->pMeshContainer = NULL;
	pFrame->pFrameSibling = NULL;
	pFrame->pFrameFirstChild = NULL;

	*ppNewFrame = pFrame;
	pFrame = NULL;

	return hr;
}
HRESULT GenerateSkinnedMesh(IDirect3DDevice9* pd3dDevice, D3DXMESHCONTAINER_DERIVED* pMeshContainer)
{
	D3DCAPS9 d3dCaps;
	pd3dDevice->GetDeviceCaps(&d3dCaps);

	if (pMeshContainer->pSkinInfo == NULL)
		return S_OK;

	SAFE_RELEASE(pMeshContainer->MeshData.pMesh);
	SAFE_RELEASE(pMeshContainer->pBoneCombinationBuf);


	if (FAILED(pMeshContainer->pSkinInfo->ConvertToBlendedMesh(
		pMeshContainer->pOrigMesh,
		D3DXMESH_MANAGED | D3DXMESHOPT_VERTEXCACHE,
		pMeshContainer->pAdjacency,
		NULL, NULL, NULL,
		&pMeshContainer->NumInfl,
		&pMeshContainer->NumAttributeGroups,
		&pMeshContainer->pBoneCombinationBuf,
		&pMeshContainer->MeshData.pMesh)))
		return E_FAIL;
	return S_OK;
}
//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::CreateMeshContainer()
// Desc: 创建蒙皮网格容器，以加载蒙皮信息
//--------------------------------------------------------------------------------------
HRESULT CreateMeshContainer(LPCSTR Name,
	CONST D3DXMESHDATA* pMeshData,
	CONST D3DXMATERIAL* pMaterials,
	CONST D3DXEFFECTINSTANCE* pEffectInstances,
	DWORD NumMaterials,
	CONST DWORD* pAdjacency,
	LPD3DXSKININFO pSkinInfo,
	LPD3DXMESHCONTAINER* ppNewMeshContainer)
{
	HRESULT hr;
	UINT NumFaces;
	UINT iMaterial;
	UINT iBone, cBones;
	LPDIRECT3DDEVICE9 pd3dDevice = NULL;
	D3DXMESHCONTAINER_DERIVED* pMeshContainer = NULL;

	LPD3DXMESH pMesh = NULL;

	*ppNewMeshContainer = NULL;

	// this sample does not handle patch meshes, so fail when one is found
	if (pMeshData->Type != D3DXMESHTYPE_MESH)
	{
		hr = E_FAIL;
		goto e_Exit;
	}

	// get the pMesh interface pointer out of the mesh data structure
	pMesh = pMeshData->pMesh;

	// this sample does not FVF compatible meshes, so fail when one is found
	if (pMesh->GetFVF() == 0)
	{
		hr = E_FAIL;
		goto e_Exit;
	}

	// allocate the overloaded structure to return as a D3DXMESHCONTAINER
	pMeshContainer = new D3DXMESHCONTAINER_DERIVED;
	memset(pMeshContainer, 0, sizeof(D3DXMESHCONTAINER_DERIVED));

	// make sure and copy the name.  All memory as input belongs to caller, interfaces can be addref'd though
	hr = AllocateName(Name, &pMeshContainer->Name);
	if (FAILED(hr))
		goto e_Exit;

	pMesh->GetDevice(&pd3dDevice);
	NumFaces = pMesh->GetNumFaces();

	// if no normals are in the mesh, add them
	if (!(pMesh->GetFVF() & D3DFVF_NORMAL))
	{
		pMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;

		// clone the mesh to make room for the normals
		hr = pMesh->CloneMeshFVF(pMesh->GetOptions(), pMesh->GetFVF() | D3DFVF_NORMAL,
			pd3dDevice, &pMeshContainer->MeshData.pMesh);
		if (FAILED(hr))
			goto e_Exit;

		// get the new pMesh pointer back out of the mesh container to use
		// NOTE: we do not release pMesh because we do not have a reference to it yet
		pMesh = pMeshContainer->MeshData.pMesh;

		// now generate the normals for the pmesh
		D3DXComputeNormals(pMesh, NULL);
	}
	else  // if no normals, just add a reference to the mesh for the mesh container
	{
		pMeshContainer->MeshData.pMesh = pMesh;
		pMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;

		pMesh->AddRef();
	}

	// allocate memory to contain the material information.  This sample uses
	//   the D3D9 materials and texture names instead of the EffectInstance style materials
	pMeshContainer->NumMaterials = max(1, NumMaterials);
	pMeshContainer->pMaterials = new D3DXMATERIAL[pMeshContainer->NumMaterials];
	//pMeshContainer->ppTextures = new LPDIRECT3DTEXTURE9[pMeshContainer->NumMaterials];
	pMeshContainer->pAdjacency = new DWORD[NumFaces * 3];

	memcpy(pMeshContainer->pAdjacency, pAdjacency, sizeof(DWORD) * NumFaces * 3);
	memset(pMeshContainer->ppTextures, 0, sizeof(LPDIRECT3DTEXTURE9) * pMeshContainer->NumMaterials);

	// if materials provided, copy them
	if (NumMaterials > 0)
	{
		memcpy(pMeshContainer->pMaterials, pMaterials, sizeof(D3DXMATERIAL) * NumMaterials);

		for (iMaterial = 0; iMaterial < NumMaterials; iMaterial++)
		{
			if (pMeshContainer->pMaterials[iMaterial].pTextureFilename != NULL)
			{
				//if (FAILED(D3DXCreateTextureFromFileA(pd3dDevice, pMeshContainer->pMaterials[iMaterial].pTextureFilename,
				//	&pMeshContainer->ppTextures[iMaterial])))
				//	pMeshContainer->ppTextures[iMaterial] = NULL;

				// don't remember a pointer into the dynamic memory, just forget the name after loading
				pMeshContainer->pMaterials[iMaterial].pTextureFilename = NULL;
			}
		}
	}
	else // if no materials provided, use a default one
	{
		pMeshContainer->pMaterials[0].pTextureFilename = NULL;
		memset(&pMeshContainer->pMaterials[0].MatD3D, 0, sizeof(D3DMATERIAL9));
		pMeshContainer->pMaterials[0].MatD3D.Diffuse.r = 0.5f;
		pMeshContainer->pMaterials[0].MatD3D.Diffuse.g = 0.5f;
		pMeshContainer->pMaterials[0].MatD3D.Diffuse.b = 0.5f;
		pMeshContainer->pMaterials[0].MatD3D.Specular = pMeshContainer->pMaterials[0].MatD3D.Diffuse;
	}

	// if there is skinning information, save off the required data and then setup for HW skinning
	if (pSkinInfo != NULL)
	{
		// first save off the SkinInfo and original mesh data
		pMeshContainer->pSkinInfo = pSkinInfo;
		pSkinInfo->AddRef();

		pMeshContainer->pOrigMesh = pMesh;
		pMesh->AddRef();

		// Will need an array of offset matrices to move the vertices from the figure space to the bone's space
		cBones = pSkinInfo->GetNumBones();
		pMeshContainer->pBoneOffsetMatrices = new D3DXMATRIX[cBones];

		// get each of the bone offset matrices so that we don't need to get them later
		for (iBone = 0; iBone < cBones; iBone++)
		{
			pMeshContainer->pBoneOffsetMatrices[iBone] = *(pMeshContainer->pSkinInfo->GetBoneOffsetMatrix(iBone));
		}

		// GenerateSkinnedMesh will take the general skinning information and transform it to a HW friendly version
		hr = GenerateSkinnedMesh(pd3dDevice, pMeshContainer);
		if (FAILED(hr))
			goto e_Exit;
	}

	*ppNewMeshContainer = pMeshContainer;
	pMeshContainer = NULL;

e_Exit:
	SAFE_RELEASE(pd3dDevice);

	// call Destroy function to properly clean up the memory allocated 
	if (pMeshContainer != NULL)
	{
		DestroyMeshContainer(pMeshContainer);
	}

	return hr;
}
//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::DestroyFrame()
// Desc: 释放骨骼框架
//--------------------------------------------------------------------------------------
HRESULT DestroyFrame(LPD3DXFRAME pFrameToFree)
{
	SAFE_DELETE_ARRAY(pFrameToFree->Name);
	SAFE_DELETE(pFrameToFree);
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::DestroyMeshContainer()
// Desc: 释放网格容器
//--------------------------------------------------------------------------------------
HRESULT DestroyMeshContainer(LPD3DXMESHCONTAINER pMeshContainerBase)
{
	UINT iMaterial;
	D3DXMESHCONTAINER_DERIVED* pMeshContainer = (D3DXMESHCONTAINER_DERIVED*)pMeshContainerBase;

	SAFE_DELETE_ARRAY(pMeshContainer->Name);
	SAFE_DELETE_ARRAY(pMeshContainer->pAdjacency);
	SAFE_DELETE_ARRAY(pMeshContainer->pMaterials);
	SAFE_DELETE_ARRAY(pMeshContainer->pBoneOffsetMatrices);

	// release all the allocated textures
	if (pMeshContainer->ppTextures != NULL)
	{
		for (iMaterial = 0; iMaterial < pMeshContainer->NumMaterials; iMaterial++)
		{
			/*	SAFE_RELEASE(pMeshContainer->ppTextures[iMaterial]);*/
		}
	}

	SAFE_DELETE_ARRAY(pMeshContainer->ppTextures);
	SAFE_DELETE_ARRAY(pMeshContainer->ppBoneMatrixPtrs);
	SAFE_RELEASE(pMeshContainer->pBoneCombinationBuf);
	SAFE_RELEASE(pMeshContainer->MeshData.pMesh);
	SAFE_RELEASE(pMeshContainer->pSkinInfo);
	SAFE_RELEASE(pMeshContainer->pOrigMesh);
	SAFE_DELETE(pMeshContainer);
	return S_OK;
}

D3DXMATRIX ToD3DXMATRIX(MATRIX matrix)
{
	D3DXMATRIX mat(
		(FLOAT)matrix._11, (FLOAT)matrix._12, (FLOAT)matrix._13, (FLOAT)matrix._14,
		(FLOAT)matrix._21, (FLOAT)matrix._22, (FLOAT)matrix._23, (FLOAT)matrix._24,
		(FLOAT)matrix._31, (FLOAT)matrix._32, (FLOAT)matrix._33, (FLOAT)matrix._34,
		(FLOAT)matrix._41, (FLOAT)matrix._42, (FLOAT)matrix._43, (FLOAT)matrix._44);
	return mat;
}
LPSTR ConstCharToLPSTR(const char* src) {
	if (src == nullptr) return nullptr;

	// 1. 计算长度（包含终止符）
	size_t len = strlen(src) + 1;
	// 2. 分配可写内存（堆内存，需手动释放）
	LPSTR dst = (LPSTR)malloc(len);
	if (dst == nullptr) return nullptr;
	// 3. 拷贝内容
	strcpy_s(dst, len, src);

	return dst;
}
static LPSTR CopyString(const LPSTR src) {
	if (!src) return nullptr;
	size_t len = strlen(src) + 1;
	LPSTR dst = (LPSTR)malloc(len);
	if (dst) strcpy_s(dst, len, src);
	return dst;
}
// 递归拷贝单个帧（包含子帧/兄弟帧）
LPD3DXFRAME CopyD3DXFrame(LPFRAME pSrcFrame, LPD3DXFRAME pParent = nullptr) {
	// 边界检查：源帧为空则返回空
	if (!pSrcFrame) return nullptr;

	// 1. 分配新帧内存并初始化
	LPD3DXFRAME pDstFrame = NULL;
	CreateFrame(pSrcFrame->Name.c_str(), &pDstFrame);

	// 2. 拷贝基础属性
	//pDstFrame->Name = ConstCharToLPSTR(pSrcFrame->Name);                // 拷贝名称
	pDstFrame->TransformationMatrix = ToD3DXMATRIX(pSrcFrame->TransformationMatrix);        // 拷贝变换矩阵

	// 3. 拷贝网格容器（深度拷贝）
	//pDstFrame->pMeshContainer = CopyMeshContainer(pSrcFrame->pMeshContainer);

	// 4. 递归拷贝子帧（FirstChild）
	if (pSrcFrame->pFrameFirstChild) {
		pDstFrame->pFrameFirstChild = CopyD3DXFrame(pSrcFrame->pFrameFirstChild, pDstFrame);
	}

	// 5. 递归拷贝兄弟帧（Sibling）
	if (pSrcFrame->pFrameSibling) {
		pDstFrame->pFrameSibling = CopyD3DXFrame(pSrcFrame->pFrameSibling, pParent);
	}

	return pDstFrame;
}

LPD3DXFRAME getFrameHierarchy(FBXModel model)
{
	return CopyD3DXFrame(model.m_modelData->Bone);
}
HRESULT GenerateSkinnedMesh(D3DXMESHCONTAINER_DERIVED* pMeshContainer, LPMESH mesh)
{
	HRESULT hr;
	pMeshContainer->pAdjacency = new DWORD[mesh->FaceCount * 3];
	pMeshContainer->pOrigMesh->GenerateAdjacency(0, pMeshContainer->pAdjacency);
	// Blend Mesh 准备工作
	hr = pMeshContainer->pSkinInfo->ConvertToBlendedMesh
	(
		pMeshContainer->pOrigMesh,
		D3DXMESH_MANAGED | D3DXMESHOPT_VERTEXCACHE,
		pMeshContainer->pAdjacency,
		NULL, NULL, NULL,
		&pMeshContainer->NumInfl,
		&pMeshContainer->NumAttributeGroups,
		&pMeshContainer->pBoneCombinationBuf,
		&pMeshContainer->MeshData.pMesh
	);
	// 假设 m_pMesh 是已加载的 LPD3DXMESH 对象
	DWORD numSubsets = 0;
	// 第一步：获取属性表的大小（即子集数量）
	pMeshContainer->pOrigMesh->GetAttributeTable(NULL, &numSubsets);
	// 假设 m_pMesh 是已加载的 LPD3DXMESH 对象
	DWORD numSubsets2 = 0;
	// 第一步：获取属性表的大小（即子集数量）
	pMeshContainer->MeshData.pMesh->GetAttributeTable(NULL, &numSubsets2);
	return S_OK;
}
#define D3DFVF_PNT (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)
HRESULT getD3DXMeshContainer(D3DXMESHCONTAINER_DERIVED* pMeshContainer, LPDIRECT3DDEVICE9 pD3DDevice, FBXModel model, LPMESH mesh)
{
	//D3DXMESHCONTAINER* pFrame = (D3DXMESHCONTAINER*)pMeshContainer;
	if (FAILED(AllocateName(mesh->Name.c_str(), (LPSTR*)&pMeshContainer->Name)))
	{
	}
	pMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;
	HRESULT hr = D3DXCreateMeshFVF(mesh->FaceCount, mesh->VertexCount, D3DXMESH_MANAGED, D3DFVF_PNT, pD3DDevice, &pMeshContainer->pOrigMesh);





	// Vertices
	void* pt;
	//Vertex* pVertices2 = NULL;
	//pMeshContainer->pOrigMesh->LockVertexBuffer(0, (void**)&pVertices2);
	//for (int i = 0; i < mesh->Vertices.size(); i++)
	//{
	//	pVertices2[i] = mesh->Vertices[i];
	//}
	//pMeshContainer->pOrigMesh->UnlockVertexBuffer();

	// Faces
	//WORD* pIndices = NULL;
	//pMeshContainer->pOrigMesh->LockIndexBuffer(0, (void**)&pIndices);
	//for (int i = 0; i < mesh->Indices.size(); i++)
	//{
	//	pIndices[i] = mesh->Indices[i];
	//}
	//pMeshContainer->pOrigMesh->UnlockIndexBuffer();

	//// Vertices
	//void* pt;
	pMeshContainer->pOrigMesh->LockVertexBuffer(0, &pt);
	memcpy(pt, mesh->Vertices.data(), sizeof(Vertex) * mesh->Vertices.size()); // 批量拷贝
	pMeshContainer->pOrigMesh->UnlockVertexBuffer();

	// Faces
	pMeshContainer->pOrigMesh->LockIndexBuffer(0, &pt);
	memcpy(pt, mesh->Indices.data(), sizeof(WORD) * mesh->Indices.size()); // 批量拷贝
	pMeshContainer->pOrigMesh->UnlockIndexBuffer();

	// Attributes
	pMeshContainer->pOrigMesh->LockAttributeBuffer(0, (DWORD**)&pt);
	memset(pt, 0, sizeof(DWORD) * mesh->FaceCount);
	pMeshContainer->pOrigMesh->UnlockAttributeBuffer();


	if (!mesh->MatD3Ds.empty())
	{
		// Materials
		pMeshContainer->NumMaterials = 10;
		//pMeshContainer->ppTextures == new LPDIRECT3DTEXTURE9[pMeshContainer->NumMaterials];
		pMeshContainer->pMaterials = new D3DXMATERIAL[pMeshContainer->NumMaterials];
		pMeshContainer->pMaterials[0].pTextureFilename = NULL;
		memset(&pMeshContainer->pMaterials[0].MatD3D, 0, sizeof(D3DMATERIAL9));
		//memset(pMeshContainer->ppTextures, 0, sizeof(LPDIRECT3DTEXTURE9) * pMeshContainer->NumMaterials);

		Material material = mesh->MatD3Ds[0];
		MATERIALInfo matInfo = material.MatD3D;
		std::string pTexture = material.pTexture;
		pMeshContainer->pMaterials[0].MatD3D.Ambient = D3DXCOLOR(
			matInfo.Ambient.r,
			matInfo.Ambient.g,
			matInfo.Ambient.b,
			matInfo.Ambient.a);
		pMeshContainer->pMaterials[0].MatD3D.Diffuse = D3DXCOLOR(
			matInfo.Diffuse.r,
			matInfo.Diffuse.g,
			matInfo.Diffuse.b,
			matInfo.Diffuse.a);
		pMeshContainer->pMaterials[0].MatD3D.Emissive = D3DXCOLOR(
			matInfo.Emissive.r,
			matInfo.Emissive.g,
			matInfo.Emissive.b,
			matInfo.Emissive.a);
		pMeshContainer->pMaterials[0].MatD3D.Specular = D3DXCOLOR(
			matInfo.Specular.r,
			matInfo.Specular.g,
			matInfo.Specular.b,
			matInfo.Specular.a);
		pMeshContainer->pMaterials[0].MatD3D.Power = matInfo.Power;
		if (!pTexture.empty())
		{
			if (FAILED(D3DXCreateTextureFromFileA(pD3DDevice, "models/Rudolf_deel.dds", &pMeshContainer->ppTextures)))
			{
				pMeshContainer->ppTextures = NULL;
			}
		}
	}



	if (!mesh->Influences.empty())
	{
		int maxBone = model.m_modelData->BoneNameToIndex.size();
		D3DXCreateSkinInfoFVF(mesh->VertexCount, D3DFVF_PNT, maxBone, &pMeshContainer->pSkinInfo);
		for (int i = 0; i < maxBone; i++)
		{
			std::string boneName = model.m_modelData->BoneNameToIndex[i];
			pMeshContainer->pSkinInfo->SetBoneName(i, boneName.c_str());

			if (mesh->Influences.count(boneName))
			{
				Influence influence = mesh->Influences[boneName];
				pMeshContainer->pSkinInfo->SetBoneInfluence(i, influence.count, influence.Vertices.data(), influence.Weights.data());
			}
		}
		GenerateSkinnedMesh(pMeshContainer, mesh);
	}
	return S_OK;
}
LPD3DXANIMATIONCONTROLLER getAnimation(FBXModel model)
{
	if (model.m_modelData->BoneNameToIndex.empty())
	{
		return NULL;
	}
	int maxBone = model.m_modelData->BoneNameToIndex.size();
	int MAX_ANIMATIONSETS = 320;
	int nTrackNum = 1;

	LPAnimationClip  Animation = model.m_modelData->Animation;
	int wNodeNum = Animation->boneKeyFrames.size();


	LPD3DXANIMATIONCONTROLLER		m_pAnimController;
	D3DXCreateAnimationController(maxBone, MAX_ANIMATIONSETS, nTrackNum + 1, 10, &m_pAnimController);
	LPD3DXKEYFRAMEDANIMATIONSET pAnimSet = NULL;
	D3DXCreateKeyframedAnimationSet(Animation->Name.c_str(), 1, D3DXPLAY_LOOP, wNodeNum, 0, NULL, &pAnimSet);

	for (const auto& pair : Animation->boneKeyFrames) {

		std::string name = pair.first;
		std::vector<AnimationKeyFrame> frames = pair.second;
		std::vector<D3DXKEY_QUATERNION> RotationKeys;
		std::vector<D3DXKEY_VECTOR3> TranslateKeys;
		int NumRotKeys = frames.size(), NumPosKeys = frames.size();

		for (int i = 0; i < frames.size(); i++)
		{
			AnimationKeyFrame f = frames[i];
			D3DXKEY_QUATERNION RotationKey;
			RotationKey.Time = f.Time;
			RotationKey.Value.x = f.Rotation.x;
			RotationKey.Value.y = f.Rotation.y;
			RotationKey.Value.z = f.Rotation.z;
			RotationKey.Value.w = f.Rotation.w;

			D3DXKEY_VECTOR3 TranslateKey;
			TranslateKey.Time = f.Time;
			TranslateKey.Value.x = f.Rotation.x;
			TranslateKey.Value.y = f.Rotation.y;
			TranslateKey.Value.z = f.Rotation.z;

			RotationKeys.push_back(RotationKey);
			TranslateKeys.push_back(TranslateKey);
		}

		pAnimSet->RegisterAnimationSRTKeys(name.c_str(), 0, NumRotKeys, NumPosKeys, NULL, RotationKeys.data(), TranslateKeys.data(), NULL);
	}
	m_pAnimController->RegisterAnimationSet(pAnimSet);
	pAnimSet->Release();
	return m_pAnimController;
}
void RegisterMatrix(LPD3DXFRAME pFrame, LPD3DXANIMATIONCONTROLLER pAC)
{
	if (pAC == NULL)
	{
		return;
	}
	//	if(pFrame->bAnimation)
	pAC->RegisterAnimationOutput(pFrame->Name, &pFrame->TransformationMatrix, NULL, NULL, NULL);

	if (pFrame->pFrameSibling)		RegisterMatrix(pFrame->pFrameSibling, pAC);
	if (pFrame->pFrameFirstChild)	RegisterMatrix(pFrame->pFrameFirstChild, pAC);
}
HRESULT WINAPI D3DXLoadMeshHierarchyFromFBX(
	std::string Filename,
	LPDIRECT3DDEVICE9 pD3DDevice,
	LPD3DXFRAME* ppFrameHierarchy,
	std::vector<LPD3DXMESHCONTAINER*>* ppMeshContainer,
	LPD3DXANIMATIONCONTROLLER* ppAnimController)
{
	FBXModel  model = FBXModel();
	model.Load(Filename);

	LPD3DXFRAME f = getFrameHierarchy(model);
	*ppFrameHierarchy = f;
	for (size_t i = 0; i < model.m_modelData->Meshs.size(); i++)
	{
		D3DXMESHCONTAINER_DERIVED* pMeshContainer = new D3DXMESHCONTAINER_DERIVED();
		memset(pMeshContainer, 0, sizeof(D3DXMESHCONTAINER_DERIVED));
		getD3DXMeshContainer(pMeshContainer, pD3DDevice, model, model.m_modelData->Meshs.at(i));
		(*ppMeshContainer).push_back((LPD3DXMESHCONTAINER*)pMeshContainer);
	}

	LPD3DXANIMATIONCONTROLLER p = getAnimation(model);
	*ppAnimController = p;

	RegisterMatrix(f, p);

	return S_OK;
}
//--------------------------------------------------------------------------------------
// Name: DrawMeshContainer()
// Desc: 绘制蒙皮容器中的蒙皮网格
//--------------------------------------------------------------------------------------
void DrawMeshContainer(IDirect3DDevice9* pd3dDevice, LPD3DXMESHCONTAINER pMeshContainerBase, LPD3DXFRAME pFrameBase)
{
	//D3DXMESHCONTAINER_DERIVED* pMeshContainer = (D3DXMESHCONTAINER_DERIVED*)pMeshContainerBase;
	//D3DXFRAME_DERIVED* pFrame = (D3DXFRAME_DERIVED*)pFrameBase;
	//UINT iMaterial;
	//UINT NumBlend;
	//UINT iAttrib;
	//DWORD AttribIdPrev;
	//LPD3DXBONECOMBINATION pBoneComb;

	//UINT iMatrixIndex;
	//D3DXMATRIXA16 matTemp;
	//D3DCAPS9 d3dCaps;
	//pd3dDevice->GetDeviceCaps(&d3dCaps);

	//// first check for skinning
	//if (pMeshContainer->pSkinInfo != NULL)
	//{
	//	AttribIdPrev = UNUSED32;
	//	pBoneComb = reinterpret_cast<LPD3DXBONECOMBINATION>(pMeshContainer->pBoneCombinationBuf->GetBufferPointer());

	//	// Draw using default vtx processing of the device (typically HW)
	//	for (iAttrib = 0; iAttrib < pMeshContainer->NumAttributeGroups; iAttrib++)
	//	{
	//		NumBlend = 0;
	//		for (DWORD i = 0; i < pMeshContainer->NumInfl; ++i)
	//		{
	//			if (pBoneComb[iAttrib].BoneId[i] != UINT_MAX)
	//			{
	//				NumBlend = i;
	//			}
	//		}

	//		if (d3dCaps.MaxVertexBlendMatrices >= NumBlend + 1)
	//		{
	//			// first calculate the world matrices for the current set of blend weights and get the accurate count of the number of blends
	//			for (DWORD i = 0; i < pMeshContainer->NumInfl; ++i)
	//			{
	//				iMatrixIndex = pBoneComb[iAttrib].BoneId[i];
	//				if (iMatrixIndex != UINT_MAX)
	//				{
	//					D3DXMatrixMultiply(&matTemp, &pMeshContainer->pBoneOffsetMatrices[iMatrixIndex],
	//						pMeshContainer->ppBoneMatrixPtrs[iMatrixIndex]);
	//					pd3dDevice->SetTransform(D3DTS_WORLDMATRIX(i), &matTemp);
	//				}
	//			}

	//			pd3dDevice->SetRenderState(D3DRS_VERTEXBLEND, NumBlend);

	//			// lookup the material used for this subset of faces
	//			if ((AttribIdPrev != pBoneComb[iAttrib].AttribId) || (AttribIdPrev == UNUSED32))
	//			{
	//				pd3dDevice->SetMaterial(&pMeshContainer->pMaterials[pBoneComb[iAttrib].AttribId].MatD3D);
	//				pd3dDevice->SetTexture(0, pMeshContainer->ppTextures[pBoneComb[iAttrib].AttribId]);
	//				AttribIdPrev = pBoneComb[iAttrib].AttribId;
	//			}

	//			// draw the subset now that the correct material and matrices are loaded
	//			pMeshContainer->MeshData.pMesh->DrawSubset(iAttrib);
	//		}
	//	}
	//	pd3dDevice->SetRenderState(D3DRS_VERTEXBLEND, 0);
	//}
	//else  // standard mesh, just draw it after setting material properties
	//{
	//	pd3dDevice->SetTransform(D3DTS_WORLD, &pFrame->CombinedTransformationMatrix);

	//	for (iMaterial = 0; iMaterial < pMeshContainer->NumMaterials; iMaterial++)
	//	{
	//		pd3dDevice->SetMaterial(&pMeshContainer->pMaterials[iMaterial].MatD3D);
	//		pd3dDevice->SetTexture(0, pMeshContainer->ppTextures[iMaterial]);
	//		pMeshContainer->MeshData.pMesh->DrawSubset(iMaterial);
	//	}
	//}
}


//--------------------------------------------------------------------------------------
// Name: DrawFrame()
// Desc: 绘制骨骼
//--------------------------------------------------------------------------------------
void DrawFrame(IDirect3DDevice9* pd3dDevice, LPD3DXFRAME pFrame, std::vector<LPD3DXMESHCONTAINER*> ppMeshContainer)
{

	D3DXFRAME_DERIVED* Frame = (D3DXFRAME_DERIVED*)pFrame;

	//pd3dDevice->SetTransform(D3DTS_WORLD, &Frame->CombinedTransformationMatrix);
	// 假设 m_pMesh 是已加载的 LPD3DXMESH 对象
	DWORD numSubsets = 0;

	for (int i = 0; i < ppMeshContainer.size(); i++)
	{
		D3DXMESHCONTAINER_DERIVED* pMeshContainer = (D3DXMESHCONTAINER_DERIVED*)ppMeshContainer[i];

		LPD3DXMESH mesh = pMeshContainer->MeshData.pMesh;
		if (mesh == NULL)
		{
			mesh = pMeshContainer->pOrigMesh;
		}
		// 第一步：获取属性表的大小（即子集数量）
		mesh->GetAttributeTable(NULL, &numSubsets);
		if (pMeshContainer->pMaterials != NULL)
		{
			pd3dDevice->SetMaterial(&pMeshContainer->pMaterials[0].MatD3D);
			pd3dDevice->SetTexture(0, pMeshContainer->ppTextures);
		}
		if (numSubsets == 0)
		{
			mesh->DrawSubset(0);
		}
		else
		{
			for (int i = 0; i < (int)numSubsets; i++)
			{
				mesh->DrawSubset(i);
			}
		}
	}
}
