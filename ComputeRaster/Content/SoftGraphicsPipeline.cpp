//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Optional/XUSGObjLoader.h"
#include "SoftGraphicsPipeline.h"

using namespace std;
using namespace DirectX;
using namespace XUSG;

SoftGraphicsPipeline::SoftGraphicsPipeline(const Device& device) :
	m_device(device),
	m_pColorTarget(nullptr),
	m_pDepth(nullptr),
	m_maxVertexCount(0),
	m_clearDepth(0xffffffff)
{
	m_shaderPool = ShaderPool::MakeUnique();
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(device);
	m_descriptorTableCache = DescriptorTableCache::MakeUnique(device);
	m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(device);
}

SoftGraphicsPipeline::~SoftGraphicsPipeline()
{
}

bool SoftGraphicsPipeline::Init(CommandList* pCommandList, vector<Resource>& uploaders)
{
	const uint32_t tileBufferSize = (UINT32_MAX >> 4) + 1;
	const uint32_t binBufferSize = tileBufferSize >> 6;

	// Create buffers
	m_tilePrimCount = StructuredBuffer::MakeUnique();
	N_RETURN(m_tilePrimCount->Create(m_device, 3, sizeof(uint32_t),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
		1, nullptr, 1, nullptr, L"TilePrimitiveCount"), false);

	m_tilePrimitives = StructuredBuffer::MakeUnique();
	N_RETURN(m_tilePrimitives->Create(m_device, tileBufferSize, sizeof(uint32_t[2]),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT, 1,
		nullptr, 1, nullptr, L"TilePrimitives"), false);

	m_binPrimCount = StructuredBuffer::MakeUnique();
	N_RETURN(m_binPrimCount->Create(m_device, 3, sizeof(uint32_t),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
		1, nullptr, 1, nullptr, L"BinPrimitiveCount"), false);

	m_binPrimitives = StructuredBuffer::MakeUnique();
	N_RETURN(m_binPrimitives->Create(m_device, binBufferSize, sizeof(uint32_t[2]),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT, 1,
		nullptr, 1, nullptr, L"BinPrimitives"), false);

	// create reset buffer for resetting TilePrimitiveCount
	N_RETURN(createResetBuffer(pCommandList, uploaders), false);

	// create command layout
	N_RETURN(createCommandLayout(), false);

	return true;
}

bool SoftGraphicsPipeline::CreateVertexShaderLayout(Util::PipelineLayout* pPipelineLayout,
	uint32_t slotCount, int32_t srvBindingMax, int32_t uavBindingMax)
{
	m_extVsTables.resize(slotCount);
	const auto numUAVs = static_cast<uint32_t>(m_vertexAttribs.size()) + 1;
	//auto pPipelineLayoutIndexed = pPipelineLayout;

	// Create pipeline layouts
	{
		pPipelineLayout->SetRange(slotCount, DescriptorType::SRV, 2,
			srvBindingMax + 1, 0, DescriptorFlag::DESCRIPTORS_VOLATILE);
		pPipelineLayout->SetRange(slotCount + 1, DescriptorType::UAV, numUAVs,
			uavBindingMax + 1, 0, DescriptorFlag::DESCRIPTORS_VOLATILE |
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE);
		X_RETURN(m_pipelineLayouts[VERTEX_PROCESS], pPipelineLayout->GetPipelineLayout(
			*m_pipelineLayoutCache, PipelineLayoutFlag::NONE, L"VertexShaderStageLayout"), false);
	}

	m_pipelineLayouts[VERTEX_INDEXED] = m_pipelineLayouts[VERTEX_PROCESS];

	return true;
}

