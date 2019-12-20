//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGType.h"

namespace XUSG
{
	class CommandList
	{
	public:
		CommandList();
		virtual ~CommandList();

		virtual bool Close() const;
		virtual bool Reset(const CommandAllocator& allocator,
			const Pipeline& initialState) const;

		virtual void ClearState(const Pipeline& initialState) const;
		virtual void Draw(
			uint32_t vertexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startVertexLocation,
			uint32_t startInstanceLocation) const;
		virtual void DrawIndexed(
			uint32_t indexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startIndexLocation,
			int32_t baseVertexLocation,
			uint32_t startInstanceLocation) const;
		virtual void Dispatch(
			uint32_t threadGroupCountX,
			uint32_t threadGroupCountY,
			uint32_t threadGroupCountZ) const;
		virtual void CopyBufferRegion(const Resource& dstBuffer, uint64_t dstOffset,
			const Resource& srcBuffer, uint64_t srcOffset, uint64_t numBytes) const;
		virtual void CopyTextureRegion(const TextureCopyLocation& dst,
			uint32_t dstX, uint32_t dstY, uint32_t dstZ,
			const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr) const;
		virtual void CopyResource(const Resource& dstResource, const Resource& srcResource) const;
		virtual void CopyTiles(const Resource& tiledResource, const TiledResourceCoord* pTileRegionStartCoord,
			const TileRegionSize* pTileRegionSize, const Resource& buffer, uint64_t bufferStartOffsetInBytes,
			TileCopyFlags flags) const;
		virtual void ResolveSubresource(const Resource& dstResource, uint32_t dstSubresource,
			const Resource& srcResource, uint32_t srcSubresource, Format format) const;
		virtual void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const;
		virtual void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const;
		virtual void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const;
		virtual void OMSetBlendFactor(const float blendFactor[4]) const;
		virtual void OMSetStencilRef(uint32_t stencilRef) const;
		virtual void SetPipelineState(const Pipeline& pipelineState) const;
		virtual void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const;
		virtual void ExecuteBundle(GraphicsCommandList& commandList) const;
		virtual void SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const;
		virtual void SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const;
		virtual void SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const;
		virtual void SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const;
		virtual void SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const;
		virtual void SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const;
		virtual void SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const;
		virtual void SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const;
		virtual void SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const;
		virtual void SetComputeRootConstantBufferView(uint32_t index, const Resource& resource, int offset = 0) const;
		virtual void SetGraphicsRootConstantBufferView(uint32_t index, const Resource& resource, int offset = 0) const;
		virtual void SetComputeRootShaderResourceView(uint32_t index, const Resource& resource, int offset = 0) const;
		virtual void SetGraphicsRootShaderResourceView(uint32_t index, const Resource& resource, int offset = 0) const;
		virtual void SetComputeRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset = 0) const;
		virtual void SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset = 0) const;
		virtual void IASetIndexBuffer(const IndexBufferView& view) const;
		virtual void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const;
		virtual void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const;
		virtual void OMSetFramebuffer(const Framebuffer& framebuffer) const;
		virtual void OMSetRenderTargets(
			uint32_t numRenderTargetDescriptors,
			const Descriptor* pRenderTargetViews,
			const Descriptor* pDepthStencilView = nullptr,
			bool rtsSingleHandleToDescriptorRange = false) const;
		virtual void ClearDepthStencilView(const Framebuffer& framebuffer, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange * pRects = nullptr) const;
		virtual void ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		virtual void ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		virtual void ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource& resource, const uint32_t values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		virtual void ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource& resource, const float values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		//virtual void DiscardResource(const Resource &resource, const D3D12_DISCARD_REGION* pRegion) const;
		//virtual void BeginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t index) const;
		//virtual void EndQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE Type, uint32_t index) const;
		//virtual void ResolveQueryData(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t startIndex,
			//uint32_t numQueries, const Resource &dstBuffer, uint64_t alignedDstBufferOffset) const;
		//virtual void SetPredication(const Resource &buffer, uint64_t alignedBufferOffset, D3D12_PREDICATION_OP op)const;
		virtual void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const;
		virtual void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const;
		virtual void EndEvent();

		GraphicsCommandList& GetCommandList();

	protected:
		GraphicsCommandList m_commandList;
	};
}
