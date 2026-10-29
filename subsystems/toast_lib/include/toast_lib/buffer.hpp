#pragma once

#include "system_types.h"
#include "toast_assert.hpp"

#include <memory.h>

namespace toaster
{
	class TST_LIB_API DataBuffer
	{
	public:
		DataBuffer() = default;

		DataBuffer(const void *p_data, const uint64 p_size) : m_data(const_cast<void *>(p_data)), m_size(p_size)
		{
		}

		template<typename Type>
		DataBuffer()
		{
			allocate<Type>();
		}

		static auto copy(const DataBuffer &p_other) -> DataBuffer
		{
			DataBuffer buffer;
			buffer.allocate(p_other.m_size);
			memcpy(buffer.m_data, p_other.m_data, p_other.m_size);
			return buffer;
		}

		static auto copy(const void *p_data, uint64 p_size) -> DataBuffer
		{
			DataBuffer buffer;
			buffer.allocate(p_size);
			if (p_size)
				memcpy(buffer.m_data, p_data, p_size);
			return buffer;
		}

		auto allocate(const uint64 p_size) -> void
		{
			release();
			m_size = p_size;

			if (m_size == 0)
				return;

			m_data = new uint8[m_size];
		}

		template<typename Type>
		auto allocate() -> void
		{
			release();
			m_size = sizeof(Type);

			if (m_size == 0)
				return;

			m_data = new uint8[m_size];
		}

		auto reallocate(const uint64 p_size) -> void
		{
			release();
			allocate(p_size);
		}

		auto release() -> void
		{
			if (m_data)
				delete[] static_cast<uint8 *>(m_data);
			m_data = nullptr;
			m_size = 0;
		}

		auto zeroInitialize() -> void
		{
			if (m_data)
				memset(m_data,0u, m_size);
		}

		template<typename Type>
		auto readType(const uint64 p_offset = 0u) -> Type &
		{
			return *reinterpret_cast<Type *>(static_cast<uint8 *>(m_data) + p_offset);
		}

		template<typename Type>
		auto readType(uint64 p_offset = 0u) const -> const Type &
		{
			return *static_cast<Type *>(static_cast<uint8 *>(m_data) + p_offset);
		}

		[[nodiscard]] auto readBytes(uint64 p_size, uint64 p_offset) const -> uint8 *
		{
			TST_ASSERT_MSG(p_offset + p_size <= m_size, "Buffer overflow!");
			const auto buffer = new uint8[p_size];
			memcpy(buffer, static_cast<uint8 *>(m_data) + p_offset, p_size);
			return buffer;
		}

		template<typename Type>
		auto writeType(const Type &p_data, uint64 p_offset = 0u) -> void
		{
			memcpy(static_cast<uint8 *>(m_data) + p_offset, reinterpret_cast<const void *>(&p_data), sizeof(Type));
		}

		auto write(DataBuffer p_buffer, uint64 p_offset = 0u) -> void
		{
			TST_ASSERT_MSG(p_offset + p_buffer.m_size <= m_size, "Buffer overflow!");
			memcpy(static_cast<uint8 *>(m_data) + p_offset, p_buffer.m_data, p_buffer.m_size);
		}

		auto write(const void *p_data, uint64 p_size, uint64 p_offset = 0u) -> void
		{
			write(DataBuffer(p_data, p_size), p_offset);
		}

		operator bool() const
		{
			return static_cast<bool>(m_data);
		}

		auto operator[](int32 p_index) -> uint8 &
		{
			return static_cast<uint8 *>(m_data)[p_index];
		}

		auto operator[](int32 p_index) const -> uint8
		{
			return static_cast<uint8 *>(m_data)[p_index];
		}

		template<typename Type>
		auto as() const -> Type *
		{
			return static_cast<Type *>(m_data);
		}

		[[nodiscard]] auto size() const -> uint64 { return m_size; }
		[[nodiscard]] auto data() const -> void * { return m_data; }

	private:
		void * m_data{nullptr};
		uint64 m_size{0u};
	};
}
