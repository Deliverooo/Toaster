#pragma once

#include "buffer.hpp"
#include <concepts>

#include "memory.hpp"

namespace toaster
{
	class TST_LIB_API CommandBuffer
	{
	public:
		using Invoker    = void(*)(void *);
		using Destructor = void(*)(void *);

		CommandBuffer(uint64 p_max_size_bytes = 10u * 1028u * 1028u /*10MB*/) : m_bufferCapacity(p_max_size_bytes)
		{
			m_buffer = static_cast<uint8 *>(alignedAlloc(m_bufferCapacity, maxAlignment));
			m_ptr    = m_buffer;
		}

		~CommandBuffer()
		{
			// Only call the destructors on the remaining functions
			uint8 *          buffer{m_buffer};
			constexpr uint64 alignment{alignof(std::max_align_t)};
			for (uint32 i{0u}; i < m_commandCount; ++i)
			{
				uint64 space{static_cast<uintptr>(m_bufferCapacity - (buffer - m_buffer))};
				void * aligned_ptr{buffer};
				std::align(alignment, sizeof(Invoker), aligned_ptr, space);
				buffer = static_cast<uint8 *>(aligned_ptr);

				buffer += sizeof(Invoker);

				Destructor destructor{nullptr};
				std::memcpy(&destructor, buffer, sizeof(Destructor));
				buffer += sizeof(Destructor);

				uint32 payload_size{0u};
				std::memcpy(&payload_size, buffer, sizeof(uint32));
				buffer += sizeof(uint32);

				void *payload{buffer};

				(*destructor)(payload);

				buffer += payload_size;
			}

			alignedFree(m_buffer);
		}

		CommandBuffer(const CommandBuffer &)            = delete;
		CommandBuffer &operator=(const CommandBuffer &) = delete;

		CommandBuffer(CommandBuffer &&p_other) noexcept : m_buffer(p_other.m_buffer), m_ptr(p_other.m_ptr), m_bufferCapacity(p_other.m_bufferCapacity),
														  m_commandCount(p_other.m_commandCount)
		{
			p_other.m_buffer         = nullptr;
			p_other.m_ptr            = nullptr;
			p_other.m_bufferCapacity = 0u;
			p_other.m_commandCount   = 0u;
		}

		CommandBuffer &operator=(CommandBuffer &&p_other) noexcept
		{
			if (this != &p_other)
			{
				alignedFree(m_buffer);
				m_buffer         = p_other.m_buffer;
				m_ptr            = p_other.m_ptr;
				m_bufferCapacity = p_other.m_bufferCapacity;
				m_commandCount   = p_other.m_commandCount;

				p_other.m_buffer         = nullptr;
				p_other.m_ptr            = nullptr;
				p_other.m_bufferCapacity = 0u;
				p_other.m_commandCount   = 0u;
			}
			return *this;
		}

		template<typename TFunc> requires std::is_invocable_v<TFunc> && std::is_nothrow_invocable_v<TFunc>
		auto enqueue(TFunc &&p_func) -> void
		{
			using DecayedFunc = std::decay_t<TFunc>;

			constexpr uint64 alignment{alignof(std::max_align_t)};

			uint8 *current_ptr{m_ptr};
			uint64 space{static_cast<uintptr>(m_bufferCapacity - (current_ptr - m_buffer))};
			void * aligned_ptr{current_ptr};

			if (!std::align(alignment, sizeof(Invoker), aligned_ptr, space))
			{
				TST_PERMA_ASSERT_MSG(false, "Failed to align pointer!");
			}

			const uint64 total_required_size{
				(static_cast<uint8 *>(aligned_ptr) - current_ptr) + sizeof(Invoker) + sizeof(Destructor) + sizeof(uint32) + sizeof(DecayedFunc)
			};

			if (m_ptr - m_buffer + total_required_size > m_bufferCapacity)
			{
				TST_PERMA_ASSERT_MSG(false, "Command buffer capacity has been exceeded!!");
			}

			m_ptr = static_cast<uint8 *>(aligned_ptr);

			const Invoker invoker{
				+[](void *payload) -> void
				{
					auto func{static_cast<DecayedFunc *>(payload)};
					(*func)();
				}
			};

			const Destructor destructor{
				+[](void *payload) -> void
				{
					auto func{static_cast<DecayedFunc *>(payload)};
					func->~DecayedFunc();
				}
			};

			std::memcpy(m_ptr, &invoker, sizeof(Invoker));
			m_ptr += sizeof(Invoker);

			std::memcpy(m_ptr, &destructor, sizeof(Destructor));
			m_ptr += sizeof(Destructor);

			const uint32 payload_size{sizeof(DecayedFunc)};
			std::memcpy(m_ptr, &payload_size, sizeof(uint32));
			m_ptr += sizeof(uint32);

			::new(static_cast<void *>(m_ptr)) DecayedFunc{std::forward<TFunc>(p_func)};
			m_ptr += sizeof(DecayedFunc);

			++m_commandCount;
		}

		auto execute() -> void
		{
			uint8 *          buffer{m_buffer};
			constexpr uint64 alignment{alignof(std::max_align_t)};
			for (uint32 i{0u}; i < m_commandCount; ++i)
			{
				uint64 space{static_cast<uintptr>(m_bufferCapacity - (buffer - m_buffer))};
				void * aligned_ptr{buffer};
				std::align(alignment, sizeof(Invoker), aligned_ptr, space);
				buffer = static_cast<uint8 *>(aligned_ptr);

				Invoker invoker{nullptr};
				std::memcpy(&invoker, buffer, sizeof(Invoker));
				buffer += sizeof(Invoker);

				Destructor destructor{nullptr};
				std::memcpy(&destructor, buffer, sizeof(Destructor));
				buffer += sizeof(Destructor);

				uint32 payload_size{0u};
				std::memcpy(&payload_size, buffer, sizeof(uint32));
				buffer += sizeof(uint32);

				void *payload{buffer};

				(*invoker)(payload);
				(*destructor)(payload);

				buffer += payload_size;
			}
			m_ptr          = m_buffer;
			m_commandCount = 0u;
		}

	private:
		uint8 *m_buffer{nullptr};
		uint8 *m_ptr{nullptr};
		uint32 m_bufferCapacity{0u};
		uint32 m_commandCount{0u};
	};
}
