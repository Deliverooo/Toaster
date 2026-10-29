#pragma once

#include "toast_render.hpp"
#include "toast_gpu/allocation.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::rnd
{
	struct RenderContext;
	struct Texture;
	TST_DECLARE_HANDLE(Texture);
	struct TextureManager;

	enum class EMaterialFlagBits : uint8
	{
		eBackfaceCulling = TST_LIB_BIT(0u),
		eTransparent     = TST_LIB_BIT(1u),
		eNoNormalMap     = TST_LIB_BIT(2u),
		eNoRoughnessMap  = TST_LIB_BIT(3u),
		eNoMetalnessMap  = TST_LIB_BIT(4u)
	};

	TST_LIB_SPECIALISE_FLAGS(EMaterialFlagBits, EMaterialFlags);

	struct TST_RENDER_API MaterialParams
	{
		math::Float4   albedoColour{1.0f, 1.0f, 1.0f, 1.0f};
		uint32         albedoMapHeapSlot{UINT32_MAX};
		uint32         normalMapHeapSlot{UINT32_MAX};
		uint32         roughnessMapHeapSlot{UINT32_MAX};
		uint32         metalnessMapHeapSlot{UINT32_MAX};
		float32        roughness{0.0f};
		float32        metalness{0.0f};
		EMaterialFlags flags{
			EMaterialFlagBits::eBackfaceCulling | EMaterialFlagBits::eNoNormalMap | EMaterialFlagBits::eNoRoughnessMap | EMaterialFlagBits::eNoMetalnessMap
		};
	};

	struct TST_RENDER_API Material
	{
		MaterialParams params; // Raw parameter data
		TextureHandle  albedoMap;
		TextureHandle  normalMap;
		TextureHandle  roughnessMap;
		TextureHandle  metalnessMap;
	};

	TST_DECLARE_HANDLE(Material);

	struct MaterialManager;

	auto TST_RENDER_API material_mngr_create(RenderContext *p_render_ctx, TextureManager *p_texture_mngr, uint32 p_max_materials) -> MaterialManager *;
	auto TST_RENDER_API material_mngr_destroy(MaterialManager *p_mngr) -> void;
	auto TST_RENDER_API material_mngr_poll_pending_textures(MaterialManager *p_mngr) -> void;
	auto TST_RENDER_API material_mngr_update_dirty_materials(MaterialManager *p_mngr, uint32 p_frame_index) -> void;
	auto TST_RENDER_API material_mngr_get_material_buffer_address(MaterialManager *p_mngr, uint32 p_frame_index) -> gpu::DeviceAddress;
	auto TST_RENDER_API material_mngr_get_default_colour_map(MaterialManager *p_mngr) -> TextureHandle;
	auto TST_RENDER_API material_mngr_get_default_normal_map(MaterialManager *p_mngr) -> TextureHandle;
	auto TST_RENDER_API material_mngr_get_default_roughness_map(MaterialManager *p_mngr) -> TextureHandle;
	auto TST_RENDER_API material_mngr_get_default_metalness_map(MaterialManager *p_mngr) -> TextureHandle;

	auto TST_RENDER_API material_create(MaterialManager *p_mngr) -> MaterialHandle;
	auto TST_RENDER_API material_destroy(MaterialManager *p_mngr, MaterialHandle p_material) -> void;

	auto TST_RENDER_API material_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *;
	auto TST_RENDER_API material_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *;
	auto TST_RENDER_API material_try_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *;
	auto TST_RENDER_API material_try_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *;

	auto TST_RENDER_API material_set_albedo_colour(MaterialManager *p_mngr, MaterialHandle p_material, const math::Float4 &p_colour) -> void;
	auto TST_RENDER_API material_set_albedo_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_albedo_map) -> void;
	auto TST_RENDER_API material_set_normal_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_normal_map) -> void;
	auto TST_RENDER_API material_set_roughness_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_roughness_map) -> void;
	auto TST_RENDER_API material_set_metalness_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_metalness_map) -> void;
	auto TST_RENDER_API material_set_roughness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_roughness) -> void;
	auto TST_RENDER_API material_set_metalness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_metalness) -> void;
	auto TST_RENDER_API material_set_flags(MaterialManager *p_mngr, MaterialHandle p_material, EMaterialFlags p_flags, bool p_enable) -> void;

	auto TST_RENDER_API material_mark_dirty(MaterialManager *p_mngr, MaterialHandle p_material) -> void;
}
