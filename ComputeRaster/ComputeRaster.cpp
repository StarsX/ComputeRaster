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

#include "Optional/XUSGObjLoader.h"
#include "ComputeRaster.h"

using namespace std;
using namespace XUSG;

ComputeRaster::ComputeRaster(uint32_t width, uint32_t height, std::wstring name) :
	DXFramework(width, height, name),
	m_numIndices(3),
	m_frameIndex(0),
	m_showFPS(true),
	m_pausing(false),
	m_tracking(false),
	m_meshFileName("Media/bunny.obj"),
	m_meshPosScale(0.0f, 0.0f, 0.0f, 1.0f)
{
#if defined (_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONIN$", "r+t", stdin);
#endif
}

ComputeRaster::~ComputeRaster()
{
#if defined (_DEBUG)
	FreeConsole();
#endif
}

void ComputeRaster::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

// Load the rendering pipeline dependencies.
void ComputeRaster::LoadPipeline()
{
	auto dxgiFactoryFlags = 0u;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		com_ptr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	com_ptr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
	com_ptr<IDXGIAdapter1> dxgiAdapter;
	auto hr = DXGI_ERROR_UNSUPPORTED;
	for (auto i = 0u; hr == DXGI_ERROR_UNSUPPORTED; ++i)
	{
		dxgiAdapter = nullptr;
		ThrowIfFailed(factory->EnumAdapters1(i, &dxgiAdapter));
		hr = D3D12CreateDevice(dxgiAdapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
	}

	dxgiAdapter->GetDesc1(&dxgiAdapterDesc);
	if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		m_title += dxgiAdapterDesc.VendorId == 0x1414 && dxgiAdapterDesc.DeviceId == 0x8c ? L" (WARP)" : L" (Software)";
	ThrowIfFailed(hr);

	// Create the command queue.
	N_RETURN(m_device->GetCommandQueue(m_commandQueue, CommandListType::DIRECT, CommandQueueFlags::NONE), ThrowIfFailed(E_FAIL));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = SoftGraphicsPipeline::FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	com_ptr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.get(),		// Swap chain needs the queue so that it can force a flush on it.
		Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create frame resources.
	// Create a RTV and a command allocator for each frame.
	for (auto n = 0u; n < SoftGraphicsPipeline::FrameCount; n++)
	{
		N_RETURN(m_renderTargets[n].CreateFromSwapChain(m_device, m_swapChain, n), ThrowIfFailed(E_FAIL));
		N_RETURN(m_device->GetCommandAllocator(m_commandAllocators[n], CommandListType::DIRECT), ThrowIfFailed(E_FAIL));
	}

	// Create Color target
	N_RETURN(m_colorTarget.Create(m_device, m_width, m_height, Format::R8G8B8A8_UNORM, 1,
		ResourceFlag::ALLOW_UNORDERED_ACCESS), ThrowIfFailed(E_FAIL));

	// Create a DSV
	N_RETURN(m_depth.Create(m_device, m_width, m_height, Format::D24_UNORM_S8_UINT,
		ResourceFlag::DENY_SHADER_RESOURCE), ThrowIfFailed(E_FAIL));
}

// Load the sample assets.
void ComputeRaster::LoadAssets()
{
	// Create the command list.
	N_RETURN(m_device->GetCommandList(m_commandList.GetCommandList(), 0, CommandListType::DIRECT,
		m_commandAllocators[m_frameIndex], nullptr), ThrowIfFailed(E_FAIL));

	m_softGraphicsPipeline = make_unique<SoftGraphicsPipeline>(m_device);
	if (!m_softGraphicsPipeline) ThrowIfFailed(E_FAIL);

	vector<Resource> uploaders(0);
	if (!m_softGraphicsPipeline->Init(m_commandList, m_width, m_height, uploaders))
		ThrowIfFailed(E_FAIL);

	Util::PipelineLayout pipelineLayout;
	pipelineLayout.SetRange(0, DescriptorType::CBV, 1, 0);
	if (!m_softGraphicsPipeline->CreateVertexShaderLayout(pipelineLayout, 1))
		ThrowIfFailed(E_FAIL);

	// Create constant buffer
	const auto& frameCount = SoftGraphicsPipeline::FrameCount;
	if (!m_cbMatrices.Create(m_device, sizeof(XMFLOAT4X4) * frameCount, frameCount))
		ThrowIfFailed(E_FAIL);
	for (auto i = 0u; i < SoftGraphicsPipeline::FrameCount; ++i)
	{
		Util::DescriptorTable utilCbvTable;
		utilCbvTable.SetDescriptors(0, 1, &m_cbMatrices.GetCBV(i));
		m_cbvTables[i] = utilCbvTable.GetCbvSrvUavTable(m_softGraphicsPipeline->GetDescriptorTableCache());
	}

#if 1
	// Load inputs
	ObjLoader objLoader;
	if (!objLoader.Import(m_meshFileName.c_str(), true, true)) ThrowIfFailed(E_FAIL);
	if (!m_softGraphicsPipeline->CreateVertexBuffer(m_commandList, m_vb, uploaders,
		objLoader.GetVertices(), objLoader.GetNumVertices(), objLoader.GetVertexStride()))
		ThrowIfFailed(E_FAIL);
	m_numIndices = objLoader.GetNumIndices();
	if (!m_softGraphicsPipeline->CreateIndexBuffer(m_commandList, m_ib,
		uploaders, objLoader.GetIndices(), m_numIndices, Format::R32_UINT))
		ThrowIfFailed(E_FAIL);
#else
	const float vbData[] =
	{
		0.0f, 9.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		5.0f, -1.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		-5.0f, -1.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
	};
	if (!m_softGraphicsPipeline->CreateVertexBuffer(m_commandList, m_vb,
		uploaders, vbData, 3, sizeof(float[6])))
		ThrowIfFailed(E_FAIL);

	const uint16_t ibData[] = { 0, 1, 2 };
	m_numIndices = 3;
	if (!m_softGraphicsPipeline->CreateIndexBuffer(m_commandList, m_ib,
		uploaders, ibData, m_numIndices, Format::R16_UINT))
		ThrowIfFailed(E_FAIL);
#endif

	// Close the command list and execute it to begin the initial GPU setup.
	ThrowIfFailed(m_commandList.Close());
	BaseCommandList* const ppCommandLists[] = { m_commandList.GetCommandList().get() };
	m_commandQueue->ExecuteCommandLists(static_cast<uint32_t>(size(ppCommandLists)), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		N_RETURN(m_device->GetFence(m_fence, m_fenceValues[m_frameIndex]++, FenceFlag::NONE), ThrowIfFailed(E_FAIL));

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForGpu();
	}

	// Projection
	const auto aspectRatio = m_width / static_cast<float>(m_height);
	const auto proj = XMMatrixPerspectiveFovLH(g_FOVAngleY, aspectRatio, g_zNear, g_zFar);
	XMStoreFloat4x4(&m_proj, proj);

	// View initialization
	m_focusPt = XMFLOAT3(0.0f, 4.0f, 0.0f);
	m_eyePt = XMFLOAT3(4.0f, 6.0f, -20.0f);
	const auto focusPt = XMLoadFloat3(&m_focusPt);
	const auto eyePt = XMLoadFloat3(&m_eyePt);
	const auto view = XMMatrixLookAtLH(eyePt, focusPt, XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
	XMStoreFloat4x4(&m_view, view);
}

// Update frame-based values.
void ComputeRaster::OnUpdate()
{
	// Timer
	static auto time = 0.0, pauseTime = 0.0;

	m_timer.Tick();
	const auto totalTime = CalculateFrameStats();
	pauseTime = m_pausing ? totalTime - time : pauseTime;
	time = totalTime - pauseTime;

	// View
	const auto eyePt = XMLoadFloat3(&m_eyePt);
	const auto view = XMLoadFloat4x4(&m_view);
	const auto proj = XMLoadFloat4x4(&m_proj);
	//m_softGraphicsPipeline->UpdateFrame(m_frameIndex, eyePt, view * proj);
	{
		const auto pCb = reinterpret_cast<XMFLOAT4X4*>(m_cbMatrices.Map(m_frameIndex));
		const auto world = XMMatrixScaling(m_meshPosScale.w, m_meshPosScale.w, m_meshPosScale.w) *
			XMMatrixTranslation(m_meshPosScale.x, m_meshPosScale.y, m_meshPosScale.z);
		XMStoreFloat4x4(pCb, XMMatrixTranspose(world * view * proj));
	}
}

// Render the scene.
void ComputeRaster::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	BaseCommandList* const ppCommandLists[] = { m_commandList.GetCommandList().get() };
	m_commandQueue->ExecuteCommandLists(static_cast<uint32_t>(size(ppCommandLists)), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(0, 0));

	MoveToNextFrame();
}

void ComputeRaster::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForGpu();

	CloseHandle(m_fenceEvent);
}

