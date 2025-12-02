#include <vector>
#ifndef _ModelData_h_
#define _ModelData_h_

typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;

// 手动实现 XMATRIX（4x4 行优先矩阵）
struct XMATRIX {
    // 矩阵元素：_11(第1行第1列) ~ _44(第4行第4列)
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;

    // 构造函数：默认初始化单位矩阵
    XMATRIX() {
        _11 = 1.0f; _12 = 0.0f; _13 = 0.0f; _14 = 0.0f;
        _21 = 0.0f; _22 = 1.0f; _23 = 0.0f; _24 = 0.0f;
        _31 = 0.0f; _32 = 0.0f; _33 = 1.0f; _34 = 0.0f;
        _41 = 0.0f; _42 = 0.0f; _43 = 0.0f; _44 = 1.0f;
    }

    // 带参数构造：直接赋值 16 个元素
    XMATRIX(
        float m11, float m12, float m13, float m14,
        float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34,
        float m41, float m42, float m43, float m44
    ) :
        _11(m11), _12(m12), _13(m13), _14(m14),
        _21(m21), _22(m22), _23(m23), _24(m24),
        _31(m31), _32(m32), _33(m33), _34(m34),
        _41(m41), _42(m42), _43(m43), _44(m44) {
    }
};
typedef struct _COLORVALUE {
    float r;
    float g;
    float b;
    float a;
} COLORVALUE;

typedef struct VECTOR {
    float x;
    float y;
    float z;
} VECTOR;

typedef struct MATERIAL {
    COLORVALUE   Diffuse;        /* Diffuse color RGBA */
    COLORVALUE   Ambient;        /* Ambient color RGB */
    COLORVALUE   Specular;       /* Specular 'shininess' */
    COLORVALUE   Emissive;       /* Emissive color RGB */
    float           Power;          /* Sharpness if specular highlight */
} MATERIAL;

typedef struct _Material
{
    MATERIAL     MatD3D;
    const char * pTexture;
    int		     TextureIndex1;
}Material;

typedef struct _Vertex
{
    float x, y, z;
    float u, v;
    float nx, ny, nz;
}Vertex;

typedef struct _MESH
{
    std::vector<Vertex> Vertices;
    std::vector<DWORD> Indices;
    std::vector<DWORD> Attributes;
}MESH, * LPMESH;

typedef struct _FRAME
{
    const char*       Name;
    XMATRIX		      TransformationMatrix;
    LPMESHCONTAINER	  pMeshContainer;
    struct _ALSFRAME* pFrameSibling;
    struct _ALSFRAME* pFrameFirstChild;
    XMATRIX	          ParentTM;
    XMATRIX	          NodeTMInverse;
    int               BoneIndex;
    Material	      pMaterial;
    WORD              MaterialID;
}FRAME, * LPFRAME;


typedef struct _MESHCONTAINER
{
    const char*             Name;
    MESH				    pOrigMesh;
}MESHCONTAINER, * LPMESHCONTAINER;

#endif //_ModelData_h_