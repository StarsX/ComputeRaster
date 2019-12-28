//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Optional/XUSGObjLoader.h"
#include "Renderer.h"

using namespace std;
using namespace DirectX;
using namespace XUSG;

Renderer::Renderer(const Device& device) :
	m_device(device)
{
}

Renderer::~Renderer()
{
}

bool Renderer::Init(const CommandList& commandList, uint32_t width,
	uint32_t height, vector<Resource>& uploaders, const char* fileName,
	const XMFLOAT4& posScale)
{
	m_viewport.x = static_cast<float>(width);
	m_viewport.y = static_cast<float>(height);
	m_posScale = posScale;

	X_RETURN(m_softGraphicsPipeline, make_unique<SoftGraphicsPipeline>(m_device), false);
	N_RETURN(m_softGraphicsPipeline->Init(commandList, uploaders), false);

	// Create Color target
	N_RETURN(m_colorTarget.Create(m_device, width, height, Format::R8G8B8A8_UNORM, 1,
		ResourceFlag::ALLOW_UNORDERED_ACCESS), false);

	// Create depth buffer
	N_RETURN(m_softGraphicsPipeline->CreateDepthBuffer(m_depth, width,
		height, Format::R32_UINT), false);
	
	{
		Util::PipelineLayout pipelineLayout;
		pipelineLayout.SetRange(0, DescriptorType::CBV, 1, 0);
		m_softGraphicsPipeline->SetAttribute(0, sizeof(uint32_t[4]), Format::R32G32B32A32_FLOAT, L"Normal");
		N_RETURN(m_softGraphicsPipeline->CreateVertexShaderLayout(pipelineLayout, 1), false);
	}

	{
		Util::PipelineLayout pipelineLayout;
		pipelineLayout.SetRange(0, DescriptorType::CBV, 1, 0);
		pipelineLayout.SetRange(1, DescriptorType::CBV, 1, 1);
		N_RETURN(m_softGraphicsPipeline->CreatePixelShaderLayout(pipelineLayout, 2, 1), false);
	}

	// Create constant buffers
	const auto& frameCount = SoftGraphicsPipeline::FrameCount;
	N_RETURN(m_cbMatrices.Create(m_device, sizeof(XMFLOAT4X4[2]) * frameCount, frameCount), false);
	for (auto i = 0u; i < SoftGraphicsPipeline::FrameCount; ++i)
	{
		Util::DescriptorTable utilCbvTable;
		utilCbvTable.SetDescriptors(0, 1, &m_cbMatrices.GetCBV(i));
		m_cbvTables[CBV_TABLE_MATRICES + i] = utilCbvTable.GetCbvSrvUavTable(m_softGraphicsPipeline->GetDescriptorTableCache());
	}

	// Per-frame lighting
	N_RETURN(m_cbLighting.Create(m_device, sizeof(XMFLOAT4[3]) * frameCount, frameCount), false);
	for (auto i = 0u; i < SoftGraphicsPipeline::FrameCount; ++i)
	{
		Util::DescriptorTable utilCbvTable;
		utilCbvTable.SetDescriptors(0, 1, &m_cbLighting.GetCBV(i));
		m_cbvTables[CBV_TABLE_LIGHTING + i] = utilCbvTable.GetCbvSrvUavTable(m_softGraphicsPipeline->GetDescriptorTableCache());
	}

	// Immutable material
	{
		XMFLOAT3 baseColor(1.0f, 1.0f, 0.5f);
		N_RETURN(m_cbMaterial.Create(m_device, sizeof(XMFLOAT3), 1, nullptr, MemoryType::DEFAULT), false);
		uploaders.emplace_back();
		m_cbMaterial.Upload(commandList, uploaders.back(), &baseColor, sizeof(XMFLOAT4));

		Util::DescriptorTable utilCbvTable;
		utilCbvTable.SetDescriptors(0, 1, &m_cbMaterial.GetCBV());
		m_cbvTables[CBV_TABLE_MATERIAL] = utilCbvTable.GetCbvSrvUavTable(m_softGraphicsPipeline->GetDescriptorTableCache());
	}

#if 1
	// Load inputs
	ObjLoader objLoader;
	N_RETURN(objLoader.Import(fileName, true, true), false);

	m_numIndices = objLoader.GetNumIndices();
	N_RETURN(m_softGraphicsPipeline->CreateVertexBuffer(commandList, m_vb, uploaders,
		objLoader.GetVertices(), objLoader.GetNumVertices(), objLoader.GetVertexStride()), false);
	N_RETURN(m_softGraphicsPipeline->CreateIndexBuffer(commandList, m_ib,
		uploaders, objLoader.GetIndices(), m_numIndices, Format::R32_UINT), false);
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
	N_RETURN(m_softGraphicsPipeline->CreateVertexBuffer(commandList, m_vb,
		uploaders, vbData, 3, sizeof(float[6])), false);

	const uint16_t ibData[] = { 0, 1, 2 };
	m_numIndices = 3;
	N_RETURN(m_softGraphicsPipeline->CreateIndexBuffer(commandList, m_ib,
		uploaders, ibData, m_numIndices, Format::R16_UINT), false);
#endif

	return true;
}

