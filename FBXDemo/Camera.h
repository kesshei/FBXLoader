#pragma once

#include <d3d9.h>
#include <d3dx9.h>

//--------------------------------------------------------------------------------------
// Name: class CCamera
// Desc: 虚拟摄像机平移、旋转
//--------------------------------------------------------------------------------------
class CCamera
{
private:
    D3DXVECTOR3         m_vRightVec;        // 右分量
    D3DXVECTOR3         m_vUpVec;           // 上分量
    D3DXVECTOR3         m_vLookVec;         // 观察方向
    D3DXVECTOR3         m_vPosition;        // 位置
    D3DXMATRIX          m_matView;          // 取景变换矩阵
    D3DXMATRIX          m_matProj;          // 投影变换矩阵
    D3DXVECTOR3         m_vLookat;
    LPDIRECT3DDEVICE9   m_pd3dDevice;

public:
    CCamera(IDirect3DDevice9 *pd3dDevice);
    virtual ~CCamera(void);

public:
    VOID GetViewMatrix(D3DXMATRIX *pMatrix);
    VOID GetProjMatrix(D3DXMATRIX *pMatrix)  { *pMatrix = m_matProj; }
    VOID GetCameraPos(D3DXVECTOR3 *pVector)  { *pVector = m_vPosition; }
    VOID GetLookVector(D3DXVECTOR3 *pVector) { *pVector = m_vLookVec; }

    VOID ResetLookatPos(D3DXVECTOR3 *pLookat = NULL);
    VOID ResetCameraPos(D3DXVECTOR3 *pVector = NULL);
    VOID ResetViewMatrix(D3DXMATRIX *pMatrix = NULL);
    VOID ResetProjMatrix(D3DXMATRIX *pMatrix = NULL);

public:
    // 沿各分量平移
    VOID MoveAlongRightVec(FLOAT fUnits);   // 沿right向量移动
    VOID MoveAlongUpVec(FLOAT fUnits);      // 沿up向量移动
    VOID MoveAlongLookVec(FLOAT fUnits);    // 沿look向量移动

    // 绕各分量旋转
    VOID RotationRightVec(FLOAT fAngle);    // 绕right向量选择
    VOID RotationUpVec(FLOAT fAngle);       // 绕up向量旋转
    VOID RotationLookVec(FLOAT fAngle);     // 绕look向量旋转

    // 绕空间点旋转
    VOID CircleRotationX(FLOAT fAngle);     // 在X方向上绕观察点旋转
    VOID CircleRotationY(FLOAT fAngle);     // 在Y方向上绕观察点旋转
    VOID CircleRotationZ(FLOAT fAngle);     // 在Z方向上绕观察点旋转
};
