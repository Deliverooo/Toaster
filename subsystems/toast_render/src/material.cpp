#include "toast_render/material.hpp"
#include "toast_render/render_context.hpp"

#include "toast_lib/map.hpp"
#include "toast_lib/pool.hpp"
#include "toast_lib/set.hpp"

#include "toast_gpu/api.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

#include <algorithm>
#include <bit>

namespace toaster::rnd
{
	enum class EMaterialTextureTypeFlagBits : uint8
	{
		eAlbedoMap    = TST_BIT(0u),
		eNormalMap    = TST_BIT(1u),
		eRoughnessMap = TST_BIT(2u),
		eMetalnessMap = TST_BIT(3u)
	};

	TST_SPECIALISE_FLAGS(EMaterialTextureTypeFlagBits, EMaterialTextureTypeFlags);

	struct MaterialManager
	{
		RenderContext *rndCtx{nullptr};

		static constexpr uint32                                                       c_materialPageSize{64u};
		FixedPool<Material, c_materialPageSize, c_max_materials / c_materialPageSize> materials;

		Vector<gpu::DeviceAddress>            bdas;
		Vector<gpu::BufferHandle>             materialSSBOs;
		Vector<UnorderedSet<MaterialHandle> > dirtyMaterials; // Per frame

		// List of materials which have textures that are not fully uploaded or created
		UnorderedMap<MaterialHandle, EMaterialTextureTypeFlags> pendingMaterialTextureUploads;

		Array<gpu::TextureHandle, c_maxMaterialTextures> defaultTextures;
		Array<uint32, c_maxMaterialTextures>             defaultTextureHeapSlots;

		mtx_t dirtyMaterialMtx;
		mtx_t textureUploadMtx;
	};