bool SoftGraphicsPipeline::CreatePixelShaderLayout(Util::PipelineLayout* pPipelineLayout,
	bool hasDepth, uint32_t numRTs, uint32_t slotCount, int32_t cbvBindingMax,
	int32_t srvBindingMax, int32_t uavBindingMax)
{
	m_extPsTables.resize(slotCount);

	// Create pipeline layouts
	{
		const auto numSRVs = static_cast<uint32_t>(m_vertexAttribs.size()) + 1;
		pPipelineLayout->SetConstants(slotCount, SizeOfInUint32(CBViewPort), cbvBindingMax + 1);
		pPipelineLayout->SetRange(slotCount + 1, DescriptorType::SRV, numSRVs, srvBindingMax + 1);
		pPipelineLayout->SetRange(slotCount + 2, DescriptorType::UAV, 1,
			uavBindingMax + 1, 0, DescriptorFlag::DESCRIPTORS_VOLATILE |
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE);
		pPipelineLayout->SetRange(slotCount + 3, DescriptorType::UAV, hasDepth ? numRTs + 2 : numRTs,
			uavBindingMax + 2, 0, DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE);
		X_RETURN(m_pipelineLayouts[PIX_RASTER], pPipelineLayout->GetPipelineLayout(
			*m_pipelineLayoutCache, PipelineLayoutFlag::NONE, L"PixelRasterLayout"), false);
	}

	return true;
}

void SoftGraphicsPipeline::SetAttribute(uint32_t i, uint32_t stride, Format format, const wchar_t* name)
{
	if (i >= m_vertexAttribs.size()) m_vertexAttribs.resize(i + 1);
	if (i >= m_attribInfo.size()) m_attribInfo.resize(i + 1);

	m_attribInfo[i].Stride = stride;
	m_attribInfo[i].Format = format;
	m_attribInfo[i].Name = name;
}

void SoftGraphicsPipeline::SetVertexBuffer(const Descriptor& vertexBufferView)
{
	m_vertexBufferView = vertexBufferView;
}

void SoftGraphicsPipeline::SetIndexBuffer(const Descriptor& indexBufferView)
{
	m_indexBufferView = indexBufferView;
}

void SoftGraphicsPipeline::SetRenderTargets(uint32_t numRTs, Texture2D* pColorTarget, DepthBuffer* pDepth)
{
	m_pColorTarget = pColorTarget;
	m_pDepth = pDepth;
	m_numColorTargets = numRTs;

	m_outTables.resize(pDepth ? numRTs + 3 : numRTs);
	for (auto i = 0u; i < numRTs; ++i)
	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique();
		descriptorTable->SetDescriptors(0, 1, &pColorTarget->GetUAV());
		m_outTables[i] = descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache);
	}

	if (pDepth)
	{
		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique();
			descriptorTable->SetDescriptors(0, 1, &pDepth->PixelZ->GetUAV());
			m_outTables[m_outTables.size() - 1] = descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache);
		}

		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique();
			descriptorTable->SetDescriptors(0, 1, &pDepth->TileZ->GetUAV());
			m_outTables[m_outTables.size() - 2] = descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache);
		}

		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique();
			descriptorTable->SetDescriptors(0, 1, &pDepth->BinZ->GetUAV());
			m_outTables[m_outTables.size() - 3] = descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache);
		}
	}
}

void SoftGraphicsPipeline::SetViewport(const Viewport& viewport)
{
	m_viewport = viewport;
}

void SoftGraphicsPipeline::VSSetDescriptorTable(uint32_t i, const DescriptorTable& descriptorTable)
{
	m_extVsTables[i] = descriptorTable;
}

void SoftGraphicsPipeline::PSSetDescriptorTable(uint32_t i, const DescriptorTable& descriptorTable)
{
	m_extPsTables[i] = descriptorTable;
}

void SoftGraphicsPipeline::ClearFloat(const Texture2D& target, const float clearValues[4])
{
	m_clears.emplace_back();
	m_clears.back().IsUint = false;
	m_clears.back().pTarget = &target;
	memcpy(m_clears.back().ClearFloat, clearValues, sizeof(float[4]));
}

void SoftGraphicsPipeline::ClearUint(const Texture2D& target, const uint32_t clearValues[4])
{
	m_clears.emplace_back();
	m_clears.back().IsUint = true;
	m_clears.back().pTarget = &target;
	memcpy(m_clears.back().ClearUint, clearValues, sizeof(uint32_t[4]));
}

void SoftGraphicsPipeline::ClearDepth(const float clearValue)
{
	m_clearDepth = reinterpret_cast<const uint32_t&>(clearValue);
}

