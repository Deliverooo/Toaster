#include "tstpch.h"
#include "Renderer3D.hpp"

#include "RenderCommand.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"
#include "platform/OpenGl/OpenGLShader.hpp"


namespace tst
{
	struct QuadVertex
	{
		glm::vec3 position;
		glm::vec4 colour;
		glm::vec2 textureCoords;
		float textureIndex;
		float tilingFactor;
	};

	struct RenderData
	{
		uint32_t maxQuads = 10000;
		uint32_t maxVertices = maxQuads * 4;
		uint32_t maxIndices = maxQuads * 6;
		static const uint32_t maxTextureSlots = 32;

		RefPtr<VertexArray> quadVertexArray;
		RefPtr<VertexBuffer> quadVertexBuffer;
		RefPtr<IndexBuffer> quadIndexBuffer;
		RefPtr<Shader> flatTextureShader;
		RefPtr<Texture2D> whiteTexture;

		uint32_t quadIndex = 0;

		QuadVertex* quadVertexBufferBase = nullptr;
		QuadVertex* quadVertexPtr = nullptr;

		std::array<RefPtr<Texture2D>, maxTextureSlots> textureSlots;
		uint32_t textureSlotIndex = 1;

		glm::vec4 quadVertexPositions[4];
		glm::vec2 quadVertexUvs[4];


		glm::vec4 cubeVertexPositions[8];

		Renderer3D::Stats stats{};
	};

	static RenderData render_data;

