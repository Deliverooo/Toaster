#pragma once
#include "Buffer.hpp"

namespace tst
{

	class TST_API VertexArray
	{
	public:
		virtual ~VertexArray() {}

		static std::shared_ptr<VertexArray> create();

		virtual void addVertexBuffer(const std::shared_ptr<VertexBuffer> &buffer) = 0;
		virtual void addIndexBuffer(const std::shared_ptr<IndexBuffer> &buffer) = 0;

		virtual const std::vector<std::shared_ptr<VertexBuffer>> &getVertexBuffers() const = 0;
		virtual const std::shared_ptr<IndexBuffer> &getIndexBuffer() const = 0;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;
	};
}
