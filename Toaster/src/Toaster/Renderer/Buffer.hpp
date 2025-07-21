#pragma once
#include <string>

#include "Toaster/Core/Log.hpp"

namespace tst
{
	enum class ShaderDataType
	{
		None = 0,

		Bool,
		Int, Int2, Int3, Int4,
		Uint, Uint2, Uint3, Uint4,
		Long, Long2, Long3, Long4,
		Short, Short2, Short3, Short4,
		Float, Float2, Float3, Float4,
		Ulong, Ulong2, Ulong3, Ulong4,
		Ushort, Ushort2, Ushort3, Ushort4,
		Double, Double2, Double3, Double4,
		LongLong, LongLong2, LongLong3, LongLong4,
		UlongLong, UlongLong2, UlongLong3, UlongLong4,
	};

	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Bool: return 1;

		case ShaderDataType::Int: return 4;
		case ShaderDataType::Int2: return 8;
		case ShaderDataType::Int3: return 12;
		case ShaderDataType::Int4: return 16;

		case ShaderDataType::Uint: return 4;
		case ShaderDataType::Uint2: return 8;
		case ShaderDataType::Uint3: return 12;
		case ShaderDataType::Uint4: return 16;

		case ShaderDataType::Long: return 4;
		case ShaderDataType::Long2: return 8;
		case ShaderDataType::Long3: return 12;
		case ShaderDataType::Long4: return 16;

		case ShaderDataType::LongLong: return 8;
		case ShaderDataType::LongLong2: return 16;
		case ShaderDataType::LongLong3: return 24;
		case ShaderDataType::LongLong4: return 32;

		case ShaderDataType::UlongLong: return 8;
		case ShaderDataType::UlongLong2: return 16;
		case ShaderDataType::UlongLong3: return 24;
		case ShaderDataType::UlongLong4: return 32;

		case ShaderDataType::Ulong: return 4;
		case ShaderDataType::Ulong2: return 8;
		case ShaderDataType::Ulong3: return 12;
		case ShaderDataType::Ulong4: return 16;

		case ShaderDataType::Short: return 2;
		case ShaderDataType::Short2: return 4;
		case ShaderDataType::Short3: return 6;
		case ShaderDataType::Short4: return 8;

		case ShaderDataType::Ushort: return 2;
		case ShaderDataType::Ushort2: return 4;
		case ShaderDataType::Ushort3: return 6;
		case ShaderDataType::Ushort4: return 8;

		case ShaderDataType::Float: return  4;
		case ShaderDataType::Float2: return 8;
		case ShaderDataType::Float3: return 12;
		case ShaderDataType::Float4: return 16;

		case ShaderDataType::Double: return 8;
		case ShaderDataType::Double2: return 16;
		case ShaderDataType::Double3: return 24;
		case ShaderDataType::Double4: return 32;

		default: TST_ASSERT(false, "Invalid Shader Data Type!");
		}
		return 0;
	}

	struct BufferAttribute
	{
		std::string name;
		ShaderDataType type;
		uint32_t offset;
		uint32_t size;
		bool normalized;

		BufferAttribute(const std::string &name, ShaderDataType type, bool normalized = false) : name(name), type(type), offset(0), normalized(normalized)
		{
			size = ShaderDataTypeSize(type);
		}

		uint32_t getAttribCount() const
		{
			switch (type)
			{
			case ShaderDataType::Bool: return 1;

			case ShaderDataType::Int:  return 1;
			case ShaderDataType::Int2: return 2;
			case ShaderDataType::Int3: return 3;
			case ShaderDataType::Int4: return 4;

			case ShaderDataType::Uint:  return 1;
			case ShaderDataType::Uint2: return 2;
			case ShaderDataType::Uint3: return 3;
			case ShaderDataType::Uint4: return 4;

			case ShaderDataType::Long:  return 1;
			case ShaderDataType::Long2: return 2;
			case ShaderDataType::Long3: return 3;
			case ShaderDataType::Long4: return 4;

			case ShaderDataType::LongLong:  return 1;
			case ShaderDataType::LongLong2: return 2;
			case ShaderDataType::LongLong3: return 3;
			case ShaderDataType::LongLong4: return 4;

			case ShaderDataType::UlongLong:  return 1;
			case ShaderDataType::UlongLong2: return 2;
			case ShaderDataType::UlongLong3: return 3;
			case ShaderDataType::UlongLong4: return 4;

			case ShaderDataType::Ulong:  return 1;
			case ShaderDataType::Ulong2: return 2;
			case ShaderDataType::Ulong3: return 3;
			case ShaderDataType::Ulong4: return 4;

			case ShaderDataType::Short:  return 1;
			case ShaderDataType::Short2: return 2;
			case ShaderDataType::Short3: return 3;
			case ShaderDataType::Short4: return 4;

			case ShaderDataType::Ushort:  return 1;
			case ShaderDataType::Ushort2: return 2;
			case ShaderDataType::Ushort3: return 3;
			case ShaderDataType::Ushort4: return 4;

			case ShaderDataType::Float:  return 1;
			case ShaderDataType::Float2: return 2;
			case ShaderDataType::Float3: return 3;
			case ShaderDataType::Float4: return 4;

			case ShaderDataType::Double:  return 1;
			case ShaderDataType::Double2: return 2;
			case ShaderDataType::Double3: return 3;
			case ShaderDataType::Double4: return 4;

			default: TST_ASSERT(false, "Invalid Shader Data Type!");
			}
			return 0;
		}
	};

	class TST_API BufferLayout
	{
	public:

		~BufferLayout() {}
		BufferLayout(const std::initializer_list<BufferAttribute> &attributes) : m_bufferAttributes(attributes)
		{
			calcOffsets();
		}
		BufferLayout() = default;

		void calcOffsets()
		{
			uint32_t offset = 0;
			m_stride = 0;
			for (auto &attrib : m_bufferAttributes)
			{
				attrib.offset = offset;
				offset += attrib.size;
				m_stride += attrib.size;
			}
		}

		const std::vector<BufferAttribute>& getAttributes() const { return m_bufferAttributes; }
		uint32_t getStride() const { return m_stride; }

		std::vector<BufferAttribute>::iterator begin() { return m_bufferAttributes.begin(); }
		std::vector<BufferAttribute>::iterator end() { return m_bufferAttributes.end(); }

		std::vector<BufferAttribute>::const_iterator begin() const { return m_bufferAttributes.cbegin(); }
		std::vector<BufferAttribute>::const_iterator end() const { return m_bufferAttributes.cend(); }


	private:
		std::vector<BufferAttribute> m_bufferAttributes;
		uint32_t m_stride = 0;
	};

	class TST_API VertexBuffer
	{
	public:
		virtual ~VertexBuffer() {}

		static RefPtr<VertexBuffer> create(float *vertices, uint32_t size);

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual void setLayout(const BufferLayout& layout) = 0;
		virtual const BufferLayout &getLayout() const = 0;
	};

	class TST_API IndexBuffer
	{
	public:
		virtual ~IndexBuffer() {}

		static RefPtr<IndexBuffer> create(uint32_t* indices, uint32_t count);

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual uint32_t count() const = 0;
	};
}