// User hot-key interactions.
void ComputeRaster::OnKeyUp(uint8_t key)
{
	switch (key)
	{
	case 0x20:	// case VK_SPACE:
		m_pausing = !m_pausing;
		break;
	case 0x70:	//case VK_F1:
		m_showFPS = !m_showFPS;
		break;
	}
}

// User camera interactions.
void ComputeRaster::OnLButtonDown(float posX, float posY)
{
	m_tracking = true;
	m_mousePt = XMFLOAT2(posX, posY);
}

void ComputeRaster::OnLButtonUp(float posX, float posY)
{
	m_tracking = false;
}

void ComputeRaster::OnMouseMove(float posX, float posY)
{
	if (m_tracking)
	{
		const auto dPos = XMFLOAT2(m_mousePt.x - posX, m_mousePt.y - posY);

		XMFLOAT2 radians;
		radians.x = XM_2PI * dPos.y / m_height;
		radians.y = XM_2PI * dPos.x / m_width;

		const auto focusPt = XMLoadFloat3(&m_focusPt);
		auto eyePt = XMLoadFloat3(&m_eyePt);

		const auto len = XMVectorGetX(XMVector3Length(focusPt - eyePt));
		auto transform = XMMatrixTranslation(0.0f, 0.0f, -len);
		transform *= XMMatrixRotationRollPitchYaw(radians.x, radians.y, 0.0f);
		transform *= XMMatrixTranslation(0.0f, 0.0f, len);

		const auto view = XMLoadFloat4x4(&m_view) * transform;
		const auto viewInv = XMMatrixInverse(nullptr, view);
		eyePt = viewInv.r[3];

		XMStoreFloat3(&m_eyePt, eyePt);
		XMStoreFloat4x4(&m_view, view);

		m_mousePt = XMFLOAT2(posX, posY);
	}
}

