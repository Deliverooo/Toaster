#pragma once
#include "Toaster/Renderer/Buffer.hpp"

namespace tst
{
	class TST_API OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(float* vertices, uint32_t count);
		OpenGLVertexBuffer(uint32_t count);
		virtual ~OpenGLVertexBuffer() override;

		virtual void bind() const override;
		virtual void unbind() const override;

		virtual void setData(const void* data, const uint32_t size) override;
		virtual void setLayout(const BufferLayout& layout) override;
		virtual const BufferLayout &getLayout() const override;

	private:
		unsigned int m_Vbo;
		BufferLayout m_bufferLayout;
		uint32_t m_count;
	};

	class TST_API OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~OpenGLIndexBuffer() override;

		virtual void bind() const override;
		virtual void unbind() const override;

		virtual uint32_t count() const override;

	private:
		unsigned int m_Ebo;
		uint32_t m_count;
	};

}

