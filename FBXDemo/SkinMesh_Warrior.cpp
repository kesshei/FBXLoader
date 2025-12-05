//--------------------------------------------------------------------------------------
// File: SkinMesh_Warrior.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件
#include "Camera.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#include "D3DXLoadMeshHierarchyFromFBX.h"
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

// 地板的顶点结构
struct CUSTOMVERTEX
{
	FLOAT _x, _y, _z;
	FLOAT _u, _v;
	CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT v)
		: _x(x), _y(y), _z(z), _u(u), _v(v) {
	}
};
#define D3DFVF_CUSTOMVERTEX  (D3DFVF_XYZ | D3DFVF_TEX1)


LPD3DXFRAME	                g_pFrameRoot = NULL;
D3DXMATRIX* g_pBoneMatrices = NULL;
LPD3DXANIMATIONCONTROLLER	g_pAnimController = NULL;
LPD3DXMESHCONTAINER 	   g_pMeshContainer = NULL;

CCamera* g_pCamera = NULL;
LPDIRECT3DVERTEXBUFFER9     g_pFloorVBuf = NULL;
LPDIRECT3DTEXTURE9          g_pFloorTexture = NULL;
LPDIRECT3DDEVICE9           g_pd3dDevice = NULL;    // Direct3D设备接口

HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta);
VOID Direct3DCleanup();

// 窗口消息处理函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//--------------------------------------------------------------------------------------
// Name: WinMain();
// Desc: Windows应用程序入口函数
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	// 初始化窗口类
	WNDCLASS wndclass;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  // 窗口背景
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);          // 光标形状
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);      // 窗口小图标
	wndclass.hInstance = hInstance;
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = L"SkinMesh_Warrior";
	wndclass.lpszMenuName = NULL;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;

	// 注册窗口类
	if (!RegisterClass(&wndclass))
		return 0;

	// 创建窗口
	HWND hWnd = CreateWindow(L"SkinMesh_Warrior", L"骨骼动画示例", WS_OVERLAPPEDWINDOW,
		100, 100, 640, 480, NULL, NULL, wndclass.hInstance, NULL);

	// 初始化Direct3D
	InitDirect3D(hWnd, hInstance);

	// 显示、更新窗口
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// 消息循环
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		static FLOAT fLastTime = (float)::timeGetTime();
		static FLOAT fCurrTime = (float)::timeGetTime();
		static FLOAT fTimeDelta = 0.0f;
		fCurrTime = (float)::timeGetTime();
		fTimeDelta = (fCurrTime - fLastTime) / 1000.0f;
		fLastTime = fCurrTime;

		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Direct3DRender(hWnd, fTimeDelta);       // 绘制3D场景
		}
	}

	UnregisterClass(L"SkinMesh_Warrior", wndclass.hInstance);
	return 0;
}

//--------------------------------------------------------------------------------------
// Name: WndProc()
// Desc: 窗口消息处理函数
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:                  // 客户区重绘消息
		//Direct3DRender(hWnd, 0.0f); // 渲染图形
		ValidateRect(hWnd, NULL);   // 更新客户区的显示
		break;
	case WM_KEYDOWN:                // 键盘按下消息
		if (wParam == VK_ESCAPE)    // ESC键
			DestroyWindow(hWnd);    // 销毁窗口, 并发送一条WM_DESTROY消息
		break;
	case WM_DESTROY:                // 窗口销毁消息
		Direct3DCleanup();          // 清理Direct3D
		PostQuitMessage(0);         // 退出程序
		break;
	}
	// 默认的消息处理
	return DefWindowProc(hWnd, message, wParam, lParam);
}

