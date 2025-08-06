#include "tstpch.h"
#include "SkyBoxRenderer.hpp"

#include "RenderCommand.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"


namespace tst
{

	struct SkyBoxRendererData
	{
		RefPtr<Shader> skyboxShader;
		RefPtr<Texture3D> skyboxTexture;

		glm::mat4 viewProjectionMatrix;

		RefPtr<VertexArray> skyboxVertexArray;
		RefPtr<VertexBuffer> skyboxVertexBuffer;
		RefPtr<IndexBuffer> skyboxIndexBuffer;


	};

	static SkyBoxRendererData render_data;

	void SkyBoxRenderer::init()
	{

		render_data.skyboxVertexArray = VertexArray::create();
		render_data.skyboxVertexBuffer = VertexBuffer::create(36 * 3 * sizeof(float));


		BufferLayout bufferLayout = {
			{ "VertexPosition", ShaderDataType::Float3 },
		};
		render_data.skyboxVertexBuffer->setLayout(bufferLayout);

		float skyboxVertices[] = {
			// Back face
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,

			// Front face
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			// Left face
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,

			// Right face
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,

			 // Bottom face
			 -1.0f, -1.0f, -1.0f,
			  1.0f, -1.0f, -1.0f,
			  1.0f, -1.0f,  1.0f,
			  1.0f, -1.0f,  1.0f,
			 -1.0f, -1.0f,  1.0f,
			 -1.0f, -1.0f, -1.0f,

			 // Top face
			 -1.0f,  1.0f, -1.0f,
			  1.0f,  1.0f, -1.0f,
			  1.0f,  1.0f,  1.0f,
			  1.0f,  1.0f,  1.0f,
			 -1.0f,  1.0f,  1.0f,
			 -1.0f,  1.0f, -1.0f
		};

		render_data.skyboxVertexBuffer->setData(skyboxVertices, 36 * 3 * sizeof(float));
		render_data.skyboxVertexArray->addVertexBuffer(render_data.skyboxVertexBuffer);

		render_data.skyboxTexture = Texture3D::create({
			TST_CORE_RESOURCE_DIR"/cubemaps/right.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/left.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/top.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/bottom.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/front.jpg",
			TST_CORE_RESOURCE_DIR"/cubemaps/back.jpg" });

		render_data.skyboxShader = Shader::create("SkyBoxShader", TST_CORE_RESOURCE_DIR"/shaders/SkyBoxShader.vert.glsl", TST_CORE_RESOURCE_DIR"/shaders/SkyBoxShader.frag.glsl");

		// Debug the shader
		render_data.skyboxShader->shaderDebugInfo();

		render_data.skyboxShader->bind();
		render_data.skyboxShader->uploadInt1(0, "u_Skybox");
	}

	void SkyBoxRenderer::terminate() noexcept
	{
		TST_CORE_INFO("Terminating SkyBoxRenderer");
	}


	void SkyBoxRenderer::render(const Camera& camera, const glm::mat4& view)
	{

		render_data.viewProjectionMatrix = camera.getProjection() * glm::mat4(glm::mat3(glm::inverse(view)));

		RenderCommand::setDepthFunc(DepthFunc::LessOrEqual);
		RenderCommand::checkError("SkyBox: After setDepthFunc");

		// Ensure clean slate

		render_data.skyboxShader->unbind();
		RenderCommand::checkError("SkyBox: After clearing shader state");

		// Validate our shader before binding
		if (!render_data.skyboxShader) {
			TST_CORE_ERROR("SkyBox: skyboxShader is null!");
			return;
		}

		TST_CORE_INFO("SkyBox: Binding shader ID: {}", render_data.skyboxShader->getId());
		render_data.skyboxShader->bind();
		RenderCommand::checkError("SkyBox: After shader bind");

		// Check if the uniform exists before uploading
		if (!render_data.skyboxShader->hasUniform("u_ViewProjection")) {
			TST_CORE_ERROR("SkyBox: u_ViewProjection uniform not found in shader!");
			return;
		}

		TST_CORE_INFO("SkyBox: Uploading u_ViewProjection uniform");
		render_data.skyboxShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
		RenderCommand::checkError("SkyBox: After uploading u_ViewProjection");

		render_data.skyboxVertexArray->bind();
		RenderCommand::checkError("SkyBox: After VAO bind");

		render_data.skyboxTexture->bind(0);
		RenderCommand::checkError("SkyBox: After texture bind");

		RenderCommand::drawArrays(render_data.skyboxVertexArray, 36);
		RenderCommand::checkError("SkyBox: After draw call");

		render_data.skyboxShader->unbind();
		RenderCommand::checkError("SkyBox: After shader unbind");

		RenderCommand::setDepthFunc(DepthFunc::Less);
		RenderCommand::checkError("SkyBox: After restoring depth func");
	}
	void SkyBoxRenderer::render(const EditorCamera &camera)
	{

		render_data.viewProjectionMatrix = camera.getProjectionMatrix() * glm::mat4(glm::mat3(camera.getViewMatrix()));

		RenderCommand::setDepthFunc(DepthFunc::LessOrEqual);
		RenderCommand::checkError("SkyBox: After setDepthFunc");

		// Ensure clean slate
		render_data.skyboxShader->unbind();
		RenderCommand::checkError("SkyBox: After clearing shader state");

		// Validate our shader before binding
		if (!render_data.skyboxShader) {
			TST_CORE_ERROR("SkyBox: skyboxShader is null!");
			return;
		}

		TST_CORE_INFO("SkyBox: Binding shader ID: {}", render_data.skyboxShader->getId());
		render_data.skyboxShader->bind();
		RenderCommand::checkError("SkyBox: After shader bind");

		// Check if the uniform exists before uploading
		if (!render_data.skyboxShader->hasUniform("u_ViewProjection")) {
			TST_CORE_ERROR("SkyBox: u_ViewProjection uniform not found in shader!");
			return;
		}

		TST_CORE_INFO("SkyBox: Uploading u_ViewProjection uniform");
		render_data.skyboxShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
		RenderCommand::checkError("SkyBox: After uploading u_ViewProjection");

		render_data.skyboxVertexArray->bind();
		RenderCommand::checkError("SkyBox: After VAO bind");

		render_data.skyboxTexture->bind(0);
		RenderCommand::checkError("SkyBox: After texture bind");

		RenderCommand::drawArrays(render_data.skyboxVertexArray, 36);
		RenderCommand::checkError("SkyBox: After draw call");

		render_data.skyboxShader->unbind();
		RenderCommand::checkError("SkyBox: After shader unbind");

		RenderCommand::setDepthFunc(DepthFunc::Less);
		RenderCommand::checkError("SkyBox: After restoring depth func");
	}

	void SkyBoxRenderer::setSkyboxTexture(const RefPtr<Texture3D>& texture)
	{
		if (texture)
		{
			render_data.skyboxTexture = texture;
		}
		else
		{
			TST_CORE_ERROR("Attempted to set a NULL skybox texture.");
		}
	}


}
