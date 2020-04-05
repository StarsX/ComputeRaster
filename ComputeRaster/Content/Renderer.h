//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "SoftGraphicsPipeline.h"

class Renderer
{
public:
	Renderer(const XUSG::Device& device);
	virtual ~Renderer();

	bool Init(const XUSG::CommandList* pCommandList, uint32_t width, uint32_t height,
		std::vector<XUSG::Resource>& uploaders, const char* fileName,
		const DirectX::XMFLOAT4& posScale);

	void UpdateFrame(uint32_t frameIndex, DirectX::CXMMATRIX view,
		DirectX::CXMMATRIX proj, const DirectX::XMFLOAT3& eyePt, double time);
	void Render(XUSG::CommandList* pCommandList, uint32_t frameIndex);

	XUSG::Texture2D& GetColorTarget();

protected:
	enum CBVTable : uint8_t
	{
		CBV_TABLE_MATRICES,
		CBV_TABLE_LIGHTING = CBV_TABLE_MATRICES + SoftGraphicsPipeline::FrameCount,
		CBV_TABLE_MATERIAL = CBV_TABLE_LIGHTING + SoftGraphicsPipeline::FrameCount,

		NUM_CBV_TABLE
	};

	XUSG::Device m_device;

	std::unique_ptr<SoftGraphicsPipeline> m_softGraphicsPipeline;
	XUSG::VertexBuffer_uptr		m_vb;
	XUSG::IndexBuffer_uptr		m_ib;
	XUSG::ConstantBuffer_uptr	m_cbMatrices;
	XUSG::ConstantBuffer_uptr	m_cbLighting;
	XUSG::ConstantBuffer_uptr	m_cbMaterial;

	XUSG::Texture2D_uptr		m_colorTarget;
	SoftGraphicsPipeline::DepthBuffer m_depth;

	XUSG::DescriptorTable	m_cbvTables[NUM_CBV_TABLE];

	DirectX::XMFLOAT2		m_viewport;
	DirectX::XMFLOAT4		m_posScale;

	uint32_t				m_numIndices;
};