void Renderer::UpdateFrame(uint32_t frameIndex, CXMMATRIX viewProj, double time)
{
	{
		struct CBMatrices
		{
			XMMATRIX WorldViewProj;
			XMMATRIX Normal;
		};
		const auto pCb = reinterpret_cast<CBMatrices*>(m_cbMatrices.Map(frameIndex));
		const auto world = XMMatrixScaling(m_posScale.w, m_posScale.w, m_posScale.w) *
			XMMatrixTranslation(m_posScale.x, m_posScale.y, m_posScale.z);
		const auto worldInv = XMMatrixInverse(nullptr, world);
		pCb->WorldViewProj = XMMatrixTranspose(world * viewProj);
		pCb->Normal = worldInv;
	}

	{
		struct CBLighting
		{
			XMFLOAT4 AmbientColor;
			XMFLOAT4 LightColor;
			XMFLOAT3 LightPt;
		};
		const auto pCb = reinterpret_cast<CBLighting*>(m_cbLighting.Map(frameIndex));
		pCb->AmbientColor = XMFLOAT4(0.6f, 0.7f, 1.0f, 0.2f);
		pCb->LightColor = XMFLOAT4(1.0f, 0.7f, 0.5f, static_cast<float>(sin(time)) * 0.3f + 0.7f);
		XMStoreFloat3(&pCb->LightPt, XMVectorSet(1.0f, 1.0f, -1.0, 0.0f));
	}
}

void Renderer::Render(CommandList& commandList, uint32_t frameIndex)
{
	// Compute raster rendering
	const float clearColor[] = { CLEAR_COLOR, 0.0f };
	m_softGraphicsPipeline->SetRenderTargets(1, &m_colorTarget, &m_depth);
	m_softGraphicsPipeline->ClearFloat(m_colorTarget, clearColor);
	m_softGraphicsPipeline->ClearDepth(1.0f);
	m_softGraphicsPipeline->SetViewport(Viewport(0.0f, 0.0f, m_viewport.x, m_viewport.y));
	m_softGraphicsPipeline->SetVertexBuffer(m_vb.GetSRV());
	m_softGraphicsPipeline->SetIndexBuffer(m_ib.GetSRV());
	m_softGraphicsPipeline->VSSetDescriptorTable(0, m_cbvTables[CBV_TABLE_MATRICES + frameIndex]);
	m_softGraphicsPipeline->PSSetDescriptorTable(0, m_cbvTables[CBV_TABLE_LIGHTING + frameIndex]);
	m_softGraphicsPipeline->PSSetDescriptorTable(1, m_cbvTables[CBV_TABLE_MATERIAL]);
	m_softGraphicsPipeline->DrawIndexed(commandList, m_numIndices);
}

Texture2D& Renderer::GetColorTarget()
{
	return m_colorTarget;
}
