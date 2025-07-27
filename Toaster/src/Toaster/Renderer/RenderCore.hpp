#pragma once
#include "Camera.hpp"
#include "Texture.hpp"
#include "VertexArray.hpp"
#include "Buffer.hpp"
#include "Shader.hpp"


namespace tst
{

	class TST_API IRenderer
	{
	public:

		virtual ~IRenderer() {}

		virtual void init() = 0;
		virtual void terminate() = 0;

		virtual void begin(const RefPtr<OrthoCamera>& camera) = 0;
		virtual void end() = 0;

		virtual void flush() = 0;
		virtual void resetStats() = 0;
	};


	template<typename VertexType, typename StatsType>
	class BatchRenderer : public IRenderer
	{
	protected:
		struct BatchData
		{
			uint32_t maxVertices = 40000;
			uint32_t maxIndices = 60000;
			static constexpr uint32_t maxTextureSlots = 32;

			RefPtr<VertexArray> vertexArray;
			RefPtr<VertexBuffer> vertexBuffer;
			RefPtr<IndexBuffer> indexBuffer;
			RefPtr<Shader> shader;
			RefPtr<Texture2D> whiteTexture;

			uint32_t indexCount = 0;
			VertexType* vertexBufferBase = nullptr;
			VertexType* vertexPtr = nullptr;

			std::array<RefPtr<Texture2D>, maxTextureSlots> textureSlots;
			uint32_t textureSlotIndex = 1;

			StatsType stats{};
		};

		BatchData m_batchData;

	public:

		const StatsType& getStats() const { return m_batchData.stats; }

	};




}
