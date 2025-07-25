#include "tstpch.h"
#include "OpenGLVertexArray.hpp"
#include <glad/glad.h>

namespace tst
{
	OpenGLVertexArray::OpenGLVertexArray()
	{
		TST_PROFILE_FN();

		glGenVertexArrays(1, &m_Vao);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		TST_PROFILE_FN();

		glDeleteVertexArrays(1, &m_Vao);
	}

	void OpenGLVertexArray::addVertexBuffer(const RefPtr<VertexBuffer> &buffer)
	{
		if (buffer->getLayout().getAttributes().empty())
		{
			TST_ASSERT(false, "Vertex buffer layouts have not been initialized!");
		}

		glBindVertexArray(m_Vao);
		buffer->bind();


		uint32_t index = 0;
		const auto& layout = buffer->getLayout();
		for (const auto& attrib : layout)
		{
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, attrib.getAttribCount(), toastDataTypeToOpenGL(attrib.type), attrib.normalized ? GL_TRUE : GL_FALSE, layout.getStride(), (const void*)attrib.offset);
			TST_CORE_INFO("Attrib {0} : {1}", index, attrib.name);
			index++;
		}

		m_vertexBuffers.push_back(buffer);
	}

	void OpenGLVertexArray::addIndexBuffer(const RefPtr<IndexBuffer> &buffer)
	{
		TST_PROFILE_FN();

		glBindVertexArray(m_Vao);
		buffer->bind();
		m_indexBuffer = buffer;
	}

	void OpenGLVertexArray::bind() const
	{
		TST_PROFILE_FN();

		glBindVertexArray(m_Vao);
	}

	void OpenGLVertexArray::unbind() const
	{
		TST_PROFILE_FN();

		glBindVertexArray(0);
	}

	const std::vector<RefPtr<VertexBuffer>>& OpenGLVertexArray::getVertexBuffers() const
	{
		return m_vertexBuffers;
	}

	const RefPtr<IndexBuffer>& OpenGLVertexArray::getIndexBuffer() const
	{
		return m_indexBuffer;
	}
}
