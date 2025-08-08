#include "tstpch.h"
#include "MeshRenderer.hpp"

#include "DebugRenderer.hpp"
#include "RenderCommand.hpp"
#include "Shader.hpp"
#include "Toaster/Util/MathUtil.hpp"

#include "glm/gtc/type_ptr.hpp"


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
			int lightType{ 0 };
			glm::vec3 position{ 1.0f, 1.0f, 1.0f };
			glm::vec3 direction{ 0.0f, 0.0f, 0.0f };
			glm::vec3 colour{ 1.0f, 1.0f, 1.0f };
			float intensity = 0.5f;

			float constant = 1.0f; // Attenuation
			float linear = 0.09f;   // Attenuation
			float quadratic = 0.032f; // Attenuation

			float innerCone = glm::radians(12.5f); // Spot light inner cone angle
			float outerCone = glm::radians(17.5f); // Spot light outer cone angle
		};

		std::vector<LightData> lights;
		uint32_t lightIndex = 0;

		MeshRenderer::Stats stats{};

		RenderQueue renderQueue;
		CullingSystem cullingSystem;

		std::vector<MeshRenderer::MaterialBatch> materialBatches;
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

		render_data.renderQueue.setCullingSystem(&render_data.cullingSystem);

		DebugRenderer::init();
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

	void MeshRenderer::begin(const EditorCamera& camera)
	{
		auto& projection = camera.getProjection();
		auto view = camera.getViewMatrix();

		render_data.viewProjectionMatrix = projection * view;
		render_data.cameraPosition = camera.getPosition();

		render_data.cullingSystem.updateFrustum(render_data.viewProjectionMatrix);

		render_data.meshShader->bind();
		render_data.meshShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
		render_data.meshShader->uploadVector3f(render_data.cameraPosition, "u_ViewPos");

		render_data.lights.clear();
		render_data.lightIndex = 0;

		render_data.renderQueue.clear();
		render_data.materialBatches.clear();
	}


	void MeshRenderer::flushLights()
	{
		render_data.meshShader->bind();

		TST_RC_CHECK_ERROR("Binding shader in flushLights");

		// Upload light count first
		render_data.meshShader->uploadInt1(static_cast<int>(render_data.lightIndex), "u_LightCount");
		TST_RC_CHECK_ERROR("Uploading light count");

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

			if (render_data.meshShader->hasUniform((base + "type").c_str()))
			{
				render_data.meshShader->uploadInt1(light.lightType, (base + "type").c_str());
				TST_RC_CHECK_ERROR("Uploading light type " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "position").c_str())) {
				render_data.meshShader->uploadVector3f(light.position, (base + "position").c_str());
				TST_RC_CHECK_ERROR("Uploading light position " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "direction").c_str())) {
				render_data.meshShader->uploadVector3f(light.direction, (base + "direction").c_str());
				TST_RC_CHECK_ERROR("Uploading light direction " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "colour").c_str())) {
				render_data.meshShader->uploadVector3f(light.colour, (base + "colour").c_str());
				TST_RC_CHECK_ERROR("Uploading light colour " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "intensity").c_str())) {
				render_data.meshShader->uploadFloat1(light.intensity, (base + "intensity").c_str());
				TST_RC_CHECK_ERROR("Uploading light intensity " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "constant").c_str())) {
				render_data.meshShader->uploadFloat1(light.constant, (base + "constant").c_str());
				TST_RC_CHECK_ERROR("Uploading light constant " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "linear").c_str())) {
				render_data.meshShader->uploadFloat1(light.linear, (base + "linear").c_str());
				TST_RC_CHECK_ERROR("Uploading light linear " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "quadratic").c_str())) {
				render_data.meshShader->uploadFloat1(light.quadratic, (base + "quadratic").c_str());
				TST_RC_CHECK_ERROR("Uploading light quadratic " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "innerCone").c_str())) {
				render_data.meshShader->uploadFloat1(light.innerCone, (base + "innerCone").c_str());
				TST_RC_CHECK_ERROR("Uploading light innerCone " + std::to_string(i));
			}

			if (render_data.meshShader->hasUniform((base + "outerCone").c_str())) {
				render_data.meshShader->uploadFloat1(light.outerCone, (base + "outerCone").c_str());
				TST_RC_CHECK_ERROR("Uploading light outerCone " + std::to_string(i));
			}
		}

		render_data.lightIndex = 0;
		render_data.lights.clear();
	}

	void MeshRenderer::terminate() noexcept
	{
		TST_CORE_TRACE("MeshRenderer terminate called");

		DebugRenderer::terminate();
	}

	void MeshRenderer::end()
	{
		render_data.lightIndex = 0;
		render_data.lights.clear();
	}

	void MeshRenderer::flush()
	{
		render_data.renderQueue.sort();

		renderPass(RenderPass::ZPrePass);
		renderPass(RenderPass::Opaque);
		renderPass(RenderPass::AlphaTest);
		renderPass(RenderPass::Transparent);

		const auto& queueStats = render_data.renderQueue.getStats();
		render_data.stats.culledObjects = queueStats.culled;
		render_data.stats.totalSubmitted = queueStats.totalSubmitted;

	}

	void MeshRenderer::renderPass(RenderPass pass)
	{
		const auto& commands = render_data.renderQueue.getCommands(pass);

		if (commands.empty()) { return; }

		groupByMaterial(commands);

		for (const auto &batch : render_data.materialBatches)
		{
			renderMaterialBatch(batch);
		}
	}

	void MeshRenderer::groupByMaterial(const std::vector<SubMeshRenderCommand>& commands)
	{
		render_data.materialBatches.clear();

		std::unordered_map<RefPtr<Material>, size_t> materialToIndex;

		for (const auto &command : commands)
		{
			auto it = materialToIndex.find(command.material);

			if (it == materialToIndex.end())
			{
				MaterialBatch batch;
				batch.material = command.material;
				batch.commands.push_back(command);

				materialToIndex[command.material] = render_data.materialBatches.size();
				render_data.materialBatches.push_back(batch);
			} else
			{
				render_data.materialBatches[it->second].commands.push_back(command);
			}
		}
	}

	void MeshRenderer::renderMaterialBatch(const MaterialBatch& batch)
	{
		batch.material->bind(render_data.meshShader);

		bool backFaceCull = batch.material->getRenderState().backfaceCulling;
		if (backFaceCull) { RenderCommand::enableBackfaceCulling(); }


		for (const auto &command : batch.commands)
		{
			renderSubmesh(command);
		}

		batch.material->unbind();

		if (backFaceCull) { RenderCommand::disableBackfaceCulling(); }
	}

	void MeshRenderer::renderSubmesh(const SubMeshRenderCommand& command)
	{

		//TST_CORE_INFO("Rendering submesh {0} With material index {1} ({2})", command.submeshIndex, command.materialIndex, command.material ? command.material->getName() : "Null");

		render_data.meshShader->uploadMatrix4f(command.transform, "u_Model");

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform)));
		render_data.meshShader->uploadMatrix3f(normalMatrix, "u_NormalMatrix");

		command.mesh->bind();

		RenderCommand::drawIndexedBaseVertex(command.mesh->getVertexArray(), command.indexCount, command.indexOffset, 0);

		render_data.stats.drawCallCount++;
		render_data.stats.triangleCount += command.indexCount / 3;
	}

	void MeshRenderer::uploadLightingData(const Light& light, const glm::vec3& lightPosition, const glm::vec3& lightDirection)
	{
		if (render_data.lightIndex >= 32)
		{
			TST_CORE_WARN("MeshRenderer: Exceeded maximum light count");
			return;
		}

		MeshRendererData::LightData lightData;
		lightData.lightType = static_cast<int>(light.type);
		lightData.position = lightPosition;
		lightData.direction = lightDirection;
		lightData.colour = light.colour;
		lightData.intensity = light.intensity;

		lightData.constant = light.constant;
		lightData.linear = light.linear;
		lightData.quadratic = light.quadratic;

		lightData.innerCone = light.innerCone;
		lightData.outerCone = light.innerCone + light.outerOffset;

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

		render_data.skyboxTexture->bind();
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

				if (material->getRenderState().backfaceCulling)
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

				material->unbind();
				RenderCommand::disableBackfaceCulling();

				render_data.stats.drawCallCount++;
				render_data.stats.triangleCount += submesh.indexCount / 3;
			}
		}

		mesh->unbind();
		RenderCommand::cleanState();

		TST_RC_CHECK_ERROR("After MeshRenderer cleanup");

		render_data.stats.meshCount++;
	}

	void MeshRenderer::drawMesh(const RefPtr<Mesh>& mesh, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawMesh(mesh, transform);
	}

	void MeshRenderer::submitMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform, uint32_t entityId)
	{
		render_data.renderQueue.submitMesh(mesh, transform, entityId, render_data.cameraPosition);
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
