#include "tstpch.h"
#include "Renderer3D.hpp"

#include "RenderCommand.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"
#include "platform/OpenGl/OpenGLShader.hpp"


namespace tst
{
	struct RenderData
	{
		RefPtr<VertexArray> quadVertexArray;
		RefPtr<Shader> flatTextureShader;

		RefPtr<Texture2D> whiteTexture;

	};

	static ScopedPtr<RenderData> render_data;


	void Renderer3D::init()
	{
		TST_PROFILE_FN();
		render_data = std::make_unique<RenderData>();

		float quadVertices[] = {
			 0.5f,  0.5f, 0.0f,		0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,		1.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,		1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f,		0.0f, 1.0f,
		};

		uint32_t quadIndices[] = {
			0, 1, 2,
			2, 3, 0
		};




		render_data->quadVertexArray = VertexArray::create();
		RefPtr<VertexBuffer> vertexBuffer = VertexBuffer::create(quadVertices, sizeof(quadVertices));
		BufferLayout bufferLayout = {
			{"VertexPosition",	 ShaderDataType::Float3},
			{"TextureCoords",	 ShaderDataType::Float2},
		};
		vertexBuffer->setLayout(bufferLayout);
		render_data->quadVertexArray->addVertexBuffer(vertexBuffer);
		auto indexBuffer = IndexBuffer::create(quadIndices, sizeof(quadIndices) / 4);
		render_data->quadVertexArray->addIndexBuffer(indexBuffer);

		render_data->whiteTexture = Texture2D::create(1, 1);
		TST_ASSERT(render_data->whiteTexture != nullptr, "No");

		uint32_t texData = 0xffffffff;
		render_data->whiteTexture->setData(&texData, sizeof(uint32_t));


		render_data->flatTextureShader = Shader::create("FlatTextureShader", "assets/shaders/FlatTextureShader.glsl");

		render_data->flatTextureShader->bind();
		render_data->flatTextureShader->uploadInt1(0, "u_Tex0");
	}


	void Renderer3D::terminate()
	{
		TST_PROFILE_FN();

	}


	void Renderer3D::begin(const RefPtr<PerspectiveCamera>& camera)
	{
		TST_PROFILE_FN();

		render_data->flatTextureShader->bind();
		render_data->flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data->flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");
	}
	void Renderer3D::begin(const RefPtr<OrthoCamera>& camera)
	{
		TST_PROFILE_FN();

		render_data->flatTextureShader->bind();
		render_data->flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data->flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");
	}

	void Renderer3D::end()
	{
		TST_PROFILE_FN();

	}

	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		render_data->whiteTexture->bind(0);
		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f(colour, "u_Colour");

		render_data->quadVertexArray->bind();
		RenderCommand::drawIndexed(render_data->quadVertexArray);
	}

	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture)
	{
		TST_PROFILE_FN();

		texture->bind(0);

		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_Colour");

		render_data->quadVertexArray->bind();
		RenderCommand::drawIndexed(render_data->quadVertexArray);

		texture->unbind();
	}

}
