#include "tstpch.h"
#include "OpenGLVertexArray.hpp"
#include <glad/glad.h>

namespace tst
{
	OpenGLVertexArray::OpenGLVertexArray()
	{
		glGenVertexArrays(1, &m_Vao);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_Vao);
	}

	void OpenGLVertexArray::addVertexBuffer(const std::shared_ptr<VertexBuffer> &buffer)
	{
		glBindVertexArray(m_Vao);
		buffer->bind();

		TST_ASSERT(buffer->getLayout().getAttributes().size(), "Vertex Buffer layout not initialized!");

		uint32_t index = 0;
		const auto& layout = buffer->getLayout();
		for (const auto& attrib : layout)
		{
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, attrib.getAttribCount(), toastDataTypeToOpenGL(attrib.type), attrib.normalized, layout.getStride(),
				reinterpret_cast<void*>(static_cast<uint64_t>(attrib.offset)));
			TST_CORE_INFO("Attrib {0} : {1}", index, attrib.name);
			index++;
		}

		m_vertexBuffers.push_back(buffer);
	}

	void OpenGLVertexArray::addIndexBuffer(const std::shared_ptr<IndexBuffer> &buffer)
	{
		glBindVertexArray(m_Vao);
		buffer->bind();
		m_indexBuffer = buffer;
	}

	void OpenGLVertexArray::bind() const
	{
		glBindVertexArray(m_Vao);
	}

	void OpenGLVertexArray::unbind() const
	{
		glBindVertexArray(0);
	}

	const std::vector<std::shared_ptr<VertexBuffer>>& OpenGLVertexArray::getVertexBuffers() const
	{
		return m_vertexBuffers;
	}

	const std::shared_ptr<IndexBuffer>& OpenGLVertexArray::getIndexBuffer() const
	{
		return m_indexBuffer;
	}
}
