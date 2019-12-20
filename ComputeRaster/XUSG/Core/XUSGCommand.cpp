//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGCommand.h"

using namespace std;
using namespace XUSG;

CommandList::CommandList()
{
}

CommandList::~CommandList()
{
}

bool CommandList::Close() const
{
	V_RETURN(m_commandList->Close(), cerr, false);

	return true;
}

bool CommandList::Reset(const CommandAllocator& allocator, const Pipeline& initialState) const
{
	V_RETURN(m_commandList->Reset(allocator.get(), initialState.get()), cerr, false);

	return true;
}

void CommandList::ClearState(const Pipeline& initialState) const
{
	m_commandList->ClearState(initialState.get());
}

void CommandList::Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation) const
{
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount,
		startVertexLocation, startInstanceLocation);
}

void CommandList::DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
	uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) const
{
	m_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount,
		startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void CommandList::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const
{
	m_commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList::CopyBufferRegion(const Resource& dstBuffer, uint64_t dstOffset,
	const Resource& srcBuffer, uint64_t srcOffset, uint64_t numBytes) const
{
	m_commandList->CopyBufferRegion(dstBuffer.get(), dstOffset, srcBuffer.get(), srcOffset, numBytes);
}

void CommandList::CopyTextureRegion(const TextureCopyLocation& dst,
	uint32_t dstX, uint32_t dstY, uint32_t dstZ, const TextureCopyLocation& src,
	const BoxRange* pSrcBox) const
{
	m_commandList->CopyTextureRegion(&dst, dstX, dstY, dstZ, &src, pSrcBox);
}

void CommandList::CopyResource(const Resource& dstResource, const Resource& srcResource) const
{
	m_commandList->CopyResource(dstResource.get(), srcResource.get());
}

void CommandList::CopyTiles(const Resource& tiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize* pTileRegionSize, const Resource& buffer, uint64_t bufferStartOffsetInBytes,
	TileCopyFlags flags) const
{
	m_commandList->CopyTiles(tiledResource.get(), pTileRegionStartCoord, pTileRegionSize,
		buffer.get(), bufferStartOffsetInBytes, flags);
}

void CommandList::ResolveSubresource(const Resource& dstResource, uint32_t dstSubresource,
	const Resource& srcResource, uint32_t srcSubresource, Format format) const
{
	m_commandList->ResolveSubresource(dstResource.get(), dstSubresource, srcResource.get(),
		srcSubresource, static_cast<DXGI_FORMAT>(format));
}

void CommandList::IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const
{
	m_commandList->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(primitiveTopology));
}

void CommandList::RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const
{
	m_commandList->RSSetViewports(numViewports, pViewports);
}

void CommandList::RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->RSSetScissorRects(numRects, pRects);
}

void CommandList::OMSetBlendFactor(const float blendFactor[4]) const
{
	m_commandList->OMSetBlendFactor(blendFactor);
}

void CommandList::OMSetStencilRef(uint32_t stencilRef) const
{
	m_commandList->OMSetStencilRef(stencilRef);
}

void CommandList::SetPipelineState(const Pipeline& pipelineState) const
{
	m_commandList->SetPipelineState(pipelineState.get());
}

void CommandList::Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const
{
	if (numBarriers > 0) m_commandList->ResourceBarrier(numBarriers, pBarriers);
}

void CommandList::ExecuteBundle(GraphicsCommandList& commandList) const
{
	m_commandList->ExecuteBundle(commandList.get());
}

void CommandList::SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const
{
	vector<DescriptorPool::element_type*> ppDescriptorPools(numDescriptorPools);
	for (auto i = 0u; i < numDescriptorPools; ++i)
		ppDescriptorPools[i] = pDescriptorPools[i].get();

	m_commandList->SetDescriptorHeaps(numDescriptorPools, ppDescriptorPools.data());
}

void CommandList::SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_commandList->SetComputeRootSignature(pipelineLayout.get());
}

