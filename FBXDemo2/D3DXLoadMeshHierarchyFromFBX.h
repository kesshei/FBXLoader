
#ifndef _D3DXLoadMeshHierarchyFromFBX_h_
#define _D3DXLoadMeshHierarchyFromFBX_h_
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#include <string>
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#include <vector>
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif
//--------------------------------------------------------------------------------------
// Name: AllocateName()
// Desc: 为骨骼或网格名称的字符串分配内存
//--------------------------------------------------------------------------------------
HRESULT AllocateName(LPCSTR Name, LPSTR* pNewName);
//-----------------------------------------------------------------------------
// Name: struct D3DXFRAME_DERIVED
// Desc: 继承自DXDXFRAME结构的结构
//-----------------------------------------------------------------------------
struct D3DXFRAME_DERIVED : public D3DXFRAME
{
    D3DXMATRIXA16 CombinedTransformationMatrix;
};


//-----------------------------------------------------------------------------
// Name: struct D3DXMESHCONTAINER_DERIVED
// Desc: 继承自D3DXMESHCONTAINER结构的结构
//-----------------------------------------------------------------------------
struct D3DXMESHCONTAINER_DERIVED : public D3DXMESHCONTAINER
{
    LPDIRECT3DTEXTURE9   ppTextures;            //纹理数组
    LPD3DXMESH           pOrigMesh;             //原始网格
    LPD3DXATTRIBUTERANGE pAttributeTable;
    DWORD                NumAttributeGroups;    //属性组数量,即子网格数量
    DWORD                NumInfl;               //每个顶点最多受多少骨骼的影响
    LPD3DXBUFFER         pBoneCombinationBuf;   //骨骼结合表
    D3DXMATRIX**         ppBoneMatrixPtrs;      //存放骨骼的组合变换矩阵
    D3DXMATRIX*          pBoneOffsetMatrices;   //存放骨骼的初始变换矩阵
    DWORD                NumPaletteEntries;     //骨骼数量上限
    bool                 UseSoftwareVP;         //标识是否使用软件顶点处理
};


HRESULT WINAPI D3DXLoadMeshHierarchyFromFBX(
    std::string Filename,
    LPDIRECT3DDEVICE9 pD3DDevice,
    LPD3DXFRAME* ppFrameHierarchy,
    std::vector<LPD3DXMESHCONTAINER*>* ppMeshContainer,
    LPD3DXANIMATIONCONTROLLER* ppAnimController);

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::CreateFrame()
// Desc: 创建框架, 仅仅是分配内存和初始化,还没有对其成员赋予合适的值
//--------------------------------------------------------------------------------------
HRESULT CreateFrame(LPCSTR Name, LPD3DXFRAME* ppNewFrame);
//--------------------------------------------------------------------------------------
// Name: GenerateSkinnedMesh
// Desc: 生成蒙皮网格模型。该函数判断当前网格容器是否包含有蒙皮信息，如果当前网格模型
//       中不包含蒙皮信息，则直接退出该函数。接下来确定所需要的矩阵调色板容量。最后调
//       用ID3DXSkinInfo::ConvertToIndexedBlendedMesh()函数生成索引蒙皮网格模型
//--------------------------------------------------------------------------------------
HRESULT GenerateSkinnedMesh(IDirect3DDevice9* pd3dDevice, D3DXMESHCONTAINER_DERIVED* pMeshContainer);

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
    LPD3DXMESHCONTAINER* ppNewMeshContainer);

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::DestroyFrame()
// Desc: 释放骨骼框架
//--------------------------------------------------------------------------------------
HRESULT DestroyFrame(LPD3DXFRAME pFrameToFree);

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::DestroyMeshContainer()
// Desc: 释放网格容器
//--------------------------------------------------------------------------------------
HRESULT DestroyMeshContainer(LPD3DXMESHCONTAINER pMeshContainerBase);
//--------------------------------------------------------------------------------------
// Name: DrawMeshContainer()
// Desc: 绘制蒙皮容器中的蒙皮网格
//--------------------------------------------------------------------------------------
void DrawMeshContainer(IDirect3DDevice9* pd3dDevice, LPD3DXMESHCONTAINER pMeshContainerBase, LPD3DXFRAME pFrameBase);


//--------------------------------------------------------------------------------------
// Name: DrawFrame()
// Desc: 绘制骨骼
//--------------------------------------------------------------------------------------
void DrawFrame(IDirect3DDevice9* pd3dDevice, LPD3DXFRAME pFrame, std::vector<LPD3DXMESHCONTAINER*> ppMeshContainer);
//--------------------------------------------------------------------------------------
// Name: SetupBoneMatrixPointers()
// Desc: 设置好各级框架的组合变换矩阵。
//--------------------------------------------------------------------------------------
HRESULT SetupBoneMatrixPointers(LPD3DXFRAME pFrameBase, LPD3DXFRAME pFrameRoot, D3DXMESHCONTAINER_DERIVED* MeshContainer);
//--------------------------------------------------------------------------------------
// Name: UpdateFrameMatrics()
// Desc: 更新框架中的变换矩阵
//--------------------------------------------------------------------------------------
void UpdateFrameMatrices(LPD3DXFRAME pFrameBase, LPD3DXMATRIX pParentMatrix);
#endif