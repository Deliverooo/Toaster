#include "tstpch.h"
#include "MeshRenderer.hpp"

#include "RenderCommand.hpp"
#include "Shader.hpp"
#include "Toaster/Util/MathUtil.hpp"

#include "glm/gtc/type_ptr.hpp"


// TODO - Remove This is is for debugging purposes only
//#include <glad/glad.h>

namespace tst
{

	struct MeshRendererData
	{
		RefPtr<Shader> meshShader;
		RefPtr<Texture2D> whiteTexture;
		RefPtr<Texture3D> skyboxTexture;
		glm::mat4 viewProjectionMatrix;
		glm::vec3 cameraPosition;

		struct LightData
		{
			glm::vec3 position{ 1.0f, 1.0f, 1.0f };
			glm::vec3 direction{ 0.0f, 0.0f, 0.0f };
			glm::vec3 colour{ 1.0f, 1.0f, 1.0f };
			float intensity = 0.5f;
			int lightType{ 1 };
		};

		std::vector<LightData> lights;
		uint32_t lightIndex = 0;

		MeshRenderer::Stats stats{};
	};

	static MeshRendererData render_data;


	void MeshRenderer::init()
	{
		RenderCommand::init();

		render_data.whiteTexture = Texture2D::create(1, 1);
		TST_ASSERT(render_data.whiteTexture != nullptr, "No");

		uint32_t texData = 0xffffffff;
		render_data.whiteTexture->setData(&texData);

		render_data.skyboxTexture = Texture3D::create({
			TST_CORE_RESOURCE_DIR"/cubemaps/right.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/left.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/top.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/bottom.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/front.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/back.jpg" });

		render_data.meshShader = Shader::create("MeshShader", TST_CORE_RESOURCE_DIR"/shaders/MeshShader.vert.glsl", TST_CORE_RESOURCE_DIR"/shaders/MeshShader.frag.glsl");

		render_data.lights.reserve(32);
	}

	void MeshRenderer::begin(const Camera& camera, const glm::mat4& transform)
	{
		auto& projection = camera.getProjection();
		auto view = glm::inverse(transform);

		render_data.viewProjectionMatrix = projection * view;
		render_data.cameraPosition = glm::vec3(transform[3]);

		render_data.meshShader->bind();
		render_data.meshShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
		render_data.meshShader->uploadVector3f(render_data.cameraPosition, "u_ViewPos");

		render_data.lights.clear();
		render_data.lightIndex = 0;
	}

