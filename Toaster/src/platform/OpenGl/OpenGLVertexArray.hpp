#pragma once
#include "glad/glad.h"
#include "Toaster/Renderer/VertexArray.hpp"

namespace tst
{
	static GLenum toastDataTypeToOpenGL(ShaderDataType type)
	{

		static const std::unordered_map<ShaderDataType, GLenum> shaderTypeMap = {
			{ShaderDataType::Bool,		GL_BOOL},

			{ShaderDataType::Int,		GL_INT},
			{ShaderDataType::Int2,		GL_INT},
			{ShaderDataType::Int3,		GL_INT},
			{ShaderDataType::Int4,		GL_INT},

			{ShaderDataType::Uint,		GL_UNSIGNED_INT},
			{ShaderDataType::Uint2,		GL_UNSIGNED_INT},
			{ShaderDataType::Uint3,		GL_UNSIGNED_INT},
			{ShaderDataType::Uint4,		GL_UNSIGNED_INT},

			{ShaderDataType::Long,		GL_INT},
			{ShaderDataType::Long2,		GL_INT},
			{ShaderDataType::Long3,		GL_INT},
			{ShaderDataType::Long4,		GL_INT},

			{ShaderDataType::Ulong,		GL_UNSIGNED_INT},
			{ShaderDataType::Ulong2,	GL_UNSIGNED_INT},
			{ShaderDataType::Ulong3,	GL_UNSIGNED_INT},
			{ShaderDataType::Ulong4,	GL_UNSIGNED_INT},

			{ShaderDataType::LongLong,	GL_INT},
			{ShaderDataType::LongLong2,	GL_INT},
			{ShaderDataType::LongLong3,	GL_INT},
			{ShaderDataType::LongLong4,	GL_INT},

			{ShaderDataType::UlongLong,	GL_UNSIGNED_INT},
			{ShaderDataType::UlongLong2,GL_UNSIGNED_INT},
			{ShaderDataType::UlongLong3,GL_UNSIGNED_INT},
			{ShaderDataType::UlongLong4,GL_UNSIGNED_INT},

			{ShaderDataType::Short,		GL_SHORT},
			{ShaderDataType::Short2,	GL_SHORT},
			{ShaderDataType::Short3,	GL_SHORT},
			{ShaderDataType::Short4,	GL_SHORT},

			{ShaderDataType::Ushort,	GL_UNSIGNED_SHORT},
			{ShaderDataType::Ushort2,	GL_UNSIGNED_SHORT},
			{ShaderDataType::Ushort3,	GL_UNSIGNED_SHORT},
			{ShaderDataType::Ushort4,	GL_UNSIGNED_SHORT},

			{ShaderDataType::Float,		GL_FLOAT},
			{ShaderDataType::Float2,	GL_FLOAT},
			{ShaderDataType::Float3,	GL_FLOAT},
			{ShaderDataType::Float4,	GL_FLOAT},

			{ShaderDataType::Double,	GL_DOUBLE},
			{ShaderDataType::Double2,	GL_DOUBLE},
			{ShaderDataType::Double3,	GL_DOUBLE},
			{ShaderDataType::Double4,	GL_DOUBLE},
		};

		auto it = shaderTypeMap.find(type);
		if (it != shaderTypeMap.end())
		{
			return it->second;
		}

		TST_ASSERT(false, "Invalid shader data type provided to vertex buffer layout!");
	}

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray() override;

		virtual void bind() const override;
		virtual void unbind() const override;

		virtual void addVertexBuffer(const RefPtr<VertexBuffer> &buffer) override;
		virtual void addIndexBuffer(const RefPtr<IndexBuffer> &buffer) override;

		virtual const std::vector<RefPtr<VertexBuffer>>& getVertexBuffers() const override;
		virtual const RefPtr<IndexBuffer>& getIndexBuffer() const override;

	private:
		unsigned int m_Vao;

		std::vector<RefPtr<VertexBuffer>> m_vertexBuffers;
		RefPtr<IndexBuffer> m_indexBuffer;
	};
}
