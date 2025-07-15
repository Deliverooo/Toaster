#pragma once
#include "glad/glad.h"
#include "Toaster/Renderer/VertexArray.hpp"

namespace tst
{
	static GLenum toastDataTypeToOpenGL(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Bool: return GL_BOOL;

		case ShaderDataType::Int:  return GL_INT;
		case ShaderDataType::Int3: return GL_INT;
		case ShaderDataType::Int2: return GL_INT;
		case ShaderDataType::Int4: return GL_INT;

		case ShaderDataType::Uint:  return GL_UNSIGNED_INT;
		case ShaderDataType::Uint2: return GL_UNSIGNED_INT;
		case ShaderDataType::Uint3: return GL_UNSIGNED_INT;
		case ShaderDataType::Uint4: return GL_UNSIGNED_INT;

		case ShaderDataType::Long:  return GL_INT;
		case ShaderDataType::Long2: return GL_INT;
		case ShaderDataType::Long3: return GL_INT;
		case ShaderDataType::Long4: return GL_INT;

		case ShaderDataType::LongLong:  return GL_INT;
		case ShaderDataType::LongLong2: return GL_INT;
		case ShaderDataType::LongLong3: return GL_INT;
		case ShaderDataType::LongLong4: return GL_INT;

		case ShaderDataType::UlongLong:  return GL_UNSIGNED_INT;
		case ShaderDataType::UlongLong2: return GL_UNSIGNED_INT;
		case ShaderDataType::UlongLong3: return GL_UNSIGNED_INT;
		case ShaderDataType::UlongLong4: return GL_UNSIGNED_INT;

		case ShaderDataType::Ulong:  return GL_UNSIGNED_INT;
		case ShaderDataType::Ulong2: return GL_UNSIGNED_INT;
		case ShaderDataType::Ulong3: return GL_UNSIGNED_INT;
		case ShaderDataType::Ulong4: return GL_UNSIGNED_INT;

		case ShaderDataType::Short:  return GL_SHORT;
		case ShaderDataType::Short2: return GL_SHORT;
		case ShaderDataType::Short3: return GL_SHORT;
		case ShaderDataType::Short4: return GL_SHORT;

		case ShaderDataType::Ushort:  return GL_UNSIGNED_SHORT;
		case ShaderDataType::Ushort2: return GL_UNSIGNED_SHORT;
		case ShaderDataType::Ushort3: return GL_UNSIGNED_SHORT;
		case ShaderDataType::Ushort4: return GL_UNSIGNED_SHORT;

		case ShaderDataType::Float:  return GL_FLOAT;
		case ShaderDataType::Float2: return GL_FLOAT;
		case ShaderDataType::Float3: return GL_FLOAT;
		case ShaderDataType::Float4: return GL_FLOAT;

		case ShaderDataType::Double:  return GL_DOUBLE;
		case ShaderDataType::Double2: return GL_DOUBLE;
		case ShaderDataType::Double3: return GL_DOUBLE;
		case ShaderDataType::Double4: return GL_DOUBLE;

		default: TST_ASSERT(false, "Invalid Shader Data Type!");
		}
		return 0;
	}

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray() override;

		virtual void bind() const override;
		virtual void unbind() const override;

		virtual void addVertexBuffer(const std::shared_ptr<VertexBuffer> &buffer) override;
		virtual void addIndexBuffer(const std::shared_ptr<IndexBuffer> &buffer) override;

		virtual const std::vector<std::shared_ptr<VertexBuffer>>& getVertexBuffers() const override;
		virtual const std::shared_ptr<IndexBuffer>& getIndexBuffer() const override;

	private:
		unsigned int m_Vao;

		std::vector<std::shared_ptr<VertexBuffer>> m_vertexBuffers;
		std::shared_ptr<IndexBuffer> m_indexBuffer;
	};
}
