#include <vector>
#ifndef _ModelData_h_
#define _ModelData_h_

typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;

// 手动实现 XMATRIX（4x4 行优先矩阵）
struct MATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };

    // 构造函数：默认初始化单位矩阵
    MATRIX() {
        _11 = 1.0f; _12 = 0.0f; _13 = 0.0f; _14 = 0.0f;
        _21 = 0.0f; _22 = 1.0f; _23 = 0.0f; _24 = 0.0f;
        _31 = 0.0f; _32 = 0.0f; _33 = 1.0f; _34 = 0.0f;
        _41 = 0.0f; _42 = 0.0f; _43 = 0.0f; _44 = 1.0f;
    }

    // 带参数构造：直接赋值 16 个元素
    MATRIX(
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
    // 核心：重载 operator()，支持 matrix(row, col) 读写访问
   // 参数：row = 行索引（0-3），col = 列索引（0-3）
    float& operator()(int row, int col) {
        // 边界检查（可选，避免越界访问）
        if (row < 0 || row >= 4 || col < 0 || col >= 4) {
            throw std::out_of_range("MATRIX 索引越界！row 和 col 必须在 0-3 之间");
        }
        return m[row][col]; // 行优先：m[行][列] 对应 _(row+1)(col+1)
    }

    // 重载 const 版本（用于 const MATRIX 对象的只读访问）
    const float& operator()(int row, int col) const {
        if (row < 0 || row >= 4 || col < 0 || col >= 4) {
            throw std::out_of_range("MATRIX 索引越界！row 和 col 必须在 0-3 之间");
        }
        return m[row][col];
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

typedef struct _MESHCONTAINER
{
    const char* Name;
    MESH				    pOrigMesh;
}MESHCONTAINER, * LPMESHCONTAINER;

typedef struct _FRAME
{
    const char*       Name;
    MATRIX		      TransformationMatrix;
    LPMESHCONTAINER	  pMeshContainer;
    struct _FRAME*    pFrameSibling;//兄弟节点指向下一个
    struct _FRAME*    pFrameFirstChild;//指向第一个子节点
    MATRIX	          ParentTM;
    MATRIX	          NodeTMInverse;
    int               BoneIndex;
    Material	      pMaterial;
    WORD              MaterialID;
}FRAME, * LPFRAME;

#endif //_ModelData_h_