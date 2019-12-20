//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGShader.h"
#include "XUSGInputLayout.h"

namespace XUSG
{
	namespace Graphics
	{
		enum BlendPreset : uint8_t
		{
			DEFAULT_OPAQUE,
			PREMULTIPLITED,
			ADDTIVE,
			NON_PRE_MUL,
			NON_PREMUL_RT0,
			ALPHA_TO_COVERAGE,
			ACCUMULATIVE,
			AUTO_NON_PREMUL,
			ZERO_ALPHA_PREMUL,
			MULTIPLITED,
			WEIGHTED,
			SELECT_MIN,
			SELECT_MAX,

			NUM_BLEND_PRESET
		};

		enum RasterizerPreset : uint8_t
		{
			CULL_BACK,
			CULL_NONE,
			CULL_FRONT,
			FILL_WIREFRAME,

			NUM_RS_PRESET
		};

		enum DepthStencilPreset : uint8_t
		{
			DEFAULT_LESS,
			DEPTH_STENCIL_NONE,
			DEPTH_READ_LESS,
			DEPTH_READ_LESS_EQUAL,
			DEPTH_READ_EQUAL,

			NUM_DS_PRESET
		};

		class PipelineCache;

		class State
		{
		public:
			struct Key
			{
				void* PipelineLayout;
				void* Shaders[Shader::Stage::NUM_GRAPHICS];
				void* Blend;
				void* Rasterizer;
				void* DepthStencil;
				void* InputLayout;
				PrimitiveTopologyType PrimTopologyType;
				uint8_t	NumRenderTargets;
				Format RTVFormats[8];
				Format	DSVFormat;
				uint8_t	SampleCount;
			};

			State();
			virtual ~State();

			void SetPipelineLayout(const PipelineLayout& layout);
			void SetShader(Shader::Stage stage, Blob shader);

			void OMSetBlendState(const Blend& blend);
			void RSSetState(const Rasterizer& rasterizer);
			void DSSetState(const DepthStencil& depthStencil);

			void OMSetBlendState(BlendPreset preset, PipelineCache& pipelineCache);
			void RSSetState(RasterizerPreset preset, PipelineCache& pipelineCache);
			void DSSetState(DepthStencilPreset preset, PipelineCache& pipelineCache);

			void IASetInputLayout(const InputLayout& layout);
			void IASetPrimitiveTopologyType(PrimitiveTopologyType type);

			void OMSetNumRenderTargets(uint8_t n);
			void OMSetRTVFormat(uint8_t i, Format format);
			void OMSetRTVFormats(const Format* formats, uint8_t n);
			void OMSetDSVFormat(Format format);

			Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const;

			const std::string& GetKey() const;

		protected:
			Key* m_pKey;
			std::string m_key;
		};

		class PipelineCache
		{
		public:
			PipelineCache();
			PipelineCache(const Device& device);
			virtual ~PipelineCache();

			void SetDevice(const Device& device);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			void SetInputLayout(uint32_t index, const InputElementTable& elementTable);
			InputLayout GetInputLayout(uint32_t index) const;
			InputLayout CreateInputLayout(const InputElementTable& elementTable);

			Pipeline CreatePipeline(const State& state, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State& state, const wchar_t* name = nullptr);

			const Blend& GetBlend(BlendPreset preset);
			const Rasterizer& GetRasterizer(RasterizerPreset preset);
			const DepthStencil& GetDepthStencil(DepthStencilPreset preset);

		protected:
			Pipeline createPipeline(const State::Key* pKey, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

			Device m_device;

			InputLayoutPool	m_inputLayoutPool;

			std::unordered_map<std::string, Pipeline> m_pipelines;
			Blend			m_blends[NUM_BLEND_PRESET];
			Rasterizer		m_rasterizers[NUM_RS_PRESET];
			DepthStencil	m_depthStencils[NUM_DS_PRESET];

			std::function<Blend()>			m_pfnBlends[NUM_BLEND_PRESET];
			std::function<Rasterizer()>		m_pfnRasterizers[NUM_RS_PRESET];
			std::function<DepthStencil()>	m_pfnDepthStencils[NUM_DS_PRESET];
		};
	}
}
