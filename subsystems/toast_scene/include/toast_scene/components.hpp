#pragma once

#include "toast_scene.hpp"
#include "toast_render/mesh.hpp"

namespace toaster::scene
{
	struct TST_SCENE_API StaticMeshComponent
	{
		render::StaticMeshHandle mesh{nullptr};
		bool                     visible{true};
	};

	struct TST_SCENE_API TransformComponent
	{
		static constexpr uint32 max_dirty_frames{3u};

		XMFLOAT3 translation{0.0f, 0.0f, 0.0f};
		XMFLOAT4 orientation{g_XMIdentityR3.f};
		XMFLOAT3 scale{1.0f, 1.0f, 1.0f};

		uint32 framesDirty{max_dirty_frames}; // This is here because it is useful to be set to 3 when calling the set functions

		[[nodiscard]] auto XM_CALLCONV getTransform() const -> XMMATRIX
		{
			XMVECTOR simd_translation{XMLoadFloat3(&translation)};
			XMVECTOR simd_orientation{XMLoadFloat4(&orientation)};
			XMVECTOR simd_scale{XMLoadFloat3(&scale)};

			XMMATRIX transformation{XMMatrixTransformation(XMVectorZero(), XMVectorZero(), simd_scale, XMVectorZero(), simd_orientation, simd_translation)};
			return transformation;
		}

		[[nodiscard]] auto XM_CALLCONV getTranslation() const -> XMVECTOR { return XMLoadFloat3(&translation); }
		[[nodiscard]] auto XM_CALLCONV getOrientation() const -> XMVECTOR { return XMLoadFloat4(&orientation); }
		[[nodiscard]] auto XM_CALLCONV getScale() const -> XMVECTOR { return XMLoadFloat3(&scale); }

		auto XM_CALLCONV setTranslation(FXMVECTOR p_translation) -> void
		{
			XMStoreFloat3(&translation, p_translation);
			framesDirty = max_dirty_frames;
		}

		auto XM_CALLCONV setOrientation(FXMVECTOR p_orientation) -> void
		{
			XMStoreFloat4(&orientation, p_orientation);
			framesDirty = max_dirty_frames;
		}

		auto XM_CALLCONV setScale(FXMVECTOR p_scale) -> void
		{
			XMStoreFloat3(&scale, p_scale);
			framesDirty = max_dirty_frames;
		}
	};

	struct TST_SCENE_API GPUTransformComponent
	{
		uint32 transformId{UINT32_MAX};
	};
}