void SoftGraphicsPipeline::Draw(CommandList* pCommandList, uint32_t numVertices)
{
	const auto descriptorTable = Util::DescriptorTable::MakeUnique();
	const Descriptor descriptors[] =
	{
		m_vertexBufferView
	};
	descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
	m_srvTables[SRV_TABLE_VS] = descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache);

	draw(pCommandList, numVertices, VERTEX_PROCESS);
}

void SoftGraphicsPipeline::DrawIndexed(CommandList* pCommandList, uint32_t numIndices)
{
	const auto descriptorTable = Util::DescriptorTable::MakeUnique();
	const Descriptor descriptors[] =
	{
		m_vertexBufferView,
		m_indexBufferView
	};
	descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
	m_srvTables[SRV_TABLE_VS] = descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache);

	draw(pCommandList, numIndices, VERTEX_INDEXED);
}

bool SoftGraphicsPipeline::CreateDepthBuffer(DepthBuffer& depth, uint32_t width, uint32_t height,
	Format format, const wchar_t* name)
{
	depth.PixelZ = Texture2D::MakeUnique();
	N_RETURN(depth.PixelZ->Create(m_device, width, height, format, 1,
		ResourceFlag::ALLOW_UNORDERED_ACCESS | ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS,
		1, 1, MemoryType::DEFAULT, false, (wstring(name) + L".PixelZ").c_str()), false);

	depth.TileZ = Texture2D::MakeUnique();
	N_RETURN(depth.TileZ->Create(m_device, DIV_UP(width, TILE_SIZE), DIV_UP(height, TILE_SIZE),
		format, 1, ResourceFlag::ALLOW_UNORDERED_ACCESS | ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS,
		1, 1, MemoryType::DEFAULT, false, (wstring(name) + L".TileZ").c_str()), false);

	depth.BinZ = Texture2D::MakeUnique();
	N_RETURN(depth.BinZ->Create(m_device, DIV_UP(width, BIN_SIZE), DIV_UP(height, BIN_SIZE),
		format, 1, ResourceFlag::ALLOW_UNORDERED_ACCESS | ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS,
		1, 1, MemoryType::DEFAULT, false, (wstring(name) + L".BinZ").c_str()), false);

	return true;
}

bool SoftGraphicsPipeline::CreateVertexBuffer(CommandList* pCommandList,
	VertexBuffer& vb, vector<Resource>& uploaders, const void* pData,
	uint32_t numVert, uint32_t srtide, const wchar_t* name) const
{
	N_RETURN(vb.Create(m_device, numVert, srtide, ResourceFlag::NONE,
		MemoryType::DEFAULT, 1, nullptr, 1, nullptr, 1, nullptr, name), false);
	uploaders.push_back(nullptr);

	return vb.Upload(pCommandList, uploaders.back(), pData, srtide * numVert);
}

bool SoftGraphicsPipeline::CreateIndexBuffer(CommandList* pCommandList,
	IndexBuffer& ib,vector<Resource>& uploaders, const void* pData,
	uint32_t numIdx, Format format, const wchar_t* name)
{
	m_maxVertexCount = (max)(m_maxVertexCount, numIdx);
	assert(format == Format::R16_UINT || format == Format::R32_UINT);
	const uint32_t byteWidth = (format == Format::R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t)) * numIdx;

	N_RETURN(ib.Create(m_device, byteWidth, format, ResourceFlag::NONE,
		MemoryType::DEFAULT, 1, nullptr, 1, nullptr, 1, nullptr, name), false);
	uploaders.push_back(nullptr);

	return ib.Upload(pCommandList, uploaders.back(), pData, byteWidth);
}

DescriptorTableCache& SoftGraphicsPipeline::GetDescriptorTableCache()
{
	return *m_descriptorTableCache;
}

