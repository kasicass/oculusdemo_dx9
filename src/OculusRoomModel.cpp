#include "RenderTiny_D3D9_Device.h"
#include <utility>

static uint16_t CubeIndices[] =
{
	0, 1, 3,
	3, 1, 2,

	5, 4, 6,
	6, 4, 7,

	8, 9, 11,
	11, 9, 10,

	13, 12, 14,
	14, 12, 15,

	16, 17, 19,
	19, 17, 18,

	21, 20, 22,
	22, 20, 23
};

#define MYD3DCOLOR_A(c) (((c)>>24)&0xff)
#define MYD3DCOLOR_R(c) (((c)>>16)&0xff)
#define MYD3DCOLOR_G(c) (((c)>>8)&0xff)
#define MYD3DCOLOR_B(c) ((c)&0xff)

static void AddSolidColorBox(Model<VertexXYZC>* pModel, float x1, float y1, float z1, float x2, float y2, float z2, D3DCOLOR c)
{
	if (x1 > x2) std::swap(x1, x2);
	if (y1 > y2) std::swap(y1, y2);
	if (z1 > z2) std::swap(z1, z2);

	Vector3f CubeVertices[] = {
		Vector3f(x1, y2, z1),
		Vector3f(x2, y2, z1),
		Vector3f(x2, y2, z2),
		Vector3f(x1, y2, z2),

		Vector3f(x1, y1, z1),
		Vector3f(x2, y1, z1),
		Vector3f(x2, y1, z2),
		Vector3f(x1, y1, z2),

		Vector3f(x1, y1, z2),
		Vector3f(x1, y1, z1),
		Vector3f(x1, y2, z1),
		Vector3f(x1, y2, z2),

		Vector3f(x2, y1, z2),
		Vector3f(x2, y1, z1),
		Vector3f(x2, y2, z1),
		Vector3f(x2, y2, z2),

		Vector3f(x1, y1, z1),
		Vector3f(x2, y1, z1),
		Vector3f(x2, y2, z1),
		Vector3f(x1, y2, z1),

		Vector3f(x1, y1, z2),
		Vector3f(x2, y1, z2),
		Vector3f(x2, y2, z2),
		Vector3f(x1, y2, z2),
	};

	uint16_t startIndex = pModel->GetNextVertexIndex();

	enum
	{
		CubeVertexCount = sizeof(CubeVertices) / sizeof(CubeVertices[0]),
		CubeIndexCount = sizeof(CubeIndices) / sizeof(CubeIndices[0])
	};

	for (int v = 0; v < CubeVertexCount; ++v)
	{
		int a = MYD3DCOLOR_A(c);
		int r = MYD3DCOLOR_R(c);
		int g = MYD3DCOLOR_G(c);
		int b = MYD3DCOLOR_B(c);
		pModel->AddVertex(VertexXYZC(CubeVertices[v], D3DCOLOR_ARGB(a, r / CubeVertexCount*(v + 1), g / CubeVertexCount*(v + 1), b / CubeVertexCount*(v + 1))));
	}

	for (int i = 0; i < CubeIndexCount / 3; ++i)
	{
		pModel->AddTriangle(CubeIndices[i * 3] + startIndex,
			CubeIndices[i * 3 + 1] + startIndex,
			CubeIndices[i * 3 + 2] + startIndex);
	}
}

struct Slab
{
	float x1, y1, z1;
	float x2, y2, z2;
	D3DCOLOR c;
};

struct SlabModel
{
	int Count;
	Slab* pSlabs;
};

Slab RoomSlabs[] =
{
	// Left Wall
	{ -10.1f, 0.0f, -20.0f, -10.0f, 4.0f, 20.0f, D3DCOLOR_XRGB(128, 128, 128) },
	// Back Wall
	{ -10.0f, -0.1f, -20.1f, 10.0f, 4.0f, -20.0f, D3DCOLOR_XRGB(128, 128, 128) },

	// Right Wall
	{ 10.0f, -0.1f, -20.0f, 10.1f, 4.0f, 20.0f, D3DCOLOR_XRGB(128, 128, 128) },
};

