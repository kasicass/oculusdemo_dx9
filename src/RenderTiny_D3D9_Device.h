#pragma once

#include "Kernel/OVR_Math.h"
#include "Kernel/OVR_Array.h"
#include "Kernel/OVR_String.h"
#include "Kernel/OVR_Color.h"
#include <d3d9.h>
#include <d3dx9.h>

namespace OVR {	namespace RenderTiny {

class RenderDevice;

enum PrimitiveType
{
	Prim_Triangles,
	Prim_Lines,
	Prim_TriangleStrip,
	Prim_Unknown,
	Prim_Count,
};

enum VertexType
{
	VertexType_XYZC = 0,
	VertexType_XYZUV,
	VertexType_Distortion,
	VertexType_Count
};

enum ShaderStage
{
	Shader_Vertex   = 0,
	Shader_Pixel    = 1,
	Shader_Count    = 2,
};

enum BuiltinShaders
{
	VShader_MVP_Color       = 0,
	VShader_MVP_UV          = 1,
	VShader_Distortion      = 2,
	VShader_Count           = 3,

	PShader_Color           = 0,
	PShader_UV              = 1,
	PShader_Distortion      = 2,
	PShader_Count           = 3,
};

enum CompareFunc {
	Compare_Always = 0,
	Compare_Less,
	Compare_Greater,
	Compare_Count
};

// Base class for vertex and pixel shaders. Stored in ShaderSet.
class ShaderBase : public RefCountBase < ShaderBase >
{
protected:
	ShaderStage Stage;

public:
	RenderDevice* Ren;
	Ptr<ID3DXConstantTable> ConstTable;

	ShaderBase(RenderDevice* r, ShaderStage sstage) : Ren(r), Stage(sstage) {}
	virtual ~ShaderBase() {}

	ShaderStage GetStage() const { return Stage; }

	virtual void Set(PrimitiveType) const {}
	virtual bool SetUniformF(const char* name, unsigned int n, const float* v);
};

template <ShaderStage SStage, class D3DShaderType>
class Shader : public ShaderBase
{
public:
	Ptr<D3DShaderType> D3DShader;

	Shader(RenderDevice* r, const char* buf, unsigned int bufsz) : ShaderBase(r, SStage)
	{
		Load(buf, bufsz);
	}

	virtual ~Shader()
	{
	}

	// These functions have specializations.
	void Load(const char* buf, unsigned int bufsz);
	virtual void Set(PrimitiveType prim) const;
};

typedef Shader<Shader_Vertex, IDirect3DVertexShader9> VertexShader;
typedef Shader<Shader_Pixel, IDirect3DPixelShader9> PixelShader;

// A group of shaders, one per stage.
// A ShaderSet is applied to a RenderDevice for rendering with a given fill.
class ShaderSet : public RefCountBase < ShaderSet >
{
protected:
	Ptr<ShaderBase> Shaders[Shader_Count];

public:
	ShaderSet() {}
	virtual ~ShaderSet() {}

	virtual void SetShader(ShaderBase *s)
	{
		Shaders[s->GetStage()] = s;
	}
	virtual void UnsetShader(int stage)
	{
		Shaders[stage] = nullptr;
	}
	ShaderBase* GetShader(int stage) { return Shaders[stage]; }

	virtual void Set(PrimitiveType prim) const
	{
		for (int i = 0; i < Shader_Count; ++i)
		{
			if (Shaders[i]) Shaders[i]->Set(prim);
		}
	}

	virtual bool SetUniform(const char* name, unsigned int n, const float* v)
	{
		bool result = false;
		for (int i = 0; i < Shader_Count; ++i)
		{
			if (Shaders) result |= Shaders[i]->SetUniformF(name, n, v);
		}
		return result;
	}
	bool SetUniform1f(const char* name, float x)
	{
		const float v[] = { x };
		return SetUniform(name, 1, v);
	}
	bool SetUniform2f(const char* name, float x, float y)
	{
		const float v[] = { x, y };
		return SetUniform(name, 2, v);
	}
	bool SetUniform3f(const char* name, float x, float y, float z)
	{
		const float v[] = { x, y, z };
		return SetUniform(name, 3, v);
	}
	bool SetUniform4f(const char* name, float x, float y, float z, float w = 1)
	{
		const float v[] = { x, y, z, w };
		return SetUniform(name, 4, v);
	}
	bool SetUniformv(const char* name, const Vector3f& v)
	{
		const float a[] = { v.x, v.y, v.z, 1 };
		return SetUniform(name, 4, a);
	}
	bool SetUniform4fv(const char* name, unsigned int n, const Vector4f* v)
	{
		return SetUniform(name, 4*n, &v[0].x);
	}
	bool SetUniform4x4f(const char* name, const Matrix4f& m)
	{
		Matrix4f mt = m.Transposed();
		return SetUniform(name, 16, &mt.M[0][0]);
	}
};

class Texture : public RefCountBase < Texture >
{
public:
	RenderDevice* Ren;
	Ptr<IDirect3DTexture9> Tex;
	unsigned int Width, Height;