void ComputeRaster::OnMouseWheel(float deltaZ, float posX, float posY)
{
	const auto focusPt = XMLoadFloat3(&m_focusPt);
	auto eyePt = XMLoadFloat3(&m_eyePt);

	const auto len = XMVectorGetX(XMVector3Length(focusPt - eyePt));
	const auto transform = XMMatrixTranslation(0.0f, 0.0f, -len * deltaZ / 16.0f);

	const auto view = XMLoadFloat4x4(&m_view) * transform;
	const auto viewInv = XMMatrixInverse(nullptr, view);
	eyePt = viewInv.r[3];

	XMStoreFloat3(&m_eyePt, eyePt);
	XMStoreFloat4x4(&m_view, view);
}

void ComputeRaster::OnMouseLeave()
{
	m_tracking = false;
}

void ComputeRaster::ParseCommandLineArgs(wchar_t* argv[], int argc)
{
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	DXFramework::ParseCommandLineArgs(argv, argc);

	for (auto i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-mesh", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/mesh", wcslen(argv[i])) == 0)
		{
			if (i + 1 < argc) m_meshFileName = converter.to_bytes(argv[i + 1]);
			m_meshPosScale.x = i + 2 < argc ? static_cast<float>(_wtof(argv[i + 2])) : m_meshPosScale.x;
			m_meshPosScale.y = i + 3 < argc ? static_cast<float>(_wtof(argv[i + 3])) : m_meshPosScale.y;
			m_meshPosScale.z = i + 4 < argc ? static_cast<float>(_wtof(argv[i + 4])) : m_meshPosScale.z;
			m_meshPosScale.w = i + 5 < argc ? static_cast<float>(_wtof(argv[i + 5])) : m_meshPosScale.w;
			break;
		}
	}
}

