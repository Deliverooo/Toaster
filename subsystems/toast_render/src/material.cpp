#include "toast_render/material.hpp"
#include "toast_render/render_context.hpp"
#include "toast_render/texture.hpp"

#include "toast_lib/map.hpp"
#include "toast_lib/pool.hpp"
#include "toast_lib/set.hpp"

#include "toast_gpu/api.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

#include <algorithm>

namespace toaster::rnd
{
	struct MaterialManager
	{
		RenderContext * rndCtx{nullptr};
		TextureManager *textureMngr{nullptr};

		Pool<Material> materials;

		Vector<gpu::DeviceAddress>            bdas;
		Vector<gpu::BufferHandle>             materialSSBOs;
		Vector<UnorderedSet<MaterialHandle> > dirtyMaterials; // Per frame

		enum class EPendingTextureType : uint8
		{
			eAlbedoMap,
			eNormalMap,
			eRoughnessMap,
			eMetalnessMap
		};

		// List of materials which have textures that are not fully uploaded or created
		UnorderedMap<MaterialHandle, UnorderedSet<EPendingTextureType> > pendingMaterialTextureUploads;

		TextureHandle defaultColourMap;
		TextureHandle defaultNormalMap;
		TextureHandle defaultRoughnessMap;
		TextureHandle defaultMetalnessMap;

		mtx_t dirtyMaterialMtx;
		mtx_t textureUploadMtx;
	};