void CommandList::SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_commandList->SetGraphicsRootSignature(pipelineLayout.get());
}

void CommandList::SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	m_commandList->SetComputeRootDescriptorTable(index, *descriptorTable);
}

void CommandList::SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	m_commandList->SetGraphicsRootDescriptorTable(index, *descriptorTable);
}

void CommandList::SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetComputeRoot32BitConstant(index, srcData, destOffsetIn32BitValues);
}

void CommandList::SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetGraphicsRoot32BitConstant(index, srcData, destOffsetIn32BitValues);
}

void CommandList::SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetComputeRoot32BitConstants(index, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void CommandList::SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetGraphicsRoot32BitConstants(index, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void CommandList::SetComputeRootConstantBufferView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetComputeRootConstantBufferView(index, resource->GetGPUVirtualAddress() + offset);
}

void CommandList::SetGraphicsRootConstantBufferView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetGraphicsRootConstantBufferView(index, resource->GetGPUVirtualAddress() + offset);
}

void CommandList::SetComputeRootShaderResourceView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetComputeRootShaderResourceView(index, resource->GetGPUVirtualAddress() + offset);
}

void CommandList::SetGraphicsRootShaderResourceView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetGraphicsRootShaderResourceView(index, resource->GetGPUVirtualAddress() + offset);
}

void CommandList::SetComputeRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetComputeRootUnorderedAccessView(index, resource->GetGPUVirtualAddress() + offset);
}

void CommandList::SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetGraphicsRootUnorderedAccessView(index, resource->GetGPUVirtualAddress() + offset);
}

void CommandList::IASetIndexBuffer(const IndexBufferView& view) const
{
	m_commandList->IASetIndexBuffer(&view);
}

void CommandList::IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const
{
	m_commandList->IASetVertexBuffers(startSlot, numViews, pViews);
}

void CommandList::SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const
{
	m_commandList->SOSetTargets(startSlot, numViews, pViews);
}

void CommandList::OMSetFramebuffer(const Framebuffer& framebuffer) const
{
	m_commandList->OMSetRenderTargets(framebuffer.NumRenderTargetDescriptors, framebuffer.RenderTargetViews.get(),
		true, framebuffer.DepthStencilView.ptr ? &framebuffer.DepthStencilView : nullptr);
}

void CommandList::OMSetRenderTargets(uint32_t numRenderTargetDescriptors, const Descriptor* pRenderTargetViews,
	const Descriptor* pDepthStencilView, bool rtsSingleHandleToDescriptorRange) const
{
	m_commandList->OMSetRenderTargets(numRenderTargetDescriptors, pRenderTargetViews,
		rtsSingleHandleToDescriptorRange, pDepthStencilView);
}

void CommandList::ClearDepthStencilView(const Framebuffer& framebuffer, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects) const
{
	ClearDepthStencilView(framebuffer.DepthStencilView, clearFlags, depth, stencil, numRects, pRects);
}

void CommandList::ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearDepthStencilView(depthStencilView, static_cast<D3D12_CLEAR_FLAGS>(clearFlags),
		depth, stencil, numRects, pRects);
}

void CommandList::ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
	uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearRenderTargetView(renderTargetView, colorRGBA, numRects, pRects);
}

void CommandList::ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const Resource& resource, const uint32_t values[4], uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearUnorderedAccessViewUint(*descriptorTable, descriptor, resource.get(), values, numRects, pRects);
}

void CommandList::ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const Resource& resource, const float values[4], uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearUnorderedAccessViewFloat(*descriptorTable, descriptor, resource.get(), values, numRects, pRects);
}

void CommandList::SetMarker(uint32_t metaData, const void* pData, uint32_t size) const
{
	m_commandList->SetMarker(metaData, pData, size);
}

void CommandList::BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const
{
	m_commandList->BeginEvent(metaData, pData, size);
}

void CommandList::EndEvent()
{
	m_commandList->EndEvent();
}

GraphicsCommandList& CommandList::GetCommandList()
{
	return m_commandList;
}
