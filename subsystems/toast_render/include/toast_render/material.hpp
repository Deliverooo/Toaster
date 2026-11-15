#pragma once

#include "toast_render.hpp"

#include "toast_gpu/fwd.hpp"

#include "toast_lib/array.hpp"
#include "toast_lib/bit.h"

#include "toast_math/math_vector.hpp"

namespace toaster::rnd
{
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

	template<typename Type>
	using PerMaterialTextureArray = TST_EARRAY(Type, EMaterialTextureType);

	struct TST_RENDER_API MaterialParams
	{
		math::Float4                    albedoColour{1.0f, 1.0f, 1.0f, 1.0f};
		PerMaterialTextureArray<uint32> heapSlots;
		float32                         roughness{0.0f};
		float32                         metalness{0.0f};
		EMaterialFlags                  flags{
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
		PerMaterialTextureArray<gpu::TextureHandle>      textures;
		PerMaterialTextureArray<gpu::StateTrackerHandle> stateTrackers;
		PerMaterialTextureArray<uint32>                  shaderReadHeapSlots;
		MaterialParams                                   params; // Raw parameter data

		// Utility function...
		[[nodiscard]] auto getTexture(EMaterialTextureType p_type) const -> MaterialTexture
		{
			return {textures[p_type], stateTrackers[p_type], shaderReadHeapSlots[p_type]};
		}
	};

	TST_DECLARE_HANDLE(Material);

	struct MaterialManager;

	// 2^14
	constexpr uint32 c_max_materials{16'384u}; // There is a max of 16,384 materials allowed... This is more than enough. It only takes up (at maximum) ~2.1 Mb of RAM

	// All the context the material manager needs to operate
	struct TST_RENDER_API MaterialManagerDesc
	{
		gpu::GPUContext *                 gpuCtx{nullptr};
		gpu::AllocationContext *          allocCtx{nullptr};
		gpu::UploadContext *              uploadCtx{nullptr};
		gpu::FrameContext *               frameCtx{nullptr};
		gpu::ResourceDescriptorHeapHandle resourceHeap{nullptr};
		uint32                            maxFramesInFlight{0u};
		// No, this is not a mistake, maxGPUMaterials is for the max number of materials on the GPU, whereas c_max_materials is the size of the CPU material pool.
		// Requires maxGPUMaterials < c_max_materials
		uint32 maxGPUMaterials{0u};
	};

	auto TST_RENDER_API material_mngr_create(const MaterialManagerDesc &p_desc) -> MaterialManager *;
	auto TST_RENDER_API material_mngr_destroy(MaterialManager *p_mngr) -> void;
	auto TST_RENDER_API material_mngr_poll_pending_textures(MaterialManager *p_mngr) -> void;
	auto TST_RENDER_API material_mngr_update_dirty_materials(MaterialManager *p_mngr, uint32 p_frame_index) -> void;
	auto TST_RENDER_API material_mngr_get_material_buffer_address(const MaterialManager *p_mngr, uint32 p_frame_index) -> gpu::DeviceAddress;
	auto TST_RENDER_API material_mngr_get_default_texture(MaterialManager *p_mngr, EMaterialTextureType p_type) -> MaterialTexture;

	auto TST_RENDER_API material_create(MaterialManager *p_mngr) -> MaterialHandle;
	auto TST_RENDER_API material_destroy(MaterialManager *p_mngr, MaterialHandle p_material) -> void;

	auto TST_RENDER_API material_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material &;
	auto TST_RENDER_API material_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material &;
	auto TST_RENDER_API material_try_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *;
	auto TST_RENDER_API material_try_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *;

	auto TST_RENDER_API material_set_texture(MaterialManager *p_mngr, MaterialHandle p_material, EMaterialTextureType p_type, const MaterialTexture &p_texture) -> void;
	auto TST_RENDER_API material_set_albedo_colour(MaterialManager *p_mngr, MaterialHandle p_material, const math::Float4 &p_colour) -> void;
	auto TST_RENDER_API material_set_roughness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_roughness) -> void;
	auto TST_RENDER_API material_set_metalness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_metalness) -> void;
	auto TST_RENDER_API material_set_flags(MaterialManager *p_mngr, MaterialHandle p_material, EMaterialFlags p_flags, bool p_enable) -> void;

	auto TST_RENDER_API material_mark_dirty(MaterialManager *p_mngr, MaterialHandle p_material) -> void;
}
