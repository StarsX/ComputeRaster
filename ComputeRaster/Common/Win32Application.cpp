//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include <windowsx.h>
#include "Win32Application.h"

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(DXFramework *pFramework, HINSTANCE hInstance, int nCmdShow, HICON hIcon)
{
	// Parse the command line parameters
	int argc;
	const auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	pFramework->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXFrameworkClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<long>(pFramework->GetWidth()), static_cast<long>(pFramework->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		pFramework->GetTitle(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// We have no parent window.
		nullptr,		// We aren't using menus.
		hInstance,
		pFramework);

	if (hIcon)
	{
		SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	}

	// Initialize the sample. OnInit is defined in each child-implementation of DXSample.
	pFramework->OnInit();

	ShowWindow(m_hwnd, nCmdShow);

	// Main sample loop.
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	pFramework->OnDestroy();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

// Main message handler for the sample.
LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;
	// Set s_fullscreen to true if defaulting to fullscreen.

	static uint32_t s_width = 0;
	static uint32_t s_height = 0;

	const auto pFramework = reinterpret_cast<DXFramework*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
		{
			// Save the DXSample* passed in to CreateWindow.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

	case WM_MOVE:
		if (pFramework)
			pFramework->OnWindowMoved();
		return 0;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			if (!s_minimized)
			{
				s_minimized = true;
				if (!s_in_suspend && pFramework)
					pFramework->OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (s_minimized)
		{
			s_minimized = false;
			if (s_in_suspend && pFramework)
				pFramework->OnResuming();
			s_in_suspend = false;
		}
		else if (!s_in_sizemove && pFramework)
		{
			s_width = LOWORD(lParam);
			s_height = HIWORD(lParam);
			pFramework->OnWindowSizeChanged(s_width, s_height);
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		return 0;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (pFramework)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);

			const auto w = rc.right - rc.left;
			const auto h = rc.bottom - rc.top;

			if (s_width != w || s_height != h)
			{
				pFramework->OnWindowSizeChanged(w, h);
				s_width = w;
				s_height = h;
			}
		}
		return 0;

	case WM_KEYDOWN:
		if (pFramework) pFramework->OnKeyDown(static_cast<uint8_t>(wParam));
		return 0;

	case WM_KEYUP:
		if (pFramework) pFramework->OnKeyUp(static_cast<uint8_t>(wParam));
		return 0;

	case WM_LBUTTONDOWN:
		if (pFramework)
			pFramework->OnLButtonDown(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
		return 0;

	case WM_LBUTTONUP:
		if (pFramework)
			pFramework->OnLButtonUp(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
		return 0;

	case WM_RBUTTONDOWN:
		if (pFramework)
			pFramework->OnRButtonDown(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
		return 0;

	case WM_RBUTTONUP:
		if (pFramework)
			pFramework->OnRButtonUp(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
		return 0;

	case WM_MOUSEMOVE:
		if (pFramework)
			pFramework->OnMouseMove(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
		{
			TRACKMOUSEEVENT csTME = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd };
			TrackMouseEvent(&csTME);
		}
		return 0;

	case WM_MOUSELEAVE:
		if (pFramework) pFramework->OnMouseLeave();
		return 0;

	case WM_MOUSEWHEEL:
		if (pFramework) pFramework->OnMouseWheel(static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA,
			static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
		return 0;

	case WM_PAINT:
		if (pFramework)
		{
			pFramework->OnUpdate();
			pFramework->OnRender();
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}
