#include "tstpch.h"
#include "MeshRenderer.hpp"

#include "RenderCommand.hpp"
#include "Shader.hpp"
#include "Toaster/Util/MathUtil.hpp"


namespace tst
{

	struct MeshRendererData
	{
		RefPtr<Shader> meshShader;
		RefPtr<Texture2D> whiteTexture;
		glm::mat4 viewProjectionMatrix;
		glm::vec3 cameraPosition;

		struct LightData
		{
			glm::vec3 position{ 5.0f, 5.0f, 5.0f };
			glm::vec3 colour{ 1.0f, 1.0f, 1.0f };
			float intensity{ 1.0f };
		} mainLight;



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

		
		render_data.meshShader = Shader::create("MeshShader", TST_CORE_RESOURCE_DIR"/shaders/MeshShader.glsl");
	}

	void MeshRenderer::begin(const Camera& camera, const glm::mat4& transform)
	{
		auto& projection = camera.getProjection();
		auto view = glm::inverse(transform);

		render_data.viewProjectionMatrix = projection * view;
		render_data.cameraPosition = glm::vec3(transform[3]); // Extract camera position from transform

		render_data.meshShader->bind();
		render_data.meshShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
		render_data.meshShader->uploadVector3f(render_data.cameraPosition, "u_ViewPos");

		// Upload lighting data
		render_data.meshShader->uploadVector3f(render_data.mainLight.position, "u_LightPos");
		render_data.meshShader->uploadVector3f(render_data.mainLight.colour * render_data.mainLight.intensity, "u_LightColour");
	}


	void MeshRenderer::terminate() noexcept
	{
		TST_CORE_TRACE("MeshRenderer terminate called");
	}

	void MeshRenderer::end()
	{

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

		mesh->unbind();
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