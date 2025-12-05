#include "Camera.h"
#include <fstream>

std::fstream g_dump;

CCamera::CCamera(IDirect3DDevice9 *pd3dDevice)
{
    m_pd3dDevice = pd3dDevice;
    m_vRightVec  = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    m_vUpVec     = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    m_vLookVec   = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    m_vPosition  = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_vLookat    = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    GetViewMatrix(&m_matView);
    D3DXMatrixPerspectiveFovLH(&m_matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 2000.0f);

    g_dump.open("dump.txt");
}

CCamera::~CCamera(void)
{
    g_dump.close();
}


VOID CCamera::GetViewMatrix(D3DXMATRIX *pMatrix) 
{
    // 使各分量相互垂直
    D3DXVec3Normalize(&m_vLookVec, &m_vLookVec);

    D3DXVec3Cross(&m_vUpVec, &m_vLookVec, &m_vRightVec);
    D3DXVec3Normalize(&m_vUpVec, &m_vUpVec);

    D3DXVec3Cross(&m_vRightVec, &m_vUpVec, &m_vLookVec);
    D3DXVec3Normalize(&m_vRightVec, &m_vRightVec);

    // 创建取景变换矩阵
    pMatrix->_11 = m_vRightVec.x;
    pMatrix->_12 = m_vUpVec.x;
    pMatrix->_13 = m_vLookVec.x;
    pMatrix->_14 = 0.0f;

    pMatrix->_21 = m_vRightVec.y;
    pMatrix->_22 = m_vUpVec.y;
    pMatrix->_23 = m_vLookVec.y;
    pMatrix->_24 = 0.0f;

    pMatrix->_31 = m_vRightVec.z;
    pMatrix->_32 = m_vUpVec.z;
    pMatrix->_33 = m_vLookVec.z;
    pMatrix->_34 = 0.0f;

    pMatrix->_41 = -D3DXVec3Dot(&m_vRightVec, &m_vPosition);
    pMatrix->_42 = -D3DXVec3Dot(&m_vUpVec, &m_vPosition);
    pMatrix->_43 = -D3DXVec3Dot(&m_vLookVec, &m_vPosition);
    pMatrix->_44 = 1.0f;
}

VOID CCamera::ResetLookatPos(D3DXVECTOR3 *pLookat) 
{
    if (pLookat != NULL)  m_vLookat = (*pLookat);
    else m_vLookat = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

    m_vLookVec = m_vLookat - m_vPosition;
    D3DXVec3Normalize(&m_vLookVec, &m_vLookVec);

    g_dump<<"reset: x="<<m_vLookVec.x<<", y="<<m_vLookVec.y<<", z="<<m_vLookVec.z<<std::endl;

    D3DXVec3Cross(&m_vUpVec, &m_vLookVec, &m_vRightVec);
    D3DXVec3Normalize(&m_vUpVec, &m_vUpVec);

    D3DXVec3Cross(&m_vRightVec, &m_vUpVec, &m_vLookVec);
    D3DXVec3Normalize(&m_vRightVec, &m_vRightVec);
}

VOID CCamera::ResetCameraPos(D3DXVECTOR3 *pVector) 
{
    D3DXVECTOR3 V = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_vPosition = pVector ? (*pVector) : V;
}

VOID CCamera::ResetViewMatrix(D3DXMATRIX *pMatrix) 
{
    if (pMatrix) m_matView = *pMatrix;
    else GetViewMatrix(&m_matView);
    m_pd3dDevice->SetTransform(D3DTS_VIEW, &m_matView);
    
    m_vRightVec = D3DXVECTOR3(m_matView._11, m_matView._12, m_matView._13);
    m_vUpVec    = D3DXVECTOR3(m_matView._21, m_matView._22, m_matView._23);
    m_vLookVec  = D3DXVECTOR3(m_matView._31, m_matView._32, m_matView._33);
}

VOID CCamera::ResetProjMatrix(D3DXMATRIX *pMatrix) 
{
    if (pMatrix != NULL) m_matProj = *pMatrix;
    else D3DXMatrixPerspectiveFovLH(&m_matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 1000.0f);
    m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m_matProj);
}

// 沿右向量平移fUnits个单位
VOID CCamera::MoveAlongRightVec(FLOAT fUnits) 
{
    m_vPosition += m_vRightVec * fUnits;
    m_vLookat   += m_vRightVec * fUnits;
}

// 沿上向量平移fUnits个单位
VOID CCamera::MoveAlongUpVec(FLOAT fUnits) 
{
    m_vPosition += m_vUpVec * fUnits;
    m_vLookat   += m_vUpVec * fUnits;
}

// 沿观察向量平移fUnits个单位
VOID CCamera::MoveAlongLookVec(FLOAT fUnits) 
{
    m_vPosition += m_vLookVec * fUnits;
    m_vLookat   += m_vLookVec * fUnits;
}

VOID CCamera::RotationRightVec(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vRightVec, fAngle);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vPosition, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    m_vLookat = m_vLookVec * D3DXVec3Length(&m_vPosition);
}

VOID CCamera::RotationUpVec(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vUpVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    m_vLookat = m_vLookVec * D3DXVec3Length(&m_vPosition);

    D3DXVECTOR3 vNormal;
    D3DXVec3Normalize(&vNormal, &m_vLookat);

    g_dump<<"rotate: x="<<m_vLookVec.x<<", y="<<m_vLookVec.y<<", z="<<m_vLookVec.z<<std::endl;
    g_dump<<"normal: x="<<vNormal.x<<", y="<<vNormal.y<<", z="<<vNormal.z<<std::endl;
}

VOID CCamera::RotationLookVec(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vLookVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vUpVec, &R);

    m_vLookat = m_vLookVec * D3DXVec3Length(&m_vPosition);
}

VOID CCamera::CircleRotationX(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vRightVec, fAngle);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vUpVec, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    float dy = m_vPosition.y - m_vLookat.y;
    float dz = m_vPosition.z - m_vLookat.z;
    m_vPosition.y = m_vLookat.y + dy * cosf(fAngle) - dz * sinf(fAngle);
    m_vPosition.z = m_vLookat.z + dy * sinf(fAngle) + dz * cosf(fAngle);

    ResetLookatPos(&m_vLookat);
}

VOID CCamera::CircleRotationY(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vUpVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    float dx = m_vPosition.x - m_vLookat.x;
    float dz = m_vPosition.z - m_vLookat.z;
    m_vPosition.x = m_vLookat.x + dx * cosf(fAngle) - dz * sinf(fAngle);
    m_vPosition.z = m_vLookat.z + dx * sinf(fAngle) + dz * cosf(fAngle);

    ResetLookatPos(&m_vLookat);
}

VOID CCamera::CircleRotationZ(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vLookVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vUpVec, &R);

    float dx = m_vPosition.x - m_vLookat.x;
    float dy = m_vPosition.y - m_vLookat.y;
    m_vPosition.x = m_vLookat.x + dx * cosf(fAngle) + dy * sinf(fAngle);
    m_vPosition.y = m_vLookat.y - dx * sinf(fAngle) + dy * cosf(fAngle);

    ResetLookatPos(&m_vLookat);
}
