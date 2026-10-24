#pragma once

#include <DirectXMath.h>
using namespace DirectX; // This is probably the only socially acceptable 'using namespace' to do

namespace toaster::math
{
	using f32 = float;
	using f64 = double;

	struct Double2
	{
		constexpr Double2() = default;

		constexpr Double2(f64 p_s) : x(p_s), y(p_s)
		{
		}

		constexpr Double2(f64 p_x, f64 p_y) : x(p_x), y(p_y)
		{
		}

		f64 x;
		f64 y;
	};

	// TODO: Maybe I will create my own SIMD library some day...

	using Float2 = XMFLOAT2;
	using Float3 = XMFLOAT3;
	using Float4 = XMFLOAT4;

	// Apparently, DirectXMath does not have a 2x2 matrix?!
	using Float3x3 = XMFLOAT3X3;
	using Float3x4 = XMFLOAT3X4;
	using Float4x3 = XMFLOAT4X3;
	using Float4x4 = XMFLOAT4X4;

	using Int2 = XMINT2;
	using Int3 = XMINT3;
	using Int4 = XMINT4;

	using UInt2 = XMUINT2;
	using UInt3 = XMUINT3;
	using UInt4 = XMUINT4;

	// SIMD types
	using Vector  = XMVECTOR;
	using FVector = FXMVECTOR;
	using Matrix  = XMMATRIX;
	using FMatrix = FXMMATRIX;

	struct Rect
	{
		constexpr Rect() = default;

		constexpr Rect(UInt2 p_size) : size(p_size), offset(0, 0)
		{
		}

		UInt2 size{0u, 0u};
		Int2  offset{0, 0};
	};

	struct Viewport // Should map directly to vk::Viewport
	{
		constexpr Viewport() = default;

		constexpr Viewport(Float2 p_size) : offset(0.0f, 0.0f), size(p_size), depthBounds(0.0f, 1.0f)
		{
		}

		constexpr Viewport(const Rect &p_rect, Float2 p_depth_bounds = Float2{0.0f, 1.0f}) : offset(static_cast<f32>(p_rect.offset.x), static_cast<f32>(p_rect.offset.y)),
																							 size(static_cast<f32>(p_rect.size.x), static_cast<f32>(p_rect.size.y)),
																							 depthBounds(p_depth_bounds)
		{
		}

		Float2 offset{0.0f, 0.0f};
		Float2 size{0.0f, 0.0f};
		Float2 depthBounds{0.0f, 1.0f}; // Min depth and max depth
	};

	inline Vector XM_CALLCONV XMVector2NormalizeSafe(FVector p_vector)
	{
		Vector length{XMVector2LengthSq(p_vector)};
		Vector control{XMVectorGreater(length, XMVectorSplatEpsilon())};
		Vector normalized{XMVector2Normalize(p_vector)};
		return XMVectorSelect(XMVectorZero(), normalized, control);
	}

	inline Vector XM_CALLCONV XMVector3NormalizeSafe(FVector p_vector)
	{
		Vector length{XMVector3LengthSq(p_vector)};
		Vector control{XMVectorGreater(length, XMVectorSplatEpsilon())};
		Vector normalized{XMVector3Normalize(p_vector)};
		return XMVectorSelect(XMVectorZero(), normalized, control);
	}

	inline Vector XM_CALLCONV XMVector4NormalizeSafe(FVector p_vector)
	{
		Vector length{XMVector4LengthSq(p_vector)};
		Vector control{XMVectorGreater(length, XMVectorSplatEpsilon())};
		Vector normalized{XMVector4Normalize(p_vector)};
		return XMVectorSelect(XMVectorZero(), normalized, control);
	}
}
