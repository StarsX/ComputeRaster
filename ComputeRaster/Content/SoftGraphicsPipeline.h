//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"
#include "SharedConst.h"

class SoftGraphicsPipeline
{
public:
	struct DepthBuffer
	{
		XUSG::Texture2D::uptr PixelZ;
		XUSG::Texture2D::uptr TileZ;
		XUSG::Texture2D::uptr BinZ;
	};

	SoftGraphicsPipeline();
	virtual ~SoftGraphicsPipeline();

	bool Init(XUSG::CommandList* pCommandList, std::vector<XUSG::Resource::uptr>& uploaders);
	bool CreateVertexShaderLayout(XUSG::Util::PipelineLayout* pPipelineLayout,
		uint32_t slotCount = 0, int32_t srvBindingMax = -1, int32_t uavBindingMax = -1);
	bool CreatePixelShaderLayout(XUSG::Util::PipelineLayout* pPipelineLayout,
		bool hasDepth, uint32_t numRTs, uint32_t slotCount = 0, int32_t cbvBindingMax = -1,
		int32_t srvBindingMax = -1, int32_t uavBindingMax = -1);
	void SetDecriptorHeaps(XUSG::CommandList* pCommandList);
	void SetAttribute(uint32_t i, uint32_t stride, XUSG::Format format, const wchar_t* name = L"Attribute");
	void SetVertexBuffer(const XUSG::Descriptor& vertexBufferView);
	void SetIndexBuffer(const XUSG::Descriptor& indexBufferView);
	void SetRenderTargets(uint32_t numRTs, XUSG::Texture2D* pColorTarget, DepthBuffer* pDepth);
	void SetViewport(const XUSG::Viewport& viewport);
	void VSSetDescriptorTable(uint32_t i, const XUSG::DescriptorTable& descriptorTable);
	void PSSetDescriptorTable(uint32_t i, const XUSG::DescriptorTable& descriptorTable);
	void ClearFloat(const XUSG::Texture2D& target, const float clearValues[4]);
	void ClearUint(const XUSG::Texture2D& target, const uint32_t clearValues[4]);
	void ClearDepth(const float clearValue);
	void Draw(XUSG::CommandList* pCommandList, uint32_t numVertices);
	void DrawIndexed(XUSG::CommandList* pCommandList, uint32_t numIndices);

	bool CreateDepthBuffer(const XUSG::Device* pDevice, DepthBuffer &depth, uint32_t width,
		uint32_t height, XUSG::Format format, const wchar_t* name = L"Depth");
	bool CreateVertexBuffer(XUSG::CommandList* pCommandList, XUSG::VertexBuffer& vb,
		std::vector<XUSG::Resource::uptr>& uploaders, const void* pData, uint32_t numVert,
		uint32_t srtide, const wchar_t* name = L"VertexBuffer") const;
	bool CreateIndexBuffer(XUSG::CommandList* pCommandList, XUSG::IndexBuffer& ib,
		std::vector<XUSG::Resource::uptr>& uploaders, const void* pData, uint32_t numIdx,
		XUSG::Format format, const wchar_t* name = L"IndexBuffer");
	XUSG::DescriptorTableLib* GetDescriptorTableLib() const;

	static const uint8_t FrameCount = FRAME_COUNT;

protected:
	enum StageIndex : uint8_t
	{
		VERTEX_PROCESS,
		VERTEX_INDEXED,
		BIN_RASTER,
		TILE_RASTER,
		PIX_RASTER,

		NUM_STAGE
	};

	enum SRVTable : uint8_t
	{
		SRV_TABLE_VS,
		SRV_TABLE_TR,
		SRV_TABLE_PS,

		NUM_SRV_TABLE
	};

	enum UAVTable : uint8_t
	{
		UAV_TABLE_VS,
		UAV_TABLE_RS,

		NUM_UAV_TABLE
	};

	struct CBViewPort
	{
		float TopLeftX;
		float TopLeftY;
		float Width;
		float Height;
		uint32_t NumTileX;
		uint32_t NumTileY;
		uint32_t NumBinX;
		uint32_t NumBinY;
	};

	struct AttributeInfo
	{
		uint32_t Stride;
		XUSG::Format Format;
		std::wstring Name;
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
	bool createResetBuffer(XUSG::CommandList* pCommandList, std::vector<XUSG::Resource::uptr>& uploaders);
	bool createCommandLayout(const XUSG::Device* pDevice);
	bool createDescriptorTables();

	void draw(XUSG::CommandList* pCommandList, uint32_t num, StageIndex vs);
	void rasterizer(XUSG::CommandList* pCommandList, uint32_t numTriangles);

	XUSG::ShaderLib::uptr				m_shaderLib;
	XUSG::Compute::PipelineLib::uptr	m_computePipelineLib;
	XUSG::PipelineLayoutLib::uptr		m_pipelineLayoutLib;
	XUSG::DescriptorTableLib::uptr		m_descriptorTableLib;

	XUSG::PipelineLayout		m_pipelineLayouts[NUM_STAGE];
	XUSG::Pipeline				m_pipelines[NUM_STAGE];
	XUSG::CommandLayout::uptr	m_commandLayout;

	std::vector<ClearInfo> m_clears;
	std::vector<XUSG::DescriptorTable> m_extVsTables;
	std::vector<XUSG::DescriptorTable> m_extPsTables;
	std::vector<XUSG::DescriptorTable> m_outTables;

	XUSG::DescriptorTable	m_cbvTable;
	XUSG::DescriptorTable	m_srvTables[NUM_SRV_TABLE];
	XUSG::DescriptorTable	m_uavTables[NUM_UAV_TABLE];
	XUSG::DescriptorTable	m_samplerTable;

	XUSG::ConstantBuffer::uptr	m_cbMatrices;
	XUSG::ConstantBuffer::uptr	m_cbPerFrame;
	XUSG::ConstantBuffer::uptr	m_cbPerObject;
	XUSG::ConstantBuffer::uptr	m_cbBound;

	XUSG::Descriptor	m_vertexBufferView;
	XUSG::Descriptor	m_indexBufferView;
	XUSG::Texture2D*	m_pColorTarget;
	DepthBuffer*		m_pDepth;

	std::vector<AttributeInfo> m_attribInfo;
	std::vector<XUSG::TypedBuffer::uptr> m_vertexAttribs;
	XUSG::StructuredBuffer::uptr	m_vertexCompletions;
	XUSG::StructuredBuffer::uptr	m_vertexPos;
	XUSG::StructuredBuffer::uptr	m_tilePrimCountReset;
	XUSG::StructuredBuffer::uptr	m_binPrimCount;
	XUSG::StructuredBuffer::uptr	m_binPrimitives;
	XUSG::StructuredBuffer::uptr	m_tilePrimCount;
	XUSG::StructuredBuffer::uptr	m_tilePrimitives;

	XUSG::Viewport			m_viewport;

	uint32_t				m_maxVertexCount;
	uint32_t				m_numColorTargets;
	uint32_t				m_clearDepth;
};