	Texture(RenderDevice* r, IDirect3DTexture9 *t) : Ren(r), Tex(t) { FillWidthHeight(); }
	Texture(RenderDevice* r) : Ren(r) {}
	virtual ~Texture() {}

	void FillWidthHeight();
	void LoadFromFile(const char *filename);
	void Set(int slot) const;
};

class ShaderFill : public RefCountBase < ShaderFill >
{
public:
	enum { TextureCount = 8 };

	Ptr<ShaderSet> Shaders;
	Ptr<Texture> Textures[TextureCount];

public:
	ShaderFill(ShaderSet* sh) : Shaders(sh) {}
	virtual ~ShaderFill() {}

	void Set(PrimitiveType prim = Prim_Unknown) const;
	void SetTexture(unsigned int i, Texture* t) { if (i < TextureCount) Textures[i] = t; }
};

//-----------------------------------------------------------------------------------

struct VertexXYZC
{
	Vector3f Pos;
	D3DCOLOR C;

	VertexXYZC(const Vector3f& p, const D3DCOLOR& c) : Pos(p), C(c)
	{}
	VertexXYZC(float x, float y, float z, const D3DCOLOR& c) : Pos(x, y, z), C(c)
	{}

	bool operator==(const VertexXYZC& b) const
	{
		return Pos == b.Pos && C == b.C;
	}
};

struct VertexXYZUV
{
	Vector3f Pos;
	float U, V;

	VertexXYZUV(const Vector3f& p, float u, float v) : Pos(p), U(u), V(v)
	{}
	VertexXYZUV(float x, float y, float z, float u, float v) : Pos(x, y, z), U(u), V(v)
	{}

	bool operator==(const VertexXYZUV& b) const
	{
		return Pos == b.Pos && U == b.U && V == b.V;
	}
};

class VertexDecl : public RefCountBase < VertexDecl >
{
public:
	void Set() const;
	unsigned int GetVertexSize() const { return VertexSize; }

private:
	RenderDevice* Ren;
	Ptr<IDirect3DVertexDeclaration9> Decl;
	unsigned int VertexSize;

	VertexDecl(RenderDevice* r, unsigned int sz) : Ren(r), VertexSize(sz) {}
	virtual ~VertexDecl() {}

public:
	static void Init(RenderDevice* r);
	static Ptr<VertexDecl> GetDecl(VertexType vt);
	static Ptr<VertexDecl> Decls[VertexType_Count];
};

class BufferBase : public RefCountBase < BufferBase >
{
public:
	BufferBase() {}
	virtual ~BufferBase() {}

	virtual void Set(size_t stride = 0) { OVR_UNUSED1(stride);  };
	virtual void Data(const void* buf, unsigned int bufsz) { OVR_UNUSED2(buf, bufsz); }
};

template <class D3DBufferType>
class Buffer : public BufferBase
{
public:
	RenderDevice *Ren;
	Ptr<D3DBufferType> D3DBuf;

	Buffer(RenderDevice* r) : Ren(r) {}
	virtual ~Buffer() {}

	virtual void Data(const void* buf, unsigned int bufsz)
	{
		if (!D3DBuf) CreateBuffer(bufsz);

		VOID *pLock = nullptr;
		HRESULT hr;
		hr = D3DBuf->Lock(0, bufsz, &pLock, 0);
		assert(SUCCEEDED(hr));

		memcpy(pLock, buf, bufsz);

		hr = D3DBuf->Unlock();
		assert(SUCCEEDED(hr));
	}

	virtual void Set(size_t stride = 0);
	void CreateBuffer(unsigned int bufsz);
};

typedef Buffer<IDirect3DVertexBuffer9> VertexBuffer;
typedef Buffer<IDirect3DIndexBuffer9> IndexBuffer;

//-----------------------------------------------------------------------------------

class Node : public RefCountBase < Node >
{
private:
	Vector3f Pos;
	Quatf Rot;

	mutable Matrix4f Mat;
	mutable bool MatCurrent;

public:
	Node() : Pos(0), MatCurrent(true) {}
	virtual ~Node() {}

	enum NodeType
	{
		Node_NonDisplay,
		Node_Container,
		Node_Model
	};
	virtual NodeType GetType() const { return Node_NonDisplay; }

	const Vector3f& GetPosition() const { return Pos; }
	const Quatf& GetOrientation() const { return Rot; }
	void SetPosition(Vector3f p) { Pos = p; MatCurrent = false; }
	void SetOrientation(Quatf q) { Rot = q; MatCurrent = false; }

	void Move(Vector3f p) { Pos += p; MatCurrent = false; }
	void Rotate(Quatf q) { Rot = q * Rot; MatCurrent = false; }

	void SetMatrix(const Matrix4f& m)
	{
		MatCurrent = true;
		Mat = m;
	}

	const Matrix4f& GetMatrix() const
	{
		if (!MatCurrent)
		{
			Mat = Matrix4f(Rot);
			Mat = Matrix4f::Translation(Pos) * Mat;
			MatCurrent = true;
		}
		return Mat;
	}

