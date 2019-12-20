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

#pragma once

#include "DXFrameworkHelper.h"
#include "Win32Application.h"

class DXFramework
{
public:
	DXFramework(uint32_t width, uint32_t height, std::wstring name);
	virtual ~DXFramework();

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	virtual void OnSuspending() {}
	virtual void OnResuming() {}
	virtual void OnWindowMoved() {}
	virtual void OnWindowSizeChanged(int width, int height) {}

	// Samples override the event handlers to handle specific messages.
	virtual void OnKeyDown(uint8_t /*key*/)   {}
	virtual void OnKeyUp(uint8_t /*key*/)     {}

	virtual void OnLButtonDown(float posX, float posY) {}
	virtual void OnLButtonUp(float posX, float posY) {}
	virtual void OnRButtonDown(float posX, float posY) {}
	virtual void OnRButtonUp(float posX, float posY) {}
	virtual void OnMouseMove(float posX, float posY) {}
	virtual void OnMouseWheel(float deltaZ, float posX, float posY) {}
	virtual void OnMouseLeave() {}

	// Accessors.
	uint32_t GetWidth() const		{ return m_width; }
	uint32_t GetHeight() const		{ return m_height; }
	const WCHAR* GetTitle() const	{ return m_title.c_str(); }

	virtual void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

protected:
	std::wstring GetAssetFullPath(LPCWSTR assetName);
	void GetHardwareAdapter(_In_ IDXGIFactory2* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);
	void SetCustomWindowText(LPCWSTR text);

	// Viewport dimensions.
	uint32_t m_width;
	uint32_t m_height;
	float m_aspectRatio;

	// Window title.
	std::wstring m_title;

private:
	// Root assets path.
	std::wstring m_assetsPath;
};
