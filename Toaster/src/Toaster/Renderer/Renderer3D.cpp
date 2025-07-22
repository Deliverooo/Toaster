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
		RefPtr<VertexArray> cubeVertexArray;
		RefPtr<VertexArray> quadVertexArray;

		RefPtr<Shader> flatTextureShader;

		RefPtr<Texture2D> whiteTexture;

	};

	static RefPtr<RenderData> render_data;


	void Renderer3D::init()
	{
		TST_PROFILE_FN();
		render_data = std::make_shared<RenderData>();

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

		float cubeVertices[] = {

			/*Position		 Texture Coords*/
		 1.0f, -1.0f, -1.0f,   0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,   1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,   1.0f, 1.0f,
		 1.0f,  1.0f, -1.0f,   0.0f, 1.0f,
							 
		-1.0f, -1.0f,  1.0f,   0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,   1.0f, 0.0f,
		-1.0f,  1.0f, -1.0f,   1.0f, 1.0f,
		-1.0f,  1.0f,  1.0f,   0.0f, 1.0f,
							 
		-1.0f,  1.0f, -1.0f,   0.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,   1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,   1.0f, 1.0f,
		-1.0f,  1.0f,  1.0f,   0.0f, 1.0f,
							 
		-1.0f, -1.0f,  1.0f,   0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,   1.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,   1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,   0.0f, 1.0f,
							 
		 1.0f, -1.0f,  1.0f,   0.0f, 0.0f,
		-1.0f, -1.0f,  1.0f,   1.0f, 0.0f,
		-1.0f,  1.0f,  1.0f,   1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,   0.0f, 1.0f,
							 
		-1.0f, -1.0f, -1.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, -1.0f,   0.0f, 1.0f,
		};

		uint32_t cubeIndices[] = {
			0,  1,  2,
			0,  2,  3,
			4,  5,  6,
			4,  6,  7,
			8,  9, 10,
			8, 10, 11,
			12, 13, 14,
			12, 14, 15,
			16, 17, 18,
			16, 18, 19,
			20, 21, 22,
			20, 22, 23
		};

		// Quad Vao setup
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
		render_data->quadVertexArray->unbind();

		// Cube Vao setup
		render_data->cubeVertexArray = VertexArray::create();
		RefPtr<VertexBuffer> cubeVertexBuffer = VertexBuffer::create(cubeVertices, sizeof(cubeVertices));
		BufferLayout cubeBufferLayout = {
			{"VertexPosition",	 ShaderDataType::Float3},
			{"TextureCoords",	 ShaderDataType::Float2},
		};
		cubeVertexBuffer->setLayout(cubeBufferLayout);
		render_data->cubeVertexArray->addVertexBuffer(cubeVertexBuffer);
		auto cubeIndexBuffer = IndexBuffer::create(cubeIndices, sizeof(cubeIndices) / 4);
		render_data->cubeVertexArray->addIndexBuffer(cubeIndexBuffer);



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
		render_data->whiteTexture->bind(0);

		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f(colour, "u_Colour");
		render_data->flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_TintColour");
		render_data->flatTextureShader->uploadFloat1(1.0f, "u_TilingScale");

		render_data->quadVertexArray->bind();
		RenderCommand::drawIndexed(render_data->quadVertexArray);
	}

	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		texture->bind(0);

		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_Colour");
		render_data->flatTextureShader->uploadVector4f(tintColour, "u_TintColour");
		render_data->flatTextureShader->uploadFloat1(tilingScale, "u_TilingScale");

		render_data->quadVertexArray->bind();
		RenderCommand::drawIndexed(render_data->quadVertexArray);

		texture->unbind();
	}


	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
	{
		render_data->whiteTexture->bind(0);

		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f(colour, "u_Colour");
		render_data->flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_TintColour");
		render_data->flatTextureShader->uploadFloat1(1.0f, "u_TilingScale");

		render_data->cubeVertexArray->bind();
		RenderCommand::drawIndexed(render_data->cubeVertexArray);
	}

	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		texture->bind(0);

		render_data->flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data->flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_Colour");
		render_data->flatTextureShader->uploadVector4f(tintColour, "u_TintColour");
		render_data->flatTextureShader->uploadFloat1(tilingScale, "u_TilingScale");

		render_data->cubeVertexArray->bind();
		RenderCommand::drawIndexed(render_data->cubeVertexArray);

		texture->unbind();
	}
}