bool SoftGraphicsPipeline::createPipelines()
{
	// Create pipeline layouts
	{
		const auto utilPipelineLayout = Util::PipelineLayout::MakeUnique();
		utilPipelineLayout->SetConstants(0, SizeOfInUint32(CBViewPort), 0);
		utilPipelineLayout->SetRange(1, DescriptorType::UAV,
			7, 0, 0, DescriptorFlag::DESCRIPTORS_VOLATILE |
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE);
		X_RETURN(m_pipelineLayouts[BIN_RASTER], utilPipelineLayout->GetPipelineLayout(
			*m_pipelineLayoutCache, PipelineLayoutFlag::NONE, L"BinRasterLayout"), false);
	}

	{
		const auto utilPipelineLayout = Util::PipelineLayout::MakeUnique();
		utilPipelineLayout->SetConstants(0, SizeOfInUint32(CBViewPort), 0);
		utilPipelineLayout->SetRange(1, DescriptorType::SRV, 1, 0, 0,
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE);
		utilPipelineLayout->SetRange(2, DescriptorType::UAV,
			5, 0, 0, DescriptorFlag::DESCRIPTORS_VOLATILE |
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE);
		X_RETURN(m_pipelineLayouts[TILE_RASTER], utilPipelineLayout->GetPipelineLayout(
			*m_pipelineLayoutCache, PipelineLayoutFlag::NONE, L"TileRasterLayout"), false);
	}

	// Create compute pipelines
	{
		N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, VERTEX_PROCESS, L"VSStage.cso"), false);

		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[VERTEX_PROCESS]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, VERTEX_PROCESS));
		X_RETURN(m_pipelines[VERTEX_PROCESS], state->GetPipeline(*m_computePipelineCache, L"VertexShaderStage"), false);
	}

	{
		N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, VERTEX_INDEXED, L"VSStageIndexed.cso"), false);

		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[VERTEX_INDEXED]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, VERTEX_INDEXED));
		X_RETURN(m_pipelines[VERTEX_INDEXED], state->GetPipeline(*m_computePipelineCache, L"VertexShaderStageIndexed"), false);
	}

	{
		N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, BIN_RASTER, L"BinRaster.cso"), false);

		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[BIN_RASTER]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, BIN_RASTER));
		X_RETURN(m_pipelines[BIN_RASTER], state->GetPipeline(*m_computePipelineCache, L"BinRaster"), false);
	}

	{
		N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, TILE_RASTER, L"TileRaster.cso"), false);

		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[TILE_RASTER]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, TILE_RASTER));
		X_RETURN(m_pipelines[TILE_RASTER], state->GetPipeline(*m_computePipelineCache, L"TileRaster"), false);
	}

	{
		N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, PIX_RASTER, L"PixelRaster.cso"), false);

		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[PIX_RASTER]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, PIX_RASTER));
		X_RETURN(m_pipelines[PIX_RASTER], state->GetPipeline(*m_computePipelineCache, L"BinRaster"), false);
	}

	return true;
}

bool SoftGraphicsPipeline::createResetBuffer(CommandList* pCommandList, vector<Resource>& uploaders)
{
	m_tilePrimCountReset = StructuredBuffer::MakeUnique();
	N_RETURN(m_tilePrimCountReset->Create(m_device, 1, sizeof(uint32_t), ResourceFlag::NONE,
		MemoryType::DEFAULT,1, nullptr, 1, nullptr, L"TilePrimitiveCountReset"), false);

	const uint32_t pDataReset[] = { 0, 1, 1 };
	uploaders.push_back(nullptr);
	N_RETURN(m_tilePrimCount->Upload(pCommandList, uploaders.back(), pDataReset, sizeof(uint32_t[3])), false);

	uploaders.push_back(nullptr);
	N_RETURN(m_binPrimCount->Upload(pCommandList, uploaders.back(), pDataReset, sizeof(uint32_t[3])), false);

	uploaders.push_back(nullptr);

	return m_tilePrimCountReset->Upload(pCommandList, uploaders.back(), pDataReset, sizeof(uint32_t));
}

bool SoftGraphicsPipeline::createCommandLayout()
{
	IndirectArgument arg;
	arg.Type = IndirectArgumentType::DISPATCH;

	return m_device->CreateCommandLayout(m_commandLayout, sizeof(uint32_t[3]), 1, &arg);
}

