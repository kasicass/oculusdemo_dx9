// Include the OculusVR SDK
#include "OVR_CAPI.h"

// Choose whether the SDK performs rendering/distortion, or the application. 
#define          SDK_RENDER 1  //Do NOT switch until you have viewed and understood the Health and Safety message.
                               //Disabling this makes it a non-compliant app, and not suitable for demonstration. In place for development only.
const bool       FullScreen = true; //Should be true for correct timing.  Use false for debug only.


// Include Non-SDK supporting Utilities from other files
#include "RenderTiny_D3D9_Device.h"
HWND Util_InitWindowAndGraphics    (Recti vp, int fullscreen, int multiSampleCount, bool UseAppWindowFrame, RenderDevice **pDevice);
void Util_ReleaseWindowAndGraphics (RenderDevice* pRender);
void PopulateRoomScene(Scene* scene, RenderDevice* render);

ovrHmd            HMD;
ovrEyeRenderDesc  EyeRenderDesc[2];
ovrRecti          EyeRenderViewport[2];
RenderDevice*     pRender = 0;
Scene*            pRoomScene = 0;
Scene*            pScene = 0;
Texture*          pRendertargetTexture = 0;

Ptr<ShaderSet>    DistortionShaders;
Ptr<VertexDecl>   DistortionDecl;
Ptr<VertexBuffer> MeshVBs[2];
Ptr<IndexBuffer>  MeshIBs[2];
unsigned int      MeshVBCnts[2];
unsigned int      MeshIBCnts[2];
ovrVector2f       UVScaleOffset[2][2];


//-------------------------------------------------------------------------------------

