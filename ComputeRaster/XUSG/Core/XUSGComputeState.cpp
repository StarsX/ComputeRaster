//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGComputeState.h"

using namespace std;
using namespace XUSG;
using namespace Compute;

State::State()
{
	// Default state
	m_key.resize(sizeof(Key));
	m_pKey = reinterpret_cast<Key*>(&m_key[0]);
	memset(m_pKey, 0, sizeof(Key));
}

State::~State()
{
}

void State::SetPipelineLayout(const PipelineLayout& layout)
{
	m_pKey->PipelineLayout = layout.get();
}

void State::SetShader(Blob shader)
{
	m_pKey->Shader = shader.get();
}

Pipeline State::CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name) const
{
	return pipelineCache.CreatePipeline(*this, name);
}

Pipeline State::GetPipeline(PipelineCache& pipelineCache, const wchar_t* name) const
{
	return pipelineCache.GetPipeline(*this, name);
}

const string& State::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

PipelineCache::PipelineCache() :
	m_device(nullptr),
	m_pipelines()
{
}

PipelineCache::PipelineCache(const Device& device) :
	PipelineCache()
{
	SetDevice(device);
}

PipelineCache::~PipelineCache()
{
}

void PipelineCache::SetDevice(const Device& device)
{
	m_device = device;
}

void PipelineCache::SetPipeline(const string& key, const Pipeline& pipeline)
{
	m_pipelines[key] = pipeline;
}

Pipeline PipelineCache::CreatePipeline(const State& state, const wchar_t* name)
{
	return createPipeline(reinterpret_cast<const State::Key*>(state.GetKey().data()), name);
}

Pipeline PipelineCache::GetPipeline(const State& state, const wchar_t* name)
{
	return getPipeline(state.GetKey(), name);
}

Pipeline PipelineCache::createPipeline(const State::Key* pKey, const wchar_t* name)
{
	// Fill desc
	PipelineDesc desc = {};
	if (pKey->PipelineLayout)
		desc.pRootSignature = static_cast<decltype(desc.pRootSignature)>(pKey->PipelineLayout);

	if (pKey->Shader)
		desc.CS = Shader::ByteCode(static_cast<BlobType*>(pKey->Shader));

	// Create pipeline
	Pipeline pipeline;
	V_RETURN(m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
	if (name) pipeline->SetName(name);

	return pipeline;
}

Pipeline PipelineCache::getPipeline(const string& key, const wchar_t* name)
{
	const auto pPipeline = m_pipelines.find(key);

	// Create one, if it does not exist
	if (pPipeline == m_pipelines.end())
	{
		const auto pKey = reinterpret_cast<const State::Key*>(key.data());
		const auto pipeline = createPipeline(pKey, name);
		m_pipelines[key] = pipeline;

		return pipeline;
	}

	return pPipeline->second;
}
