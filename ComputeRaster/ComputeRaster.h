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

#include "DXFramework.h"
#include "StepTimer.h"
#include "SoftGraphicsPipeline.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().

class ComputeRaster : public DXFramework
{
public:
	ComputeRaster(uint32_t width, uint32_t height, std::wstring name);
	virtual ~ComputeRaster();

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();

	virtual void OnKeyUp(uint8_t /*key*/);
	virtual void OnLButtonDown(float posX, float posY);
	virtual void OnLButtonUp(float posX, float posY);
	virtual void OnMouseMove(float posX, float posY);
	virtual void OnMouseWheel(float deltaZ, float posX, float posY);
	virtual void OnMouseLeave();

	virtual void ParseCommandLineArgs(wchar_t* argv[], int argc);

private:
	XUSG::SwapChain			m_swapChain;
	XUSG::CommandAllocator	m_commandAllocators[SoftGraphicsPipeline::FrameCount];
	XUSG::CommandQueue		m_commandQueue;

	XUSG::Device			m_device;
	XUSG::RenderTarget		m_renderTargets[SoftGraphicsPipeline::FrameCount];
	XUSG::CommandList		m_commandList;

	// App resources.
	std::unique_ptr<SoftGraphicsPipeline> m_softGraphicsPipeline;
	XUSG::VertexBuffer	m_vb;
	XUSG::IndexBuffer	m_ib;
	XUSG::ConstantBuffer m_cbMatrices;
	XUSG::Texture2D		m_colorTarget;
	SoftGraphicsPipeline::DepthBuffer m_depth;
	XMFLOAT4X4			m_proj;
	XMFLOAT4X4	m_view;
	XMFLOAT3	m_focusPt;
	XMFLOAT3	m_eyePt;

	uint32_t	m_numIndices;

	XUSG::DescriptorTable m_cbvTables[SoftGraphicsPipeline::FrameCount];

	// Synchronization objects.
	uint32_t	m_frameIndex;
	HANDLE		m_fenceEvent;
	XUSG::Fence	m_fence;
	uint64_t	m_fenceValues[SoftGraphicsPipeline::FrameCount];

	// Application state
	bool		m_showFPS;
	bool		m_pausing;
	StepTimer	m_timer;

	// User camera interactions
	bool m_tracking;
	XMFLOAT2 m_mousePt;

	// User external settings
	std::string m_meshFileName;
	XMFLOAT4 m_meshPosScale;

	void LoadPipeline();
	void LoadAssets();

	void PopulateCommandList();
	void WaitForGpu();
	void MoveToNextFrame();
	double CalculateFrameStats(float* fTimeStep = nullptr);
};
