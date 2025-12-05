#include <vector>
#include <map>
#ifndef _ModelData_h_
#define _ModelData_h_
// 手动定义 D3DX_PI（值与官方定义一致：3.14159265358979323846f）
#ifndef PI
#define PI 3.14159265358979323846f
#endif

//默认有一个骨骼，一个蒙皮，一个动画，多个网格

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
	// 1. 默认构造函数：初始化为黑色不透明（r=0, g=0, b=0, a=1）
	_COLORVALUE() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}

	// 2. 三参数构造函数：指定 RGB（Alpha 默认为 1.0）
	_COLORVALUE(float red, float green, float blue)
		: r(red), g(green), b(blue), a(1.0f) {
	}

	// 3. 四参数构造函数：指定 RGBA 全通道
	_COLORVALUE(float red, float green, float blue, float alpha)
		: r(red), g(green), b(blue), a(alpha) {
	}

} COLORVALUE;

typedef struct _VECTOR3 {
	// VECTOR3 默认构造（可选，初始化为零向量）
	_VECTOR3() : x(0.0f), y(0.0f), z(0.0f) {}
	// 带参数构造（方便赋值）
	_VECTOR3(FLOAT _x, FLOAT _y, FLOAT _z) : x(_x), y(_y), z(_z) {}
	float x;
	float y;
	float z;
} VECTOR3;

typedef struct _MATERIALInfo {
	COLORVALUE   Diffuse;        /* Diffuse color RGBA */
	COLORVALUE   Ambient;        /* Ambient color RGB */
	COLORVALUE   Specular;       /* Specular 'shininess' */
	COLORVALUE   Emissive;       /* Emissive color RGB */
	float        Power;       /* Sharpness if specular highlight */
	float        Opacity;       /* Transparency factor */
} MATERIALInfo;

typedef struct _Vertex
{
	float x, y, z;
	float u, v;
	float nx, ny, nz;
}Vertex;

typedef struct _Influence
{
	int count;
	std::vector<uint32_t> Vertices;
	std::vector<float> Weights;
}Influence;

typedef struct _Material
{
	MATERIALInfo  MatD3D;
	const char* pTexture;
}Material;

typedef struct _MESH
{
	const char* Name;
	int VertexCount;
	int FaceCount;
	std::vector<Vertex> Vertices;
	std::vector<DWORD>  Indices;
	//std::vector<DWORD>  Attributes;
	std::vector<Material>   MatD3Ds;
	std::map<const char*, Influence> Influences;
}MESH, * LPMESH;

typedef struct _MESHCONTAINER
{
	const char* Name;
	MESH		 pOrigMesh;
}MESHCONTAINER, * LPMESHCONTAINER;

typedef struct _FRAME
{
	const char* Name;
	MATRIX		      TransformationMatrix;
	LPMESHCONTAINER	  pMeshContainer;
	struct _FRAME* pFrameSibling;   //兄弟节点指向下一个
	struct _FRAME* pFrameFirstChild;//指向第一个子节点
	MATRIX	          ParentTM;
	MATRIX	          NodeTMInverse;
	int               BoneIndex;
	int               ParentBoneIndex;
	//Material	      pMaterial;
	WORD              MaterialID;
}FRAME, * LPFRAME;

typedef struct _KEY_VECTOR3
{
	FLOAT Time;
	VECTOR3 Value;
} KEY_VECTOR3, * LPKEY_VECTOR3;

typedef struct _QUATERNION
{
public:
	// 默认构造（类内实现）
	_QUATERNION() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
	// 带参数构造（类内实现，关键修复：解决链接错误）
	_QUATERNION(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w)
		: x(_x), y(_y), z(_z), w(_w) {
	}
	FLOAT x, y, z, w;
} QUATERNION, * LPDQUATERNION;

typedef struct _KEY_QUATERNION
{
	FLOAT Time;
	QUATERNION Value;
} KEY_QUATERNION, * LPXKEY_QUATERNION;

typedef struct _AnimationKeyFrame
{
	FLOAT Time;
	VECTOR3      Translation;
	QUATERNION   Rotation;
	//VECTOR3      Scale; //默认不用这个
	// 默认构造函数（C++03 兼容）
	_AnimationKeyFrame() {
		Time = 0.0f;                          // 时间默认从 0 开始
		//Scale = VECTOR3(1.0f, 1.0f, 1.0f);    // 默认单位缩放（避免缩放为 0 导致模型消失）
		Rotation = QUATERNION(0.0f, 0.0f, 0.0f, 1.0f); // 单位四元数（无旋转）
		Translation = VECTOR3(0.0f, 0.0f, 0.0f); // 零平移
	}
}AnimationKeyFrame, * LPAnimationKeyFrame;
//
//typedef struct _AnimationKeyFrame
//{
//	const char* Name;
//	FLOAT Time;
//	std::vector<VECTOR3>      Scales; //默认不用这个
//	std::vector<QUATERNION>   Rotations;
//	std::vector<VECTOR3>      Translations;
//}AnimationKeyFrame, * LPAnimationKeyFrame;

typedef struct _AnimationClip
{
	const char* Name;
	float duration;            // 动画总时长（秒）
	//std::vector<AnimationKeyFrame>      AnimationKeys;
	std::map<std::string, std::vector<AnimationKeyFrame>> boneKeyFrames; // 骨骼索引→关键帧列表
}AnimationClip, * LPAnimationClip;


typedef struct _ModelData
{
	std::vector<LPFRAME>          Bones;           // 骨骼列表 默认一个骨骼对象
	std::map<int, std::string>    BoneNameToIndex; // 骨骼名称到索引的映射
	std::vector<LPAnimationClip>  Animations;      // 动画列表 (默认一个动画)
	std::vector<LPMESH>           Meshs;            // 网格（带蒙皮信息） 默认至少一个网格对象
}ModelData, * LPModelData;

#endif //_ModelData_h_