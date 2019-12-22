//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"
#include "SharedConst.h"

class SoftGraphicsPipeline
{
public:
	SoftGraphicsPipeline(const XUSG::Device& device);
	virtual ~SoftGraphicsPipeline();

	bool Init(const XUSG::CommandList& commandList, uint32_t width, uint32_t height,
		std::vector<XUSG::Resource>& uploaders);
	bool CreateVertexShaderLayout(XUSG::Util::PipelineLayout& utilPipelineLayout, uint32_t slotCount);
	bool SetAttribute(uint32_t i, uint32_t stride, XUSG::Format format, const wchar_t* name = L"Attribute");
	void SetVertexBuffer(const XUSG::Descriptor& vertexBufferView);
	void SetIndexBuffer(const XUSG::Descriptor& indexBufferView);
	void SetRenderTargets(XUSG::Texture2D* pColorTarget, XUSG::Texture2D* pDepth);
	void VSSetDescriptorTable(uint32_t i, const XUSG::DescriptorTable& descriptorTable);
	void Clear(const XUSG::Texture2D& target, const float clearValues[4], bool asUint = false);
	void Draw(XUSG::CommandList& commandList, uint32_t numVertices);
	void DrawIndexed(XUSG::CommandList& commandList, uint32_t numIndices);

	bool CreateVertexBuffer(const XUSG::CommandList& commandList, XUSG::VertexBuffer& vb,
		std::vector<XUSG::Resource>& uploaders, const void* pData, uint32_t numVert,
		uint32_t srtide, const wchar_t* name = L"VertexBuffer") const;
	bool CreateIndexBuffer(const XUSG::CommandList& commandList, XUSG::IndexBuffer& ib,
		std::vector<XUSG::Resource>& uploaders, const void* pData, uint32_t numIdx,
		XUSG::Format format, const wchar_t* name = L"IndexBuffer") const;
	XUSG::DescriptorTableCache& GetDescriptorTableCache();

	static const uint32_t FrameCount = FRAME_COUNT;

protected:
	enum StageIndex : uint8_t
	{
		VERTEX_PROCESS,
		VERTEX_INDEXED,
		BIN_RASTER,
		PIX_RASTER,

		NUM_STAGE
	};

	enum SRVTable : uint8_t
	{
		SRV_TABLE_VS,
		SRV_TABLE_RASTER,

		NUM_SRV_TABLE
	};

	enum UAVTable : uint8_t
	{
		UAV_TABLE_VS,
		UAV_TABLE_BIN,

		NUM_UAV_TABLE
	};

	struct CBViewPort
	{
		float x;
		float y;
		float w;
		float h;
		uint32_t numTileX;
		uint32_t numTileY;
	};

	struct ClearInfo
	{
		bool IsUint;
		const XUSG::Texture2D* pTarget;
		union
		{
			float ClearFloat[4];
			uint32_t ClearUint[4];
		};
	};

	bool createPipelines();
	bool createResetBuffer(const XUSG::CommandList& commandList, std::vector<XUSG::Resource>& uploaders);
	bool createCommandLayout();
	bool createDescriptorTables();

	void draw(XUSG::CommandList& commandList, uint32_t num, StageIndex vs);
	void rasterizer(XUSG::CommandList& commandList, uint32_t numTriangles);

	XUSG::Device m_device;

	XUSG::ShaderPool				m_shaderPool;
	XUSG::Compute::PipelineCache	m_computePipelineCache;
	XUSG::PipelineLayoutCache		m_pipelineLayoutCache;
	XUSG::DescriptorTableCache		m_descriptorTableCache;

	XUSG::PipelineLayout	m_pipelineLayouts[NUM_STAGE];
	XUSG::Pipeline			m_pipelines[NUM_STAGE];
	XUSG::CommandLayout		m_commandLayout;

	std::vector<ClearInfo> m_clears;
	std::vector<XUSG::DescriptorTable> m_extVsTables;
	std::vector<XUSG::DescriptorTable> m_outTables;

	XUSG::DescriptorTable	m_cbvTable;
	XUSG::DescriptorTable	m_srvTables[NUM_SRV_TABLE];
	XUSG::DescriptorTable	m_uavTables[NUM_UAV_TABLE];
	XUSG::DescriptorTable	m_samplerTable;

	XUSG::ConstantBuffer	m_cbMatrices;
	XUSG::ConstantBuffer	m_cbPerFrame;
	XUSG::ConstantBuffer	m_cbPerObject;
	XUSG::ConstantBuffer	m_cbBound;

	XUSG::Descriptor		m_vertexBufferView;
	XUSG::Descriptor		m_indexBufferView;
	XUSG::Texture2D*		m_pColorTarget;
	XUSG::Texture2D*		m_pDepth;

	std::vector<XUSG::TypedBuffer> m_vertexAttribs;
	XUSG::TypedBuffer		m_vertexPos;
	XUSG::StructuredBuffer	m_tilePrimCountReset;
	XUSG::StructuredBuffer	m_tilePrimCount;
	XUSG::StructuredBuffer	m_tiledPrimitives;

	DirectX::XMFLOAT2		m_viewport;
};
