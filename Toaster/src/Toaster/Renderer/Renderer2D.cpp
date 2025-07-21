#include "tstpch.h"
#include "Renderer2D.hpp"

#include "RenderCommand.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"


namespace tst
{
	struct RenderData
	{
		RefPtr<VertexArray> vertexArray;
		RefPtr<Shader> flatTextureShader;

		RefPtr<Texture2D> whiteTexture;
	};

	static ScopedPtr<RenderData> render_data;

	void Renderer2D::init()
	{
		TST_PROFILE_FN();

		render_data = std::make_unique<RenderData>();

		float vertices[] = {
			 0.5f,  0.5f, 0.0f,		0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,		1.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,		1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f,		0.0f, 1.0f,
		};

		uint32_t indices[] = {
			0, 1, 2,
			2, 3, 0
		};

		render_data->vertexArray = VertexArray::create();
		RefPtr<VertexBuffer> vertexBuffer = VertexBuffer::create(vertices, sizeof(vertices));
		BufferLayout bufferLayout = {
			{"VertexPosition",	 ShaderDataType::Float3},
			{"TextureCoords",	 ShaderDataType::Float2},
		};
		vertexBuffer->setLayout(bufferLayout);
		render_data->vertexArray->addVertexBuffer(vertexBuffer);
		auto indexBuffer = IndexBuffer::create(indices, sizeof(indices) / 4);
		render_data->vertexArray->addIndexBuffer(indexBuffer);

		render_data->whiteTexture = Texture2D::create(1, 1);
		TST_ASSERT(render_data->whiteTexture != nullptr, "No");

		uint32_t texData = 0xffffffff;
		render_data->whiteTexture->setData(&texData, sizeof(uint32_t));


		render_data->flatTextureShader = Shader::create("FlatTextureShader", "assets/shaders/FlatTextureShader.glsl");

		render_data->flatTextureShader->bind();
		render_data->flatTextureShader->uploadInt1(0, "u_Tex0");
	}

	void Renderer2D::terminate()
	{
		TST_PROFILE_FN();

		render_data.reset();
	}


	void Renderer2D::begin(const RefPtr<OrthoCamera2D>& camera)
	{
		TST_PROFILE_FN();

		render_data->flatTextureShader->bind();	
		render_data->flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data->flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");
	}

	void Renderer2D::end()
	{
		TST_PROFILE_FN();

	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		render_data->whiteTexture->bind(0);
		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f(colour, "u_Colour");

		render_data->vertexArray->bind();
		RenderCommand::drawIndexed(render_data->vertexArray);
	}
	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		render_data->whiteTexture->bind(0);

		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f(colour, "u_Colour");

		render_data->vertexArray->bind();
		RenderCommand::drawIndexed(render_data->vertexArray);
	}
	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& scale, const float rotation, const RefPtr<Texture2D>& texture)
	{
		TST_PROFILE_FN();

		texture->bind(0);

		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_Colour");

		render_data->vertexArray->bind();
		RenderCommand::drawIndexed(render_data->vertexArray);

		texture->unbind();

	}
	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& scale, const float rotation, const RefPtr<Texture2D>& texture)
	{
		TST_PROFILE_FN();

		texture->bind(0);
		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f({1.0f, 1.0f, 1.0f, 1.0f}, "u_Colour");

		render_data->vertexArray->bind();
		RenderCommand::drawIndexed(render_data->vertexArray);

		texture->unbind();
	}

	void Renderer2D::drawTri(const glm::vec2& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour)
	{
		
	}
	void Renderer2D::drawTri(const glm::vec3& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour)
	{
		
	}

}
