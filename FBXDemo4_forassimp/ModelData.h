#include <vector>
#include <map>
#ifndef _ModelData_h_
#define _ModelData_h_
// 手动定义 D3DX_PI（值与官方定义一致：3.14159265358979323846f）
#ifndef PI
#define PI 3.14159265358979323846f
#endif

typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef DWORD* PDWORD;

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
		return m[row][col]; // 行优先：m[行][列] 对应 _(row+1)(col+1)
	}

	// 重载 const 版本（用于 const MATRIX 对象的只读访问）
	const float& operator()(int row, int col) const {
		return m[row][col];
	}
};

typedef struct _VECTOR2 {
	// VECTOR2 默认构造（可选，初始化为零向量）
	_VECTOR2() : x(0.0f), y(0.0f) {}
	// 带参数构造（方便赋值）
	_VECTOR2(FLOAT _x, FLOAT _y) : x(_x), y(_y) {}
	float x;
	float y;
} VECTOR2;

typedef struct _VECTOR3 {
	// VECTOR3 默认构造（可选，初始化为零向量）
	_VECTOR3() : x(0.0f), y(0.0f), z(0.0f) {}
	// 带参数构造（方便赋值）
	_VECTOR3(FLOAT _x, FLOAT _y, FLOAT _z) : x(_x), y(_y), z(_z) {}
	float x;
	float y;
	float z;
} VECTOR3;

typedef struct _VECTOR4 {
	// VECTOR4 默认构造（可选，初始化为零向量）
	_VECTOR4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	// 带参数构造（方便赋值）
	_VECTOR4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w) : x(_x), y(_y), z(_z), w(_w) {}
	float x;
	float y;
	float z;
	float w;
} VECTOR4;

typedef struct _MATERIALProperties {
	VECTOR4   Ambient;        /* Ambient color RGB */
	VECTOR4   Diffuse;        /* Diffuse color RGBA */
	VECTOR4   Specular;       /* Specular 'shininess' */
	VECTOR4   Emissive;       /* Emissive color RGB */
	float     Power;          /* Sharpness if specular highlight */
	float     Opacity;        /* Transparency factor */
} MATERIALProperties;

typedef struct _Vertex
{
	VECTOR3  position;
	VECTOR3  normal;
	VECTOR2  texCoord;
	VECTOR4  color; // 可选：顶点颜色（RGBA）

	std::vector<unsigned long> Bones;
	std::vector<float>         Weights;
}Vertex;


typedef struct _Material
{
	MATERIALProperties  MatProps;
	std::string TexturePath;
}Material, * LPMaterial;


typedef struct _MESH
{
	std::string Name;

	int VertexCount;
	std::vector<Vertex> Vertices;
	int FacesCount;
	std::vector<WORD>  Faces;//默认dx支持16位索引

	int Material_index; // 材质索引，指向模型数据中的材质列表

	_MESH()
		: Name(""), VertexCount(0), FacesCount(0), Material_index(0) {
	}
}MESH, * LPMESH;

typedef struct _AnimationKeyFrame
{
	FLOAT Time;
	//Position 对应的这个
	VECTOR3      Translation;
	VECTOR3      Scale;
	VECTOR4      Rotation;
	_AnimationKeyFrame() {
		Time = 0.0f;                          // 时间默认从 0 开始
        Scale = VECTOR3(1.0f, 1.0f, 1.0f);    // 默认单位缩放（避免缩放为 0 导致模型消失）
		Rotation = VECTOR4(0.0f, 0.0f, 0.0f, 1.0f); // 单位四元数（无旋转）
		Translation = VECTOR3(0.0f, 0.0f, 0.0f); // 零平移
	}
}AnimationKeyFrame, * LPAnimationKeyFrame;

typedef struct _AnimationClip
{
	std::string Name;
	float duration;             // 动画总时长（秒）
	int keyframes;              //帧数
	std::map<std::string, std::vector<LPAnimationKeyFrame>> boneKeyFrames; // 骨骼索引→关键帧列表

	_AnimationClip()
		: Name(""), duration(0.0f), keyframes(0) {
	}
}AnimationClip, * LPAnimationClip;


typedef struct _Bone
{
	std::string Name;
	int    ParentBoneIndex;
	MATRIX BoneSpaceMatrix;
	MATRIX ParentBoneSpaceMatrix;
	_Bone() : Name(""), BoneSpaceMatrix(), ParentBoneSpaceMatrix(), ParentBoneIndex(-1) {}
}Bone, * LPBone;

typedef struct _BoneNode
{
	LPBone pBone;              //指向骨骼数据
	LPBone pFrameSibling;      //兄弟节点指向下一个
	LPBone pFrameFirstChild;   //指向第一个子节点
}BoneNode, * LPBoneNode;

//模型数据结构，包含骨骼、动画和网格等信息
typedef struct _ModelData
{
	int MaterialsCount;                      // 材质数量
	std::vector<LPMaterial> Materials;          // 材质列表 默认一个材质对象

	int MeshsCount;                          // 网格数量
	std::vector<LPMESH>  Meshs;              // 网格（带蒙皮信息） 默认至少一个网格对象

	int BonesCount;                          // 骨骼数量
	std::vector<LPBone> Bones;               // 骨骼列表 默认一个骨骼对象
	BoneNode BoneHierarchyRoot;              // 骨骼层次结构根节点

	int AnimationsCount;                     // 动画数量
	std::vector<LPAnimationClip> Animations; // 动画列表 默认一个动画对象
}ModelData, * LPModelData;

#endif //_ModelData_h_