	auto material_mngr_create(RenderContext *p_render_ctx, TextureManager *p_texture_mngr, uint32 p_max_materials) -> MaterialManager *
	{
		const auto mngr{new MaterialManager{}};

		mngr->rndCtx      = p_render_ctx;
		mngr->textureMngr = p_texture_mngr;

		mtx_init(&mngr->dirtyMaterialMtx, mtx_plain);
		mtx_init(&mngr->textureUploadMtx, mtx_plain);

		gpu::GPUContext *gpu_ctx{p_render_ctx->gpuCtx};

		gpu::BufferDesc material_buffer_desc{};
		material_buffer_desc.size       = sizeof(MaterialParams) * p_max_materials;
		material_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		material_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		mngr->materialSSBOs.resize(p_render_ctx->maxFramesInFlight);
		mngr->bdas.resize(p_render_ctx->maxFramesInFlight);
		for (uint32 i{0u}; i < p_render_ctx->maxFramesInFlight; ++i)
		{
			mngr->materialSSBOs[i] = gpu::buffer_create(gpu_ctx, material_buffer_desc);
			mngr->bdas[i]          = gpu::buffer_get_address(gpu_ctx, mngr->materialSSBOs[i]);
		}

		mngr->dirtyMaterials.resize(p_render_ctx->maxFramesInFlight);

		// Helps by batching default uploads into one state to be tracked and waited on
		const gpu::StateTrackerHandle default_resource_state_tracker{gpu::state_tracker_register(p_render_ctx->uploadCtx, 4u)};

		{
			gpu::TextureDesc colour_texture_desc{};
			colour_texture_desc.extent      = {1u, 1u, 1u};
			colour_texture_desc.mipCount    = 1u;
			colour_texture_desc.layerCount  = 1u;
			colour_texture_desc.type        = gpu::ETextureType::e2D;
			colour_texture_desc.sampleCount = gpu::ESampleCount::e1;
			colour_texture_desc.format      = gpu::EFormat::eR8G8B8A8Srgb;
			colour_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			mngr->defaultColourMap          = texture_create(mngr->textureMngr, colour_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(mngr->textureMngr, mngr->defaultColourMap, default_resource_state_tracker);

			uint32 colour_data{0xFFFFFFFF};
			texture_set_data(mngr->textureMngr, mngr->defaultColourMap, &colour_data, sizeof(uint32));
		}
		{
			gpu::TextureDesc normal_texture_desc{};
			normal_texture_desc.extent      = {1u, 1u, 1u};
			normal_texture_desc.mipCount    = 1u;
			normal_texture_desc.layerCount  = 1u;
			normal_texture_desc.type        = gpu::ETextureType::e2D;
			normal_texture_desc.sampleCount = gpu::ESampleCount::e1;
			normal_texture_desc.format      = gpu::EFormat::eR8G8B8A8Unorm;
			normal_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			mngr->defaultNormalMap          = texture_create(mngr->textureMngr, normal_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(mngr->textureMngr, mngr->defaultNormalMap, default_resource_state_tracker);

			uint32 normal_data{0xFFFF0000};
			texture_set_data(mngr->textureMngr, mngr->defaultNormalMap, &normal_data, sizeof(uint32));
		}
		{
			gpu::TextureDesc roughness_texture_desc{};
			roughness_texture_desc.extent      = {1u, 1u, 1u};
			roughness_texture_desc.mipCount    = 1u;
			roughness_texture_desc.layerCount  = 1u;
			roughness_texture_desc.type        = gpu::ETextureType::e2D;
			roughness_texture_desc.sampleCount = gpu::ESampleCount::e1;
			roughness_texture_desc.format      = gpu::EFormat::eR8Unorm;
			roughness_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			mngr->defaultRoughnessMap          = texture_create(mngr->textureMngr, roughness_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(mngr->textureMngr, mngr->defaultRoughnessMap, default_resource_state_tracker);

			uint8 roughness_data{0xFF};
			texture_set_data(mngr->textureMngr, mngr->defaultRoughnessMap, &roughness_data, sizeof(uint8));
		}
		{
			gpu::TextureDesc metalness_texture_desc{};
			metalness_texture_desc.extent      = {1u, 1u, 1u};
			metalness_texture_desc.mipCount    = 1u;
			metalness_texture_desc.layerCount  = 1u;
			metalness_texture_desc.type        = gpu::ETextureType::e2D;
			metalness_texture_desc.sampleCount = gpu::ESampleCount::e1;
			metalness_texture_desc.format      = gpu::EFormat::eR8Unorm;
			metalness_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			mngr->defaultMetalnessMap          = texture_create(mngr->textureMngr, metalness_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(mngr->textureMngr, mngr->defaultMetalnessMap, default_resource_state_tracker);

			uint8 metalness_data{0x00};
			texture_set_data(mngr->textureMngr, mngr->defaultMetalnessMap, &metalness_data, sizeof(uint8));
		}

		gpu::state_tracker_wait_for(p_render_ctx->uploadCtx, default_resource_state_tracker);

		return mngr;
	}

	auto material_mngr_destroy(MaterialManager *p_mngr) -> void
	{
		for (const auto ssbo: p_mngr->materialSSBOs)
			gpu::defer_buffer_deletion(p_mngr->rndCtx->frameCtx, ssbo);

		p_mngr->materials.clear();

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

		for (auto &[handle, types]: p_mngr->pendingMaterialTextureUploads)
		{
			Material &mat{p_mngr->materials[handle]};

			bool need_update{false};
			for (auto type_it{types.begin()}; type_it != types.end();)
			{
				if (*type_it == MaterialManager::EPendingTextureType::eAlbedoMap)
				{
					const Texture *               albedo_map{texture_get(p_mngr->textureMngr, mat.albedoMap)};
					const gpu::StateTrackerHandle state_tracker{albedo_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
					{
						mat.params.albedoMapHeapSlot = albedo_map->shaderReadHeapSlot;
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == MaterialManager::EPendingTextureType::eNormalMap)
				{
					const Texture *               normal_map{texture_get(p_mngr->textureMngr, mat.normalMap)};
					const gpu::StateTrackerHandle state_tracker{normal_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
					{
						mat.params.normalMapHeapSlot = normal_map->shaderReadHeapSlot;
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == MaterialManager::EPendingTextureType::eRoughnessMap)
				{
					const Texture *               roughness_map{texture_get(p_mngr->textureMngr, mat.roughnessMap)};
					const gpu::StateTrackerHandle state_tracker{roughness_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
					{
						mat.params.roughnessMapHeapSlot = roughness_map->shaderReadHeapSlot;
						need_update                     = true;
						type_it                         = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == MaterialManager::EPendingTextureType::eMetalnessMap)
				{
					const Texture *               metalness_map{texture_get(p_mngr->textureMngr, mat.metalnessMap)};
					const gpu::StateTrackerHandle state_tracker{metalness_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
					{
						mat.params.metalnessMapHeapSlot = metalness_map->shaderReadHeapSlot;
						need_update                     = true;
						type_it                         = types.erase(type_it);
						continue;
					}
				}
				++type_it;
			}

			if (need_update)
				material_mark_dirty(p_mngr, handle);
		}

		// Erase all the uploads that are ready
		std::erase_if(p_mngr->pendingMaterialTextureUploads, [](const auto &pair) -> bool { return pair.second.empty(); });

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
			const Material *material{p_mngr->materials.tryGet(handle)}; // The material may have been destroyed prior to calling this function
			if (!material)
				continue;

			gpu::buffer_write_data(p_mngr->rndCtx->gpuCtx, p_mngr->materialSSBOs[p_frame_index], &material->params, sizeof(MaterialParams),
								   sizeof(MaterialParams) * handle.getId());
		}
		dirty_list.clear();

		mtx_unlock(&p_mngr->dirtyMaterialMtx);
	}

	auto material_mngr_get_material_buffer_address(MaterialManager *p_mngr, uint32 p_frame_index) -> gpu::DeviceAddress
	{
		return p_mngr->bdas[p_frame_index];
	}

	auto material_mngr_get_default_colour_map(MaterialManager *p_mngr) -> TextureHandle
	{
		return p_mngr->defaultColourMap;
	}

	auto material_mngr_get_default_normal_map(MaterialManager *p_mngr) -> TextureHandle
	{
		return p_mngr->defaultNormalMap;
	}

	auto material_mngr_get_default_roughness_map(MaterialManager *p_mngr) -> TextureHandle
	{
		return p_mngr->defaultRoughnessMap;
	}

	auto material_mngr_get_default_metalness_map(MaterialManager *p_mngr) -> TextureHandle
	{
		return p_mngr->defaultMetalnessMap;
	}

	auto material_create(MaterialManager *p_mngr) -> MaterialHandle
	{
		Material material{};
		material.albedoMap                   = p_mngr->defaultColourMap;
		material.normalMap                   = p_mngr->defaultNormalMap;
		material.params.albedoMapHeapSlot    = texture_get(p_mngr->textureMngr, p_mngr->defaultColourMap)->shaderReadHeapSlot;
		material.params.normalMapHeapSlot    = texture_get(p_mngr->textureMngr, p_mngr->defaultNormalMap)->shaderReadHeapSlot;
		material.params.roughnessMapHeapSlot = texture_get(p_mngr->textureMngr, p_mngr->defaultRoughnessMap)->shaderReadHeapSlot;
		material.params.metalnessMapHeapSlot = texture_get(p_mngr->textureMngr, p_mngr->defaultMetalnessMap)->shaderReadHeapSlot;
		material.params.roughness            = 0.0f;
		material.params.metalness            = 0.0f;
		material.params.flags                = EMaterialFlagBits::eBackfaceCulling | EMaterialFlagBits::eNoNormalMap | EMaterialFlagBits::eNoRoughnessMap |
											   EMaterialFlagBits::eNoMetalnessMap;

		const MaterialHandle out_handle{p_mngr->materials.emplace(material)};
		material_mark_dirty(p_mngr, out_handle);

		return out_handle;
	}

	auto material_destroy(MaterialManager *p_mngr, MaterialHandle p_material) -> void
	{
		p_mngr->materials.destroy(p_material);
	}

	auto material_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *
	{
		return &p_mngr->materials[p_material];
	}

	auto material_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *
	{
		return &p_mngr->materials[p_material];;
	}

	auto material_try_get(MaterialManager *p_mngr, MaterialHandle p_material) -> Material *
	{
		return p_mngr->materials.tryGet(p_material);
	}

	auto material_try_get(const MaterialManager *p_mngr, MaterialHandle p_material) -> const Material *
	{
		return p_mngr->materials.tryGet(p_material);
	}

	auto material_set_albedo_colour(MaterialManager *p_mngr, MaterialHandle p_material, const math::Float4 &p_colour) -> void
	{
		Material &material{p_mngr->materials[p_material]};
		material.params.albedoColour = p_colour;
		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_albedo_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_albedo_map) -> void
	{
		Material &material{p_mngr->materials[p_material]};

		material.albedoMap = p_albedo_map;

		const Texture *albedo_map{texture_get(p_mngr->textureMngr, p_albedo_map)};

		const gpu::StateTrackerHandle state_tracker{albedo_map->stateTracker};

		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
		{
			mtx_lock(&p_mngr->textureUploadMtx);
			p_mngr->pendingMaterialTextureUploads[p_material].insert(MaterialManager::EPendingTextureType::eAlbedoMap);
			mtx_unlock(&p_mngr->textureUploadMtx);

			material.params.albedoMapHeapSlot = texture_get(p_mngr->textureMngr, p_mngr->defaultColourMap)->shaderReadHeapSlot;
		}
		else
			material.params.albedoMapHeapSlot = albedo_map->shaderReadHeapSlot;

		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_normal_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_normal_map) -> void
	{
		Material &material{p_mngr->materials[p_material]};

		material.params.flags &= ~EMaterialFlagBits::eNoNormalMap;
		material.normalMap    = p_normal_map;

		const Texture *normal_map{texture_get(p_mngr->textureMngr, p_normal_map)};

		const gpu::StateTrackerHandle state_tracker{normal_map->stateTracker};
		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
		{
			mtx_lock(&p_mngr->textureUploadMtx);
			p_mngr->pendingMaterialTextureUploads[p_material].insert(MaterialManager::EPendingTextureType::eNormalMap);
			mtx_unlock(&p_mngr->textureUploadMtx);

			material.params.normalMapHeapSlot = texture_get(p_mngr->textureMngr, p_mngr->defaultNormalMap)->shaderReadHeapSlot;
		}
		else
			material.params.normalMapHeapSlot = normal_map->shaderReadHeapSlot;

		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_roughness_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_roughness_map) -> void
	{
		Material &material{p_mngr->materials[p_material]};

		material.params.flags &= ~EMaterialFlagBits::eNoRoughnessMap;
		material.roughnessMap = p_roughness_map;

		const Texture *roughness_map{texture_get(p_mngr->textureMngr, p_roughness_map)};

		const gpu::StateTrackerHandle state_tracker{roughness_map->stateTracker};
		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
		{
			mtx_lock(&p_mngr->textureUploadMtx);
			p_mngr->pendingMaterialTextureUploads[p_material].insert(MaterialManager::EPendingTextureType::eRoughnessMap);
			mtx_unlock(&p_mngr->textureUploadMtx);

			material.params.roughnessMapHeapSlot = texture_get(p_mngr->textureMngr, p_mngr->defaultRoughnessMap)->shaderReadHeapSlot;
		}
		else
			material.params.roughnessMapHeapSlot = roughness_map->shaderReadHeapSlot;

		material_mark_dirty(p_mngr, p_material);
	}

	auto material_set_metalness_map(MaterialManager *p_mngr, MaterialHandle p_material, TextureHandle p_metalness_map) -> void
	{
		Material &material{p_mngr->materials[p_material]};

		material.params.flags &= ~EMaterialFlagBits::eNoMetalnessMap;
		material.metalnessMap = p_metalness_map;

		const Texture *metalness_map{texture_get(p_mngr->textureMngr, p_metalness_map)};

		const gpu::StateTrackerHandle state_tracker{metalness_map->stateTracker};
		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, state_tracker))
		{
			mtx_lock(&p_mngr->textureUploadMtx);
			p_mngr->pendingMaterialTextureUploads[p_material].insert(MaterialManager::EPendingTextureType::eMetalnessMap);
			mtx_unlock(&p_mngr->textureUploadMtx);

			material.params.metalnessMapHeapSlot = texture_get(p_mngr->textureMngr, p_mngr->defaultMetalnessMap)->shaderReadHeapSlot;
		}
		else
			material.params.metalnessMapHeapSlot = metalness_map->shaderReadHeapSlot;

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
