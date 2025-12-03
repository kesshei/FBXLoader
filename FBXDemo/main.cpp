#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "FBXModel.h"

//// 窗口类名和标题
//const wchar_t CLASS_NAME[] = L"DirectX9 Window Class";
//const wchar_t WINDOW_TITLE[] = L"DirectX9 简单示例";
//
//// 全局变量
//HWND hWnd;                  // 窗口句柄
//LPDIRECT3D9 d3d;            // Direct3D 对象
//LPDIRECT3DDEVICE9 d3dDevice;// Direct3D 设备
//
//// 三角形顶点结构（位置 + 颜色）
//struct CouseVertex {
//    float x, y, z;          // 位置
//    DWORD color;            // 颜色（ARGB）
//};
//#define D3DFVF_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE) // 顶点格式
//
//// 窗口过程（消息处理）
//LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
//    switch (uMsg) {
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        return 0;
//    case WM_PAINT: {
//        PAINTSTRUCT ps;
//        HDC hdc = BeginPaint(hWnd, &ps);
//        EndPaint(hWnd, &ps);
//    }
//                 return 0;
//    }
//    return DefWindowProc(hWnd, uMsg, wParam, lParam);
//}
//
//// 初始化窗口
//bool InitWindow(HINSTANCE hInstance, int nCmdShow) {
//    // 注册窗口类
//    WNDCLASS wc = {};
//    wc.lpfnWndProc = WindowProc;
//    wc.hInstance = hInstance;
//    wc.lpszClassName = CLASS_NAME;
//    wc.style = CS_HREDRAW | CS_VREDRAW;
//
//    if (!RegisterClass(&wc)) return false;
//
//    // 创建窗口
//    hWnd = CreateWindowEx(
//        0,
//        CLASS_NAME,
//        WINDOW_TITLE,
//        WS_OVERLAPPEDWINDOW,
//        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
//        NULL, NULL, hInstance, NULL
//    );
//
//    if (hWnd == NULL) return false;
//
//    ShowWindow(hWnd, nCmdShow);
//    UpdateWindow(hWnd);
//    return true;
//}
//
//// 初始化 Direct3D
//bool InitD3D() {
//    // 创建 Direct3D 对象
//    d3d = Direct3DCreate9(D3D_SDK_VERSION);
//    if (!d3d) return false;
//
//    // 获取窗口客户区大小
//    RECT rc;
//    GetClientRect(hWnd, &rc);
//    int width = rc.right - rc.left;
//    int height = rc.bottom - rc.top;
//
//    // 设备参数设置
//    D3DPRESENT_PARAMETERS d3dpp = {};
//    d3dpp.Windowed = TRUE;                  // 窗口模式
//    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; // 交换链模式
//    d3dpp.hDeviceWindow = hWnd;             // 渲染窗口
//    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8; // 后缓冲区格式
//    d3dpp.BackBufferCount = 1;              // 后缓冲区数量
//    d3dpp.BackBufferWidth = width;          // 宽度
//    d3dpp.BackBufferHeight = height;        // 高度
//    d3dpp.EnableAutoDepthStencil = TRUE;    // 启用深度测试
//    d3dpp.AutoDepthStencilFormat = D3DFMT_D16; // 深度缓冲区格式
//
//    // 创建 Direct3D 设备
//    HRESULT hr = d3d->CreateDevice(
//        D3DADAPTER_DEFAULT,                // 默认显卡
//        D3DDEVTYPE_HAL,                    // 硬件加速
//        hWnd,
//        D3DCREATE_HARDWARE_VERTEXPROCESSING, // 硬件顶点处理
//        &d3dpp,
//        &d3dDevice
//    );
//    if (FAILED(hr)) return false;
//
//    // 设置渲染状态（关闭光照，使用顶点颜色）
//    d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
//    return true;
//}
//
//// 渲染函数
//void Render() {
//    if (!d3dDevice) return;
//
//    // 清屏（黑色背景）
//    d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
//
//    // 开始渲染
//    if (SUCCEEDED(d3dDevice->BeginScene())) {
//        // 三角形顶点数据（位置和颜色）
//        CouseVertex vertices[] = {
//            { 0.0f,  0.5f, 0.0f, D3DCOLOR_XRGB(255, 0, 0) },  // 上顶点（红色）
//            { 0.5f, -0.5f, 0.0f, D3DCOLOR_XRGB(0, 255, 0) },  // 右下（绿色）
//            { -0.5f, -0.5f, 0.0f, D3DCOLOR_XRGB(0, 0, 255) }  // 左下（蓝色）
//        };
//
//        // 设置顶点格式和数据
//        d3dDevice->SetFVF(D3DFVF_VERTEX);
//        d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(CouseVertex));
//
//        // 结束渲染
//        d3dDevice->EndScene();
//    }
//
//    // 交换缓冲区（显示渲染结果）
//    d3dDevice->Present(NULL, NULL, NULL, NULL);
//}
//
//// 释放资源
//void Cleanup() {
//    if (d3dDevice) d3dDevice->Release();
//    if (d3d) d3d->Release();
//}

// 程序入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char* lFilename = "models/pet.fbx";
    FBXModel  model = FBXModel();
    model.Load(lFilename);
    //// 初始化窗口
    //if (!InitWindow(hInstance, nCmdShow)) return 0;

    //// 初始化 Direct3D
    //if (!InitD3D()) {
    //    MessageBox(hWnd, L"Direct3D 初始化失败", L"错误", MB_OK);
    //    return 0;
    //}

    //// 消息循环
    //MSG msg = {};
    //while (msg.message != WM_QUIT) {
    //    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    }
    //    else {
    //        // 渲染
    //        Render();
    //    }
    //}

    //// 释放资源
    //Cleanup();
    return 0;
}