void ComputeRaster::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList.Reset(m_commandAllocators[m_frameIndex], nullptr));

	ResourceBarrier barriers[2];
	auto numBarriers = m_colorTarget.SetBarrier(barriers, ResourceState::UNORDERED_ACCESS);
	m_commandList.Barrier(numBarriers, barriers);

	// Record commands.
	{
		//const float clearColor[] = { CLEAR_COLOR, 0.0f };
		//m_commandList.ClearRenderTargetView(m_renderTargets[m_frameIndex].GetRTV(), clearColor);
	}
	//m_commandList.ClearDepthStencilView(m_depth.GetDSV(), ClearFlag::DEPTH, 1.0f);

	// Compute raster rendering
	const float clearColor[] = { CLEAR_COLOR, 0.0f };
	m_softGraphicsPipeline->SetRenderTargets(m_colorTarget);
	m_softGraphicsPipeline->Clear(m_colorTarget, clearColor);
	m_softGraphicsPipeline->SetVertexBuffer(m_vb.GetSRV());
	m_softGraphicsPipeline->SetIndexBuffer(m_ib.GetSRV());
	m_softGraphicsPipeline->VSSetDescriptorTable(0, m_cbvTables[m_frameIndex]);
	m_softGraphicsPipeline->DrawIndexed(m_commandList, m_numIndices);
	//m_softGraphicsPipeline->Draw(m_commandList, 3);

	{
		const TextureCopyLocation dst(m_renderTargets[m_frameIndex].GetResource().get(), 0);
		const TextureCopyLocation src(m_colorTarget.GetResource().get(), 0);

		numBarriers = m_renderTargets[m_frameIndex].SetBarrier(barriers, ResourceState::COPY_DEST);
		numBarriers = m_colorTarget.SetBarrier(barriers, ResourceState::COPY_SOURCE, numBarriers);
		m_commandList.Barrier(numBarriers, barriers);

		m_commandList.CopyTextureRegion(dst, 0, 0, 0, src);

		// Indicate that the back buffer will now be used to present.
		numBarriers = m_renderTargets[m_frameIndex].SetBarrier(barriers, ResourceState::PRESENT);
		m_commandList.Barrier(numBarriers, barriers);
	}

	ThrowIfFailed(m_commandList.Close());
}

// Wait for pending GPU work to complete.
void ComputeRaster::WaitForGpu()
{
	// Schedule a Signal command in the queue.
	ThrowIfFailed(m_commandQueue->Signal(m_fence.get(), m_fenceValues[m_frameIndex]));

	// Wait until the fence has been processed.
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_fenceValues[m_frameIndex]++;
}

// Prepare to render the next frame.
void ComputeRaster::MoveToNextFrame()
{
	// Schedule a Signal command in the queue.
	const auto currentFenceValue = m_fenceValues[m_frameIndex];
	ThrowIfFailed(m_commandQueue->Signal(m_fence.get(), currentFenceValue));

	// Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

double ComputeRaster::CalculateFrameStats(float* pTimeStep)
{
	static int frameCnt = 0;
	static double elapsedTime = 0.0;
	static double previousTime = 0.0;
	const auto totalTime = m_timer.GetTotalSeconds();
	++frameCnt;

	const auto timeStep = static_cast<float>(totalTime - elapsedTime);

	// Compute averages over one second period.
	if ((totalTime - elapsedTime) >= 1.0f)
	{
		float fps = static_cast<float>(frameCnt) / timeStep;	// Normalize to an exact second.

		frameCnt = 0;
		elapsedTime = totalTime;

		wstringstream windowText;
		windowText << L"    fps: ";
		if (m_showFPS) windowText << setprecision(2) << fixed << fps;
		else windowText << L"[F1]";
		SetCustomWindowText(windowText.str().c_str());
	}

	if (pTimeStep)* pTimeStep = static_cast<float>(totalTime - previousTime);
	previousTime = totalTime;

	return totalTime;
}
