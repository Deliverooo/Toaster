#include "tstpch.h"
#include "Renderer2D.hpp"

#include "RenderCommand.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"


namespace tst
{
	struct QuadVertex
	{
		glm::vec3 position;
		glm::vec4 colour;
		glm::vec2 textureCoords;
	};

	struct RenderData
	{
		uint32_t maxQuads = 10000;
		uint32_t maxVertices = maxQuads * 4;
		uint32_t maxIndices = maxQuads * 6;

		RefPtr<VertexArray> quadVertexArray;
		RefPtr<VertexBuffer> quadVertexBuffer;
		RefPtr<IndexBuffer> quadIndexBuffer;
		RefPtr<Shader> flatTextureShader;
		RefPtr<Texture2D> whiteTexture;

		uint32_t quadIndex = 0;

		QuadVertex* quadVertexBufferBase = nullptr;
		QuadVertex* quadVertexPtr = nullptr;
	};

	static RenderData render_data;


	void Renderer2D::init()
	{

		TST_PROFILE_FN();	


		render_data.quadVertexArray = VertexArray::create();

		render_data.quadVertexBuffer = VertexBuffer::create(render_data.maxVertices * sizeof(QuadVertex));

		render_data.quadVertexBuffer->setLayout({
			{"VertexPosition",	 ShaderDataType::Float3},
			{"VertexColour",	 ShaderDataType::Float4},
			{"TextureCoords",	 ShaderDataType::Float2},
			});
		render_data.quadVertexArray->addVertexBuffer(render_data.quadVertexBuffer);

		render_data.quadVertexBufferBase = new QuadVertex[render_data.maxVertices];

		uint32_t* quadIndices = new uint32_t[render_data.maxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < render_data.maxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		render_data.quadIndexBuffer = IndexBuffer::create(quadIndices, render_data.maxIndices);
		render_data.quadVertexArray->addIndexBuffer(render_data.quadIndexBuffer);

		TST_ASSERT(quadIndices != nullptr, "NOOOO");

		delete[] quadIndices;

		render_data.whiteTexture = Texture2D::create(1, 1);
		TST_ASSERT(render_data.whiteTexture != nullptr, "No");

		uint32_t texData = 0xffffffff;
		render_data.whiteTexture->setData(&texData, sizeof(uint32_t));


		render_data.flatTextureShader = Shader::create("FlatTextureShader", "assets/shaders/FlatTextureShader.glsl");

		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadInt1(0, "u_Tex0");
	}

	void Renderer2D::terminate()
	{
		TST_PROFILE_FN();

		// Release GPU resources
		render_data.quadVertexArray.reset();
		render_data.quadVertexBuffer.reset();
		render_data.flatTextureShader.reset();
		render_data.whiteTexture.reset();
		render_data.quadIndexBuffer.reset();

		render_data.quadVertexArray = nullptr;
		render_data.quadVertexBuffer = nullptr;
		render_data.flatTextureShader = nullptr;
		render_data.whiteTexture = nullptr;
		render_data.quadIndexBuffer = nullptr;


		TST_ASSERT(render_data.quadVertexArray == nullptr, "Nooo");
		TST_ASSERT(render_data.quadVertexBuffer == nullptr, "Nooo");
		TST_ASSERT(render_data.flatTextureShader == nullptr, "Nooo");
		TST_ASSERT(render_data.whiteTexture == nullptr, "Nooo");
		TST_ASSERT(render_data.quadIndexBuffer == nullptr, "Nooo");

		delete[] render_data.quadVertexBufferBase;
		render_data.quadVertexBufferBase = nullptr;
		render_data.quadVertexPtr = nullptr;
		render_data.quadIndex = 0;

		render_data.maxQuads = 0;
		render_data.maxVertices = 0;
		render_data.maxIndices = 0;
	}


	void Renderer2D::begin(const RefPtr<OrthoCamera2D>& camera)
	{
		TST_PROFILE_FN();

		render_data.flatTextureShader->bind();	
		render_data.flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data.flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");


		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
	}
	void Renderer2D::begin(const ScopedPtr<OrthoCamera2D>& camera)
	{
		TST_PROFILE_FN();

		render_data.flatTextureShader->bind();	
		render_data.flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data.flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");


		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
	}

	void Renderer2D::end()
	{
		TST_PROFILE_FN();

		uint32_t dataSize = (uint8_t*)render_data.quadVertexPtr - (uint8_t*)render_data.quadVertexBufferBase;
		render_data.quadVertexBuffer->setData(render_data.quadVertexBufferBase, dataSize);
	
		RenderCommand::drawIndexed(render_data.quadVertexArray, render_data.quadIndex);

	}

	void Renderer2D::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}


