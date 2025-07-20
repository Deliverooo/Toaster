#pragma once
#include "Buffer.hpp"

namespace tst
{

	class TST_API VertexArray
	{
	public:
		virtual ~VertexArray() {}

		static RefPtr<VertexArray> create();

		virtual void addVertexBuffer(const RefPtr<VertexBuffer> &buffer) = 0;
		virtual void addIndexBuffer(const RefPtr<IndexBuffer> &buffer) = 0;

		virtual const std::vector<RefPtr<VertexBuffer>> &getVertexBuffers() const = 0;
		virtual const RefPtr<IndexBuffer> &getIndexBuffer() const = 0;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;
	};
}
