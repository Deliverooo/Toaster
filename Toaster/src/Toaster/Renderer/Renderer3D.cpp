#include "tstpch.h"
#include "Renderer3D.hpp"
//
//#include "RenderCommand.hpp"
//#include "Shader.hpp"
//#include "VertexArray.hpp"
//#include "platform/OpenGl/OpenGLShader.hpp"
//#include "Toaster/Util/MathUtil.hpp"
//
//
//namespace tst
//{
//
//	struct QuadVertex
//	{
//		glm::vec3 position;
//		glm::vec4 colour;
//		glm::vec2 textureCoords;
//		float textureIndex;
//		float tilingFactor;
//	};
//
//	struct RenderData
//	{
//		uint32_t maxQuads = 10000;
//		uint32_t maxVertices = maxQuads * 4;
//		uint32_t maxIndices = maxQuads * 6;
//		static const uint32_t maxTextureSlots = 32;
//
//		RefPtr<VertexArray> quadVertexArray;
//		RefPtr<VertexBuffer> quadVertexBuffer;
//		RefPtr<IndexBuffer> quadIndexBuffer;
//		RefPtr<Shader> flatTextureShader;
//		RefPtr<Shader> meshShader;
//
//		RefPtr<Texture2D> whiteTexture;
//
//		uint32_t quadIndex = 0;
//
//		QuadVertex* quadVertexBufferBase = nullptr;
//		QuadVertex* quadVertexPtr = nullptr;
//
//		std::array<RefPtr<Texture2D>, maxTextureSlots> textureSlots;
//		uint32_t textureSlotIndex = 1;
//
//		glm::vec4 quadVertexPositions[4];
//		glm::vec2 quadVertexUvs[4];
//
//		glm::vec4 cubeVertexPositions[8];
//
//		glm::mat4 viewMatrix;
//		glm::mat4 projectionMatrix;
//		glm::mat4 viewProjectionMatrix;
//		glm::vec3 cameraPosition;
//
//		struct LightData
//		{
//			glm::vec3 position{ 5.0f, 5.0f, 5.0f };
//			glm::vec3 colour  { 1.0f, 1.0f, 1.0f };
//			float intensity	  { 1.0f };
//		} mainLight;
//
//		Renderer3D::Stats stats{};
//
//
//		// Add state tracking
//		bool isInitialized = false;
//		bool isTerminated = false;
//
//		// Add destructor for safety during static destruction
//		~RenderData()
//		{
//			if (isInitialized && !isTerminated)
//			{
//				// Call cleanup safely during static destruction
//				try {
//					cleanupSafe();
//				}
//				catch (...) {
//					// Never throw from destructor
//				}
//			}
//		}
//
//	private:
//		void cleanupSafe() noexcept
//		{
//			if (isTerminated) return;
//
//			isTerminated = true;
//
//			// **CRITICAL: Clean up CPU memory first**
//			if (quadVertexBufferBase)
//			{
//				delete[] quadVertexBufferBase;
//				quadVertexBufferBase = nullptr;
//				quadVertexPtr = nullptr;
//			}
//
//			// **CRITICAL: Exception-safe smart pointer cleanup**
//			try {
//				// Clear texture slots first to break circular references
//				textureSlots.fill(nullptr);
//
//				// Reset shader pointers in specific order (meshShader first, then flatTextureShader)
//				if (meshShader) meshShader.reset();
//				if (flatTextureShader) flatTextureShader.reset();
//
//				// Reset other GPU resources
//				if (whiteTexture) whiteTexture.reset();
//				if (quadVertexArray) quadVertexArray.reset();
//				if (quadVertexBuffer) quadVertexBuffer.reset();
//				if (quadIndexBuffer) quadIndexBuffer.reset();
//			}
//			catch (...) {
//				// Ignore exceptions during cleanup
//			}
//
//			// Reset state
//			quadIndex = 0;
//			textureSlotIndex = 0;
//			maxQuads = 0;
//			maxVertices = 0;
//			maxIndices = 0;
//			stats = {};
//			isInitialized = false;
//		}
//
//		friend void Renderer3D::terminate() noexcept;
//	};
//
//	static RenderData render_data;
//
//	void Renderer3D::init()
//	{
//		TST_PROFILE_FN();
//
//		if (render_data.isInitialized)
//		{
//			TST_CORE_WARN("Renderer3D already initialized");
//			return;
//		}
//
//		RenderCommand::init();
//
//		render_data.quadVertexArray = VertexArray::create();
//
//		render_data.quadVertexBuffer = VertexBuffer::create(render_data.maxVertices * sizeof(QuadVertex));
//
//		render_data.quadVertexBuffer->setLayout({
//			{"VertexPosition",	 ShaderDataType::Float3},
//			{"VertexColour",	 ShaderDataType::Float4},
//			{"TextureCoords",	 ShaderDataType::Float2},
//			{"TextureIndex",	 ShaderDataType::Float},
//			{"TilingFactor",	 ShaderDataType::Float},
//			});
//		render_data.quadVertexArray->addVertexBuffer(render_data.quadVertexBuffer);
//
//		render_data.quadVertexBufferBase = new QuadVertex[render_data.maxVertices];
//
//		uint32_t* quadIndices = new uint32_t[render_data.maxIndices];
//
//		uint32_t offset = 0;
//		for (uint32_t i = 0; i < render_data.maxIndices; i += 6)
//		{
//			quadIndices[i + 0] = offset + 0;
//			quadIndices[i + 1] = offset + 1;
//			quadIndices[i + 2] = offset + 2;
//
//			quadIndices[i + 3] = offset + 2;
//			quadIndices[i + 4] = offset + 3;
//			quadIndices[i + 5] = offset + 0;
//
//			offset += 4;
//		}
//
//		render_data.quadIndexBuffer = IndexBuffer::create(quadIndices, render_data.maxIndices);
//		render_data.quadVertexArray->addIndexBuffer(render_data.quadIndexBuffer);
//
//		TST_ASSERT(quadIndices != nullptr, "NOOOO");
//
//		delete[] quadIndices;
//
//		render_data.whiteTexture = Texture2D::create(1, 1);
//		TST_ASSERT(render_data.whiteTexture != nullptr, "No");
//
//		uint32_t texData = 0xffffffff;
//		render_data.whiteTexture->setData(&texData);
//
//
//		int textureSamplers[render_data.maxTextureSlots];
//		for (int i = 0; i < render_data.maxTextureSlots; i++)
//		{
//			textureSamplers[i] = i;
//		}
//
//
//
//		render_data.flatTextureShader = Shader::create("FlatTextureShader", TST_REL_PATH"shaders/FlatTextureShader.glsl");
//		render_data.flatTextureShader->bind();
//		render_data.flatTextureShader->uploadIntArray(textureSamplers, render_data.maxTextureSlots, "u_Textures");
//
//
//		render_data.meshShader = Shader::create("MeshShader", TST_CORE_RESOURCE_DIR"/shaders/MeshShader.glsl");
//
//		render_data.textureSlots[0] = render_data.whiteTexture;
//
//		render_data.quadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
//		render_data.quadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
//		render_data.quadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
//		render_data.quadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
//
//		render_data.quadVertexUvs[0] = { 0.0f, 0.0f };
//		render_data.quadVertexUvs[1] = { 1.0f, 0.0f };
//		render_data.quadVertexUvs[2] = { 1.0f, 1.0f };
//		render_data.quadVertexUvs[3] = { 0.0f, 1.0f };
//
//
//		render_data.cubeVertexPositions[0] = { -0.5f, -0.5f, -0.5f, 1.0f };
//		render_data.cubeVertexPositions[1] = { 0.5f, -0.5f, -0.5f, 1.0f };
//		render_data.cubeVertexPositions[2] = { 0.5f,  0.5f, -0.5f, 1.0f };
//		render_data.cubeVertexPositions[3] = { -0.5f,  0.5f, -0.5f, 1.0f };
//		render_data.cubeVertexPositions[4] = { -0.5f, -0.5f,  0.5f, 1.0f };
//		render_data.cubeVertexPositions[5] = { 0.5f, -0.5f,  0.5f, 1.0f };
//		render_data.cubeVertexPositions[6] = { 0.5f,  0.5f,  0.5f, 1.0f };
//		render_data.cubeVertexPositions[7] = { -0.5f,  0.5f,  0.5f, 1.0f };
//
//		render_data.isInitialized = true;
//		render_data.isTerminated = false;
//
//	}
//
//
//	void Renderer3D::terminate() noexcept
//	{
//		if (!render_data.isInitialized || render_data.isTerminated)
//		{
//			return;
//		}
//
//		render_data.cleanupSafe();
//	}
//
//
//	void Renderer3D::begin(const RefPtr<PerspectiveCamera>& camera)
//	{
//		TST_PROFILE_FN();
//
//		if (!render_data.isInitialized || render_data.isTerminated) {
//			TST_CORE_ERROR("Renderer3D not initialized or already terminated");
//			return;
//		}
//
//		auto& projection = camera->getProjectionMatrix();
//		auto& view = camera->getViewMatrix();
//
//		// Store matrices for both shaders
//		render_data.projectionMatrix = projection;
//		render_data.viewMatrix = view;
//		render_data.viewProjectionMatrix = projection * view;
//
//		//// Setup flat texture shader for batched rendering
//		//render_data.flatTextureShader->bind();
//		//render_data.flatTextureShader->uploadMatrix4f(projection, "u_Projection");
//		//render_data.flatTextureShader->uploadMatrix4f(view, "u_View");
//
//		// Pre-configure mesh shader
//		render_data.meshShader->bind();
//		render_data.meshShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
//
//		render_data.quadIndex = 0;
//		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
//		render_data.textureSlotIndex = 1;
//	}
//	void Renderer3D::begin(const Camera& camera, const glm::mat4& transform)
//	{
//		TST_PROFILE_FN();
//
//		if (!render_data.isInitialized || render_data.isTerminated) {
//			TST_CORE_ERROR("Renderer3D not initialized or already terminated");
//			return;
//		}
//
//		auto& projection = camera.getProjection();
//		auto view = glm::inverse(transform);
//
//		// Store matrices for both shaders
//		render_data.projectionMatrix = projection;
//		render_data.viewMatrix = view;
//		render_data.viewProjectionMatrix = projection * view;
//
//		//// Setup flat texture shader for batched rendering
//		//render_data.flatTextureShader->bind();
//		//render_data.flatTextureShader->uploadMatrix4f(projection, "u_Projection");
//		//render_data.flatTextureShader->uploadMatrix4f(view, "u_View");
//
//		// Pre-configure mesh shader
//		render_data.meshShader->bind();
//		render_data.meshShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
//
//		render_data.quadIndex = 0;
//		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
//		render_data.textureSlotIndex = 1;
//	}
//	void Renderer3D::begin(const RefPtr<OrthoCamera>& camera)
//	{
//		TST_PROFILE_FN();
//
//		if (!render_data.isInitialized || render_data.isTerminated) {
//			TST_CORE_ERROR("Renderer3D not initialized or already terminated");
//			return;
//		}
//
//		render_data.flatTextureShader->bind();
//		render_data.flatTextureShader->uploadMatrix4f(camera->getProjectionMatrix(), "u_Projection");
//		render_data.flatTextureShader->uploadMatrix4f(camera->getViewMatrix(), "u_View");
//		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
//		render_data.quadIndex = 0;
//		render_data.textureSlotIndex = 1;
//	}
//
//	void Renderer3D::end()
//	{
//		TST_PROFILE_FN();
//
//		flush();
//		render_data.quadIndex = 0;
//		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
//		render_data.textureSlotIndex = 1;
//	}
//
//	void Renderer3D::flush()
//	{
//
//		if (render_data.quadIndex == 0)
//		{
//			return;
//		}
//
//		uint32_t dataSize = (uint8_t*)render_data.quadVertexPtr - (uint8_t*)render_data.quadVertexBufferBase;
//		render_data.quadVertexBuffer->setData(render_data.quadVertexBufferBase, dataSize);
//
//		for (uint32_t i = 0; i < render_data.textureSlotIndex; i++)
//		{
//			if (render_data.textureSlots[i])
//			{
//				render_data.textureSlots[i]->bind(i);
//			}
//		}
//
//		RenderCommand::drawIndexed(render_data.quadVertexArray, render_data.quadIndex);
//
//		render_data.stats.drawCallCount++;
//		render_data.stats.verticesSubmitted += (render_data.quadVertexPtr - render_data.quadVertexBufferBase);
//	}
//
//	void Renderer3D::beginNewBatch()
//	{
//		uint32_t dataSize = (uint8_t*)render_data.quadVertexPtr - (uint8_t*)render_data.quadVertexBufferBase;
//
//		if (dataSize > 0)
//		{
//			render_data.quadVertexBuffer->setData(render_data.quadVertexBufferBase, dataSize);
//
//			for (uint32_t i = 0; i < render_data.textureSlotIndex; i++)
//			{
//				if (render_data.textureSlots[i])
//				{
//					render_data.textureSlots[i]->bind(i);
//				}
//			}
//
//			RenderCommand::drawIndexed(render_data.quadVertexArray, render_data.quadIndex);
//
//			render_data.stats.drawCallCount++;
//		}
//
//
//		render_data.quadIndex = 0;
//		render_data.quadVertexPtr = render_data.quadVertexBufferBase;
//		render_data.textureSlotIndex = 1;
//
//		render_data.stats.batchesPerFrame++;
//	}
//
//	uint32_t Renderer3D::enumerateTextureIndex(const RefPtr<Texture2D>& texture)
//	{
//		if (render_data.textureSlotIndex >= RenderData::maxTextureSlots) {
//			beginNewBatch();
//		}
//
//		int texIndex = 0;
//
//		for (uint32_t i = 1; i < render_data.textureSlotIndex; i++)
//		{
//			if (render_data.textureSlots[i]->getId() == texture->getId())
//			{
//				texIndex = i;
//				break;
//			}
//		}
//
//		if (texIndex == 0)
//		{
//			texIndex = render_data.textureSlotIndex;
//			render_data.textureSlots[render_data.textureSlotIndex] = texture;
//			render_data.textureSlotIndex++;
//		}
//
//		return texIndex;
//	}
//
//
//	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
//	{
//		glm::mat4 transform = transformationMat(position, rotation, scale);
//		drawQuad(transform, colour);
//	}
//
//	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
//	{
//
//		glm::mat4 transform = transformationMat(position, rotation, scale);
//		drawQuad(transform, texture, tilingScale, tintColour);
//	}
//
//	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
//	{
//		glm::mat4 transform = transformationMat(position, rotation, scale);
//		drawQuad(transform, texture, tilingScale, tintColour);
//	}
//
//
//	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
//	{
//		drawQuad(position, rotation, { scale.x, scale.y, 1.0f }, texture, tilingScale, tintColour);
//	}
//
//	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const glm::vec4& colour)
//	{
//		drawQuad(position, rotation, {scale.x, scale.y, 1.0f}, colour);
//	}
//
//	void Renderer3D::drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
//	{
//		drawQuad(position, rotation, { scale.x, scale.y, 1.0f }, texture, tilingScale, tintColour);
//	}
//
//	void Renderer3D::drawQuad(const glm::mat4& transform, const glm::vec4& colour)
//	{
//		TST_PROFILE_FN();
//
//		if (!render_data.quadVertexPtr) {
//			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
//			return;
//		}
//
//		if (render_data.quadIndex >= render_data.maxIndices)
//		{
//			beginNewBatch();
//		}
//
//		// There is no texture being bound, so it will be white (slot 0)
//		float texIndex = 0.0f;
//
//
//		// for each vertex of the quad :)
//		for (int i = 0; i < 4; i++)
//		{
//			render_data.quadVertexPtr->position = transform * render_data.quadVertexPositions[i];
//			render_data.quadVertexPtr->colour = colour;
//			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
//			render_data.quadVertexPtr->textureIndex = texIndex;
//			render_data.quadVertexPtr->tilingFactor = 1.0f;
//			render_data.quadVertexPtr++;
//		}
//
//		render_data.quadIndex += 6;
//
//		render_data.stats.quadCount++;
//	}
//
//	void Renderer3D::drawQuad(const glm::mat4& transform, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
//	{
//		if (!render_data.quadVertexPtr) {
//			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
//			return;
//		}
//
//		if (render_data.quadIndex >= render_data.maxIndices)
//		{
//			beginNewBatch();
//		}
//
//		constexpr glm::vec4 whiteColour = { 1.0f, 1.0f, 1.0f, 1.0f };
//
//		float texIndex = static_cast<float>(enumerateTextureIndex(texture));
//
//		// for each vertex of the quad :)
//		for (int i = 0; i < 4; i++)
//		{
//			render_data.quadVertexPtr->position = transform * render_data.quadVertexPositions[i];
//			render_data.quadVertexPtr->colour = whiteColour * tintColour;
//			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
//			render_data.quadVertexPtr->textureIndex = texIndex;
//			render_data.quadVertexPtr->tilingFactor = tilingScale;
//			render_data.quadVertexPtr++;
//		}
//
//		render_data.quadIndex += 6;
//
//		render_data.stats.quadCount++;
//	}
//
//	void Renderer3D::drawQuad(const glm::mat4& transform, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
//	{
//		if (!render_data.quadVertexPtr) {
//			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer2D::begin()?");
//			return;
//		}
//
//		if (render_data.quadIndex >= render_data.maxIndices)
//		{
//			beginNewBatch();
//		}
//
//		constexpr glm::vec4 whiteColour = { 1.0f, 1.0f, 1.0f, 1.0f };
//
//		float texIndex = static_cast<float>(enumerateTextureIndex(texture->getBaseTexture()));
//
//		// for each vertex of the quad :)
//		for (int i = 0; i < 4; i++)
//		{
//			render_data.quadVertexPtr->position = transform * render_data.quadVertexPositions[i];
//			render_data.quadVertexPtr->colour = whiteColour * tintColour;
//			render_data.quadVertexPtr->textureCoords = render_data.quadVertexUvs[i];
//			render_data.quadVertexPtr->textureIndex = texIndex;
//			render_data.quadVertexPtr->tilingFactor = tilingScale;
//			render_data.quadVertexPtr++;
//		}
//
//		render_data.quadIndex += 6;
//
//		render_data.stats.quadCount++;
//	}
//
//
//	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour)
//	{
//		glm::mat4 transform = transformationMat(position, rotation, scale);
//		drawCube(transform, colour);
//	}
//
//	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
//	{
//		glm::mat4 transform = transformationMat(position, rotation, scale);
//		drawCube(transform, texture, tilingScale, tintColour);
//	}
//
//	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale, const glm::vec4& tintColour)
//	{
//		glm::mat4 transform = transformationMat(position, rotation, scale);
//		drawCube(transform, texture, tilingScale, tintColour);
//	}
//
//	void Renderer3D::drawCube(const glm::mat4& transform, const glm::vec4& colour)
//	{
//
//		if (!render_data.quadVertexPtr) {
//			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer3D::begin()?");
//			return;
//		}
//
//		if (render_data.quadIndex + 36 >= render_data.maxIndices)
//		{
//			beginNewBatch();
//		}
//
//		float texIndex = 0.0f;
//
//		// Define cube faces as quads with proper normals and UVs
//		struct CubeFace {
//			glm::vec3 vertices[4];
//			glm::vec2 uvs[4];
//		};
//
//		CubeFace faces[6] = {
//			// Front face
//			{{{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Back face
//			{{{ 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Left face
//			{{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Right face
//			{{{ 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Top face
//			{{{-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Bottom face
//			{{{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}}
//		};
//
//
//		uint32_t cubeIndices[36] = {
//			// Back face
//			0, 1, 2, 2, 3, 0,
//			// Front face
//			4, 5, 6, 6, 7, 4,
//			// Left face
//			0, 4, 7, 7, 3, 0,
//			// Right face
//			1, 5, 6, 6, 2, 1,
//			// Bottom face
//			0, 1, 5, 5, 4, 0,
//			// Top face
//			3, 2, 6, 6, 7, 3
//		};
//
//		// Draw each face as a quad
//		for (int face = 0; face < 6; face++)
//		{
//			for (int vertex = 0; vertex < 4; vertex++)
//			{
//				render_data.quadVertexPtr->position = transform * glm::vec4(faces[face].vertices[vertex], 1.0f);
//				render_data.quadVertexPtr->colour = colour;
//				render_data.quadVertexPtr->textureCoords = faces[face].uvs[vertex];
//				render_data.quadVertexPtr->textureIndex = texIndex;
//				render_data.quadVertexPtr->tilingFactor = 1.0f;
//				render_data.quadVertexPtr++;
//			}
//		}
//
//		render_data.quadIndex += 36; // Cube indices
//		render_data.stats.quadCount += 6;
//	}
//
//	void Renderer3D::drawCube(const glm::mat4& transform, const RefPtr<Texture2D>& texture, float tilingScale, const glm::vec4& tintColour)
//	{
//		if (!render_data.quadVertexPtr) {
//			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer3D::begin()?");
//			return;
//		}
//
//		if (render_data.quadIndex + 36 >= render_data.maxIndices) {	beginNewBatch(); }
//
//		constexpr glm::vec4 whiteColour = { 1.0f, 1.0f, 1.0f, 1.0f };
//
//		float texIndex = static_cast<float>(enumerateTextureIndex(texture));
//
//		// Define cube faces as quads with proper normals and UVs
//		struct CubeFace {
//			glm::vec3 vertices[4];
//			glm::vec2 uvs[4];
//		};
//
//		CubeFace faces[6] = {
//			// Front face
//			{{{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Back face
//			{{{ 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Left face
//			{{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Right face
//			{{{ 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Top face
//			{{{-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Bottom face
//			{{{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}}
//		};
//
//
//		uint32_t cubeIndices[36] = {
//			// Back face
//			0, 1, 2, 2, 3, 0,
//			// Front face
//			4, 5, 6, 6, 7, 4,
//			// Left face
//			0, 4, 7, 7, 3, 0,
//			// Right face
//			1, 5, 6, 6, 2, 1,
//			// Bottom face
//			0, 1, 5, 5, 4, 0,
//			// Top face
//			3, 2, 6, 6, 7, 3
//		};
//
//		// Draw each face as a quad
//		for (int face = 0; face < 6; face++)
//		{
//			for (int vertex = 0; vertex < 4; vertex++)
//			{
//				render_data.quadVertexPtr->position = transform * glm::vec4(faces[face].vertices[vertex], 1.0f);
//				render_data.quadVertexPtr->colour = whiteColour * tintColour;
//				render_data.quadVertexPtr->textureCoords = faces[face].uvs[vertex];
//				render_data.quadVertexPtr->textureIndex = texIndex;
//				render_data.quadVertexPtr->tilingFactor = 1.0f;
//				render_data.quadVertexPtr++;
//			}
//		}
//
//		render_data.quadIndex += 36; // Cube indices
//		render_data.stats.quadCount += 6;
//
//	}
//
//
//	void Renderer3D::drawCube(const glm::mat4 &transform, const RefPtr<SubTexture2D>& texture, float tilingScale, const glm::vec4& tintColour)
//	{
//		if (!render_data.quadVertexPtr) {
//			TST_ASSERT(false, "quadVertexPtr is nullptr! Did you forget to call Renderer3D::begin()?");
//			return;
//		}
//
//		if (render_data.quadIndex + 36 >= render_data.maxIndices) { beginNewBatch(); }
//
//		constexpr glm::vec4 whiteColour = { 1.0f, 1.0f, 1.0f, 1.0f };
//
//		float texIndex = static_cast<float>(enumerateTextureIndex(texture->getBaseTexture()));
//
//		// Define cube faces as quads with proper normals and UVs
//		struct CubeFace {
//			glm::vec3 vertices[4];
//			glm::vec2 uvs[4];
//		};
//
//		CubeFace faces[6] = {
//			// Front face
//			{{{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Back face
//			{{{ 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Left face
//			{{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Right face
//			{{{ 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Top face
//			{{{-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}},
//			// Bottom face
//			{{{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}},
//			{{ 0.0f,  0.0f}, { 1.0f,  0.0f}, { 1.0f,  1.0f}, { 0.0f,  1.0f}}}
//		};
//
//
//		uint32_t cubeIndices[36] = {
//			// Back face
//			0, 1, 2, 2, 3, 0,
//			// Front face
//			4, 5, 6, 6, 7, 4,
//			// Left face
//			0, 4, 7, 7, 3, 0,
//			// Right face
//			1, 5, 6, 6, 2, 1,
//			// Bottom face
//			0, 1, 5, 5, 4, 0,
//			// Top face
//			3, 2, 6, 6, 7, 3
//		};
//
//		// Draw each face as a quad
//		for (int face = 0; face < 6; face++)
//		{
//			for (int vertex = 0; vertex < 4; vertex++)
//			{
//				render_data.quadVertexPtr->position = transform * glm::vec4(faces[face].vertices[vertex], 1.0f);
//				render_data.quadVertexPtr->colour = whiteColour * tintColour;
//				render_data.quadVertexPtr->textureCoords = faces[face].uvs[vertex];
//				render_data.quadVertexPtr->textureIndex = texIndex;
//				render_data.quadVertexPtr->tilingFactor = 1.0f;
//				render_data.quadVertexPtr++;
//			}
//		}
//
//		render_data.quadIndex += 36; // Cube indices
//		render_data.stats.quadCount += 6;
//
//	}
//
//	// Add this to the Renderer3D class (add to header first)
//	void Renderer3D::drawMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform, const glm::vec4& colour)
//	{
//		if (!mesh) {
//			return;
//		}
//
//		// For now, flush current batch and render mesh separately
//		// In a more advanced system, you'd batch meshes too
//		flush();
//
//		// Switch to mesh shader and configure it properly
//		render_data.meshShader->bind();
//
//		// Upload transformation matrices
//		render_data.meshShader->uploadMatrix4f(render_data.viewProjectionMatrix, "u_ViewProjection");
//		render_data.meshShader->uploadMatrix4f(transform, "u_Model");
//
//		// Calculate and upload normal matrix for proper lighting
//		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
//		render_data.meshShader->uploadMatrix3f(normalMatrix, "u_NormalMatrix");
//
//		// Upload material properties
//		render_data.meshShader->uploadVector4f(colour, "u_Colour");
//
//		// Upload lighting data
//		render_data.meshShader->uploadVector3f(render_data.mainLight.position, "u_LightPos");
//		render_data.meshShader->uploadVector3f(render_data.mainLight.colour * render_data.mainLight.intensity, "u_LightColour");
//		render_data.meshShader->uploadVector3f(render_data.cameraPosition, "u_ViewPos");
//
//		render_data.whiteTexture->bind(0);
//
//		render_data.meshShader->uploadInt1(0, "u_Texture");
//
//		// Enable depth testing for proper 3D rendering
//		RenderCommand::enableDepthTesting();
//
//		// Bind and render mesh
//		mesh->bind();
//		RenderCommand::drawIndexed(mesh->getVertexArray(), mesh->getIndexCount());
//		mesh->unbind();
//
//		// Update statistics
//		render_data.stats.drawCallCount++;
//		render_data.stats.triangleCount += mesh->getIndexCount() / 3;
//		render_data.stats.meshCount++;
//
//	}
//
//	void Renderer3D::drawMesh(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Mesh>& mesh, const glm::vec4& colour)
//	{
//		glm::mat4 transform = transformationMat(position, rotation, scale);
//		drawMesh(mesh, transform, colour);
//	}
//
//	Renderer3D::Stats Renderer3D::getStats()
//	{
//		return render_data.stats;
//	}
//
//	void Renderer3D::resetStats()
//	{
//		render_data.stats.drawCallCount = 0;
//		render_data.stats.quadCount = 0;
//		render_data.stats.triangleCount = 0;
//		render_data.stats.circleCount = 0;
//		render_data.stats.lineCount = 0;
//		render_data.stats.batchesPerFrame = 1;
//		render_data.stats.textureBindings = 0;
//		render_data.stats.verticesSubmitted = 0;
//		render_data.stats.meshCount = 0;
//	}
//}
