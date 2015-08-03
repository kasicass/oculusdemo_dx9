#include "RenderTiny_D3D9_Device.h"
#include "OVR_CAPI.h"

namespace OVR {	namespace RenderTiny {

//
// ShaderBase
//

bool ShaderBase::SetUniformF(const char* name, unsigned int n, const float* v)
{
	D3DXHANDLE handle = ConstTable->GetConstantByName(NULL, name);
	if (handle)
	{
		HRESULT hr = ConstTable->SetFloatArray(Ren->Device, handle, v, n);
		return SUCCEEDED(hr);
	}
	else
	{
		return false;
	}
}

//
// Shader
//

template<> void Shader<Shader_Vertex, IDirect3DVertexShader9>::Load(const char* buf, unsigned int bufsz)
{
	Ptr<ID3DXBuffer> pShaderBuffer, pErrBuffer;
	HRESULT hr;

	hr = D3DXCompileShader(buf, bufsz, NULL, NULL, "main", "vs_2_0",
		D3DXSHADER_DEBUG, &pShaderBuffer.GetRawRef(), &pErrBuffer.GetRawRef(), &ConstTable.GetRawRef());
	if (FAILED(hr))
	{
		OutputDebugString("[tiny] D3DXCompileShader Err (vs): ");
		OutputDebugString((char*)pErrBuffer->GetBufferPointer());
		OutputDebugString("\n");
	}
	assert(SUCCEEDED(hr));

	hr = Ren->Device->CreateVertexShader((DWORD*)pShaderBuffer->GetBufferPointer(), &D3DShader.GetRawRef());
	assert(SUCCEEDED(hr));
}

template<> void Shader<Shader_Pixel, IDirect3DPixelShader9>::Load(const char* buf, unsigned int bufsz)
{
	Ptr<ID3DXBuffer> pShaderBuffer, pErrBuffer;
	HRESULT hr;

	hr = D3DXCompileShader(buf, bufsz, NULL, NULL, "main", "ps_2_0",
		D3DXSHADER_DEBUG, &pShaderBuffer.GetRawRef(), &pErrBuffer.GetRawRef(), &ConstTable.GetRawRef());
	if (FAILED(hr))
	{
		OutputDebugString("[tiny] D3DXCompileShader Err (ps): ");
		OutputDebugString((char*)pErrBuffer->GetBufferPointer());
		OutputDebugString("\n");
	}
	assert(SUCCEEDED(hr));

	hr = Ren->Device->CreatePixelShader((DWORD*)pShaderBuffer->GetBufferPointer(), &D3DShader.GetRawRef());
	assert(SUCCEEDED(hr));
}

template<> void Shader<Shader_Vertex, IDirect3DVertexShader9>::Set(PrimitiveType) const
{
	Ren->Device->SetVertexShader(D3DShader);
}

template<> void Shader<Shader_Pixel, IDirect3DPixelShader9>::Set(PrimitiveType) const
{
	Ren->Device->SetPixelShader(D3DShader);
}

//
// Texture
//

void Texture::LoadFromFile(const char *filename)
{
	HRESULT hr = D3DXCreateTextureFromFile(Ren->Device, filename, &Tex.GetRawRef());
	OVR_ASSERT(SUCCEEDED(hr));

	FillWidthHeight();
}

void Texture::FillWidthHeight()
{
	D3DSURFACE_DESC desc;
	HRESULT hr = Tex->GetLevelDesc(0, &desc);
	OVR_ASSERT(SUCCEEDED(hr));

	Width = desc.Width;
	Height = desc.Height;
}

void Texture::Set(int slot) const
{
	Ren->Device->SetTexture(slot, Tex);
}

//
// ShaderFill
//

void ShaderFill::Set(PrimitiveType prim) const
{
	Shaders->Set(prim);
	for (int i = 0; i < TextureCount; ++i)
	{
		if (Textures[i]) Textures[i]->Set(i);
	}
}

//
// VertexDecl
//

void VertexDecl::Set() const
{
	Ren->Device->SetVertexDeclaration(Decl);
}

Ptr<VertexDecl> VertexDecl::Decls[VertexType_Count];
void VertexDecl::Init(RenderDevice* r)  // static
{
	D3DVERTEXELEMENT9 DeclElementsXYZC[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		D3DDECL_END(),
	};
	Decls[VertexType_XYZC] = new VertexDecl(r, sizeof(VertexXYZC));
	HRESULT hr = r->Device->CreateVertexDeclaration(DeclElementsXYZC, &Decls[VertexType_XYZC]->Decl.GetRawRef());
	OVR_ASSERT(SUCCEEDED(hr));

	D3DVERTEXELEMENT9 DeclElementsXYZUV[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END(),
	};
	Decls[VertexType_XYZUV] = new VertexDecl(r, sizeof(VertexXYZUV));
	hr = r->Device->CreateVertexDeclaration(DeclElementsXYZUV, &Decls[VertexType_XYZUV]->Decl.GetRawRef());
	OVR_ASSERT(SUCCEEDED(hr));

	D3DVERTEXELEMENT9 DeclElementsDistortion[] = {
		{ 0, 0,  D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 8,  D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
		{ 0, 12, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 2 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		{ 0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
		D3DDECL_END(),
	};
	Decls[VertexType_Distortion] = new VertexDecl(r, sizeof(ovrDistortionVertex));
	hr = r->Device->CreateVertexDeclaration(DeclElementsDistortion, &Decls[VertexType_Distortion]->Decl.GetRawRef());
	OVR_ASSERT(SUCCEEDED(hr));
}

Ptr<VertexDecl> VertexDecl::GetDecl(VertexType vt) // static
{
	OVR_ASSERT(vt < VertexType_Count);
	return Decls[vt];
}

//
// Buffer
//

template<> void Buffer<IDirect3DVertexBuffer9>::Set(size_t stride)
{
	HRESULT hr = Ren->Device->SetStreamSource(0, D3DBuf, 0, (UINT)stride);
	assert(SUCCEEDED(hr));
}

template<> void Buffer<IDirect3DIndexBuffer9>::Set(size_t stride)
{
	OVR_UNUSED1(stride);
	HRESULT hr = Ren->Device->SetIndices(D3DBuf);
	assert(SUCCEEDED(hr));
}

template<> void Buffer<IDirect3DVertexBuffer9>::CreateBuffer(unsigned int bufsz)
{
	HRESULT hr = Ren->Device->CreateVertexBuffer(bufsz, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &D3DBuf.GetRawRef(), NULL);
	assert(SUCCEEDED(hr));
}

template<> void Buffer<IDirect3DIndexBuffer9>::CreateBuffer(unsigned int bufsz)
{
	HRESULT hr = Ren->Device->CreateIndexBuffer(bufsz, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &D3DBuf.GetRawRef(), NULL);
	assert(SUCCEEDED(hr));
}

//
// Container
//

void Container::Render(const Matrix4f& ltw, RenderDevice* ren)
{
	Matrix4f m = ltw * GetMatrix();
	for (unsigned int i = 0; i < Nodes.GetSize(); ++i)
	{
		Nodes[i]->Render(m, ren);
	}
}

void Scene::Render(RenderDevice* ren, const Matrix4f& view)
{
	ren->Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	ren->Device->SetRenderState(D3DRS_LIGHTING, FALSE);

	World.Render(view, ren);
}

//
// Shaders
//

static const char MVP_Color_VertexShaderSrc[] =
	"float4x4 WorldViewProj;\n"
	"\n"
	"struct VS_INPUT\n"
	"{\n"
	"	float4 Pos : POSITION0;\n"
	"	float3 Color : COLOR0;\n"
	"};\n"
	"\n"
	"struct VS_OUTPUT\n"
	"{\n"
	"	float4 Pos : POSITION0;\n"
	"	float3 Color : COLOR0;\n"
	"};\n"
	"\n"
	"VS_OUTPUT main(VS_INPUT Input)\n"
	"{\n"
	"	VS_OUTPUT Out = (VS_OUTPUT)0;\n"
	"	//Out.Pos = mul(WorldViewProj, Input.Pos);\n"
	"	Out.Pos = mul(Input.Pos, WorldViewProj);\n"
	"	Out.Color = Input.Color;\n"
	"	return Out;\n"
	"}\n";

static const char MVP_Color_PixelShaderSrc[] =
	"struct VS_OUTPUT\n"
	"{\n"
	"	float4 Pos : POSITION0;\n"
	"	float4 Color : COLOR0;\n"
	"};\n"
	"\n"
	"float4 main(VS_OUTPUT input) : COLOR0\n"
	"{\n"
	"	float4 oColor = input.Color;\n"
	"	oColor.a = 1;\n"
	"	return oColor;\n"
	"}\n";

static const char MVP_UV_VertexShaderSrc[] =
	"float4x4 WorldViewProj;\n"
	"\n"
	"struct VS_INPUT\n"
	"{\n"
	"	float4 Pos : POSITION0;\n"
	"	float2 TexCoord : TEXCOORD0;\n"
	"};\n"
	"\n"
	"struct VS_OUTPUT\n"
	"{\n"
	"	float4 Pos : POSITION0;\n"
	"	float2 TexCoord : TEXCOORD0;\n"
	"};\n"
	"\n"
	"VS_OUTPUT main(VS_INPUT Input)\n"
	"{\n"
	"	VS_OUTPUT Out = (VS_OUTPUT)0;\n"
	"	//Out.Pos = mul(WorldViewProj, Input.Pos);\n"
	"	Out.Pos = mul(Input.Pos, WorldViewProj);\n"
	"	Out.TexCoord = Input.TexCoord;\n"
	"	return Out;\n"
	"}\n";

static const char MVP_UV_PixelShaderSrc[] =
	"texture myTexture;\n"
	"\n"
	"sampler2D mySampler = sampler_state{\n"
	"	Texture = (myTexture);\n"
	"	MinFilter = Linear;\n"
	"	MagFilter = Linear;\n"
	"	AddressU = Clamp;\n"
	"	AddressV = Clamp;\n"
	"};\n"
	"\n"
	"struct VS_OUTPUT\n"
	"{\n"
	"	float4 Pos : POSITION0;\n"
	"	float2 TexCoord : TEXCOORD0;\n"
	"};\n"
	"\n"
	"float4 main(VS_OUTPUT input) : COLOR0\n"
	"{\n"
	"	float4 oColor = tex2D(mySampler, input.TexCoord);\n"
	"	oColor.a = 1;\n"
	"	return oColor;\n"
	"}\n";

static const char Distortion_VertexShaderSrc[] =
	"float2 EyeToSourceUVScale, EyeToSourceUVOffset;                                        \n"
	"float4x4 EyeRotationStart, EyeRotationEnd;                                             \n"
	"float2 TimewarpTexCoord(float2 TexCoord, float4x4 rotMat)                              \n"
	"{                                                                                      \n"
	// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic 
	// aberration and distortion). These are now "real world" vectors in direction (x,y,1) 
	// relative to the eye of the HMD.	Apply the 3x3 timewarp rotation to these vectors.
	"    float3 transformed = float3( mul ( rotMat, float4(TexCoord.xy, 1, 1) ).xyz);       \n"
	// Project them back onto the Z=1 plane of the rendered images.
	"    float2 flattened = (transformed.xy / transformed.z);                               \n"
	// Scale them into ([0,0.5],[0,1]) or ([0.5,0],[0,1]) UV lookup space (depending on eye)
	"    return(EyeToSourceUVScale * flattened + EyeToSourceUVOffset);                      \n"
	"}                                                                                      \n"
	"void main(in float2  Position   : POSITION,  in float timewarpLerpFactor : POSITION1,  \n"
	"          in float   Vignette   : POSITION2, in float2 TexCoord0         : TEXCOORD0,  \n"
	"          in float2  TexCoord1  : TEXCOORD1, in float2 TexCoord2         : TEXCOORD2,  \n"
	"          out float4 oPosition  : SV_Position,                                         \n"
	"          out float2 oTexCoord0 : TEXCOORD0, out float2 oTexCoord1 : TEXCOORD1,        \n"
	"          out float2 oTexCoord2 : TEXCOORD2, out float  oVignette  : TEXCOORD3)        \n"
	"{                                                                                      \n"
	"    float4x4 lerpedEyeRot = lerp(EyeRotationStart, EyeRotationEnd, timewarpLerpFactor);\n"
	"    oTexCoord0  = TimewarpTexCoord(TexCoord0,lerpedEyeRot);                            \n"
	"    oTexCoord1  = TimewarpTexCoord(TexCoord1,lerpedEyeRot);                            \n"
	"    oTexCoord2  = TimewarpTexCoord(TexCoord2,lerpedEyeRot);                            \n"
	"    oPosition = float4(Position.xy, 0.5, 1.0);    oVignette = Vignette;                \n"
	"}";

static const char Distortion_PixelShaderSrc[] =
//"Texture2D Texture   : register(t0);                                                    \n"
//"SamplerState Linear : register(s0);                                                    \n"
"texture myTexture;\n"
"\n"
"sampler2D mySampler = sampler_state{\n"
"	Texture = (myTexture0);\n"
"	MinFilter = Linear;\n"
"	MagFilter = Linear;\n"
"	AddressU = Clamp;\n"
"	AddressV = Clamp;\n"
"};\n"
"float4 main(in float4 oPosition  : SV_Position,  in float2 oTexCoord0 : TEXCOORD0,     \n"
"            in float2 oTexCoord1 : TEXCOORD1,    in float2 oTexCoord2 : TEXCOORD2,     \n"
"            in float  oVignette  : TEXCOORD3)    : SV_Target                           \n"
"{                                                                                      \n"
"    // 3 samples for fixing chromatic aberrations                                      \n"
"    //float R = Texture.Sample(Linear, oTexCoord0.xy).r;                                 \n"
"    float R = tex2D(mySampler, oTexCoord0.xy).r;\n"
"    //float G = Texture.Sample(Linear, oTexCoord1.xy).g;                                 \n"
"    //float B = Texture.Sample(Linear, oTexCoord2.xy).b;                                 \n"
"    float G = tex2D(mySampler, oTexCoord1.xy).g;\n"
"    float B = tex2D(mySampler, oTexCoord2.xy).b;\n"
"    return float4(R,G,B,1); //(oVignette*float4(R,G,B,1));                                                \n"
	"}";


//
// RenderDevice
//

RenderDevice*
RenderDevice::CreateDevice(const RendererParams& rp, void* oswnd)
{
	RenderDevice* p = new RenderDevice(rp, (HWND)oswnd);
	if (p)
	{
		if (!p->Device)
		{
			p->Release();
			p = 0;
		}
	}

	VertexDecl::Init(p);

	return p;
}

RenderDevice::RenderDevice(const RendererParams& p, HWND window)
{
	RECT rc;
	if (p.Resolution == Sizei(0))
	{
		GetClientRect(window, &rc);
		WindowWidth = rc.right - rc.left;
		WindowHeight = rc.bottom - rc.top;
	}
	else
	{
		WindowWidth = p.Resolution.w;
		WindowHeight = p.Resolution.h;
	}

	Window = window;

	Ptr<IDirect3D9> pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	assert(pD3D != NULL);

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	//d3dpp.EnableAutoDepthStencil = TRUE;
	//d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	//d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;

	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &Device.GetRawRef());
	OVR_ASSERT(Device != NULL);

	HRESULT hr = Device->GetRenderTarget(0, &BackBufferRT.GetRawRef());
	OVR_ASSERT(SUCCEEDED(hr));

	IDirect3DTexture9 *ztexture = 0;
	hr = Device->CreateTexture(WindowWidth*2, WindowHeight*2, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24S8, D3DPOOL_DEFAULT, &ztexture, NULL);
	OVR_ASSERT(SUCCEEDED(hr));

	IDirect3DSurface9 *zsurface = 0;
	hr = ztexture->GetSurfaceLevel(0, &zsurface);
	OVR_ASSERT(SUCCEEDED(hr));

	hr = Device->SetDepthStencilSurface(zsurface);
	OVR_ASSERT(SUCCEEDED(hr));

	zsurface->Release();
	ztexture->Release();

	InitShaders();
}

RenderDevice::~RenderDevice()
{
}

void RenderDevice::InitShaders()
{
	VertexShaders[VShader_MVP_Color] = new VertexShader(this, MVP_Color_VertexShaderSrc, sizeof(MVP_Color_VertexShaderSrc));
	VertexShaders[VShader_MVP_UV] = new VertexShader(this, MVP_UV_VertexShaderSrc, sizeof(MVP_UV_VertexShaderSrc));
	PixelShaders[PShader_Color] = new PixelShader(this, MVP_Color_PixelShaderSrc, sizeof(MVP_Color_PixelShaderSrc));
	PixelShaders[PShader_UV] = new PixelShader(this, MVP_UV_PixelShaderSrc, sizeof(MVP_UV_PixelShaderSrc));

	VertexShaders[VShader_Distortion] = new VertexShader(this, Distortion_VertexShaderSrc, sizeof(Distortion_VertexShaderSrc));
	PixelShaders[PShader_Distortion] = new PixelShader(this, Distortion_PixelShaderSrc, sizeof(Distortion_PixelShaderSrc));
}

void RenderDevice::Render(const ShaderFill* fill, VertexDecl* decl, VertexBuffer* vb, IndexBuffer* ib, int stride,
	const Matrix4f& matrix, unsigned int numVertices, unsigned int numPrimitives, PrimitiveType prim)
{
	fill->Set(prim);
	fill->Shaders->SetUniform4x4f("WorldViewProj", Proj*matrix); // *Proj);

	decl->Set();
	vb->Set(stride);
	ib->Set();

	OVR_ASSERT(prim == Prim_Triangles);
	Device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numPrimitives);
}

void RenderDevice::BeginScene()
{
	HRESULT hr = Device->BeginScene();
	OVR_ASSERT(SUCCEEDED(hr));
}

void RenderDevice::EndScene()
{
	HRESULT hr = Device->EndScene();
	OVR_ASSERT(SUCCEEDED(hr));
}

void RenderDevice::Present()
{
	Device->Present(NULL, NULL, NULL, NULL);
}

ShaderBase *RenderDevice::LoadBuiltinShader(ShaderStage stage, int shader)
{
	switch (stage)
	{
	case Shader_Vertex:
		return VertexShaders[shader];
	case Shader_Pixel:
		return PixelShaders[shader];
	default:
		return NULL;
	}
}

Texture* RenderDevice::CreateRenderTarget(unsigned int width, unsigned int height)
{
	IDirect3DTexture9 *rt = 0;
	HRESULT hr = Device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &rt, NULL);
	OVR_ASSERT(SUCCEEDED(hr));

	return new Texture(this, rt);
}

void RenderDevice::SetRenderTarget(Texture* rt)
{
	if (rt == NULL)
	{
		Device->SetRenderTarget(0, BackBufferRT);
		return;
	}

	Ptr<IDirect3DSurface9> MyRT;
	HRESULT hr = rt->Tex->GetSurfaceLevel(0, &MyRT.GetRawRef());
	OVR_ASSERT(SUCCEEDED(hr));

	Device->SetRenderTarget(0, MyRT);
}

void RenderDevice::SetViewport(const Recti& vp)
{
	D3DVIEWPORT9 vp9;
	vp9.Width = (float)vp.w;
	vp9.Height = (float)vp.h;
	vp9.MinZ = 0;
	vp9.MaxZ = 1;
	vp9.X = (float)vp.x;
	vp9.Y = (float)vp.y;

	HRESULT hr = Device->SetViewport(&vp9);
	OVR_ASSERT(SUCCEEDED(hr));
}

void RenderDevice::Clear(float r, float g, float b, float a, float depth)
{
	this->SetDepthMode(true, true, Compare_Always);

	//pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	HRESULT hr = Device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_COLORVALUE(r, g, b, a), depth, 0);
	//HRESULT hr = Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	//HRESULT hr = Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_COLORVALUE(r, g, b, a), depth, 0);
	OVR_ASSERT(SUCCEEDED(hr));
}

void RenderDevice::SetProjection(const Matrix4f& proj)
{
	Proj = proj;
}

void RenderDevice::SetFullViewport()
{
	D3DVIEWPORT9 vp9;
	vp9.Width = (float)WindowWidth;
	vp9.Height = (float)WindowHeight;
	vp9.MinZ = 0;
	vp9.MaxZ = 1;
	vp9.X = 0;
	vp9.Y = 0;

	HRESULT hr = Device->SetViewport(&vp9);
	OVR_ASSERT(SUCCEEDED(hr));
}

void RenderDevice::SetDepthMode(bool enable, bool write, CompareFunc func)
{
	HRESULT hr;
	hr = Device->SetRenderState(D3DRS_ZENABLE, enable ? D3DZB_TRUE : D3DZB_FALSE);
	OVR_ASSERT(SUCCEEDED(hr));
	hr = Device->SetRenderState(D3DRS_ZWRITEENABLE, write ? D3DZB_TRUE : D3DZB_FALSE);
	OVR_ASSERT(SUCCEEDED(hr));

	D3DCMPFUNC f = D3DCMP_LESS;
	switch (func) {
	case Compare_Always: f = D3DCMP_ALWAYS; break;
	case Compare_Less: f = D3DCMP_LESS; break;
	case Compare_Greater: f = D3DCMP_GREATER; break;
	default: f = D3DCMP_LESS; break;
	}

	hr = Device->SetRenderState(D3DRS_ZFUNC, f);
	OVR_ASSERT(SUCCEEDED(hr));
}

}}