		render_data.quadVertexPtr->position = { position.x, position.y, 0.0f };
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 0.0f, 0.0f };
		render_data.quadVertexPtr++;

		render_data.quadVertexPtr->position = { position.x + scale.x, position.y, 0.0f };
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 1.0f, 0.0f };
		render_data.quadVertexPtr++;

		render_data.quadVertexPtr->position = { position.x + scale.x, position.y + scale.y, 0.0f };
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 1.0f, 1.0f };
		render_data.quadVertexPtr++;

		render_data.quadVertexPtr->position = { position.x, position.y + scale.y, 0.0f };
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 0.0f, 1.0f };
		render_data.quadVertexPtr++;

		render_data.quadIndex += 6;

		//render_data.whiteTexture->bind(0);
		//render_data.flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		//render_data.flatTextureShader->uploadVector4f(colour, "u_Colour");
		//render_data.flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_TintColour");
		//render_data.flatTextureShader->uploadFloat1(1.0f, "u_TilingScale");

		//render_data.vertexArray->bind();
		//RenderCommand::drawIndexed(render_data.vertexArray);
	}
	void Renderer2D::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}



		render_data.quadVertexPtr->position = position;
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 0.0f, 0.0f };
		render_data.quadVertexPtr++;

		render_data.quadVertexPtr->position = { position.x + scale.x, position.y, 0.0f };
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 1.0f, 0.0f };
		render_data.quadVertexPtr++;

		render_data.quadVertexPtr->position = { position.x + scale.x, position.y + scale.y, 0.0f };
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 1.0f, 1.0f };
		render_data.quadVertexPtr++;

		render_data.quadVertexPtr->position = { position.x, position.y + scale.y, 0.0f };
		render_data.quadVertexPtr->colour = colour;
		render_data.quadVertexPtr->textureCoords = { 0.0f, 1.0f };
		render_data.quadVertexPtr++;

		render_data.quadIndex += 6;

		//render_data.whiteTexture->bind(0);

		//render_data.flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		//render_data.flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_TintColour");
		//render_data.flatTextureShader->uploadFloat1(1.0f, "u_TilingScale");

		//render_data.vertexArray->bind();
		//RenderCommand::drawIndexed(render_data.vertexArray);
	}
	void Renderer2D::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if ((render_data.quadVertexPtr - render_data.quadVertexBufferBase) + 4 > render_data.maxVertices)
		{
			TST_ASSERT(false, "Nooo");
		}

		texture->bind(0);

		render_data.flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data.flatTextureShader->uploadVector4f({ 1.0f, 1.0f, 1.0f, 1.0f }, "u_Colour");
		render_data.flatTextureShader->uploadVector4f(tintColour, "u_TintColour");
		render_data.flatTextureShader->uploadFloat1(tilingScale, "u_TilingScale");

		render_data.quadVertexArray->bind();
		RenderCommand::drawIndexed(render_data.quadVertexArray, 0);

		texture->unbind();

	}
	void Renderer2D::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if ((render_data.quadVertexPtr - render_data.quadVertexBufferBase) + 4 > render_data.maxVertices)
		{
			TST_ASSERT(false, "Nooo");
		}

		texture->bind(0);
		render_data.flatTextureShader->uploadMatrix4f(constructMatrix(position, rotation, scale), "u_Transform");
		render_data.flatTextureShader->uploadVector4f({1.0f, 1.0f, 1.0f, 1.0f}, "u_Colour");
		render_data.flatTextureShader->uploadVector4f(tintColour, "u_TintColour");
		render_data.flatTextureShader->uploadFloat1(tilingScale, "u_TilingScale");

		render_data.quadVertexArray->bind();
		RenderCommand::drawIndexed(render_data.quadVertexArray, 0);

		texture->unbind();
	}

	void Renderer2D::drawTri(const glm::vec2& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour)
	{
		
	}
	void Renderer2D::drawTri(const glm::vec3& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour)
	{
		
	}

}
