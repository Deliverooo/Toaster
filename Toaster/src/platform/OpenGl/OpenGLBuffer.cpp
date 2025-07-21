#include "tstpch.h"
#include "OpenGLBuffer.hpp"
#include <glad/glad.h>

namespace tst
{
	OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t count) : m_count(count)
	{
		TST_PROFILE_FN();

		glGenBuffers(1, &m_Vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * count, vertices, GL_STATIC_DRAW);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		TST_PROFILE_FN();

		glDeleteBuffers(1, &m_Vbo);
	}

	void OpenGLVertexBuffer::bind() const
	{
		TST_PROFILE_FN();

		glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
	}

	void OpenGLVertexBuffer::unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::setLayout(const BufferLayout &layout)
	{
		m_bufferLayout = layout;
	}

	const BufferLayout& OpenGLVertexBuffer::getLayout() const
	{
		return m_bufferLayout;
	}


	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : m_count(count)
	{
		glGenBuffers(1, &m_Ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &m_Ebo);
	}

	void OpenGLIndexBuffer::bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
	}

	void OpenGLIndexBuffer::unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	uint32_t OpenGLIndexBuffer::count() const
	{
		return m_count;
	}
}