SlabModel Room = { sizeof(RoomSlabs) / sizeof(Slab), RoomSlabs };

Slab FloorSlabs[] = {
	{ -10.0f, -0.1f, -20.0f, 10.0f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) }
};

SlabModel Floor = { sizeof(FloorSlabs) / sizeof(Slab), FloorSlabs };

Slab CeilingSlabs[] =
{
	{ -10.0f, 4.0f, -20.0f, 10.0f, 4.1f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) }
};

SlabModel Ceiling = { sizeof(FloorSlabs) / sizeof(Slab), CeilingSlabs };

Slab FixtureSlabs[] =
{
	// Right side shelf
	{ 9.5f, 0.75f, 3.0f, 10.1f, 2.5f, 3.1f, D3DCOLOR_XRGB(128, 128, 128) }, // Verticals
	{ 9.5f, 0.95f, 3.7f, 10.1f, 2.75f, 3.8f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 9.5f, 1.20f, 2.5f, 10.1f, 1.30f, 3.8f, D3DCOLOR_XRGB(128, 128, 128) }, // Horizontals
	{ 9.5f, 2.00f, 3.0f, 10.1f, 2.10f, 4.2f, D3DCOLOR_XRGB(128, 128, 128) },

	// Right railing    
	{ 5.0f, 1.1f, 20.0f, 10.0f, 1.2f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	// Bars
	{ 9.0f, 1.1f, 20.0f, 9.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 8.0f, 1.1f, 20.0f, 8.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 7.0f, 1.1f, 20.0f, 7.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 6.0f, 1.1f, 20.0f, 6.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 5.0f, 1.1f, 20.0f, 5.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },

	// Left railing    
	{ -10.0f, 1.1f, 20.0f, -5.0f, 1.2f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	// Bars
	{ -9.0f, 1.1f, 20.0f, -9.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ -8.0f, 1.1f, 20.0f, -8.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ -7.0f, 1.1f, 20.0f, -7.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ -6.0f, 1.1f, 20.0f, -6.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ -5.0f, 1.1f, 20.0f, -5.1f, 0.0f, 20.1f, D3DCOLOR_XRGB(128, 128, 128) },

	// Bottom Floor 2
	{ -15.0f, -6.1f, 18.0f, 15.0f, -6.0f, 30.0f, D3DCOLOR_XRGB(128, 128, 128) },
};

SlabModel Fixtures = { sizeof(FixtureSlabs) / sizeof(Slab), FixtureSlabs };

Slab FurnitureSlabs[] =
{
	// Table
	{ -1.8f, 0.7f, 1.0f, 0.0f, 0.8f, 0.0f, D3DCOLOR_XRGB(128, 128, 88) },
	{ -1.8f, 0.7f, 0.0f, -1.8f + 0.1f, 0.0f, 0.0f + 0.1f, D3DCOLOR_XRGB(128, 128, 88) }, // Leg 1
	{ -1.8f, 0.7f, 1.0f, -1.8f + 0.1f, 0.0f, 1.0f - 0.1f, D3DCOLOR_XRGB(128, 128, 88) }, // Leg 2
	{ 0.0f, 0.7f, 1.0f, 0.0f - 0.1f, 0.0f, 1.0f - 0.1f, D3DCOLOR_XRGB(128, 128, 88) }, // Leg 2
	{ 0.0f, 0.7f, 0.0f, 0.0f - 0.1f, 0.0f, 0.0f + 0.1f, D3DCOLOR_XRGB(128, 128, 88) }, // Leg 2

	// Chair
	{ -1.4f, 0.5f, -1.1f, -0.8f, 0.55f, -0.5f, D3DCOLOR_XRGB(88, 88, 128) }, // Set
	{ -1.4f, 1.0f, -1.1f, -1.4f + 0.06f, 0.0f, -1.1f + 0.06f, D3DCOLOR_XRGB(88, 88, 128) }, // Leg 1
	{ -1.4f, 0.5f, -0.5f, -1.4f + 0.06f, 0.0f, -0.5f - 0.06f, D3DCOLOR_XRGB(88, 88, 128) }, // Leg 2
	{ -0.8f, 0.5f, -0.5f, -0.8f - 0.06f, 0.0f, -0.5f - 0.06f, D3DCOLOR_XRGB(88, 88, 128) }, // Leg 2
	{ -0.8f, 1.0f, -1.1f, -0.8f - 0.06f, 0.0f, -1.1f + 0.06f, D3DCOLOR_XRGB(88, 88, 128) }, // Leg 2
	{ -1.4f, 0.97f, -1.05f, -0.8f, 0.92f, -1.10f, D3DCOLOR_XRGB(88, 88, 128) }, // Back high bar
};

SlabModel Furniture = { sizeof(FurnitureSlabs) / sizeof(Slab), FurnitureSlabs };

Slab PostsSlabs[] =
{
	// Posts
	{ 0, 0.0f, 0.0f, 0.1f, 1.3f, 0.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 0.4f, 0.1f, 1.3f, 0.5f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 0.8f, 0.1f, 1.3f, 0.9f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 1.2f, 0.1f, 1.3f, 1.3f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 1.6f, 0.1f, 1.3f, 1.7f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 2.0f, 0.1f, 1.3f, 2.1f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 2.4f, 0.1f, 1.3f, 2.5f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 2.8f, 0.1f, 1.3f, 2.9f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 3.2f, 0.1f, 1.3f, 3.3f, D3DCOLOR_XRGB(128, 128, 128) },
	{ 0, 0.0f, 3.6f, 0.1f, 1.3f, 3.7f, D3DCOLOR_XRGB(128, 128, 128) },
};

SlabModel Posts = { sizeof(PostsSlabs) / sizeof(Slab), PostsSlabs };


Model<VertexXYZC>* CreateModel(Vector3f pos, SlabModel* sm, Ptr<ShaderFill> fill, Ptr<VertexDecl> decl)
{
	Model<VertexXYZC>* pModel = new Model<VertexXYZC>();
	pModel->Decl = decl;
	pModel->Fill = fill;

	for (int i = 0; i < sm->Count; ++i)
	{
		Slab &s = sm->pSlabs[i];
		AddSolidColorBox(pModel, s.x1, s.y1, s.z1, s.x2, s.y2, s.z2, s.c);
	}

	return pModel;
}

void PopulateRoomScene(Scene* pRoomScene, RenderDevice* pRender)
{
	ShaderSet* ss = pRender->CreateShaderSet();
	ss->SetShader(pRender->LoadBuiltinShader(Shader_Vertex, VShader_MVP_Color));
	ss->SetShader(pRender->LoadBuiltinShader(Shader_Pixel, PShader_Color));
	Ptr<ShaderFill> fill = new ShaderFill(ss);
	Ptr<VertexDecl> decl = VertexDecl::GetDecl(VertexType_XYZC);

	pRoomScene->World.Add(CreateModel(Vector3f(0, 0, 0), &Room, fill, decl));
	pRoomScene->World.Add(CreateModel(Vector3f(0, 0, 0), &Floor, fill, decl));
	pRoomScene->World.Add(CreateModel(Vector3f(0, 0, 0), &Ceiling, fill, decl));
	pRoomScene->World.Add(CreateModel(Vector3f(0, 0, 0), &Fixtures, fill, decl));
	pRoomScene->World.Add(CreateModel(Vector3f(0, 0, 0), &Furniture, fill, decl));
	pRoomScene->World.Add(CreateModel(Vector3f(0, 0, 4), &Furniture, fill, decl));
	pRoomScene->World.Add(CreateModel(Vector3f(-3, 0, 3), &Posts, fill, decl));
}