//--------------------------------------------------------------------------------------
// Name: InitDirect3D()
// Desc: 初始化Direct3D
//--------------------------------------------------------------------------------------
HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance)
{
	// 创建IDirect3D接口
	LPDIRECT3D9 pD3D = NULL;                    // IDirect3D9接口
	pD3D = Direct3DCreate9(D3D_SDK_VERSION);    // 创建IDirect3D9接口对象
	if (pD3D == NULL) return E_FAIL;

	// 获取硬件设备信息
	D3DCAPS9 caps; int vp = 0;
	pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// 创建Direct3D设备接口
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferWidth = 640;
	d3dpp.BackBufferHeight = 480;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Windowed = true;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		hWnd, vp, &d3dpp, &g_pd3dDevice);
	pD3D->Release();

	// 创建骨骼动画
	D3DXLoadMeshHierarchyFromFBX("models/pet.fbx", g_pd3dDevice, &g_pFrameRoot,&g_pMeshContainer, &g_pAnimController);
	//g_pAllocateHier = new CAllocateHierarchy();
	//D3DXLoadMeshHierarchyFromX(L"warrior.x", D3DXMESH_MANAGED, g_pd3dDevice,
	//	g_pAllocateHier, NULL, &g_pFrameRoot, &g_pAnimController);
	//SetupBoneMatrixPointers(g_pFrameRoot, g_pFrameRoot);

	// 创建地面
	g_pd3dDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), 0,
		D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pFloorVBuf, NULL);

	CUSTOMVERTEX* pVertices = NULL;
	g_pFloorVBuf->Lock(0, 0, (void**)&pVertices, 0);
	pVertices[0] = CUSTOMVERTEX(-1000.0f, -150.0f, -1000.0f, 0.0f, 10.0f);
	pVertices[1] = CUSTOMVERTEX(-1000.0f, -150.0f, 1000.0f, 0.0f, 0.0f);
	pVertices[2] = CUSTOMVERTEX(1000.0f, -150.0f, -1000.0f, 10.0f, 10.0f);
	pVertices[3] = CUSTOMVERTEX(1000.0f, -150.0f, 1000.0f, 10.0f, 0.0f);
	g_pFloorVBuf->Unlock();

	//创建地面纹理
	D3DXCreateTextureFromFile(g_pd3dDevice, L"res/Floor.jpg", &g_pFloorTexture);
	g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	//设置光照
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
	g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DXCOLOR(0.4f, 0.4f, 0.4f, 1.0f));

	// 设置虚拟摄像机
	g_pCamera = new CCamera(g_pd3dDevice);
	D3DXVECTOR3 dd = D3DXVECTOR3(0.0f, 30.0f, -1000.0f);
	g_pCamera->ResetCameraPos(&dd);
	g_pCamera->ResetViewMatrix();

	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 2000.0f);
	g_pCamera->ResetProjMatrix(&matProj);


	//LPD3DXANIMATIONSET pAnimationSet = NULL;
	//g_pAnimController->GetAnimationSetByName("stay", &pAnimationSet);
	//g_pAnimController->SetTrackAnimationSet(0, pAnimationSet);
	//SAFE_RELEASE(pAnimationSet);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: 绘制3D场景
