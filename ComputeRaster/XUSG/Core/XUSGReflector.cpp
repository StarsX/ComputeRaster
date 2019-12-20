//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGReflector.h"
#include "dxcapi.h"
#include <Psapi.h>

using namespace std;
using namespace XUSG;

static const size_t g_dxilContainerHashSize = 16;
static const uint16_t g_dxilContainerVersionMajor = 1;		// Current major version
static const uint16_t g_dxilContainerVersionMinor = 0;		// Current minor version
static const uint32_t g_dxilContainerMaxSize = 0x80000000;	// Max size for container.

struct DxilContainerVersion
{
	uint16_t Major;
	uint16_t Minor;
};

// Use this type to describe a DXIL container of parts.
struct DxilContainerHeader
{
	uint32_t				HeaderFourCC;
	uint8_t					Hash[g_dxilContainerHashSize];
	DxilContainerVersion	Version;
	uint32_t				ContainerSizeInBytes; // From start of this header
	uint32_t				PartCount;
	// Structure is followed by uint32_t PartOffset[PartCount];
	// The offset is to a DxilPartHeader.
};

// Use this type to describe the size and type of a DXIL container part.
struct DxilPartHeader
{
	uint32_t PartFourCC; // Four char code for part type.
	uint32_t PartSize;   // Byte count for PartData.
	// Structure is followed by uint8_t PartData[PartSize].
};

#define DXIL_FOURCC(ch0, ch1, ch2, ch3) ( \
(uint32_t)(uint8_t)(ch0)		| (uint32_t)(uint8_t)(ch1) << 8	| \
(uint32_t)(uint8_t)(ch2) << 16	| (uint32_t)(uint8_t)(ch3) << 24 \
)

enum DxilFourCC
{
	DFCC_Container = DXIL_FOURCC('D', 'X', 'B', 'C'), // for back-compat with tools that look for DXBC containers
	DFCC_ResourceDef = DXIL_FOURCC('R', 'D', 'E', 'F'),
	DFCC_InputSignature = DXIL_FOURCC('I', 'S', 'G', '1'),
	DFCC_OutputSignature = DXIL_FOURCC('O', 'S', 'G', '1'),
	DFCC_PatchConstantSignature = DXIL_FOURCC('P', 'S', 'G', '1'),
	DFCC_ShaderStatistics = DXIL_FOURCC('S', 'T', 'A', 'T'),
	DFCC_ShaderDebugInfoDXIL = DXIL_FOURCC('I', 'L', 'D', 'B'),
	DFCC_ShaderDebugName = DXIL_FOURCC('I', 'L', 'D', 'N'),
	DFCC_FeatureInfo = DXIL_FOURCC('S', 'F', 'I', '0'),
	DFCC_PrivateData = DXIL_FOURCC('P', 'R', 'I', 'V'),
	DFCC_RootSignature = DXIL_FOURCC('R', 'T', 'S', '0'),
	DFCC_DXIL = DXIL_FOURCC('D', 'X', 'I', 'L'),
	DFCC_PipelineStateValidation = DXIL_FOURCC('P', 'S', 'V', '0'),
	DFCC_RuntimeData = DXIL_FOURCC('R', 'D', 'A', 'T'),
};

// Gets a part header by index.
inline const DxilPartHeader* GetDxilContainerPart(const DxilContainerHeader* pHeader, uint32_t index)
{
	const auto pLinearContainer = reinterpret_cast<const uint8_t*>(pHeader);
	const auto pPartOffsetTable = reinterpret_cast<const uint32_t*>(pHeader + 1);

	return reinterpret_cast<const DxilPartHeader*>(pLinearContainer + pPartOffsetTable[index]);
}

static const DxilContainerHeader* IsDxilContainerLike(const void* ptr, size_t length)
{
	C_RETURN(!ptr || length < 4, nullptr);

	C_RETURN(DFCC_Container != *reinterpret_cast<const uint32_t*>(ptr), nullptr);

	return reinterpret_cast<const DxilContainerHeader*>(ptr);
}

static bool IsValidDxilContainer(const DxilContainerHeader* pHeader, size_t length)
{
	// Validate that the header is where it's supposed to be.
	N_RETURN(pHeader, false);
	N_RETURN(length >= sizeof(DxilContainerHeader), false);

	// Validate the header values.
	N_RETURN(pHeader->HeaderFourCC == DFCC_Container, false);
	N_RETURN(pHeader->Version.Major == g_dxilContainerVersionMajor, false);
	N_RETURN(pHeader->ContainerSizeInBytes <= length, false);
	N_RETURN(pHeader->ContainerSizeInBytes <= g_dxilContainerMaxSize, false);

	// Make sure that the count of offsets fits.
	const auto partOffsetTableBytes = sizeof(uint32_t) * pHeader->PartCount;
	N_RETURN(partOffsetTableBytes + sizeof(DxilContainerHeader) <= pHeader->ContainerSizeInBytes, false);

	// Make sure that each part is within the bounds.
	const auto pLinearContainer = reinterpret_cast<const uint8_t*>(pHeader);
	const auto pPartOffsetTable = reinterpret_cast<const uint32_t*>(pHeader + 1);
	for (auto i = 0u; i < pHeader->PartCount; ++i)
	{
		// The part header should fit.
		N_RETURN(pPartOffsetTable[i] + sizeof(DxilPartHeader) <= pHeader->ContainerSizeInBytes, false);

		// The contents of the part should fit.
		const auto pPartHeader = reinterpret_cast<const DxilPartHeader*>(pLinearContainer + pPartOffsetTable[i]);

		N_RETURN(pPartOffsetTable[i] + sizeof(DxilPartHeader) + pPartHeader->PartSize <=
			pHeader->ContainerSizeInBytes, false);
	}

	// Note: the container parts may overlap and there may be holes
	// based on this validation

	return true;
}