	virtual void Render(const Matrix4f& ltw, RenderDevice* ren) { OVR_UNUSED2(ltw, ren); }
};

template <typename MyVertex>
class Model : public Node
{
public:
	Ptr<VertexDecl> Decl;
	Array<MyVertex> Vertices;
	Array<uint16_t> Indices;
	PrimitiveType Type;
	Ptr<ShaderFill> Fill;
	bool Visible;

	Ptr<VertexBuffer> VB;
	Ptr<IndexBuffer> IB;

	Model(PrimitiveType t = Prim_Triangles) : Type(t), Visible(true) {}
	virtual ~Model() {}

	PrimitiveType GetPrimType() const { return Type; }

	void SetVisible(bool visible) { Visible = visible; }
	bool IsVisible() const { return Visible; }

	virtual NodeType GetType() const { return Node_Model; }
	virtual void Render(const Matrix4f& ltw, RenderDevice* ren)
	{
		if (Visible)
		{
			Matrix4f m = ltw * GetMatrix();
			ren->Render(m, this);
		}
	}

	uint16_t GetNextVertexIndex() const
	{
		return (uint16_t)Vertices.GetSize();
	}

	uint16_t AddVertex(const MyVertex& v)
	{
		OVR_ASSERT(!VB && !IB);
		uint16_t index = (uint16_t)Vertices.GetSize();
		Vertices.PushBack(v);
		return index;
	}

	void AddTriangle(uint16_t a, uint16_t b, uint16_t c)
	{
		Indices.PushBack(a);
		Indices.PushBack(b);
		Indices.PushBack(c);
	}
};

class Container : public Node
{
public:
	Array<Ptr<Node>> Nodes;

	Container() {}
	~Container() {}

	virtual NodeType GetType() const { return Node_Container; }

	virtual void Render(const Matrix4f& ltw, RenderDevice* ren);

	void Add(Node *n) { Nodes.PushBack(n); }
	void Clear() { Nodes.Clear(); }
};

class Scene : public NewOverrideBase
{
public:
	Container World;

public:
	void Render(RenderDevice* ren, const Matrix4f& view);
};

//-----------------------------------------------------------------------------------

struct RendererParams
{
	int    Multisample;
	int    Fullscreen;

	Sizei  Resolution;

	RendererParams(int ms = 1) : Multisample(ms), Fullscreen(0), Resolution(0) {}
};

class RenderDevice : public RefCountBase<RenderDevice>
{
public:
	int WindowWidth, WindowHeight;
	HWND Window;

	Matrix4f Proj;
	Ptr<IDirect3DDevice9> Device;

	Ptr<VertexShader> VertexShaders[VShader_Count];
	Ptr<PixelShader> PixelShaders[PShader_Count];

	Ptr<IDirect3DSurface9> BackBufferRT;

public:
	RenderDevice(const RendererParams& p, HWND window);
	virtual ~RenderDevice();

	static RenderDevice* CreateDevice(const RendererParams& rp, void* oswnd);

	void InitShaders();

	template <typename MyModel>
	void Render(const Matrix4f& view, MyModel* model)
	{
		if (!model->VB)
		{
			Ptr<VertexBuffer> vb = new VertexBuffer(this);
			vb->Data(&model->Vertices[0], (unsigned int)(model->Vertices.GetSize() * model->Decl->GetVertexSize()));
			model->VB = vb;
		}
		if (!model->IB)
		{
			Ptr<IndexBuffer> ib = new IndexBuffer(this);
			ib->Data(&model->Indices[0], (unsigned int)(model->Indices.GetSize() * sizeof(uint16_t)));
			model->IB = ib;
		}

		this->Render(model->Fill, model->Decl, model->VB, model->IB, (unsigned int)model->Decl->GetVertexSize(), view,
			model->Vertices.GetSize(), model->Indices.GetSize() / 3, Prim_Triangles);
	}

	virtual void Render(const ShaderFill* fill, VertexDecl* decl, VertexBuffer* vb, IndexBuffer* ib, int stride,
		const Matrix4f& matrix, unsigned int numVertices, unsigned int numPrimitives, PrimitiveType prim);

	virtual void BeginScene();
	virtual void EndScene();
	virtual void Present();

	virtual ShaderBase *LoadBuiltinShader(ShaderStage stage, int shader);
	virtual ShaderSet *CreateShaderSet() { return new ShaderSet; }
	virtual VertexBuffer* CreateVertexBuffer() { return new VertexBuffer(this); }
	virtual IndexBuffer* CreateIndexBuffer() { return new IndexBuffer(this); }

	virtual Texture* CreateRenderTarget(unsigned int width, unsigned int height);
	virtual void SetRenderTarget(Texture* rt);
	virtual void SetDefaultRenderTarget() { SetRenderTarget(NULL); }
	virtual void SetViewport(const Recti& vp);
	virtual void Clear(float r = 0, float g = 0, float b = 0, float a = 1, float depth = 1);

	virtual void SetProjection(const Matrix4f& proj);
	virtual void SetFullViewport();
	virtual void SetDepthMode(bool enable, bool write, CompareFunc func = Compare_Less);
};

}}

using namespace OVR;
using namespace OVR::RenderTiny;