bool SoftGraphicsPipeline::createDescriptorTables()
{
	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique();
		const Descriptor descriptors[] =
		{
			m_binPrimitives->GetSRV()
		};
		descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
		X_RETURN(m_srvTables[SRV_TABLE_TR], descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache), false);
	}

	const auto numAttribs = static_cast<uint32_t>(m_vertexAttribs.size());
	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique();
		vector<Descriptor> descriptors;
		descriptors.reserve(numAttribs + 1);
		descriptors.push_back(m_tilePrimitives->GetSRV());
		for (const auto& attrib : m_vertexAttribs) descriptors.push_back(attrib->GetSRV());
		descriptorTable->SetDescriptors(0, static_cast<uint32_t>(descriptors.size()), descriptors.data());
		X_RETURN(m_srvTables[SRV_TABLE_PS], descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache), false);
	}

	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique();
		vector<Descriptor> descriptors;
		descriptors.reserve(numAttribs + 1);
		descriptors.push_back(m_vertexPos->GetUAV());
		for (const auto& attrib : m_vertexAttribs) descriptors.push_back(attrib->GetUAV());
		descriptorTable->SetDescriptors(0, static_cast<uint32_t>(descriptors.size()), descriptors.data());
		X_RETURN(m_uavTables[UAV_TABLE_VS], descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache), false);
	}

	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique();
		vector<Descriptor> descriptors;
		descriptors.reserve(7);
		descriptors.push_back(m_vertexPos->GetUAV()),
		descriptors.push_back(m_tilePrimCount->GetUAV());
		descriptors.push_back(m_tilePrimitives->GetUAV());
		if (m_pDepth)
		{
			descriptors.push_back(m_pDepth->TileZ->GetUAV());
			descriptors.push_back(m_pDepth->BinZ->GetUAV());
		}
		descriptors.push_back(m_binPrimCount->GetUAV());
		descriptors.push_back(m_binPrimitives->GetUAV());
		descriptorTable->SetDescriptors(0, static_cast<uint32_t>(descriptors.size()), descriptors.data());
		X_RETURN(m_uavTables[UAV_TABLE_RS], descriptorTable->GetCbvSrvUavTable(*m_descriptorTableCache), false);
	}

	return true;
}

void SoftGraphicsPipeline::draw(CommandList* pCommandList, uint32_t num, StageIndex vs)
{
	static auto firstTime = true;
	if (firstTime)
	{
		m_vertexPos = StructuredBuffer::MakeUnique();
		m_vertexPos->Create(m_device, m_maxVertexCount, sizeof(float[4]),
			ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
			1, nullptr, 1, nullptr, L"VertexPositions");

		m_vertexCompletions = StructuredBuffer::MakeUnique();
		m_vertexCompletions->Create(m_device, m_maxVertexCount, sizeof(uint32_t),
			ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
			1, nullptr, 1, nullptr, L"VertexCompletions");

		const auto attribCount = static_cast<uint32_t>(m_vertexAttribs.size());
		for (auto i = 0u; i < attribCount; ++i)
		{
			m_vertexAttribs[i] = TypedBuffer::MakeUnique();
			m_vertexAttribs[i]->Create(m_device, m_maxVertexCount,
				m_attribInfo[i].Stride, m_attribInfo[i].Format,
				ResourceFlag::ALLOW_UNORDERED_ACCESS,
				MemoryType::DEFAULT, 1, nullptr, 1, nullptr,
				m_attribInfo[i].Name.c_str());
		}

		createPipelines();
		createDescriptorTables();
		firstTime = false;
	}

	const DescriptorPool descriptorPools[] =
	{
		m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL),
		//m_descriptorTableCache->GetDescriptorPool(SAMPLER_POOL)
	};
	pCommandList->SetDescriptorPools(static_cast<uint32_t>(size(descriptorPools)), descriptorPools);

	// Clear depth
	if (m_pDepth && m_clearDepth != 0xffffffff)
	{
		pCommandList->ClearUnorderedAccessViewUint(m_outTables[m_outTables.size() - 1],
			m_pDepth->PixelZ->GetUAV(), m_pDepth->PixelZ->GetResource(), &m_clearDepth);
		pCommandList->ClearUnorderedAccessViewUint(m_outTables[m_outTables.size() - 2],
			m_pDepth->TileZ->GetUAV(), m_pDepth->TileZ->GetResource(), &m_clearDepth);
#if USE_TRIPPLE_RASTER
		pCommandList->ClearUnorderedAccessViewUint(m_outTables[m_outTables.size() - 3],
			m_pDepth->BinZ->GetUAV(), m_pDepth->BinZ->GetResource(), &m_clearDepth);
#endif
		m_clearDepth = 0xffffffff;
	}

	// Set resource barriers and clear
	ResourceBarrier barrier;
	// Due to auto promotions, no need to call commandList.Barrier()
	const uint32_t clearCount = static_cast<uint32_t>(m_clears.size());
	for (auto i = 0u; i < clearCount; ++i)
	{
		m_pColorTarget[i].SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS);
		if (m_clears[i].IsUint)
			pCommandList->ClearUnorderedAccessViewUint(m_outTables[i], m_clears[i].pTarget->GetUAV(),
				m_clears[i].pTarget->GetResource(), m_clears[i].ClearUint);
		else pCommandList->ClearUnorderedAccessViewFloat(m_outTables[i], m_clears[i].pTarget->GetUAV(),
			m_clears[i].pTarget->GetResource(), m_clears[i].ClearFloat);
	}
	m_clears.clear();

	m_tilePrimCount->SetBarrier(&barrier, ResourceState::COPY_DEST);
