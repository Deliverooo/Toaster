#include "tstpch.h"
#include "Renderer2D.hpp"

#include "RenderCommand.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"
#include "Toaster/Util/MathUtil.hpp"


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

		Renderer2D::Stats stats{};

		glm::mat4 viewProjectionMatrix;
		glm::vec3 cameraPosition;

		// Add safety flags
		bool isInitialized = false;
		bool isTerminated = false;

		// Add destructor to handle cleanup during static destruction
		~RenderData()
		{
			TST_CORE_TRACE("Renderer2D RenderData destructor - Initialized: {0}, Terminated: {1}", isInitialized, isTerminated);

			// If terminate() wasn't called explicitly, clean up safely
			if (isInitialized && !isTerminated)
			{
				cleanupSafe();
			}
		}

	private:
		void cleanupSafe()
		{
			if (isTerminated)
			{
				return;
			}

			isTerminated = true;

			// Clean up CPU memory first
			if (quadVertexBufferBase)
			{
				delete[] quadVertexBufferBase;
				quadVertexBufferBase = nullptr;
				quadVertexPtr = nullptr;
			}

			// CRITICAL: Clear RefPtr objects in a safe way
			// Clear texture slots first to break circular references
			try {
				for (auto& slot : textureSlots) {
					if (slot) {
						slot.reset();
					}
				}
				textureSlots.fill(nullptr);
			}
			catch (...) {
				// Ignore exceptions during cleanup
			}

			// Clear other RefPtr objects safely
			try {
				if (quadVertexArray) quadVertexArray.reset();
				if (quadVertexBuffer) quadVertexBuffer.reset();
				if (quadIndexBuffer) quadIndexBuffer.reset();
				if (whiteTexture) whiteTexture.reset();
				if (flatTextureShader) flatTextureShader.reset();
			}
			catch (...) {
				// Ignore exceptions during cleanup
			}

			// Reset counters
			quadIndex = 0;
			maxQuads = 0;
			maxVertices = 0;
			maxIndices = 0;
			textureSlotIndex = 0;

			// Clear stats
			stats = {};
			isInitialized = false;
		}

		friend void Renderer2D::terminate();
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
		render_data.whiteTexture->setData(&texData);


		int textureSamplers[render_data.maxTextureSlots];
		for (int i = 0; i < render_data.maxTextureSlots; i++)
		{
			textureSamplers[i] = i;
		}

		render_data.flatTextureShader = Shader::create("FlatTextureShader", TST_CORE_RESOURCE_DIR"/shaders/FlatTextureShader.glsl");
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

		render_data.isInitialized = true;
		render_data.isTerminated = false;
	}

	void Renderer2D::terminate()
	{
		TST_PROFILE_FN();

		if (!render_data.isInitialized || render_data.isTerminated)
		{
			return;
		}

		render_data.cleanupSafe();
	}


	void Renderer2D::begin(const RefPtr<OrthoCamera2D>& camera)
	{
		auto& projection = camera->getProjectionMatrix();
		auto view = camera->getViewMatrix();

		render_data.viewProjectionMatrix = projection * view;
		render_data.cameraPosition = camera->getPosition();

		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");

		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;

		render_data.textureSlotIndex = 1;
	}
	void Renderer2D::begin(const Camera& camera, const glm::mat4 &transform)
	{
		auto& projection = camera.getProjection();
		auto view = glm::inverse(transform);

		render_data.viewProjectionMatrix = projection * view;
		render_data.cameraPosition = glm::vec3(transform[3]);

		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");

		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;

		render_data.textureSlotIndex = 1;
	}
	void Renderer2D::begin(const EditorCamera& camera)
	{
		auto& projection = camera.getProjection();
		auto view = camera.getViewMatrix();

		render_data.viewProjectionMatrix = projection * view;
		render_data.cameraPosition = camera.getPosition();

		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");

		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;

		render_data.textureSlotIndex = 1;
	}

	void Renderer2D::begin(const RefPtr<PerspectiveCamera>& camera)
	{

		auto& projection = camera->getProjectionMatrix();
		auto view = camera->getViewMatrix();

		render_data.viewProjectionMatrix = projection * view;
		render_data.cameraPosition = camera->getPosition();

		render_data.flatTextureShader->bind();
		render_data.flatTextureShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");

		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;

		render_data.textureSlotIndex = 1;
	}


	void Renderer2D::end()
	{
		TST_PROFILE_FN();


		flush();
		render_data.quadIndex = 0;
		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
		render_data.textureSlotIndex = 1;

		render_data.flatTextureShader->unbind();
		render_data.quadVertexArray->unbind();
	}

	void Renderer2D::flush()
	{

		if (render_data.quadIndex <= 0)
		{

			return;
		}

		uint32_t dataSize = (uint8_t*)render_data.quadVertexPtr - (uint8_t*)render_data.quadVertexBufferBase;
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
		render_data.stats.verticesSubmitted += (render_data.quadVertexPtr - render_data.quadVertexBufferBase);
	}

	void Renderer2D::beginNewBatch()
	{
		TST_PROFILE_FN();

		uint32_t dataSize = (uint8_t*)render_data.quadVertexPtr - (uint8_t*)render_data.quadVertexBufferBase;

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

	uint32_t Renderer2D::enumerateTextureIndex(const RefPtr<Texture2D>& texture)
	{
		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
			beginNewBatch();
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

		return texIndex;
	}

	void Renderer2D::drawQuad(const glm::mat4& transform, const glm::vec4& colour)
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


		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transform * render_data.quadVertexPositions[i];
			render_data.quadVertexPtr->colour = colour;
			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
			render_data.quadVertexPtr->textureIndex = texIndex;
			render_data.quadVertexPtr->tilingFactor = 1.0f;
			render_data.quadVertexPtr++;
		}

		render_data.quadIndex += 6;

		render_data.stats.quadCount++;
	}

	void Renderer2D::drawQuad(const glm::mat4& transform, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
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
		

		float texIndexf = static_cast<float>(enumerateTextureIndex(texture));

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transform * render_data.quadVertexPositions[i];
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

	void Renderer2D::drawQuad(const glm::mat4& transform, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
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


		float texIndexf = static_cast<float>(enumerateTextureIndex(texture->getBaseTexture()));

		// for each vertex of the quad :)
		for (int i = 0; i < 4; i++)
		{
			render_data.quadVertexPtr->position = transform * render_data.quadVertexPositions[i];
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

	void Renderer2D::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawQuad(transform, colour);
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawQuad(transform, colour);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawQuad(transform, texture, tilingScale, tintColour);
	}
	void Renderer2D::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawQuad(transform, texture, tilingScale, tintColour);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawQuad(transform, texture, tilingScale, tintColour);
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		glm::mat4 transform = transformationMat(position, rotation, scale);
		drawQuad(transform, texture, tilingScale, tintColour);
	}


	Renderer2D::Stats Renderer2D::getStats()
	{
		return render_data.stats;
	}

	void Renderer2D::resetStats()
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

	void Renderer2D::drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
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
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
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

	void Renderer2D::drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour)
	{
		drawTri({ position.x, position.y, 0.0f }, rotation, scale, colour);
	}

	void Renderer2D::drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
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
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
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

	void Renderer2D::drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
	{
		drawTri({ position.x, position.y, 0.0f }, rotation, scale, texture, tilingScale, tintColour);
	}


}
