// RenderTester.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RenderTester.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst = NULL;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

HMODULE g_hModuleRender = NULL;
HWND g_hWnd = (HWND) INVALID_HANDLE_VALUE;
HWND g_hwndScreen = (HWND) INVALID_HANDLE_VALUE;
void * g_Screen = NULL;
TCHAR g_RenderDllName[32];

// Forward declarations of functions included in this code module:
BOOL				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	ScreenWndProc(HWND, UINT, WPARAM, LPARAM);


void DoCommandSelectRender(LPCTSTR renderDllName);


void AlertWarning(LPCTSTR sMessage)
{
    ::MessageBox(NULL, sMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
}
void AlertWarningFormat(LPCTSTR sFormat, ...)
{
    TCHAR buffer[512];

    va_list ptr;
    va_start(ptr, sFormat);
    _vsntprintf_s(buffer, 512, 512 - 1, sFormat, ptr);
    va_end(ptr);

    ::MessageBox(NULL, buffer, szTitle, MB_OK | MB_ICONEXCLAMATION);
}

RENDER_INIT_CALLBACK RenderInitProc = NULL;
RENDER_DONE_CALLBACK RenderDoneProc = NULL;
RENDER_DRAW_CALLBACK RenderDrawProc = NULL;

void CreateScreen()
{
    RECT rc;  ::GetClientRect(g_hWnd, &rc);

    g_hwndScreen = CreateWindow(_T("ScreenWindow"), NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, rc.right, rc.bottom, g_hWnd, NULL, hInst, NULL);
}
void DestroyScreen()
{
    ::DestroyWindow(g_hwndScreen);
    g_hwndScreen = (HWND) INVALID_HANDLE_VALUE;
}

BOOL InitRender(LPCTSTR szRenderLibraryName)
{
    g_hModuleRender = ::LoadLibrary(szRenderLibraryName);
    if (g_hModuleRender == NULL)
    {
        AlertWarningFormat(_T("Failed to load render library \"%s\" (0x%08lx)."),
            szRenderLibraryName, ::GetLastError());
        return FALSE;
    }

    RenderInitProc = (RENDER_INIT_CALLBACK) ::GetProcAddress(g_hModuleRender, "RenderInit");
    if (RenderInitProc == NULL)
    {
        AlertWarningFormat(_T("Failed to retrieve RenderInit address (0x%08lx)."), ::GetLastError());
        return FALSE;
    }
    RenderDoneProc = (RENDER_DONE_CALLBACK) ::GetProcAddress(g_hModuleRender, "RenderDone");
    if (RenderDoneProc == NULL)
    {
        AlertWarningFormat(_T("Failed to retrieve RenderDone address (0x%08lx)."), ::GetLastError());
        return FALSE;
    }
    RenderDrawProc = (RENDER_DRAW_CALLBACK) ::GetProcAddress(g_hModuleRender, "RenderDraw");
    if (RenderDrawProc == NULL)
    {
        AlertWarningFormat(_T("Failed to retrieve RenderDraw address (0x%08lx)."), ::GetLastError());
        return FALSE;
    }

    CreateScreen();
    
    if (!RenderInitProc(SCREEN_WIDTH, SCREEN_HEIGHT, g_hwndScreen))
    {
        AlertWarning(_T("Failed to initialize the render."));
        DestroyScreen();
        return FALSE;
    }

    return TRUE;
}

void DoneRender()
{
    if (g_hModuleRender != NULL)
    {
        if (RenderDoneProc != NULL)
            RenderDoneProc();

        RenderInitProc = NULL;
        RenderDoneProc = NULL;
        RenderDrawProc = NULL;

        ::FreeLibrary(g_hModuleRender);
        g_hModuleRender = NULL;

        DestroyScreen();
    }
}

void UpdateWindowTitle()
{
    RECT rc;  GetClientRect(g_hWnd, &rc);
    TCHAR buffer[120];
    wsprintf(buffer, _T("RenderTester - %s - %d x %d"), g_RenderDllName, rc.right, rc.bottom);
    ::SetWindowText(g_hWnd, buffer);
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_RENDERTESTER, szWindowClass, MAX_LOADSTRING);
	if (!MyRegisterClass(hInstance))
        return FALSE;

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
		return FALSE;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RENDERTESTER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    DoneRender();

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
BOOL MyRegisterClass(HINSTANCE hInstance)
{
    {
	    WNDCLASSEX wcex;
	    wcex.cbSize = sizeof(WNDCLASSEX);
	    wcex.style			= CS_HREDRAW | CS_VREDRAW;
	    wcex.lpfnWndProc	= WndProc;
	    wcex.cbClsExtra		= 0;
	    wcex.cbWndExtra		= 0;
	    wcex.hInstance		= hInstance;
	    wcex.hIcon			= 0; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RENDERTESTER));
	    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	    wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	    wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RENDERTESTER);
	    wcex.lpszClassName	= szWindowClass;
	    wcex.hIconSm		= 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    
        if (!RegisterClassEx(&wcex))
            return FALSE;
    }
    {
	    WNDCLASSEX wcex;
	    wcex.cbSize = sizeof(WNDCLASSEX);
	    wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	    wcex.lpfnWndProc	= ScreenWndProc;
	    wcex.cbClsExtra		= 0;
	    wcex.cbWndExtra		= 0;
	    wcex.hInstance		= hInstance;
	    wcex.hIcon			= 0;
	    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	    wcex.hbrBackground	= NULL;
	    wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RENDERTESTER);
	    wcex.lpszClassName	= _T("ScreenWindow");
	    wcex.hIconSm		= 0;

        if (!RegisterClassEx(&wcex))
            return FALSE;
    }
    return TRUE;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (!g_hWnd)
        return FALSE;

    g_Screen = ::malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);

    FILE* fpFile = ::_tfopen(_T("test.scr"), _T("rb"));
    if (fpFile == NULL)
    {
        ::AlertWarning(_T("Failed to open test.scr file."));
    }
    ::fread(g_Screen, 1, SCREEN_WIDTH * SCREEN_HEIGHT * 4, fpFile);
    ::fclose(fpFile);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
        case ID_FILE_RESIZE1:
            {
                int cx = ::GetSystemMetrics(SM_CXFRAME) * 2 + 640;
                int cy = ::GetSystemMetrics(SM_CYFRAME) * 2 + 288 + ::GetSystemMetrics(SM_CYMENU) + ::GetSystemMetrics(SM_CYCAPTION);
                ::SetWindowPos(g_hWnd, NULL, 0,0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);

                UpdateWindowTitle();
            }
            break;
        case ID_FILE_RESIZE2:
            {
                int cx = ::GetSystemMetrics(SM_CXFRAME) * 2 + (640 + 320);
                int cy = ::GetSystemMetrics(SM_CYFRAME) * 2 + 288 * 2 + ::GetSystemMetrics(SM_CYMENU) + ::GetSystemMetrics(SM_CYCAPTION);
                ::SetWindowPos(g_hWnd, NULL, 0,0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);

                UpdateWindowTitle();
            }
            break;
        case ID_RENDER_VFW:
            DoCommandSelectRender(_T("RenderVfw.dll"));
			break;
        case ID_RENDER_DX9:
            DoCommandSelectRender(_T("RenderDX9.dll"));
            break;
        case ID_RENDER_OPENGL:
            DoCommandSelectRender(_T("RenderOpenGL.dll"));
            break;
        case ID_RENDER_UNLOAD:
            DoCommandSelectRender(NULL);
            break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
    case WM_SIZE:
        {
            RECT rc;  ::GetClientRect(g_hWnd, &rc);
            ::SetWindowPos(g_hwndScreen, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOZORDER);

            UpdateWindowTitle();
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK ScreenWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

   	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
        if (RenderDrawProc == NULL)
        {
            ::PatBlt(ps.hdc,
                ps.rcPaint.left, ps.rcPaint.top,
                ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
                WHITENESS);
        }
        else
        {
            RenderDrawProc(g_Screen, ps.hdc);
        }
		EndPaint(hWnd, &ps);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void DoCommandSelectRender(LPCTSTR renderDllName)
{
    DoneRender();
    g_RenderDllName[0] = 0;

    if (renderDllName != NULL)
    {
        if (!InitRender(renderDllName))
            AlertWarningFormat(_T("Failed to initialize render %s"), renderDllName);

        wcscpy(g_RenderDllName, renderDllName);

        ::InvalidateRect(g_hwndScreen, NULL, TRUE);
    }

    UpdateWindowTitle();
}