	auto material_mngr_create(RenderContext *p_render_ctx, uint16 p_max_gpu_materials) -> MaterialManager *
	{
		const auto mngr{new MaterialManager{}};

		mngr->rndCtx = p_render_ctx;

		mtx_init(&mngr->dirtyMaterialMtx, mtx_plain);
		mtx_init(&mngr->textureUploadMtx, mtx_plain);

		gpu::GPUContext *   gpu_ctx{p_render_ctx->gpuCtx};
		gpu::UploadContext *upload_ctx{p_render_ctx->uploadCtx};

		gpu::BufferDesc material_buffer_desc{};
		material_buffer_desc.size       = sizeof(MaterialParams) * static_cast<uint64>(p_max_gpu_materials);
		material_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		material_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		mngr->materialSSBOs.resize(p_render_ctx->maxFramesInFlight);
		mngr->bdas.resize(p_render_ctx->maxFramesInFlight);
		gpu::buffer_create_n(gpu_ctx, material_buffer_desc, mngr->materialSSBOs);
		gpu::buffer_get_address_n(gpu_ctx, mngr->materialSSBOs, mngr->bdas);

		mngr->dirtyMaterials.resize(p_render_ctx->maxFramesInFlight);

		Array<gpu::EFormat, c_maxMaterialTextures> default_texture_formats{
			gpu::EFormat::eR8G8B8A8Srgb,
			gpu::EFormat::eR8G8B8A8Unorm,
			gpu::EFormat::eR8Unorm,
			gpu::EFormat::eR8Unorm
		};

		constexpr uint32 colour_data{0xFFFFFFFF};
		constexpr uint32 normal_data{0xFFFF0000};
		constexpr uint8  roughness_data{0xFF};
		constexpr uint8  metalness_data{0x00};

		Array<const void *, c_maxMaterialTextures> default_texture_upload_datas{&colour_data, &normal_data, &roughness_data, &metalness_data};
		// I don't want to mess anything up
		Array<uint32, c_maxMaterialTextures> default_texture_upload_sizes{
			sizeof(decltype(colour_data)),
			sizeof(decltype(normal_data)),
			sizeof(decltype(roughness_data)),
			sizeof(decltype(metalness_data))
		};

		const gpu::ResourceDescriptorHeapHandle resource_heap{p_render_ctx->resourceHeap};

		Array<gpu::TextureDesc, c_maxMaterialTextures> default_texture_descs;
		for (uint8 i{0u}; i < c_maxMaterialTextures; ++i)
		{
			gpu::TextureDesc &default_texture_desc{default_texture_descs[i]};
			default_texture_desc.extent      = {1u, 1u, 1u};
			default_texture_desc.mipCount    = 1u;
			default_texture_desc.layerCount  = 1u;
			default_texture_desc.type        = gpu::ETextureType::e2D;
			default_texture_desc.sampleCount = gpu::ESampleCount::e1;
			default_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			default_texture_desc.format      = default_texture_formats[i];
		}

		gpu::texture_create_n(gpu_ctx, default_texture_descs, mngr->defaultTextures);

		// Helps by batching default uploads into one state to be tracked and waited on
		const gpu::StateTrackerHandle default_resource_state_tracker{gpu::state_tracker_register(p_render_ctx->uploadCtx, 1u)};

		gpu::BufferTextureCopyRegion copy_region{};
		copy_region.bufferOffset       = 0ull;
		copy_region.textureOffset      = {0, 0, 0};
		copy_region.textureExtent      = {0u, 0u, 0u};
		copy_region.textureSubresource = gpu::TextureSubresource{0u, 0u, 1u};

		Array<gpu::TextureUploadDesc, c_maxMaterialTextures> upload_descs;

		for (uint8 i{0u}; i < c_maxMaterialTextures; ++i)
		{
			gpu::TextureUploadDesc &upload_desc{upload_descs[i]};
			upload_desc.data          = default_texture_upload_datas[i];
			upload_desc.size          = static_cast<uint64>(default_texture_upload_sizes[i]);
			upload_desc.uploadRegions = copy_region;
		}
		gpu::upload_data_to_textures(upload_ctx, mngr->defaultTextures, upload_descs, default_resource_state_tracker);

		gpu::resource_descriptor_heap_alloc_texture_slots(gpu_ctx, resource_heap, mngr->defaultTextureHeapSlots);
		gpu::resource_descriptor_heap_write_textures(gpu_ctx, resource_heap, mngr->defaultTextureHeapSlots, mngr->defaultTextures, {false, false, false, false}, {
														 gpu::c_allMipLevels,
														 gpu::c_allMipLevels,
														 gpu::c_allMipLevels,
														 gpu::c_allMipLevels
													 });

		gpu::state_tracker_wait_for(p_render_ctx->uploadCtx, default_resource_state_tracker);

		return mngr;
	}

	auto material_mngr_destroy(MaterialManager *p_mngr) -> void
	{
		const RenderContext *                   render_ctx{p_mngr->rndCtx};
		gpu::GPUContext *                       gpu_ctx{render_ctx->gpuCtx};
		gpu::FrameContext *                     frame_ctx{render_ctx->frameCtx};
		const gpu::ResourceDescriptorHeapHandle resource_heap{render_ctx->resourceHeap};

		gpu::defer_buffer_deletion_n(frame_ctx, p_mngr->materialSSBOs);
		p_mngr->materials.clear();

		gpu::resource_descriptor_heap_free_texture_slots(gpu_ctx, resource_heap, p_mngr->defaultTextureHeapSlots);

		gpu::texture_destroy_n(gpu_ctx, p_mngr->defaultTextures);

		mtx_destroy(&p_mngr->textureUploadMtx);
		mtx_destroy(&p_mngr->dirtyMaterialMtx);
	}