int Init()
{
	ovr_Initialize();
	HMD = ovrHmd_Create(0);
	if (!HMD)
	{
		MessageBox(NULL, "Oculus Rift not detected.", "", MB_OK);
		return 1;
	}
	if (HMD->ProductName[0] == '\0')
	{
		MessageBox(NULL, "Rift detected, display not enabled.", "", MB_OK);
	}

	//Setup Window and Graphics - use window frame if relying on Oculus driver
	const int backBufferMultisample = 1;
	bool UseAppWindowFrame = true;
	HWND window = Util_InitWindowAndGraphics(Recti(HMD->WindowsPos, HMD->Resolution), 
		FullScreen, backBufferMultisample, UseAppWindowFrame, &pRender);
	if (!window) return 1;
	ovrHmd_AttachToWindow(HMD, window, NULL, NULL);

	Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(HMD, ovrEye_Left, HMD->DefaultEyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(HMD, ovrEye_Right, HMD->DefaultEyeFov[1], 1.0f);
	Sizei RenderTargetSize;
	RenderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	RenderTargetSize.h = max(recommenedTex0Size.h, recommenedTex1Size.h);

	RenderTargetSize.w = HMD->Resolution.w;
	RenderTargetSize.h = HMD->Resolution.h;

	//const int eyeRenderMultisample = 1;
	pRendertargetTexture = pRender->CreateRenderTarget(RenderTargetSize.w/2, RenderTargetSize.h/2);
	//pRendertargetTexture = pRender->CreateRenderTarget(512, 512);
	RenderTargetSize.w = pRendertargetTexture->Width;
	RenderTargetSize.h = pRendertargetTexture->Height;

	IDirect3DSurface9 *zb = 0;
	pRender->Device->GetDepthStencilSurface(&zb);
	D3DSURFACE_DESC d;
	zb->GetDesc(&d);

	// Initialize eye rendering information.
	// The viewport sizes are re-computed in case RenderTargetSize due to HW limitations.
	ovrFovPort eyeFov[2] = { HMD->DefaultEyeFov[0], HMD->DefaultEyeFov[1] };

	EyeRenderViewport[0].Pos  = Vector2i(0, 0);
	EyeRenderViewport[0].Size = Sizei(RenderTargetSize.w / 2, RenderTargetSize.h);
	EyeRenderViewport[1].Pos  = Vector2i((RenderTargetSize.w + 1) / 2, 0);
	EyeRenderViewport[1].Size = EyeRenderViewport[0].Size;

	// ---------------------

	DistortionShaders = pRender->CreateShaderSet();
	DistortionShaders->SetShader(pRender->LoadBuiltinShader(Shader_Vertex, VShader_Distortion));
	DistortionShaders->SetShader(pRender->LoadBuiltinShader(Shader_Pixel, PShader_Distortion));
	DistortionDecl = VertexDecl::GetDecl(VertexType_Distortion);

	for (int eyeNum = 0; eyeNum < 2; ++eyeNum)
	{
		ovrDistortionMesh meshData;
		ovrHmd_CreateDistortionMesh(HMD, (ovrEyeType)eyeNum, eyeFov[eyeNum],
			ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp, &meshData);
		MeshVBs[eyeNum] = pRender->CreateVertexBuffer();
		MeshVBs[eyeNum]->Data(meshData.pVertexData, sizeof(ovrDistortionVertex)*meshData.VertexCount);
		MeshIBs[eyeNum] = pRender->CreateIndexBuffer();
		MeshIBs[eyeNum]->Data(meshData.pIndexData, sizeof(unsigned short)*meshData.IndexCount);

		MeshVBCnts[eyeNum] = meshData.VertexCount;
		MeshIBCnts[eyeNum] = meshData.IndexCount;
		ovrHmd_DestroyDistortionMesh(&meshData);

		EyeRenderDesc[eyeNum] = ovrHmd_GetRenderDesc(HMD, (ovrEyeType)eyeNum, eyeFov[eyeNum]);

		ovrHmd_GetRenderScaleAndOffset(eyeFov[eyeNum], RenderTargetSize, EyeRenderViewport[eyeNum], UVScaleOffset[eyeNum]);
	}

	ovrHmd_SetEnabledCaps(HMD, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

	ovrHmd_ConfigureTracking(HMD,
		ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);

	// ---------------------

	pRoomScene = new Scene;
	PopulateRoomScene(pRoomScene, pRender);

	// texture model
	ShaderSet* ss = pRender->CreateShaderSet();
	ss->SetShader(pRender->LoadBuiltinShader(Shader_Vertex, VShader_MVP_UV));
	ss->SetShader(pRender->LoadBuiltinShader(Shader_Pixel, PShader_UV));

	Model<VertexXYZUV> *pModel2 = new Model<VertexXYZUV>();
	pModel2->Decl = VertexDecl::GetDecl(VertexType_XYZUV);
	pModel2->Fill = new ShaderFill(ss);

	//Texture* ttt = new Texture(pRender);
	//ttt->LoadFromFile("face.tga");
	pModel2->Fill->SetTexture(0, pRendertargetTexture);

	pModel2->AddVertex(VertexXYZUV(0.5f, -1.0f, 0.0f, 0.0f, 0.0f));
	pModel2->AddVertex(VertexXYZUV(2.5f, -1.0f, 0.0f, 1.0f, 0.0f));
	pModel2->AddVertex(VertexXYZUV(0.5f, 1.0f, 0.0f, 0.0f, 1.0f));
	pModel2->AddVertex(VertexXYZUV(2.5f, 1.0f, 0.0f, 1.0f, 1.0f));

	pModel2->AddVertex(VertexXYZUV(-1.0f, -1.5f, -1.0f, 0.0f, 0.0f));
	pModel2->AddVertex(VertexXYZUV(1.0f, -1.5f, -1.0f, 1.0f, 0.0f));
	pModel2->AddVertex(VertexXYZUV(-1.0f, -1.5f, 1.0f, 0.0f, 1.0f));
	pModel2->AddVertex(VertexXYZUV(1.0f, -1.5f, 1.0f, 1.0f, 1.0f));

	pModel2->AddTriangle(0, 1, 2);
	pModel2->AddTriangle(2, 1, 3);
	pModel2->AddTriangle(4, 5, 6);
	pModel2->AddTriangle(6, 5, 7);

	pScene = new Scene;
	pScene->World.Add(pModel2);

    return (0);
}

//-------------------------------------------------------------------------------------
void ProcessAndRender()
{
#if 0
	//HRESULT hr = pRender->Device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	//OVR_ASSERT(SUCCEEDED(hr));

	pRender->Clear();
	pRender->BeginScene();

	Vector3f eye(0.0f, 0.0f, -5.0f);
	Vector3f lookat(0.0f, 0.0f, 0.0f);
	Vector3f up(0.0f, 1.0f, 0.0f);
	Matrix4f view = Matrix4f::LookAtLH(eye, lookat, up);
	//Matrix4f proj = Matrix4f::PerspectiveLH(3.14145f / 2, 800.0f / 600.0f, 1.0f, 10000.0f);

	ovrFovPort fov = { 1, 1, 1, 1 };
	Matrix4f proj = ovrMatrix4f_Projection(fov, 1.0f, 10000.0f, false);

	pRender->SetProjection(proj);
	pRoomScene->Render(pRender, view);

	pRender->EndScene();
	pRender->Present();
#endif

	static ovrPosef eyeRenderPose[2];
	ovrHmd_BeginFrameTiming(HMD, 0);

	// Adjust eye position and rotation from controls, maintaining y position from HMD.
	static float BodyYaw(3.141592f);
	static Vector3f HeadPos(0.0f, 1.6f, -5.0f);
	HeadPos.y = ovrHmd_GetFloat(HMD, OVR_KEY_EYE_HEIGHT, HeadPos.y);
	bool freezeEyeRender = false;

	pRender->BeginScene();

	if (!freezeEyeRender)
	{
		pRender->SetRenderTarget(pRendertargetTexture);
		pRender->SetViewport(Recti(0, 0, pRendertargetTexture->Width, pRendertargetTexture->Height));
		pRender->Clear();
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; ++eyeIndex)
		{
			ovrEyeType eye = HMD->EyeRenderOrder[eyeIndex];
			eyeRenderPose[eye] = ovrHmd_GetEyePose(HMD, eye);

			// Get view and projection matrices
			Matrix4f rollPitchYaw = Matrix4f::RotationY(BodyYaw);
			Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(eyeRenderPose[eye].Orientation);
			Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
			Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
			Vector3f shiftedEyePos      = HeadPos + rollPitchYaw.Transform(eyeRenderPose[eye].Position);
			//Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp); 
			//Matrix4f proj = ovrMatrix4f_Projection(EyeRenderDesc[eye].Fov, 0.01f, 10000.0f, true);
			Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
			Matrix4f proj = ovrMatrix4f_Projection(EyeRenderDesc[eye].Fov, 0.01f, 10000.0f, false);

			pRender->SetViewport(Recti(EyeRenderViewport[eye]));
			pRender->SetProjection(proj);
			pRender->SetDepthMode(true, true);
			pRoomScene->Render(pRender, Matrix4f::Translation(EyeRenderDesc[eye].ViewAdjust) * view);
		}
	}

	pRender->SetDefaultRenderTarget();
	pRender->SetFullViewport();
	pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f);

	ShaderFill distortionShaderFill(DistortionShaders);
	distortionShaderFill.SetTexture(0, pRendertargetTexture);

	for (int eyeNum = 0; eyeNum < ovrEye_Count; eyeNum++)
	{
		// Get and set shader constants
		DistortionShaders->SetUniform2f("EyeToSourceUVScale", UVScaleOffset[eyeNum][0].x, UVScaleOffset[eyeNum][0].y);
		DistortionShaders->SetUniform2f("EyeToSourceUVOffset", UVScaleOffset[eyeNum][1].x, UVScaleOffset[eyeNum][1].y);
		ovrMatrix4f timeWarpMatrices[2];
		ovrHmd_GetEyeTimewarpMatrices(HMD, (ovrEyeType)eyeNum, eyeRenderPose[eyeNum], timeWarpMatrices);
		DistortionShaders->SetUniform4x4f("EyeRotationStart", timeWarpMatrices[0]);  //Nb transposed when set
		DistortionShaders->SetUniform4x4f("EyeRotationEnd", timeWarpMatrices[1]);  //Nb transposed when set
		// Perform distortion
		pRender->Render(&distortionShaderFill, DistortionDecl, MeshVBs[eyeNum], MeshIBs[eyeNum],
			sizeof(ovrDistortionVertex), Matrix4f(), MeshVBCnts[eyeNum], MeshIBCnts[eyeNum], Prim_Triangles);
		//Render(fill, vertices, indices, stride, Matrix4f(), 0,(int)vertices->GetSize(), Prim_Triangles, false);
		//(&distortionShaderFill, MeshVBs[eyeNum], MeshIBs[eyeNum],sizeof(ovrDistortionVertex));
	}

	/*
	pRender->SetDefaultRenderTarget();
	pRender->SetFullViewport();
	pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f);

	Vector3f eye(0.0f, 0.0f, -5.0f);
	Vector3f lookat(0.0f, 0.0f, 0.0f);
	Vector3f up(0.0f, 1.0f, 0.0f);
	Matrix4f view = Matrix4f::LookAtLH(eye, lookat, up);
	Matrix4f proj = Matrix4f::PerspectiveLH(3.14145f / 4, 800.0f / 600.0f, 1.0f, 10000.0f);

	pRender->Proj = proj;
	pScene->Render(pRender, view);
	*/
	//pRender->SetDefaultRenderTarget();

	pRender->EndScene();
	pRender->Present();

	//if (HMD->HmdCaps & ovrHmdCap_ExtendDesktop)
	//	pRender->WaitUntilG
	ovrHmd_EndFrameTiming(HMD);
}

//-------------------------------------------------------------------------------------

void Release(void)
{
	if (pRendertargetTexture) { pRendertargetTexture->Release(); pRendertargetTexture = NULL; }

	for (int eyeNum = 0; eyeNum < 2; ++eyeNum)
	{
		MeshVBs[eyeNum] = nullptr;
		MeshIBs[eyeNum] = nullptr;
	}

	if (DistortionShaders)
	{
		DistortionShaders->UnsetShader(Shader_Vertex);
		DistortionShaders->UnsetShader(Shader_Pixel);
		DistortionShaders = nullptr;
	}

	DistortionDecl = nullptr;

	ovrHmd_Destroy(HMD);
    Util_ReleaseWindowAndGraphics(pRender);
	if (pRoomScene) delete pRoomScene;
	if (pScene) delete pScene;

	ovr_Shutdown();
}

