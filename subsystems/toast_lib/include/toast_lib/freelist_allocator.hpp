#pragma once

#include <span>

#include "initialiser_list.hpp"
#include "toast_assert.hpp"
#include "vector.hpp"

namespace toaster
{
	template<typename Type = uint32> requires std::is_integral_v<Type>
	struct TST_LIB_API FreelistAllocator
	{
		Vector<Type> freeSlots;
		Type         capacity;
		Type         nextFreeSlot;

		constexpr auto alloc_slot() -> Type
		{
			if (!freeSlots.empty())
			{
				Type slot{freeSlots.back()};
				freeSlots.pop_back();
				return slot;
			}

			TST_ASSERT_MSG(nextFreeSlot < capacity, "Allocation capacity exceeded when attempting to allocate slot using freelist allocator");
			return nextFreeSlot++;
		}

		constexpr auto alloc_slots(std::span<Type> p_out_slots) -> void
		{
			const Type count{static_cast<Type>(p_out_slots.size())};
			const Type free_slot_count{static_cast<Type>(freeSlots.size())};
			const Type required_new_slots{(count > free_slot_count) ? count - free_slot_count : static_cast<Type>(0)};

			TST_ASSERT_MSG(nextFreeSlot + required_new_slots < capacity, "Allocation capacity exceeded when attempting to allocate slot using freelist allocator");

			if (count <= free_slot_count)
			{
				for (uint32 &slot: p_out_slots)
				{
					slot = freeSlots.back();
					freeSlots.pop_back();
				}
				return;
			}

			// If the number of free lists is smaller than the required count, drain the free lists and then allocate the remaining ones
			for (Type i{static_cast<Type>(0)}; i < free_slot_count; ++i)
			{
				p_out_slots[i] = freeSlots.back();
				freeSlots.pop_back();
			}

			for (Type i{free_slot_count}; i < count; ++i)
				p_out_slots[i] = nextFreeSlot++;
		}

		constexpr auto free_slot(Type p_slot) -> void
		{
			TST_ASSERT_MSG(p_slot < capacity, "Attempted to free invalid slot from freelist allocator");
			freeSlots.push_back(p_slot);
		}

		constexpr auto free_slots(InitialiserList<const Type> p_slots) -> void
		{
			freeSlots.insert(freeSlots.end(), p_slots.begin(), p_slots.end());
		}

		// Is this too much...?
		template<typename TFunc> requires std::is_invocable_r_v<Type, TFunc, const Type>
		auto free_slots_fn(InitialiserList<const Type> p_slots, TFunc &&p_func) -> void
		{
			freeSlots.reserve(freeSlots.size() + p_slots.size());
			for (const Type slot: p_slots)
				freeSlots.push_back(p_func(slot));
		}
	};
}