	void MeshRenderer::flushLights()
	{
		render_data.meshShader->bind();

		RenderCommand::checkError("Binding shader in flushLights");

		// Upload light count first
		render_data.meshShader->uploadInt1(static_cast<int>(render_data.lightIndex), "u_LightCount");
		RenderCommand::checkError("Uploading light count");

		// If no lights, we're done
		if (render_data.lightIndex == 0) {
			render_data.meshShader->uploadInt1(0, "u_LightCount");
			return;
		}

		// Upload all light data with proper validation
		for (uint32_t i = 0; i < render_data.lightIndex && i < render_data.lights.size() && i < 32; i++)
		{
			const auto& light = render_data.lights[i];

			// Try a simpler approach - check if the base uniform exists first
			std::string baseUniformTest = "u_Lights[0].position";

			if (!render_data.meshShader->hasUniform(baseUniformTest.c_str())) {
				TST_CORE_ERROR("Light uniform array 'u_Lights' not found in shader! Available uniforms listed above.");
				break;
			}

			// Create uniform names
			std::string base = "u_Lights[" + std::to_string(i) + "].";

			// Upload with validation

			if (render_data.meshShader->hasUniform((base + "position").c_str())) {
				render_data.meshShader->uploadVector3f(light.position, (base + "position").c_str());
				RenderCommand::checkError("Uploading light position " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "direction").c_str())) {
				render_data.meshShader->uploadVector3f(light.direction, (base + "direction").c_str());
				RenderCommand::checkError("Uploading light direction " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "colour").c_str())) {
				render_data.meshShader->uploadVector3f(light.colour, (base + "colour").c_str());
				RenderCommand::checkError("Uploading light colour " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "intensity").c_str())) {
				render_data.meshShader->uploadFloat1(light.intensity, (base + "intensity").c_str());
				RenderCommand::checkError("Uploading light intensity " + std::to_string(i));
			}
		}

		render_data.lightIndex = 0;
		render_data.lights.clear();
	}

	void MeshRenderer::terminate() noexcept
	{
		TST_CORE_TRACE("MeshRenderer terminate called");
	}

	void MeshRenderer::end()
	{
		render_data.lightIndex = 0;
		render_data.lights.clear();
	}

	void MeshRenderer::uploadLightingData(const Light& light, const glm::vec3& lightPosition, const glm::vec3& lightDirection)
	{
		if (render_data.lightIndex >= 32)
		{
			TST_CORE_WARN("MeshRenderer: Exceeded maximum light count");
			return;
		}

		MeshRendererData::LightData lightData;
		lightData.position = lightPosition;
		lightData.direction = lightDirection;
		lightData.colour = light.colour;
		lightData.intensity = light.intensity;

		if (render_data.lightIndex >= render_data.lights.size())
		{
			render_data.lights.push_back(lightData);
		}
		else
		{
			render_data.lights[render_data.lightIndex] = lightData;
		}

		render_data.lightIndex++;
	}




	void MeshRenderer::drawMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform)
	{
		if (!mesh) {
			return;
		}

		render_data.meshShader->bind();
		// Upload transformation matrix
		render_data.meshShader->uploadMatrix4f(transform, "u_Model");

		// Calculate and upload normal matrix for proper lighting
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
		render_data.meshShader->uploadMatrix3f(normalMatrix, "u_NormalMatrix");

		// Enable depth testing for proper 3D rendering
		RenderCommand::enableDepthTesting();

		mesh->bind();

		// Render each submesh with its material
		const auto& submeshes = mesh->getSubMeshes();
		const auto& materials = mesh->getMaterials();

		render_data.skyboxTexture->bind(0);
		render_data.meshShader->uploadInt1(0, "u_Skybox");

		if (submeshes.empty()) {
			TST_CORE_WARN("Submeshes empty");
		}
		else {
			// Render each submesh with its material
			for (const auto& submesh : submeshes)
			{
				// Get the material for this submesh
				auto material = materials.getMaterial(submesh.materialIndex);

				material->bind(render_data.meshShader);

				if (material->getMaterialProperties().backfaceCulling)
				{
					RenderCommand::enableBackfaceCulling();
				}
				// Draw the submesh
				RenderCommand::drawIndexedBaseVertex(
					mesh->getVertexArray(),
					submesh.indexCount,
					submesh.indexOffset,
					0
				);

				RenderCommand::disableBackfaceCulling();

				render_data.stats.drawCallCount++;
				render_data.stats.triangleCount += submesh.indexCount / 3;
			}
		}

		// **CRITICAL FIX**: Properly clean up after mesh rendering
		mesh->unbind();

		RenderCommand::cleanState();

		// Check for any OpenGL errors after mesh rendering
		RenderCommand::checkError("After MeshRenderer cleanup");

		render_data.stats.meshCount++;
	}



	void MeshRenderer::drawMesh(const RefPtr<Mesh>& mesh, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawMesh(mesh, transform);
	}


	MeshRenderer::Stats MeshRenderer::getStats()
	{
		return render_data.stats;
	}

	void MeshRenderer::resetStats()
	{
		render_data.stats.drawCallCount = 0;
		render_data.stats.triangleCount = 0;
		render_data.stats.textureBindings = 0;
		render_data.stats.verticesSubmitted = 0;
		render_data.stats.meshCount = 0;
	}
}