//--------------------------------------------------------------------------------------
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta)
{
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	g_pd3dDevice->BeginScene();                     // 开始绘制

	// 控制动作
	static float fCurrTime = 0.0f;
	static FLOAT fMoveSpeed = 0.0f; // walk: 0.8f, run: 1.2f
	fCurrTime += fTimeDelta;

	LPD3DXANIMATIONSET pAnimationSet = NULL;
	/*if (::GetAsyncKeyState(0x31) & 0x8000f)
	{
		fMoveSpeed = 0.0f;
		g_pAnimController->GetAnimationSetByName("stay", &pAnimationSet);
		SmoothChangeAnimation(g_pAnimController, pAnimationSet, fCurrTime);
	}
	if (::GetAsyncKeyState(0x32) & 0x8000f)
	{
		fMoveSpeed = 0.8f;
		g_pAnimController->GetAnimationSetByName("walk", &pAnimationSet);
		SmoothChangeAnimation(g_pAnimController, pAnimationSet, fCurrTime);
	}
	if (::GetAsyncKeyState(0x33) & 0x8000f)
	{
		fMoveSpeed = 0.0f;
		g_pAnimController->GetAnimationSetByName("attack", &pAnimationSet);
		SmoothChangeAnimation(g_pAnimController, pAnimationSet, fCurrTime);
	}
	if (::GetAsyncKeyState(0x34) & 0x8000f)
	{
		fMoveSpeed = 0.0f;
		g_pAnimController->GetAnimationSetByName("behit", &pAnimationSet);
		SmoothChangeAnimation(g_pAnimController, pAnimationSet, fCurrTime);
	}
	if (::GetAsyncKeyState(0x35) & 0x8000f)
	{
		fMoveSpeed = 0.0f;
		g_pAnimController->GetAnimationSetByName("dieing", &pAnimationSet);
		SmoothChangeAnimation(g_pAnimController, pAnimationSet, fCurrTime);
	}
	if (::GetAsyncKeyState(0x36) & 0x8000f)
	{
		fMoveSpeed = 0.0f;
		g_pAnimController->GetAnimationSetByName("dead", &pAnimationSet);
		SmoothChangeAnimation(g_pAnimController, pAnimationSet, fCurrTime);
	}*/
	SAFE_RELEASE(pAnimationSet);

	// 控制运动方向
	static FLOAT fMoveAngle = 0.0f;
	static D3DXVECTOR3 vRolePos = D3DXVECTOR3(0.0f, -140.0f, 0.0f);
	if (::GetAsyncKeyState('A') & 0x8000f) fMoveAngle += 0.01f;
	if (::GetAsyncKeyState('D') & 0x8000f) fMoveAngle -= 0.01f;

	// 设置世界变换矩阵
	D3DXMATRIX matRole, Rt;
	D3DXMatrixIdentity(&matRole);
	D3DXMatrixRotationY(&Rt, -fMoveAngle);

	vRolePos.x -= fMoveSpeed * sinf(-fMoveAngle);
	vRolePos.z -= fMoveSpeed * cosf(-fMoveAngle);
	D3DXMatrixTranslation(&matRole, vRolePos.x, vRolePos.y, vRolePos.z);
	matRole = Rt * matRole;

	// 重新设置摄像机的位置
	D3DXVECTOR3 vCameraPos;
	g_pCamera->GetCameraPos(&vCameraPos);
	vCameraPos.x = vRolePos.x;
	vCameraPos.y = vRolePos.y + 120.0f;
	vCameraPos.z = vRolePos.z - 500.0f;
	g_pCamera->ResetCameraPos(&vCameraPos);

	// 重新设置取景变换矩阵
	D3DXMATRIX matView;
	g_pCamera->GetViewMatrix(&matView);
	g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

	// 更新骨骼动画
	//g_pd3dDevice->SetTransform(D3DTS_WORLD, &matRole);
	//g_pAnimController->AdvanceTime(fTimeDelta, NULL);
	//UpdateFrameMatrices(g_pFrameRoot, &matRole);

	//// 绘制蒙皮网格
	//DrawFrame(g_pd3dDevice, g_pFrameRoot);

	// 绘制地板
	D3DXMATRIX matFloor;
	D3DXMatrixTranslation(&matFloor, 0.0f, 0.0f, 0.0f);
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matFloor);

	g_pd3dDevice->SetStreamSource(0, g_pFloorVBuf, 0, sizeof(CUSTOMVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
	g_pd3dDevice->SetTexture(0, g_pFloorTexture);
	g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	g_pd3dDevice->EndScene();                       // 结束绘制
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup()
{
	//D3DXFrameDestroy(g_pFrameRoot, g_pAllocateHier);
	SAFE_RELEASE(g_pFloorVBuf);
	SAFE_RELEASE(g_pFloorTexture);
	SAFE_RELEASE(g_pAnimController);
	//SAFE_DELETE(g_pAllocateHier);
	SAFE_RELEASE(g_pd3dDevice);
}
