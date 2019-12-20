//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGShader.h"

using namespace std;
using namespace XUSG;
using namespace Shader;

ShaderPool::ShaderPool() :
	m_shaders(),
	m_reflectors()
{
}

ShaderPool::~ShaderPool()
{
}

void ShaderPool::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader)
{
	checkShaderStorage(stage, index) = shader;
}

void ShaderPool::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const ReflectorPtr& reflector)
{
	SetShader(stage, index, shader);
	SetReflector(stage, index, reflector);
}

void ShaderPool::SetReflector(Shader::Stage stage, uint32_t index, const ReflectorPtr& reflector)
{
	checkReflectorStorage(stage, index) = reflector;
}

Blob ShaderPool::CreateShader(Shader::Stage stage, uint32_t index, const wstring& fileName)
{
	auto& shader = checkShaderStorage(stage, index);
	V_RETURN(D3DReadFileToBlob(fileName.c_str(), &shader), cerr, nullptr);

	auto& reflector = checkReflectorStorage(stage, index);
	reflector = make_shared<Reflector>();
	N_RETURN(reflector->SetShader(shader), nullptr);

	return shader;
}

Blob ShaderPool::GetShader(Shader::Stage stage, uint32_t index) const
{
	return index < m_shaders[stage].size() ? m_shaders[stage][index] : nullptr;
}

ReflectorPtr ShaderPool::GetReflector(Shader::Stage stage, uint32_t index) const
{
	return index < m_reflectors[stage].size() ? m_reflectors[stage][index] : nullptr;
}

Blob& ShaderPool::checkShaderStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_shaders[stage].size())
		m_shaders[stage].resize(index + 1);

	return m_shaders[stage][index];
}

ReflectorPtr& ShaderPool::checkReflectorStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_reflectors[stage].size())
		m_reflectors[stage].resize(index + 1);

	return m_reflectors[stage][index];
}
