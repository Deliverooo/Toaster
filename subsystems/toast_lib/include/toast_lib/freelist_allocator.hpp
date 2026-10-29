#pragma once

#include "system_types.h"
#include "toast_assert.h"
#include "vector.hpp"

namespace toaster
{
	// I think this may be a free list allocator...
	template<typename TSlotType = uint32> requires std::is_integral_v<TSlotType>
	struct FreelistAllocator
	{
		FreelistAllocator() = default;

		// You can change the first slot. E.g. make 0 an invalid slot...
		FreelistAllocator(TSlotType p_capacity, TSlotType p_first_slot = TSlotType{0}) : m_capacity(p_capacity + p_first_slot), m_nextFreeIndex(p_first_slot)
		{
		}

		FreelistAllocator(const FreelistAllocator &)            = delete;
		FreelistAllocator &operator=(const FreelistAllocator &) = delete;

		FreelistAllocator(FreelistAllocator &&p_other) noexcept : m_capacity(p_other.m_capacity), m_nextFreeIndex(p_other.m_nextFreeIndex),
																  m_recycledSlots(std::move(p_other.m_recycledSlots))
		{
			p_other.m_capacity      = 0u;
			p_other.m_nextFreeIndex = 0u;
			p_other.m_recycledSlots.clear();
		}

		FreelistAllocator &operator=(FreelistAllocator &&p_other) noexcept
		{
			if (this != &p_other)
			{
				m_capacity      = p_other.m_capacity;
				m_nextFreeIndex = p_other.m_nextFreeIndex;
				m_recycledSlots = std::move(p_other.m_recycledSlots);

				p_other.m_capacity      = 0u;
				p_other.m_nextFreeIndex = 0u;
				p_other.m_recycledSlots.clear();
			}
			return *this;
		}

		auto allocSlot() -> TSlotType
		{
			if (!m_recycledSlots.empty())
			{
				TSlotType slot{m_recycledSlots.back()};
				m_recycledSlots.pop_back();
				return slot;
			}

			TST_ASSERT_MSG(m_nextFreeIndex < m_capacity, "Allocation capacity reached!");
			return m_nextFreeIndex++;
		}

		auto freeSlot(TSlotType p_slot) -> void
		{
			TST_ASSERT_MSG(p_slot < m_capacity, "Invalid slot!");
			m_recycledSlots.emplace_back(p_slot);
		}

		auto capacity() const -> TSlotType { return m_capacity; }

	private:
		TSlotType         m_capacity;
		TSlotType         m_nextFreeIndex{0u};
		Vector<TSlotType> m_recycledSlots;
	};
}
