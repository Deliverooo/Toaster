#pragma once

#include "toast_render.hpp"
#include "toast_gpu/allocation.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::rnd
{
	struct RenderContext;

	enum class EMaterialFlagBits : uint8
	{
		eBackfaceCulling = TST_BIT(0u),
		eTransparent     = TST_BIT(1u),
		eNoNormalMap     = TST_BIT(2u),
		eNoRoughnessMap  = TST_BIT(3u),
		eNoMetalnessMap  = TST_BIT(4u)
	};

	TST_SPECIALISE_FLAGS(EMaterialFlagBits, EMaterialFlags);

	enum class EMaterialTextureType : uint8
	{
		eAlbedoMap    = 0u,
		eNormalMap    = 1u,
		eRoughnessMap = 2u,
		eMetalnessMap = 3u,
		eMax          = 4u
	};

	constexpr uint8 c_max_material_textures{TST_ENUM_VAL(EMaterialTextureType::eMax)};

	struct TST_RENDER_API MaterialParams
	{
		math::Float4                           albedoColour{1.0f, 1.0f, 1.0f, 1.0f};
		Array<uint32, c_max_material_textures> heapSlots;
		float32                                roughness{0.0f};
		float32                                metalness{0.0f};
		EMaterialFlags                         flags{
			EMaterialFlagBits::eBackfaceCulling | EMaterialFlagBits::eNoNormalMap | EMaterialFlagBits::eNoRoughnessMap | EMaterialFlagBits::eNoMetalnessMap
		};
	};

	// Represents all the necessary data a material needs from a texture.
	struct TST_RENDER_API MaterialTexture
	{
		gpu::TextureHandle texture{nullptr};
		// Optional. If not nullptr, the material's actual shaderReadHeapSlot will be set when this signals that it is ready
		gpu::StateTrackerHandle stateTracker{nullptr};
		uint32                  shaderReadHeapSlot{UINT32_MAX};
	};

	struct TST_RENDER_API Material // Sorry, this is maximising space efficiency (SOA vs AOS)
	{
		Array<gpu::TextureHandle, c_max_material_textures>      textures;
		Array<gpu::StateTrackerHandle, c_max_material_textures> stateTrackers;
		Array<uint32, c_max_material_textures>                  shaderReadHeapSlots;
		MaterialParams                                          params; // Raw parameter data

		// Utility function...
		[[nodiscard]] auto getTexture(EMaterialTextureType p_type) const -> MaterialTexture
		{
			return {textures[TST_UT(p_type)], stateTrackers[TST_UT(p_type)], shaderReadHeapSlots[TST_UT(p_type)]};
		}
	};

	TST_DECLARE_HANDLE(Material);

	struct MaterialManager;

	constexpr uint32 c_max_materials{16'384u}; // There is a max of 16,384 materials allowed... This is more than enough. It only takes up (at maximum) ~2.1 Mb of RAM

	auto TST_RENDER_API material_mngr_create(RenderContext *p_render_ctx) -> MaterialManager *;
	auto TST_RENDER_API material_mngr_destroy(MaterialManager *p_mngr) -> void;
	auto TST_RENDER_API material_mngr_poll_pending_textures(MaterialManager *p_mngr) -> void;
	auto TST_RENDER_API material_mngr_update_dirty_materials(MaterialManager *p_mngr, uint32 p_frame_index) -> void;
	auto TST_RENDER_API material_mngr_get_material_buffer_address(const MaterialManager *p_mngr, uint32 p_frame_index) -> gpu::DeviceAddress;
	auto TST_RENDER_API material_mngr_get_default_texture(MaterialManager *p_mngr, EMaterialTextureType p_type) -> MaterialTexture;

	auto TST_RENDER_API material_create(MaterialManager *p_mngr) -> MaterialHandle;
	auto TST_RENDER_API material_destroy(MaterialManager *p_mngr, MaterialHandle p_material) -> void;

	auto TST_RENDER_API material_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *;
	auto TST_RENDER_API material_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *;
	auto TST_RENDER_API material_try_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *;
	auto TST_RENDER_API material_try_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *;

	auto TST_RENDER_API material_set_texture(MaterialManager *p_mngr, MaterialHandle p_material, EMaterialTextureType p_type, const MaterialTexture &p_texture) -> void;
	auto TST_RENDER_API material_set_albedo_colour(MaterialManager *p_mngr, MaterialHandle p_material, const math::Float4 &p_colour) -> void;
	auto TST_RENDER_API material_set_roughness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_roughness) -> void;
	auto TST_RENDER_API material_set_metalness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_metalness) -> void;
	auto TST_RENDER_API material_set_flags(MaterialManager *p_mngr, MaterialHandle p_material, EMaterialFlags p_flags, bool p_enable) -> void;

	auto TST_RENDER_API material_mark_dirty(MaterialManager *p_mngr, MaterialHandle p_material) -> void;
}
