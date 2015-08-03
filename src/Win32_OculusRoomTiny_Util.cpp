#include "RenderTiny_D3D9_Device.h"

// Win32 System Variables
HWND                hWnd = NULL;
HINSTANCE           hInstance;    
POINT               WindowCenter; 

// User inputs
bool                Quit = 0;

// Functions from Win32_OculusRoomTiny.cpp
int     Init();
void    ProcessAndRender();
void    Release();


//-------------------------------------------------------------------------------------

void OnKey(unsigned vk, bool down)
{
}

void OnMouseMove(int x)
{
}

LRESULT CALLBACK systemWindowProc(HWND arg_hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
    {
		case(WM_NCCREATE):  hWnd = arg_hwnd; break;
#if 0
		case WM_MOUSEMOVE:	{
							// Convert mouse motion to be relative
							// (report the offset and re-center).
							POINT newPos = { LOWORD(lp), HIWORD(lp) };
							::ClientToScreen(hWnd, &newPos);
							if ((newPos.x == WindowCenter.x) && (newPos.y == WindowCenter.y))
								break;
							::SetCursorPos(WindowCenter.x, WindowCenter.y);
							OnMouseMove(newPos.x - WindowCenter.x);
							break;
							}

		case WM_MOVE:		RECT r;
							GetClientRect(hWnd, &r);
							WindowCenter.x = r.right/2;
							WindowCenter.y = r.bottom/2;
							::ClientToScreen(hWnd, &WindowCenter);
							break;

		case WM_KEYDOWN:    OnKey((unsigned)wp, true);    break;
		case WM_KEYUP:      OnKey((unsigned)wp, false);   break;
		case WM_CREATE:     SetTimer(hWnd, 0, 100, NULL); break;
		case WM_TIMER:      KillTimer(hWnd, 0);

		case WM_SETFOCUS:   
							SetCursorPos(WindowCenter.x, WindowCenter.y);
							SetCapture(hWnd);
							ShowCursor(FALSE);
							break;
	   case WM_KILLFOCUS:
							ReleaseCapture();
							ShowCursor(TRUE);
							break;
#endif
		case WM_QUIT:
		case WM_CLOSE:      Quit = true;
                            return 0;
    }

    return DefWindowProc(hWnd, msg, wp, lp);
}

HWND Util_InitWindowAndGraphics(Recti vp, int fullscreen, int multiSampleCount, bool UseAppWindowFrame, RenderDevice **returnedDevice)
{
	RendererParams renderParams;

	// Window
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpszClassName = "OVRAppWindow";
	wc.style = CS_OWNDC;
    wc.lpfnWndProc   = systemWindowProc;
    wc.cbWndExtra    = NULL;
	//wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    DWORD wsStyle = WS_POPUP;
    DWORD sizeDivisor = 1;

	if (UseAppWindowFrame)
	{
		// If using our driver, displaya window frame with a smaller window.
		// Original HMD resolution is still passed into the renderer for proper swap chain.
		wsStyle |= WS_OVERLAPPEDWINDOW;
		renderParams.Resolution = vp.GetSize();
		sizeDivisor = 2;
		renderParams.Resolution /= sizeDivisor;
	}

	RECT winSize = { 0, 0, vp.w / sizeDivisor, vp.h / sizeDivisor };
    AdjustWindowRect(&winSize, wsStyle, false);
    hWnd = CreateWindowW(L"OVRAppWindow", L"OculusRoomTinyDX9",
                         wsStyle |WS_VISIBLE,
		                 10, 10,
                         winSize.right-winSize.left, winSize.bottom-winSize.top,
                         NULL, NULL, hInstance, NULL);

	POINT center = { vp.w / 2 / sizeDivisor, vp.h / 2 / sizeDivisor};
	::ClientToScreen(hWnd, &center);
	WindowCenter = center;

	if (!hWnd) return(NULL);

	renderParams.Multisample = multiSampleCount;
	renderParams.Fullscreen = fullscreen;

	*returnedDevice = RenderDevice::CreateDevice(renderParams, (void*)hWnd);

    return(hWnd);
}


void Util_ReleaseWindowAndGraphics(RenderDevice * pRender)
{    
	if (pRender)
		pRender->Release();

    if (hWnd)
    {
        // Release window resources.
        ::DestroyWindow(hWnd);
        UnregisterClass("OVRAppWindow", hInstance);
    }
}


//-------------------------------------------------------------------------------------
// ***** Program Startup
// 
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR , int)
{
	hInstance = hinst;

    if (!Init())
    {
        // Processes messages and calls OnIdle() to do rendering.
	   while (!Quit)
		{
			MSG msg;
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				ProcessAndRender();

				// Keep sleeping when we're minimized.
				if (IsIconic(hWnd)) Sleep(10);
			}
		}
    }
  	Release();
    //OVR_ASSERT(!_CrtDumpMemoryLeaks());
    return (0);
}

