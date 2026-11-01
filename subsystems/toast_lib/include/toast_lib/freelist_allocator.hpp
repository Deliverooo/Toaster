#pragma once

#include <span>

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
	};

	template<typename Type>
	auto TST_LIB_API freelist_allocator_create(FreelistAllocator<Type> &p_out_allocator, Type p_capacity, Type p_first_slot = Type{0}) -> void
	{
		p_out_allocator.capacity     = p_capacity + p_first_slot;
		p_out_allocator.nextFreeSlot = p_first_slot;
	}

	template<typename Type>
	auto TST_GPU_API freelist_allocator_alloc_slot(FreelistAllocator<Type> &p_allocator) -> Type
	{
		if (!p_allocator.freeSlots.empty())
		{
			Type slot{p_allocator.freeSlots.back()};
			p_allocator.freeSlots.pop_back();
			return slot;
		}

		TST_ASSERT_MSG(p_allocator.nextFreeSlot < p_allocator.capacity, "Allocation capacity exceeded when attempting to allocate slot using freelist allocator");
		return p_allocator.nextFreeSlot++;
	}

	template<typename Type>
	auto TST_GPU_API freelist_allocator_alloc_slots(FreelistAllocator<Type> &p_allocator, std::span<Type> p_out_slots) -> void
	{
		const Type count{static_cast<Type>(p_out_slots.size())};
		const Type free_slot_count{static_cast<Type>(p_allocator.freeSlots.size())};
		const Type required_new_slots{(count > free_slot_count) ? count - free_slot_count : static_cast<Type>(0)};

		TST_ASSERT_MSG(p_allocator.nextFreeSlot + required_new_slots < p_allocator.capacity,
					   "Allocation capacity exceeded when attempting to allocate slot using freelist allocator");

		if (free_slot_count >= required_new_slots)
		{
			for (uint32 &slot: p_out_slots)
			{
				slot = p_allocator.freeSlots.back();
				p_allocator.freeSlots.pop_back();
			}
			return;
		}

		// If the number of free lists is smaller than the required count, drain the free lists and then allocate the remaining ones
		for (Type i{static_cast<Type>(0)}; i < free_slot_count; ++i)
		{
			p_out_slots[i] = p_allocator.freeSlots.back();
			p_allocator.freeSlots.pop_back();
		}

		for (Type i{free_slot_count}; i < count; ++i)
			p_out_slots[i] = p_allocator.nextFreeSlot++;
	}

	template<typename Type>
	auto TST_GPU_API freelist_allocator_free_slot(FreelistAllocator<Type> &p_allocator, Type p_slot) -> void
	{
		TST_ASSERT_MSG(p_slot < p_allocator.capacity, "Attempted to free invalid slot from freelist allocator");
		p_allocator.freeSlots.push_back(p_slot);
	}

	template<typename Type>
	auto TST_GPU_API freelist_allocator_free_slots(FreelistAllocator<Type> &p_allocator, InitialiserList<const Type> p_slots) -> void
	{
		p_allocator.freeSlots.insert(p_allocator.freeSlots.end(), p_slots.begin(), p_slots.end());
	}

	// Is this too much...?
	template<typename Type, typename TFunc> requires std::is_invocable_r_v<Type, TFunc, const Type>
	auto TST_GPU_API freelist_allocator_free_slots_fn(FreelistAllocator<Type> &p_allocator, InitialiserList<const Type> p_slots, TFunc &&p_func) -> void
	{
		p_allocator.freeSlots.reserve(p_allocator.freeSlots.size() + p_slots.size());
		for (const Type slot: p_slots)
			p_allocator.freeSlots.push_back(p_func(slot));
	}
}
