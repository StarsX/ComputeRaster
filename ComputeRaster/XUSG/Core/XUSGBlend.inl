//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

using namespace std;

namespace XUSG
{
	namespace Graphics
	{
		Blend DefaultOpaque()
		{
			return make_shared<D3D12_BLEND_DESC>(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		}

		Blend Premultiplied()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend Additive()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend NonPremultiplied()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend NonPremultipliedRT0()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = TRUE;

			auto& desc = blend->RenderTarget[0];
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			// Default
			D3D12_RENDER_TARGET_BLEND_DESC descDefault;
			descDefault.BlendEnable = FALSE;
			descDefault.LogicOpEnable = FALSE;
			descDefault.SrcBlend = D3D12_BLEND_ONE;
			descDefault.DestBlend = D3D12_BLEND_ZERO;
			descDefault.BlendOp = D3D12_BLEND_OP_ADD;
			descDefault.SrcBlendAlpha = D3D12_BLEND_ONE;
			descDefault.DestBlendAlpha = D3D12_BLEND_ZERO;
			descDefault.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			descDefault.LogicOp = D3D12_LOGIC_OP_NOOP;
			descDefault.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto i = 1u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				blend->RenderTarget[i] = descDefault;

			return blend;
		}

		Blend AlphaToCoverage()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = TRUE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = FALSE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend Accumulative()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend AutoNonPremultiplied()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend ZeroAlphaNonPremultiplied()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ZERO;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend Multiplied()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ZERO;
			desc.DestBlend = D3D12_BLEND_SRC_COLOR;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ZERO;
			desc.DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend Weighted()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = TRUE;

			// Accumulation
			auto& desc0 = blend->RenderTarget[0];
			desc0.BlendEnable = TRUE;
			desc0.LogicOpEnable = FALSE;
			desc0.SrcBlend = D3D12_BLEND_ONE; // Premultiplied for flexibility
			desc0.DestBlend = D3D12_BLEND_ONE;
			desc0.BlendOp = D3D12_BLEND_OP_ADD;
			desc0.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc0.DestBlendAlpha = D3D12_BLEND_ONE;
			desc0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc0.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc0.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			// Production
			auto& desc1 = blend->RenderTarget[1];
			desc1.BlendEnable = TRUE;
			desc1.LogicOpEnable = FALSE;
			desc1.SrcBlend = D3D12_BLEND_ZERO;
			desc1.DestBlend = D3D12_BLEND_SRC_COLOR;
			desc1.BlendOp = D3D12_BLEND_OP_ADD;
			desc1.SrcBlendAlpha = D3D12_BLEND_ZERO;
			desc1.DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			desc1.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc1.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc1.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			// Default
			D3D12_RENDER_TARGET_BLEND_DESC descDefault;
			descDefault.BlendEnable = FALSE;
			descDefault.LogicOpEnable = FALSE;
			descDefault.SrcBlend = D3D12_BLEND_ONE;
			descDefault.DestBlend = D3D12_BLEND_ZERO;
			descDefault.BlendOp = D3D12_BLEND_OP_ADD;
			descDefault.SrcBlendAlpha = D3D12_BLEND_ONE;
			descDefault.DestBlendAlpha = D3D12_BLEND_ZERO;
			descDefault.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			descDefault.LogicOp = D3D12_LOGIC_OP_NOOP;
			descDefault.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto i = 2u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				blend->RenderTarget[i] = descDefault;

			return blend;
		}

		Blend SelectMin()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_MIN;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.BlendOpAlpha = D3D12_BLEND_OP_MIN;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}

		Blend SelectMax()
		{
			const auto blend = make_shared<D3D12_BLEND_DESC>();
			blend->AlphaToCoverageEnable = FALSE;
			blend->IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_MAX;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.BlendOpAlpha = D3D12_BLEND_OP_MAX;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (auto& renderTarget : blend->RenderTarget)
				renderTarget = desc;

			return blend;
		}
	}
}
