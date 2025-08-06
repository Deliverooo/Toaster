#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace tst {

    // Simplified light data structure
    struct Light
    {
        enum class Type
        {
            Directional = 0,
            Point = 1,
            Spot = 2
        };

        Type type{ Type::Directional };
        glm::vec3 colour{ 1.0f };
        float intensity{ 1.0f };

        // Attenuation (only for point/spot lights)
        float constant{ 1.0f };
        float linear{ 0.09f };
        float quadratic{ 0.032f };

        // Spot light specific
        float innerCone{ 40.0f };
        float outerOffset{ 0.1f };
        //float outerCone{ 40.0f };

        
    };
}