	void Renderer3D::init()
	{
		TST_PROFILE_FN();


		render_data.quadVertexArray = VertexArray::create();

		render_data.quadVertexBuffer = VertexBuffer::create(render_data.maxVertices * sizeof(QuadVertex));

		render_data.quadVertexBuffer->setLayout({
			{"VertexPosition",	 ShaderDataType::Float3},
			{"VertexColour",	 ShaderDataType::Float4},
			{"TextureCoords",	 ShaderDataType::Float2},
			{"TextureIndex",	 ShaderDataType::Float},
			{"TilingFactor",	 ShaderDataType::Float},
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


		int textureSamplers[render_data.maxTextureSlots];
		for (int i = 0; i < render_data.maxTextureSlots; i++)
		{
			textureSamplers[i] = i;
		}

		render_data.flatTextureShader = Shader::create("FlatTextureShader", TST_REL_PATH"shaders/FlatTextureShader.glsl");
		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadIntArray(textureSamplers, render_data.maxTextureSlots, "u_Textures");

		render_data.textureSlots[0] = render_data.whiteTexture;


		render_data.quadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		render_data.quadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		render_data.quadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		render_data.quadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		render_data.quadVertexUvs[0] = { 0.0f, 0.0f };
		render_data.quadVertexUvs[1] = { 1.0f, 0.0f };
		render_data.quadVertexUvs[2] = { 1.0f, 1.0f };
		render_data.quadVertexUvs[3] = { 0.0f, 1.0f };


		render_data.cubeVertexPositions[0] = { -0.5f, -0.5f, -0.5f, 1.0f };
		render_data.cubeVertexPositions[1] = {  0.5f, -0.5f, -0.5f, 1.0f };
		render_data.cubeVertexPositions[2] = {  0.5f,  0.5f, -0.5f, 1.0f };
		render_data.cubeVertexPositions[3] = { -0.5f,  0.5f, -0.5f, 1.0f };
		render_data.cubeVertexPositions[4] = { -0.5f, -0.5f,  0.5f, 1.0f };
		render_data.cubeVertexPositions[5] = {  0.5f, -0.5f,  0.5f, 1.0f };
		render_data.cubeVertexPositions[6] = {  0.5f,  0.5f,  0.5f, 1.0f };
		render_data.cubeVertexPositions[7] = { -0.5f,  0.5f,  0.5f, 1.0f };

	}


	void Renderer3D::terminate()
	{
		TST_PROFILE_FN();

		// Release GPU resources
		render_data.quadVertexArray = nullptr;
		render_data.quadVertexBuffer = nullptr;
		render_data.flatTextureShader = nullptr;
		render_data.whiteTexture = nullptr;
		render_data.quadIndexBuffer = nullptr;

		render_data.textureSlots.fill(nullptr);

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
		render_data.textureSlotIndex = 0;
	}


	void Renderer3D::begin(const RefPtr<PerspectiveCamera>& camera)
	{
		TST_PROFILE_FN();

		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data.flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");

		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;

		render_data.textureSlotIndex = 1;
	}
	void Renderer3D::begin(const RefPtr<OrthoCamera>& camera)
	{
		TST_PROFILE_FN();

		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data.flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");

		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;

		render_data.textureSlotIndex = 1;
	}

	void Renderer3D::end()
	{
		TST_PROFILE_FN();

		uint32_t dataSize = (uint8_t*)render_data.quadVertexPtr - (uint8_t*)render_data.quadVertexBufferBase;
		render_data.quadVertexBuffer->setData(render_data.quadVertexBufferBase, dataSize);

		flush();
	}

	void Renderer3D::flush()
	{

		for (uint32_t i = 0; i < render_data.textureSlotIndex; i++)
		{
			render_data.textureSlots[i]->bind(i);

		}
		RenderCommand::drawIndexed(render_data.quadVertexArray, render_data.quadIndex);

		render_data.stats.drawCallCount++;
	}

	void Renderer3D::beginNewBatch()
	{
		end();

		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;

		render_data.textureSlotIndex = 1;

		render_data.stats.batchesPerFrame++;
	}

	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		// There is no texture being bound, so it will be white (slot 0)
		float texIndex = 0.0f;


		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation)))
			* glm::scale(glm::mat4(1.0f), scale);


		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * render_data.quadVertexPositions[i];
			render_data.quadVertexPtr->colour = colour;
			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndex;
			render_data.quadVertexPtr->tilingFactor = 1.0f;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;

		render_data.stats.quadCount++;
	}

	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		constexpr glm::vec4 whiteColour = { 1.0f, 1.0f, 1.0f, 1.0f };

		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			TST_ERROR("yes");
		}

		int texIndex = 0;

		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i]->getId() == texture->getId())
			{
				texIndex = i;
				break;
			}
		}

		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture;
			render_data.textureSlotIndex++;
		}

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation)))
			* glm::scale(glm::mat4(1.0f), scale);

		float texIndexf = static_cast<float>(texIndex);

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * render_data.quadVertexPositions[i];
			render_data.quadVertexPtr->colour = whiteColour;
			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexf;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;

		render_data.stats.quadCount++;
	}


	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
	{

		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer3D::begin()?");
			return;
		}

		if (render_data.quadIndex + 36 >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		float texIndex = 0.0f;

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation)))
			* glm::scale(glm::mat4(1.0f), scale);

		// Define cube faces as quads with proper normals and UVs
		struct CubeFace {
			glm::vec3 vertices[4];
			glm::vec2 uvs[4];
		};

		CubeFace faces[6] = {
			// Front face
			{{{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}},
				{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
			// Back face
			{{{ 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}},
				{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
			// Left face
			{{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}},
			    {{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
			// Right face
			{{{ 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}},
				{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
			// Top face
			{{{-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}},
				{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
			// Bottom face
			{{{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}},
				{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}}
		};


		uint32_t cubeIndices[36] = {
			// Back face
			0, 1, 2, 2, 3, 0,
			// Front face
			4, 5, 6, 6, 7, 4,
			// Left face
			0, 4, 7, 7, 3, 0,
			// Right face
			1, 5, 6, 6, 2, 1,
			// Bottom face
			0, 1, 5, 5, 4, 0,
			// Top face
			3, 2, 6, 6, 7, 3
		};

		// Draw each face as a quad
		for (int face = 0; face < 6; face++)
		{
			for (int vertex = 0; vertex < 4; vertex++)
			{
				render_data.quadVertexPtr->position = transformation * glm::vec4(faces[face].vertices[vertex], 1.0f);
				render_data.quadVertexPtr->colour = colour;
				render_data.quadVertexPtr->textureCoords = faces[face].uvs[vertex];
				render_data.quadVertexPtr->textureIndex = texIndex;
				render_data.quadVertexPtr->tilingFactor = 1.0f;
				render_data.quadVertexPtr++;
			}
		}

		render_data.quadIndex += 36; // Cube indices
		render_data.stats.quadCount += 6;
	}

	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{


		drawQuad(position, rotation, scale, texture, tilingScale, tintColour);
	}

	Renderer3D::Stats Renderer3D::getStats()
	{
		return render_data.stats;
	}

	void Renderer3D::resetStats()
	{
		render_data.stats.drawCallCount = 0;
		render_data.stats.quadCount = 0;
		render_data.stats.batchesPerFrame = 1;
	}
}