static bool IsDxil(LPCVOID pSrcData, SIZE_T SrcDataSize)
{
	const auto pHeader = IsDxilContainerLike(pSrcData, SrcDataSize);
	N_RETURN(pHeader, false);

	N_RETURN(IsValidDxilContainer(pHeader, SrcDataSize), false);

	for (auto i = 0u; i < pHeader->PartCount; ++i)
	{
		const auto pPart = GetDxilContainerPart(pHeader, i);
		C_RETURN(pPart->PartFourCC == DFCC_DXIL || pPart->PartFourCC == DFCC_ShaderDebugInfoDXIL, true);
	}

	return false;
}

static DxcCreateInstanceProc GetDxcCreateInstanceProc()
{
	static auto isGot = false;
	static DxcCreateInstanceProc pfnDxcCreateInstance = nullptr;

	if (!isGot)
	{
		isGot = true;

		const auto hProcess = GetCurrentProcess();
		HMODULE hMods[1024];
		unsigned long cbNeeded;

		bool isDXCompilerLoaded = false;

		// First search if dxcompiler.dll is loaded
		if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for (auto i = 0u; i < cbNeeded / sizeof(HMODULE); ++i)
			{
				wchar_t szModName[MAX_PATH];
				if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(char)))
				{
					// Remove path
					wchar_t* p = wcsrchr(szModName, L'\\');
					if (!p) p = wcsrchr(szModName, L'/');
					if (!p)
					{
						p = szModName;
						--p;
					}
					if (_wcsicmp(p + 1, L"dxcompiler.dll") == 0)
					{
						pfnDxcCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(hMods[i], "DxcCreateInstance"));
						isDXCompilerLoaded = true;
						break;
					}
				}
			}
		}

		// If dxcompiler.dll is not loaded, try some default candidates
		if (!isDXCompilerLoaded)
		{
			static const wchar_t* modules[] = { L"dxcompiler.dll" };

			for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); i++)
			{
				const auto hModule = LoadLibrary(modules[i]);
				pfnDxcCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(hModule, "DxcCreateInstance"));
				if (pfnDxcCreateInstance) break;
			}
		}
	}

	return pfnDxcCreateInstance;
}

Reflector::Reflector() :
	m_reflection(nullptr)
{
}

Reflector::~Reflector()
{
}

bool Reflector::SetShader(const Blob& shader)
{
	if (IsDxil(shader->GetBufferPointer(), shader->GetBufferSize()))
	{
		const auto DxcCreateInstance = GetDxcCreateInstanceProc();
		if (!DxcCreateInstance) return false;

		com_ptr<IDxcLibrary> library = nullptr;;
		V_RETURN(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)), cerr, false);
		com_ptr<IDxcBlobEncoding> blob = nullptr;
		library->CreateBlobWithEncodingFromPinned(shader->GetBufferPointer(),
			static_cast<uint32_t>(shader->GetBufferSize()), 0, &blob);

		auto shaderIdx = ~0u;
		com_ptr<IDxcContainerReflection> reflection = nullptr;
		V_RETURN(DxcCreateInstance(CLSID_DxcContainerReflection,
			IID_PPV_ARGS(&reflection)), cerr, false);

		reflection->Load(blob.get());
		V_RETURN(reflection->FindFirstPartKind(DXIL_FOURCC('D', 'X', 'I', 'L'), &shaderIdx), cerr, false);
		V_RETURN(reflection->GetPartReflection(shaderIdx,
			IID_PPV_ARGS(&m_reflection)), cerr, false);
	}
	else V_RETURN(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(),
		IID_PPV_ARGS(&m_reflection)), cerr, false);

	return true;
}

bool Reflector::IsValid() const
{
	return m_reflection;
}

uint32_t Reflector::GetResourceBindingPointByName(const char* name, uint32_t defaultVal) const
{
	D3D12_SHADER_INPUT_BIND_DESC desc;
	const auto hr = m_reflection->GetResourceBindingDescByName(name, &desc);

	return SUCCEEDED(hr) ? desc.BindPoint : defaultVal;
}
