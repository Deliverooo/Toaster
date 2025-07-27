#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace tst
{

	class TST_API Transform
	{
	public:

		Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);

		static Transform create2D(const glm::vec2 &positon, const float rotation, const glm::vec2 &scale);
		static Transform create3D(const glm::vec3 &positon, const glm::vec3 &rotation, const glm::vec3 &scale);


		glm::mat4 getMatrix()		 const { return m_matrix; }
		glm::mat4 getInverseMatrix() const { return glm::inverse(m_matrix); }

		glm::vec3 position{ 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 scale	  { 1.0f, 1.0f, 1.0f };

	private:

		glm::mat4 m_matrix;
	};
}