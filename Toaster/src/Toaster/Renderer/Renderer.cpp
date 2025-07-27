#include "tstpch.h"

#include "Renderer.hpp"

#include "RenderCommand.hpp"
#include "platform/OpenGl/OpenGLShader.hpp"
#include "Toaster/Util/MathUtil.hpp"

namespace tst
{

	Renderer::SceneData* Renderer::m_sceneData = new Renderer::SceneData;

	struct QuadVertex
	{
		glm::vec3 position;
		glm::vec3 normal;        // Add normal for lighting calculations
		glm::vec4 colour;
		glm::vec2 textureCoords;
		float textureIndex;
		float tilingFactor;
		float materialId;        // For different material properties
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
		RefPtr<Shader> lightingShader;      // New lighting-enabled shader
		RefPtr<Texture2D> whiteTexture;

		uint32_t quadIndex = 0;
		QuadVertex* quadVertexBufferBase = nullptr;
		QuadVertex* quadVertexPtr = nullptr;

		std::array<RefPtr<Texture2D>, maxTextureSlots> textureSlots;
		uint32_t textureSlotIndex = 1;

		glm::vec4 quadVertexPositions[4];
		glm::vec2 quadVertexUvs[4];
		glm::vec3 quadVertexNormals[4];


		Renderer::Stats stats{};
	};


	static RenderData render_data;

	void Renderer::init()
	{
		RenderCommand::init();

		render_data.quadVertexArray = VertexArray::create();
		render_data.quadVertexBuffer = VertexBuffer::create(render_data.maxVertices * sizeof(QuadVertex));


		render_data.quadVertexBuffer->setLayout({
				{"VertexPosition",   ShaderDataType::Float3},
				//{"VertexNormal",     ShaderDataType::Float3},
				{"VertexColour",     ShaderDataType::Float4},
				{"TextureCoords",    ShaderDataType::Float2},
				{"TextureIndex",     ShaderDataType::Float},
				{"TilingFactor",     ShaderDataType::Float},

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

		render_data.lightingShader = Shader::create("LightingShader", TST_REL_PATH"shaders/FlatTextureShader.glsl");
		render_data.lightingShader->bind();
		render_data.lightingShader->uploadIntArray(textureSamplers, render_data.maxTextureSlots, "u_Textures");

		render_data.textureSlots[0] = render_data.whiteTexture;

		render_data.quadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		render_data.quadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		render_data.quadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		render_data.quadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		//render_data.quadVertexNormals[0] = glm::vec3(0.0f, 0.0f, 1.0f);
		//render_data.quadVertexNormals[1] = glm::vec3(0.0f, 0.0f, 1.0f);
		//render_data.quadVertexNormals[2] = glm::vec3(0.0f, 0.0f, 1.0f);
		//render_data.quadVertexNormals[3] = glm::vec3(0.0f, 0.0f, 1.0f);

		render_data.quadVertexUvs[0] = { 0.0f, 0.0f };
		render_data.quadVertexUvs[1] = { 1.0f, 0.0f };
		render_data.quadVertexUvs[2] = { 1.0f, 1.0f };
		render_data.quadVertexUvs[3] = { 0.0f, 1.0f };
	}

	void Renderer::terminate()
	{
		// Release GPU resources
		render_data.quadVertexArray = nullptr;
		render_data.quadVertexBuffer = nullptr;
		render_data.lightingShader = nullptr;
		render_data.whiteTexture = nullptr;
		render_data.quadIndexBuffer = nullptr;

		render_data.textureSlots.fill(nullptr);

		TST_ASSERT(render_data.quadVertexArray == nullptr, "Nooo");
		TST_ASSERT(render_data.quadVertexBuffer == nullptr, "Nooo");
		TST_ASSERT(render_data.lightingShader == nullptr, "Nooo");
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

#define GLM_VEC_3_PARAM(expr) expr.x, expr.y, expr.z


	// TODO remove this
	void Renderer::begin(const RefPtr<Camera>& camera)
	{
		m_sceneData->projectionMatrix = camera->getProjectionMatrix();
		m_sceneData->viewMatrix		  = camera->getViewMatrix();
	}


	void Renderer::begin(const RefPtr<PerspectiveCamera>& camera)
	{
		render_data.lightingShader->bind();
		render_data.lightingShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data.lightingShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");


		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
		render_data.textureSlotIndex = 1;
	}
	void Renderer::begin(const RefPtr<OrthoCamera>& camera)
	{
		render_data.lightingShader->bind();
		render_data.lightingShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data.lightingShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");


		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
		render_data.quadIndex = 0;
		render_data.textureSlotIndex = 1;
	}
	void Renderer::begin(const RefPtr<OrthoCamera2D>& camera)
	{
		render_data.lightingShader->bind();
		render_data.lightingShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
		render_data.lightingShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");

		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
		render_data.quadIndex = 0;
		render_data.textureSlotIndex = 1;
	}


	void Renderer::end()
	{
		flush();
		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
		render_data.textureSlotIndex = 1;
	}


	void Renderer::flush()
	{
		if (render_data.quadIndex == 0)
		{
			return;
		}

		// Calculate actual data size
		uint32_t dataSize = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(render_data.quadVertexPtr) -
			reinterpret_cast<uint8_t*>(render_data.quadVertexBufferBase));

		// Validate data size
		uint32_t maxBufferSize = render_data.maxVertices * sizeof(QuadVertex);
		if (dataSize > maxBufferSize) {
			TST_CORE_ERROR("Buffer overflow! Data size: {0}, Max buffer size: {1}", dataSize, maxBufferSize);
			TST_CORE_ERROR("Vertices written: {0}, Max vertices: {1}",
				(render_data.quadVertexPtr - render_data.quadVertexBufferBase), render_data.maxVertices);
			return;
		}

		// Upload lighting data first
		render_data.lightingShader->bind();

		// Upload vertex data
		render_data.quadVertexBuffer->setData(render_data.quadVertexBufferBase, dataSize);

		// Bind textures
		for (uint32_t i = 0; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i])
			{
				render_data.textureSlots[i]->bind(i);
			}
		}

		RenderCommand::drawIndexed(render_data.quadVertexArray, render_data.quadIndex);

		render_data.stats.drawCallCount++;
		render_data.stats.verticesSubmitted += (render_data.quadVertexPtr - render_data.quadVertexBufferBase);
	}

