#include "tstpch.h"
#include "RenderQueue.hpp"

namespace tst
{
	uint32_t SubMeshRenderCommand::calcSortKey() const
	{
		uint32_t key = 0;

		if (material)
		{
			key |= (std::hash<void*>{}(material.get()) & 0xFFFF);
		}

		if (mesh)
		{
			key |= (std::hash<void*>{}(mesh.get()) & 0xFFFF);
		}

		return key;
	}


	void SubMeshRenderCommand::calcWorldBounds()
	{
		worldPosition = glm::vec3(transform[3]);

		glm::vec3 scale = glm::vec3(
			glm::length(glm::vec3(transform[0])),
			glm::length(glm::vec3(transform[1])),
			glm::length(glm::vec3(transform[2]))
		);

		boundingSphereRadius = glm::max(scale.x, glm::max(scale.y, scale.z));

		glm::vec3 halfExtents = scale * 0.5f;
		worldAABB = AABB(worldPosition - halfExtents, worldPosition + halfExtents);
	}

	void SubMeshRenderCommand::calcWorldBounds(const AABB& meshAABB)
	{
		worldPosition = glm::vec3(transform[3]);

		worldAABB = meshAABB.transform(transform);

		glm::vec3 extent = worldAABB.getExtent();
		boundingSphereRadius = glm::length(extent);
	}

	RenderQueue::RenderQueue()
	{
		m_stats.reset();
	}


	void RenderQueue::submitMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform, uint32_t entityId, const glm::vec3& cameraPos)
	{
		if (!mesh) { return; }

		const auto& submeshes = mesh->getSubMeshes();
		const auto& materials = mesh->getMaterials();

		float distance = glm::length(glm::vec3(transform[3]) - cameraPos);

		std::vector<SubMeshRenderCommand> candidates;
		candidates.reserve(submeshes.size());

		for (uint32_t i = 0; i < submeshes.size(); i++)
		{
			const auto& submesh = submeshes[i];
			auto material = materials.getMaterial(submesh.materialIndex);

			if (!material) { continue; }

			SubMeshRenderCommand smCommand;
			smCommand.mesh = mesh;
			smCommand.material = material;
			smCommand.transform = transform;
			smCommand.entityId = entityId;
			smCommand.distanceToCamera = distance;
			smCommand.submeshIndex = i;

			smCommand.indexOffset = submesh.indexOffset;
			smCommand.indexCount = submesh.indexCount;
			smCommand.materialIndex = submesh.materialIndex;

			smCommand.calcWorldBounds();

			candidates.push_back(smCommand);
			m_stats.totalSubmitted++;
		}

		if (m_cullingSystem && !candidates.empty())
		{
			std::vector<AABB> aabbs;
			aabbs.reserve(candidates.size());

			for (const auto& cmd : candidates)
			{
				aabbs.push_back(cmd.worldAABB);
			}

			std::vector<bool> cullResults;
			m_cullingSystem->cullObjects(aabbs, cullResults);

			m_stats.frustumTests += candidates.size();

			for (size_t i = 0; i < candidates.size(); ++i)
			{
				if (cullResults[i])
				{
					m_stats.culled++;
					continue;
				}

				candidates[i].sortKey = candidates[i].calcSortKey();
				RenderPass pass = RenderPass::Opaque;
				m_renderPasses[pass].push_back(candidates[i]);
				m_stats.rendered++;
			}
		} else
		{
			for (auto& cmd : candidates)
			{
				cmd.sortKey = cmd.calcSortKey();
				RenderPass pass = RenderPass::Opaque;
				m_renderPasses[pass].push_back(cmd);
				m_stats.rendered++;
			}
		}
	}

	void RenderQueue::sort()
	{
		auto& opaqueCommands = m_renderPasses[RenderPass::Opaque];
		std::sort(opaqueCommands.begin(), opaqueCommands.end(), sortOpaque);

		auto& transparentCommands = m_renderPasses[RenderPass::Transparent];
		std::sort(transparentCommands.begin(), transparentCommands.end(), sortTransparent);
	}

	const std::vector<SubMeshRenderCommand>& RenderQueue::getCommands(RenderPass pass) const
	{
		static std::vector<SubMeshRenderCommand> fallback;
		auto it = m_renderPasses.find(pass);
		return (it != m_renderPasses.end()) ? it->second : fallback;
	}


	bool RenderQueue::sortOpaque(const SubMeshRenderCommand& a, const SubMeshRenderCommand& b)
	{
		if (a.sortKey != b.sortKey) {
			return a.sortKey < b.sortKey;
		}

		return a.distanceToCamera < b.distanceToCamera;
	}
	bool RenderQueue::sortTransparent(const SubMeshRenderCommand& a, const SubMeshRenderCommand& b)
	{
		// Sort back to front for correct alpha blending
		return a.distanceToCamera > b.distanceToCamera;
	}

	void RenderQueue::clear()
	{
		for (auto& [pass, commands] : m_renderPasses)
		{
			commands.clear();
		}
		m_stats.reset();
	}



}