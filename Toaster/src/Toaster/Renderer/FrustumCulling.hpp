#pragma once
#include <glm/glm.hpp>
#include <array>

namespace tst {

	struct AABB
	{
		glm::vec3 min;
		glm::vec3 max;

		AABB() : min(FLT_MAX), max(-FLT_MAX){}
		AABB(const glm::vec3& minPoint, const glm::vec3& maxPoint) : min(minPoint), max(maxPoint) {}

		glm::vec3 getCenter() const { return (min + max) * 0.5f; }
		glm::vec3 getExtent() const { return (max - min) * 0.5f; }

		AABB transform(const glm::mat4& mat) const;
	};

	struct Frustum
	{
		std::array<glm::vec4, 6> planes;

		void extractFromMatrix(const glm::mat4& viewProjection);
		bool isBoxVisible(const glm::vec3& minPoint, const glm::vec3& maxPoint) const;
		bool isBoxVisible(const AABB& aabb) const;
		bool isSphereVisible(const glm::vec3& center, float radius) const;

		bool isBoxVisibleFast(const AABB& aabb) const;
	};

	class CullingSystem
	{
	public:

		void updateFrustum(const glm::mat4& viewProj);
		bool shouldCull(const glm::vec3& pos, const glm::vec3& scale);

		bool shouldCull(const AABB& aabb);

		void cullObjects(const std::vector<AABB>& aabbs, std::vector<bool>& results);

	private:
		Frustum m_Frustum;

		struct FrustumCache {
			glm::mat4 lastViewProj;
			bool valid{ false };
		} m_Cache;

	};


}