#if USE_TRIPPLE_RASTER
	m_binPrimCount->SetBarrier(&barrier, ResourceState::COPY_DEST);
#endif
	for (auto& attrib : m_vertexAttribs)
		attrib->SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS);

	// Vertex shader
	{
		// Set descriptor tables
		const auto baseIdx = static_cast<uint32_t>(m_extVsTables.size());
		pCommandList->SetComputePipelineLayout(m_pipelineLayouts[vs]);
		for (auto i = 0u; i < baseIdx; ++i)
			pCommandList->SetComputeDescriptorTable(i, m_extVsTables[i]);
		pCommandList->SetComputeDescriptorTable(baseIdx, m_srvTables[SRV_TABLE_VS]);
		pCommandList->SetComputeDescriptorTable(baseIdx + 1, m_uavTables[UAV_TABLE_VS]);

		// Set pipeline state
		pCommandList->SetPipelineState(m_pipelines[vs]);

		// Dispatch
		pCommandList->Dispatch(DIV_UP(num, 64), 1, 1);
	}

	// Rasterizations
	rasterizer(pCommandList, num / 3);
}

void SoftGraphicsPipeline::rasterizer(CommandList* pCommandList, uint32_t numTriangles)
{
	CBViewPort cbViewport;
	cbViewport.TopLeftX = m_viewport.TopLeftX;
	cbViewport.TopLeftY = m_viewport.TopLeftY;
	cbViewport.Width = m_viewport.Width;
	cbViewport.Height = m_viewport.Height;
	cbViewport.NumTileX = static_cast<uint32_t>(ceil(cbViewport.Width / TILE_SIZE));
	cbViewport.NumTileY = static_cast<uint32_t>(ceil(cbViewport.Height / TILE_SIZE));
	cbViewport.NumBinX = static_cast<uint32_t>(ceil(cbViewport.Width / BIN_SIZE));
	cbViewport.NumBinY = static_cast<uint32_t>(ceil(cbViewport.Height / BIN_SIZE));

	// Reset TilePrimitiveCount
	pCommandList->CopyBufferRegion(m_tilePrimCount->GetResource(), 0,
		m_tilePrimCountReset->GetResource(), 0, sizeof(uint32_t));
#if USE_TRIPPLE_RASTER
	// Reset BinPrimitiveCount
	pCommandList->CopyBufferRegion(m_binPrimCount->GetResource(), 0,
		m_tilePrimCountReset->GetResource(), 0, sizeof(uint32_t));
#endif

	// Set resource barriers
	vector<ResourceBarrier> barriers(m_vertexAttribs.size() + 2);
	auto numBarriers = m_tilePrimCount->SetBarrier(barriers.data(), ResourceState::UNORDERED_ACCESS);
#if USE_TRIPPLE_RASTER
	numBarriers = m_binPrimCount->SetBarrier(barriers.data(), ResourceState::UNORDERED_ACCESS, numBarriers);
#endif
	pCommandList->Barrier(numBarriers, barriers.data());

	// Due to auto promotions, no need to call commandList.Barrier()
	m_tilePrimitives->SetBarrier(barriers.data(), ResourceState::UNORDERED_ACCESS);
#if USE_TRIPPLE_RASTER
	m_binPrimitives->SetBarrier(barriers.data(), ResourceState::UNORDERED_ACCESS);
#endif

	// Bin raster
	{
		// Set descriptor tables
		pCommandList->SetComputePipelineLayout(m_pipelineLayouts[BIN_RASTER]);
		pCommandList->SetCompute32BitConstants(0, SizeOfInUint32(cbViewport), &cbViewport);
		pCommandList->SetComputeDescriptorTable(1, m_uavTables[UAV_TABLE_RS]);

		// Set pipeline state
		pCommandList->SetPipelineState(m_pipelines[BIN_RASTER]);

		// Dispatch
		pCommandList->Dispatch(DIV_UP(numTriangles, 64), 1, 1);
	}

#if USE_TRIPPLE_RASTER
	// Set resource barriers
	numBarriers = m_binPrimCount->SetBarrier(barriers.data(), ResourceState::INDIRECT_ARGUMENT);
	numBarriers = m_binPrimitives->SetBarrier(barriers.data(), ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
	pCommandList->Barrier(numBarriers, barriers.data());

	// Tile raster
	{
		// Set descriptor tables
		pCommandList->SetComputePipelineLayout(m_pipelineLayouts[TILE_RASTER]);
		pCommandList->SetCompute32BitConstants(0, SizeOfInUint32(cbViewport), &cbViewport);
		pCommandList->SetComputeDescriptorTable(1, m_srvTables[SRV_TABLE_TR]);
		pCommandList->SetComputeDescriptorTable(2, m_uavTables[UAV_TABLE_RS]);

		// Set pipeline state
		pCommandList->SetPipelineState(m_pipelines[TILE_RASTER]);

		// Dispatch indirect
		pCommandList->ExecuteIndirect(m_commandLayout, 1, m_binPrimCount->GetResource(),
			0, m_binPrimCount->GetResource());
	}
#endif

	// Set resource barriers
	numBarriers = m_tilePrimCount->SetBarrier(barriers.data(), ResourceState::INDIRECT_ARGUMENT);
	numBarriers = m_tilePrimitives->SetBarrier(barriers.data(), ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
	for (auto& attrib : m_vertexAttribs)
		numBarriers = attrib->SetBarrier(barriers.data(), ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
	pCommandList->Barrier(numBarriers, barriers.data());

	// Pixel raster
	{
		// Set descriptor tables
		const auto baseIdx = static_cast<uint32_t>(m_extPsTables.size());
		pCommandList->SetComputePipelineLayout(m_pipelineLayouts[PIX_RASTER]);
		for (auto i = 0u; i < baseIdx; ++i)
			pCommandList->SetComputeDescriptorTable(i, m_extPsTables[i]);
		pCommandList->SetCompute32BitConstants(baseIdx, SizeOfInUint32(cbViewport), &cbViewport);
		pCommandList->SetComputeDescriptorTable(baseIdx + 1, m_srvTables[SRV_TABLE_PS]);
		pCommandList->SetComputeDescriptorTable(baseIdx + 2, m_uavTables[UAV_TABLE_RS]);
		pCommandList->SetComputeDescriptorTable(baseIdx + 3, m_outTables[0]);

		// Set pipeline state
		pCommandList->SetPipelineState(m_pipelines[PIX_RASTER]);

		// Dispatch indirect
		pCommandList->ExecuteIndirect(m_commandLayout, 1, m_tilePrimCount->GetResource(),
			0, m_tilePrimCount->GetResource());
	}
}
