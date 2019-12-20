//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGShader.h"

namespace XUSG
{
	enum class DescriptorType : uint8_t
	{
		SRV,
		UAV,
		CBV,
		SAMPLER,
		CONSTANT,
		ROOT_SRV,
		ROOT_UAV,
		ROOT_CBV,

		NUM
	};

	class PipelineLayoutCache;

	struct DescriptorRange
	{
		DescriptorType ViewType;
		uint32_t NumDescriptors;
		uint32_t BaseBinding;
		uint32_t Space;
		DescriptorRangeFlag Flags;
	};

	namespace Util
	{
		class PipelineLayout
		{
		public:
			PipelineLayout();
			virtual ~PipelineLayout();

			void SetShaderStage(uint32_t index, Shader::Stage stage);
			void SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, DescriptorRangeFlag flags = DescriptorRangeFlag::NONE);
			void SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t binding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootSRV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorRangeFlag flags = DescriptorRangeFlag::DATA_STATIC, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootUAV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorRangeFlag flags = DescriptorRangeFlag::NONE, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootCBV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorRangeFlag flags = DescriptorRangeFlag::NONE, Shader::Stage stage = Shader::Stage::ALL);

			XUSG::PipelineLayout CreatePipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr);
			XUSG::PipelineLayout GetPipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr);

			DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const;
			DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const;

			const std::vector<std::string>& GetDescriptorTableLayoutKeys() const;
			std::string& GetPipelineLayoutKey(PipelineLayoutCache* pPipelineLayoutCache);

		protected:
			std::string& checkKeySpace(uint32_t index);

			std::vector<std::string> m_descriptorTableLayoutKeys;
			std::string m_pipelineLayoutKey;

			bool m_isTableLayoutsCompleted;
		};
	}

	class PipelineLayoutCache
	{
	public:
		PipelineLayoutCache();
		PipelineLayoutCache(const Device& device);
		virtual ~PipelineLayoutCache();

		void SetDevice(const Device& device);
		void SetPipelineLayout(const std::string& key, const PipelineLayout& pipelineLayout);

		PipelineLayout CreatePipelineLayout(Util::PipelineLayout& util, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr);
		PipelineLayout GetPipelineLayout(Util::PipelineLayout& util, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, bool create = true);

		DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout& util);
		DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout& util);

	protected:
		PipelineLayout createPipelineLayout(const std::string& key, const wchar_t* name) const;
		PipelineLayout getPipelineLayout(const std::string& key, const wchar_t* name, bool create);

		DescriptorTableLayout createDescriptorTableLayout(const std::string& key);
		DescriptorTableLayout getDescriptorTableLayout(const std::string& key);

		Device m_device;

		std::unordered_map<std::string, PipelineLayout> m_pipelineLayouts;
		std::unordered_map<std::string, DescriptorTableLayout> m_descriptorTableLayouts;
	};
}
