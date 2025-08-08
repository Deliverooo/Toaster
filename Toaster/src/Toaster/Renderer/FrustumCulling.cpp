#include "tstpch.h"
#include "FrustumCulling.hpp"

namespace tst
{

    AABB AABB::transform(const glm::mat4& mat) const
    {
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };

        AABB result;
        for (int i = 0; i < 8; ++i) {
            glm::vec3 transformed = glm::vec3(mat * glm::vec4(corners[i], 1.0f));
            result.min = glm::min(result.min, transformed);
            result.max = glm::max(result.max, transformed);
        }

        return result;
    }

	void Frustum::extractFromMatrix(const glm::mat4& viewProjection)
	{
        // Left plane
        planes[0] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][0],
            viewProjection[1][3] + viewProjection[1][0],
            viewProjection[2][3] + viewProjection[2][0],
            viewProjection[3][3] + viewProjection[3][0]
        );

        // Right plane
        planes[1] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][0],
            viewProjection[1][3] - viewProjection[1][0],
            viewProjection[2][3] - viewProjection[2][0],
            viewProjection[3][3] - viewProjection[3][0]
        );

        // Bottom plane
        planes[2] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][1],
            viewProjection[1][3] + viewProjection[1][1],
            viewProjection[2][3] + viewProjection[2][1],
            viewProjection[3][3] + viewProjection[3][1]
        );

        // Top plane
        planes[3] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][1],
            viewProjection[1][3] - viewProjection[1][1],
            viewProjection[2][3] - viewProjection[2][1],
            viewProjection[3][3] - viewProjection[3][1]
        );

        // Near plane
        planes[4] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][2],
            viewProjection[1][3] + viewProjection[1][2],
            viewProjection[2][3] + viewProjection[2][2],
            viewProjection[3][3] + viewProjection[3][2]
        );

        // Far plane
        planes[5] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][2],
            viewProjection[1][3] - viewProjection[1][2],
            viewProjection[2][3] - viewProjection[2][2],
            viewProjection[3][3] - viewProjection[3][2]
        );

        for (auto& plane : planes)
        {
            float length = glm::length(glm::vec3(plane));
            if (length > 0.0f)
            {
                plane /= length;
            }
        }
	}

    bool Frustum::isBoxVisible(const glm::vec3& minPoint, const glm::vec3& maxPoint) const
    {
	    for (const auto &plane : planes)
	    {
			glm::vec3 normal = glm::vec3(plane);
			float distance = plane.w;

            glm::vec3 positiveVertex = minPoint;
            if (normal.x >= 0.0f) positiveVertex.x = maxPoint.x;
            if (normal.y >= 0.0f) positiveVertex.y = maxPoint.y;
            if (normal.z >= 0.0f) positiveVertex.z = maxPoint.z;

            if (glm::dot(normal, positiveVertex) + distance < 0.0f)
            {
                return false;
            }
	    }

        return true;
    }

    bool Frustum::isSphereVisible(const glm::vec3& center, float radius) const
    {
        for (const auto& plane : planes)
        {
            glm::vec3 normal = glm::vec3(plane);
            float distance = plane.w;

            float distanceToPlane = glm::dot(normal, center) + distance;

            if (distanceToPlane < -radius)
            {
                return false;
            }
        }

        return true;
    }

    bool Frustum::isBoxVisible(const AABB& aabb) const
    {
        return isBoxVisible(aabb.min, aabb.max);
    }

    void CullingSystem::updateFrustum(const glm::mat4& viewProj)
    {
        // Only update if view-projection matrix changed
        if (!m_Cache.valid || m_Cache.lastViewProj != viewProj)
        {
            m_Frustum.extractFromMatrix(viewProj);
            m_Cache.lastViewProj = viewProj;
            m_Cache.valid = true;
        }
    }

    bool CullingSystem::shouldCull(const glm::vec3& pos, const glm::vec3& scale)
    {
        glm::vec3 halfExtents = scale * 0.5f;
        AABB aabb(pos - halfExtents, pos + halfExtents);

        return !m_Frustum.isBoxVisibleFast(aabb);
    }

    bool CullingSystem::shouldCull(const AABB& aabb)
    {
        return !m_Frustum.isBoxVisibleFast(aabb);
    }

    void CullingSystem::cullObjects(const std::vector<AABB>& aabbs, std::vector<bool>& results)
    {
        results.resize(aabbs.size());

        for (size_t i = 0; i < aabbs.size(); ++i)
        {
            results[i] = !m_Frustum.isBoxVisibleFast(aabbs[i]);
        }
    }

    bool Frustum::isBoxVisibleFast(const AABB& aabb) const
    {
        const glm::vec3& minPoint = aabb.min;
        const glm::vec3& maxPoint = aabb.max;

        // Test each plane
        for (int i = 0; i < 6; ++i)
        {
            const glm::vec4& plane = planes[i];
            const glm::vec3 normal = glm::vec3(plane);

            const float px = (normal.x >= 0.0f) ? maxPoint.x : minPoint.x;
            const float py = (normal.y >= 0.0f) ? maxPoint.y : minPoint.y;
            const float pz = (normal.z >= 0.0f) ? maxPoint.z : minPoint.z;

            if (normal.x * px + normal.y * py + normal.z * pz + plane.w < 0.0f)
            {
                return false;
            }
        }

        return true;
    }







}