	void Renderer::resizeViewport(uint32_t width, uint32_t height)
	{
		RenderCommand::resizeViewport(width, height);
	}

	void Renderer::beginNewBatch()
	{
		TST_PROFILE_FN();

		uint32_t dataSize = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(render_data.quadVertexPtr) - reinterpret_cast<uint8_t*>(render_data.quadVertexBufferBase));

		if (dataSize > 0)
		{
			render_data.quadVertexBuffer->setData(render_data.quadVertexBufferBase, dataSize);

			for (uint32_t i = 0; i < render_data.textureSlotIndex; i++)
			{
				if (render_data.textureSlots[i])
				{
					render_data.textureSlots[i]->bind(i);
				}
			}

			RenderCommand::drawIndexed(render_data.quadVertexArray, render_data.quadIndex);

			render_data.stats.drawCallCount++;
		}


		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
		render_data.textureSlotIndex = 1;

		render_data.stats.batchesPerFrame++;
	}


	void Renderer::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
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
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 0.0f });


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

	void Renderer::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		drawQuad(glm::vec3(position.x, position.y, 0.0f), rotation, scale, colour);
	}

	void Renderer::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
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
		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}


		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getId())
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
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 0.0f });

		float texIndexf = static_cast<float>(texIndex);

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * render_data.quadVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexf;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		drawQuad(glm::vec3(position.x, position.y, 0.0f), rotation, scale, texture, tilingScale, tintColour);
	}

	void Renderer::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
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
		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}


		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getBaseTexture()->getId())
			{
				texIndex = i;
				break;
			}
		}

		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture->getBaseTexture();
			render_data.textureSlotIndex++;
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 0.0f });

		float texIndexf = static_cast<float>(texIndex);

		const glm::vec2* uvs = texture->getTextureCoords();


		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * render_data.quadVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = uvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexf;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		drawQuad(glm::vec3(position.x, position.y, 0.0f), rotation, scale, texture, tilingScale, tintColour);
	}


	void Renderer::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
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

		glm::mat4 transformation = transformationMat(position, rotation, scale);

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

	void Renderer::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		drawQuad(position, rotation, {scale.x, scale.y, 1.0f}, colour);
	}

	void Renderer::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
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
		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}


		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getId())
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
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = transformationMat(position, rotation, scale);

		float texIndexf = static_cast<float>(texIndex);

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * render_data.quadVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexf;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		drawQuad(position, rotation, { scale.x, scale.y, 1.0f }, texture, tilingScale, tintColour);
	}

	void Renderer::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
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
		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}


		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getBaseTexture()->getId())
			{
				texIndex = i;
				break;
			}
		}

		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture->getBaseTexture();
			render_data.textureSlotIndex++;
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = transformationMat(position, rotation, scale);

		float texIndexf = static_cast<float>(texIndex);

		const glm::vec2* uvs = texture->getTextureCoords();


		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * render_data.quadVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = uvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexf;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		drawQuad(position, rotation, { scale.x, scale.y, 1.0f }, texture, tilingScale, tintColour);
	}


	Renderer::Stats Renderer::getStats()
	{
		return render_data.stats;
	}

	void Renderer::resetStats()
	{
		render_data.stats.drawCallCount = 0;
		render_data.stats.quadCount = 0;
		render_data.stats.triangleCount = 0;
		render_data.stats.circleCount = 0;
		render_data.stats.lineCount = 0;
		render_data.stats.batchesPerFrame = 1;
		render_data.stats.textureBindings = 0;
		render_data.stats.verticesSubmitted = 0;
	}

	void Renderer::drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex + 6 >= render_data.maxIndices)
		{
			beginNewBatch();
		}


		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 0.0f });

		constexpr float texIndex = 0.0f;


		constexpr glm::vec4 triangleVertexPositions[4] = {
			{ 0.0f,  0.5f, 0.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f}
		};

		constexpr glm::vec2 triangleVertexUvs[4] = {
			{0.5f, 1.0f},
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 0.0f},
		};

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * triangleVertexPositions[i];
			render_data.quadVertexPtr->colour = colour;
			render_data.quadVertexPtr->textureCoords = triangleVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndex;
			render_data.quadVertexPtr->tilingFactor = 1.0f;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		drawTri({ position.x, position.y, 0.0f }, rotation, scale, colour);
	}

	void Renderer::drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex + 6 >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}

		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getId())
			{
				texIndex = i;
				break;
			}
		}

		constexpr glm::vec4 triangleVertexPositions[4] = {
			{ 0.0f,  0.5f, 0.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f}
		};

		constexpr glm::vec2 triangleVertexUvs[4] = {
			{0.5f, 1.0f},
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 0.0f},
		};

		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture;
			render_data.textureSlotIndex++;
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 0.0f });


		float texIndexF = static_cast<float>(texIndex);

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * triangleVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = triangleVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexF;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		drawTri({ position.x, position.y, 0.0f }, rotation, scale, texture, tilingScale, tintColour);
	}

	void Renderer::drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex + 6 >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}

		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getBaseTexture()->getId())
			{
				texIndex = i;
				break;
			}
		}

		constexpr glm::vec4 triangleVertexPositions[4] = {
			{ 0.0f,  0.5f, 0.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f}
		};

		constexpr glm::vec2 triangleVertexUvs[4] = {
			{0.5f, 1.0f},
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 0.0f},
		};

		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture->getBaseTexture();
			render_data.textureSlotIndex++;
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 0.0f });


		float texIndexF = static_cast<float>(texIndex);

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * triangleVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = triangleVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexF;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		drawTri({ position.x, position.y, 0.0f }, rotation, scale, texture, tilingScale, tintColour);
	}


	void Renderer::drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex + 6 >= render_data.maxIndices)
		{
			beginNewBatch();
		}


		glm::mat4 transformation = transformationMat(position, rotation, scale);

		constexpr float texIndex = 0.0f;


		constexpr glm::vec4 triangleVertexPositions[4] = {
			{ 0.0f,  0.5f, 0.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f}
		};

		constexpr glm::vec2 triangleVertexUvs[4] = {
			{0.5f, 1.0f},
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 0.0f},
		};

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * triangleVertexPositions[i];
			render_data.quadVertexPtr->colour = colour;
			render_data.quadVertexPtr->textureCoords = triangleVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndex;
			render_data.quadVertexPtr->tilingFactor = 1.0f;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		drawTri(position, rotation, { scale.x, scale.y, 1.0f }, colour);
	}

	void Renderer::drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex + 6 >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}

		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getId())
			{
				texIndex = i;
				break;
			}
		}

		constexpr glm::vec4 triangleVertexPositions[4] = {
			{ 0.0f,  0.5f, 0.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f}
		};

		constexpr glm::vec2 triangleVertexUvs[4] = {
			{0.5f, 1.0f},
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 0.0f},
		};

		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture;
			render_data.textureSlotIndex++;
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = transformationMat(position, rotation, scale);

		float texIndexF = static_cast<float>(texIndex);

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * triangleVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = triangleVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexF;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		drawTri(position, rotation, { scale.x, scale.y, 1.0f }, texture, tilingScale, tintColour);
	}

	void Renderer::drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		TST_PROFILE_FN();

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
			return;
		}

		if (render_data.quadIndex + 6 >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			// Flush the batch and start a new one
			beginNewBatch();
		}

		uint32_t texIndex = 0;

		// searches for an existing texture in the current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getBaseTexture()->getId())
			{
				texIndex = i;
				break;
			}
		}

		constexpr glm::vec4 triangleVertexPositions[4] = {
			{ 0.0f,  0.5f, 0.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f}
		};

		constexpr glm::vec2 triangleVertexUvs[4] = {
			{0.5f, 1.0f},
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 0.0f},
		};

		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture->getBaseTexture();
			render_data.textureSlotIndex++;
			render_data.stats.textureBindings++;
		}

		glm::mat4 transformation = transformationMat(position, rotation, scale);

		float texIndexF = static_cast<float>(texIndex);

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transformation * triangleVertexPositions[i];
			render_data.quadVertexPtr->colour = tintColour;
			render_data.quadVertexPtr->textureCoords = triangleVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndexF;
			render_data.quadVertexPtr->tilingFactor = tilingScale;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;
		render_data.stats.quadCount++;
		render_data.stats.verticesSubmitted += 4;
	}

	void Renderer::drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		drawTri(position, rotation, { scale.x, scale.y, 1.0f }, texture, tilingScale, tintColour);
	}


	void Renderer::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
	{

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer3D::begin()?");
			return;
		}

		if (render_data.quadIndex + 36 >= render_data.maxIndices)
		{
			beginNewBatch();
		}


		int texIndex = 0;

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation)))
			* glm::scale(glm::mat4(1.0f), scale);

		float texIndexf = static_cast<float>(texIndex);

		// Define cube faces as quads with proper normals and UVs
		struct CubeFace {
			glm::vec3 vertices[4];
			glm::vec2 uvs[4];
		};

		constexpr float tilingScale = 1.0f;

		CubeFace faces[6] = {
			// Front face
			{{{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Back face
			{{{ 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Left face
			{{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Right face
			{{{ 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Top face
			{{{-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Bottom face
			{{{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}}
		};

		// Draw each face as a quad
		for (int face = 0; face < 6; face++)
		{
			for (int vertex = 0; vertex < 4; vertex++)
			{
				render_data.quadVertexPtr->position = transformation * glm::vec4(faces[face].vertices[vertex], 1.0f);
				render_data.quadVertexPtr->colour = colour;
				render_data.quadVertexPtr->textureCoords = faces[face].uvs[vertex];
				render_data.quadVertexPtr->textureIndex = texIndexf;
				render_data.quadVertexPtr->tilingFactor = 1.0f; // Already applied to UVs
				render_data.quadVertexPtr++;
			}
		}

		render_data.quadIndex += 36; // 6 faces * 6 indices per face
		render_data.stats.quadCount += 6;
	}

	void Renderer::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer3D::begin()?");
			return;
		}

		if (render_data.quadIndex + 36 >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		// Check if we need to start a new batch due to texture slots being full
		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			beginNewBatch();
		}

		int texIndex = 0;

		// Search for existing texture in current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getId())
			{
				texIndex = i;
				break;
			}
		}

		// If texture not found, add it to the batch
		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture;
			render_data.textureSlotIndex++;
		}

		glm::mat4 transformation = transformationMat(position, rotation, scale);

		float texIndexf = static_cast<float>(texIndex);

		// Define cube faces as quads with proper normals and UVs
		struct CubeFace {
			glm::vec3 vertices[4];
			glm::vec2 uvs[4];
		};

		CubeFace faces[6] = {
			// Front face
			{{{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Back face
			{{{ 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Left face
			{{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Right face
			{{{ 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Top face
			{{{-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Bottom face
			{{{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}}
		};

		// Draw each face as a quad
		for (int face = 0; face < 6; face++)
		{
			for (int vertex = 0; vertex < 4; vertex++)
			{
				render_data.quadVertexPtr->position = transformation * glm::vec4(faces[face].vertices[vertex], 1.0f);
				render_data.quadVertexPtr->colour = tintColour;
				render_data.quadVertexPtr->textureCoords = faces[face].uvs[vertex];
				render_data.quadVertexPtr->textureIndex = texIndexf;
				render_data.quadVertexPtr->tilingFactor = 1.0f; // Already applied to UVs
				render_data.quadVertexPtr++;
			}
		}

		render_data.quadIndex += 36; // 6 faces * 6 indices per face
		render_data.stats.quadCount += 6;
	}

	void Renderer::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{

		if (!render_data.quadVertexPtr) {
			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer3D::begin()?");
			return;
		}

		if (render_data.quadIndex + 36 >= render_data.maxIndices)
		{
			beginNewBatch();
		}

		// Check if we need to start a new batch due to texture slots being full
		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			beginNewBatch();
		}

		int texIndex = 0;

		// Search for existing texture in current batch
		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
		{
			if (render_data.textureSlots[i] && render_data.textureSlots[i]->getId() == texture->getBaseTexture()->getId())
			{
				texIndex = i;
				break;
			}
		}

		// If texture not found, add it to the batch
		if (texIndex == 0)
		{
			texIndex = render_data.textureSlotIndex;
			render_data.textureSlots[render_data.textureSlotIndex] = texture->getBaseTexture();
			render_data.textureSlotIndex++;
		}

		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
			* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation)))
			* glm::scale(glm::mat4(1.0f), scale);

		float texIndexf = static_cast<float>(texIndex);

		// Define cube faces as quads with proper normals and UVs
		struct CubeFace {
			glm::vec3 vertices[4];
			glm::vec2 uvs[4];
		};

		CubeFace faces[6] = {
			// Front face
			{{{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Back face
			{{{ 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Left face
			{{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Right face
			{{{ 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Top face
			{{{-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}},
			// Bottom face
			{{{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}},
			{{ 0.0f,  0.0f}, { tilingScale,  0.0f}, { tilingScale,  tilingScale}, { 0.0f,  tilingScale}}}
		};

		// Draw each face as a quad
		for (int face = 0; face < 6; face++)
		{
			for (int vertex = 0; vertex < 4; vertex++)
			{
				render_data.quadVertexPtr->position = transformation * glm::vec4(faces[face].vertices[vertex], 1.0f);
				render_data.quadVertexPtr->colour = tintColour;
				render_data.quadVertexPtr->textureCoords = faces[face].uvs[vertex];
				render_data.quadVertexPtr->textureIndex = texIndexf;
				render_data.quadVertexPtr->tilingFactor = 1.0f; // Already applied to UVs
				render_data.quadVertexPtr++;
			}
		}

		render_data.quadIndex += 36; // 6 faces * 6 indices per face
		render_data.stats.quadCount += 6;
	}

}