	auto material_mngr_poll_pending_textures(MaterialManager *p_mngr) -> void
	{
		mtx_lock(&p_mngr->textureUploadMtx);

		if (p_mngr->pendingMaterialTextureUploads.empty())
		{
			mtx_unlock(&p_mngr->textureUploadMtx);
			return;
		}

		for (auto &[handle, type_mask]: p_mngr->pendingMaterialTextureUploads)
		{
			Material &mat{p_mngr->materials[handle]};

			bool                      need_update{false};
			EMaterialTextureTypeFlags mask_copy{type_mask};
			while (static_cast<uint8>(mask_copy) > 0u)
			{
				const int32 bit_index{std::countr_zero(static_cast<uint8>(mask_copy))};
				const uint8 texture_index{static_cast<uint8>(bit_index)};

				// If the state tracker was removed, the texture should be considered "ready"
				const gpu::StateTrackerHandle state_tracker{mat.stateTrackers[texture_index]};
				if (!state_tracker || gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
				{
					mat.params.heapSlots[texture_index] = mat.shaderReadHeapSlots[texture_index];
					need_update                         = true;
					// Remove the bit from the type mask
					type_mask &= ~static_cast<EMaterialTextureTypeFlagBits>(TST_BIT(bit_index));
				}

				mask_copy &= (EMaterialTextureTypeFlags) ((uint8) mask_copy - 1u);
			}

			if (need_update)
				material_mark_dirty(p_mngr, handle);
		}

		// Erase all the uploads that are ready
		std::erase_if(p_mngr->pendingMaterialTextureUploads, [](const std::pair<MaterialHandle, EMaterialTextureTypeFlags> &p_pair) -> bool
		{
			// Returns true if the type flags are set to 0
			return !p_pair.second;
		});

		mtx_unlock(&p_mngr->textureUploadMtx);
	}

	auto material_mngr_update_dirty_materials(MaterialManager *p_mngr, uint32 p_frame_index) -> void
	{
		mtx_lock(&p_mngr->dirtyMaterialMtx);

		auto &dirty_list{p_mngr->dirtyMaterials[p_frame_index]};
		if (dirty_list.empty())
		{
			mtx_unlock(&p_mngr->dirtyMaterialMtx);
			return;
		}

		for (auto handle: dirty_list)
		{
			const Material *material{p_mngr->materials.try_get(handle)}; // The material may have been destroyed prior to calling this function
			if (!material)
				continue;

			gpu::buffer_write_data(p_mngr->rndCtx->gpuCtx, p_mngr->materialSSBOs[p_frame_index], &material->params, sizeof(MaterialParams),
								   sizeof(MaterialParams) * handle.getId());
		}
		dirty_list.clear();

		mtx_unlock(&p_mngr->dirtyMaterialMtx);
	}

	auto material_mngr_get_material_buffer_address(const MaterialManager *p_mngr, uint32 p_frame_index) -> gpu::DeviceAddress
	{
		return p_mngr->bdas[p_frame_index];
	}

	auto material_mngr_get_default_texture(MaterialManager *p_mngr, EMaterialTextureType p_type) -> MaterialTexture
	{
		const gpu::TextureHandle texture{p_mngr->defaultTextures[TST_UT(p_type)]};
		const uint32             heap_slot{p_mngr->defaultTextureHeapSlots[TST_UT(p_type)]};
		return MaterialTexture{texture, nullptr, heap_slot};
	}

	auto material_create(MaterialManager *p_mngr) -> MaterialHandle
	{
		// Initialise the material with all default textures
		Material material{};
		material.textures            = p_mngr->defaultTextures;
		material.shaderReadHeapSlots = p_mngr->defaultTextureHeapSlots;
		material.params.heapSlots    = p_mngr->defaultTextureHeapSlots;
		material.stateTrackers.fill(nullptr);
		material.params.roughness = 0.0f;
		material.params.metalness = 0.0f;
		material.params.flags     = EMaterialFlagBits::eBackfaceCulling | EMaterialFlagBits::eNoNormalMap | EMaterialFlagBits::eNoRoughnessMap |
									EMaterialFlagBits::eNoMetalnessMap;

		const MaterialHandle out_handle{p_mngr->materials.emplace(material)};
		material_mark_dirty(p_mngr, out_handle);

		return out_handle;
	}

	auto material_destroy(MaterialManager *p_mngr, MaterialHandle p_material) -> void
	{
		p_mngr->materials.destroy(p_material);
		mtx_lock(&p_mngr->textureUploadMtx);
		// Remove the material from the pending texture uploads tracking map
		std::erase_if(p_mngr->pendingMaterialTextureUploads, [p_material](const std::pair<MaterialHandle, EMaterialTextureTypeFlags> &p_pair) -> bool
		{
			return p_pair.first == p_material;
		});
		mtx_unlock(&p_mngr->textureUploadMtx);
	}

	auto material_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material &
	{
		return p_mngr->materials[p_material];
	}

	auto material_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material &
	{
		return p_mngr->materials[p_material];;
	}

	auto material_try_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *
	{
		return p_mngr->materials.try_get(p_material);
	}

	auto material_try_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *
	{
		return p_mngr->materials.try_get(p_material);
	}

	auto material_set_albedo_colour(MaterialManager *p_mngr, MaterialHandle p_material, const math::Float4 &p_colour) -> void
	{
		Material &material{p_mngr->materials[p_material]};
		material.params.albedoColour = p_colour;
		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_texture(MaterialManager *p_mngr, MaterialHandle p_material, EMaterialTextureType p_type, const MaterialTexture &p_texture) -> void
	{
		Material &material{p_mngr->materials[p_material]};

		switch (p_type)
		{
			case EMaterialTextureType::eNormalMap:
			{
				material.params.flags &= ~EMaterialFlagBits::eNoNormalMap;
				break;
			}
			case EMaterialTextureType::eRoughnessMap:
			{
				material.params.flags &= ~EMaterialFlagBits::eNoRoughnessMap;
				break;
			}
			case EMaterialTextureType::eMetalnessMap:
			{
				material.params.flags &= ~EMaterialFlagBits::eNoMetalnessMap;
				break;
			}

			default: break;
		}

		const uint8 texture_index{TST_UT(p_type)};
		material.textures[texture_index]            = p_texture.texture;
		material.stateTrackers[texture_index]       = p_texture.stateTracker;
		material.shaderReadHeapSlots[texture_index] = p_texture.shaderReadHeapSlot;

		// If there is no state tracker, the texture should be considered "ready"
		const gpu::StateTrackerHandle state_tracker{p_texture.stateTracker};
		if (state_tracker && !gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
		{
			mtx_lock(&p_mngr->textureUploadMtx);
			p_mngr->pendingMaterialTextureUploads[p_material] |= static_cast<EMaterialTextureTypeFlagBits>(TST_BIT(TST_UT(p_type)));
			mtx_unlock(&p_mngr->textureUploadMtx);

			material.params.heapSlots[texture_index] = p_mngr->defaultTextureHeapSlots[texture_index];
		}
		else
		{
			TST_ASSERT_MSG(p_texture.shaderReadHeapSlot != UINT32_MAX, "Invalid heap slot");
			material.params.heapSlots[texture_index] = p_texture.shaderReadHeapSlot;
		}

		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_roughness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_roughness) -> void
	{
		Material &material{p_mngr->materials[p_material]};
		material.params.roughness = p_roughness;
		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_metalness(MaterialManager *p_mngr, MaterialHandle p_material, float32 p_metalness) -> void
	{
		Material &material{p_mngr->materials[p_material]};
		material.params.metalness = p_metalness;
		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_flags(MaterialManager *p_mngr, MaterialHandle p_material, EMaterialFlags p_flags, bool p_enable) -> void
	{
		Material &material{p_mngr->materials[p_material]};
		if (p_enable)
			material.params.flags |= p_flags;
		else
			material.params.flags &= ~p_flags;

		material_mark_dirty(p_mngr, p_material);
	}

	auto material_mark_dirty(MaterialManager *p_mngr, MaterialHandle p_material) -> void
	{
		mtx_lock(&p_mngr->dirtyMaterialMtx);

		for (uint32 i{0u}; i < p_mngr->rndCtx->maxFramesInFlight; ++i)
			p_mngr->dirtyMaterials[i].insert(p_material); // Inserts if it is not already present

		mtx_unlock(&p_mngr->dirtyMaterialMtx);
	}
}
