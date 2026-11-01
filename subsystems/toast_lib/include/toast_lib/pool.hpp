#pragma once

#include <atomic>
#include <span>
#include <threads.h>

#include "handle.hpp"
#include "toast_assert.hpp"
#include "vector.hpp"

namespace toaster
{
	template<typename TData, uint32 PageSize = 64u, uint32 MaxPages = 32u>
	class FixedPool
	{
	public:
		static constexpr uint64 c_numElements{static_cast<uint64>(PageSize) * static_cast<uint64>(MaxPages)};

		using DestructorFn = void(*)(TData *, void *);
		using HandleType   = Handle<TData>;

		FixedPool()
		{
			for (auto &page: m_pages)
				page.store(nullptr, std::memory_order_relaxed);

			mtx_init(&m_allocationMtx, mtx_plain);
		}

		~FixedPool()
		{
			clear();
			mtx_destroy(&m_allocationMtx);
		}

		FixedPool(const FixedPool &)            = delete;
		FixedPool &operator=(const FixedPool &) = delete;

		auto setDestructorFn(DestructorFn p_destructor_fn) { m_destructorFn = p_destructor_fn; }
		auto setDestructorUserData(void *p_user_data) { m_destructorUserData = p_user_data; }

		// Constructs a single TData and uses p_args as it's constructor arguments, returning a handle
		template<typename... TArgs>
		auto emplace(TArgs &&... p_args) -> HandleType
		{
			mtx_lock(&m_allocationMtx);

			uint32 id{0u};
			uint32 magic{1u};

			if (!m_freeIndices.empty())
			{
				id = m_freeIndices.back();
				m_freeIndices.pop_back();

				const uint32 metadata_value{
					m_pages[_get_page_index(id)].load(std::memory_order_relaxed)[_get_slot_index(id)].metadata.metadata.load(std::memory_order_relaxed)
				};
				magic = (metadata_value & c_magicMask);
			}
			else
			{
				TST_PERMA_ASSERT_MSG(m_totalSlots < c_numElements, "Pool capacity reached!");

				id = m_totalSlots++;

				uint32 page_index{_get_page_index(id)};
				if (m_pages[page_index].load(std::memory_order_relaxed) == nullptr)
					_allocate_page(page_index);
			}

			PageSlot *page{m_pages[_get_page_index(id)].load(std::memory_order_relaxed)};
			PageSlot &slot{page[_get_slot_index(id)]};

			std::construct_at(std::launder(reinterpret_cast<TData *>(&slot.storage)), std::forward<TArgs>(p_args)...);

			slot.metadata.metadata.store(c_aliveMask | magic, std::memory_order_release);

			HandleType out_handle{id, magic};
			mtx_unlock(&m_allocationMtx);

			return out_handle;
		}

		// Constructs p_out_handles.size() TDatas and uses the same arguments as each TData's constructor arguments, writing the handles to p_out_handles
		template<typename... TArgs>
		auto emplace_n_same(std::span<HandleType> p_out_handles, TArgs &&... p_args) -> void
		{
			const uint32 count{static_cast<uint32>(p_out_handles.size())};
			TST_ASSERT_MSG(count > 0u, "Cannot emplace 0 elements!");

			mtx_lock(&m_allocationMtx);

			const uint32 required_new_allocations{(count > m_freeIndices.size()) ? (count - static_cast<uint32>(m_freeIndices.size())) : 0u};
			if (m_totalSlots + required_new_allocations > c_numElements)
			{
				TST_PERMA_ASSERT_MSG(false, "Pool capacity reached!");
				mtx_unlock(&m_allocationMtx);
				return;
			}

			if (required_new_allocations > 0u)
			{
				for (uint32 id{m_totalSlots}; id < m_totalSlots + required_new_allocations; ++id)
				{
					uint32 page_index{_get_page_index(id)};
					if (m_pages[page_index].load(std::memory_order_relaxed) == nullptr)
						_allocate_page(page_index);
				}
			}

			for (uint32 i{0u}; i < count; ++i)
			{
				uint32 id{0u};
				uint32 magic{1u};

				if (!m_freeIndices.empty())
				{
					id = m_freeIndices.back();
					m_freeIndices.pop_back();

					const uint32 metadata_value{
						m_pages[_get_page_index(id)].load(std::memory_order_relaxed)[_get_slot_index(id)].metadata.metadata.load(std::memory_order_relaxed)
					};
					magic = (metadata_value & c_magicMask);
				}
				else
					id = m_totalSlots++;

				PageSlot *page{m_pages[_get_page_index(id)].load(std::memory_order_relaxed)};
				PageSlot &slot{page[_get_slot_index(id)]};

				std::construct_at(std::launder(reinterpret_cast<TData *>(&slot.storage)), std::forward<TArgs>(p_args)...);

				slot.metadata.metadata.store(c_aliveMask | magic, std::memory_order_release);

				p_out_handles[i] = HandleType{id, magic};
			}

			mtx_unlock(&m_allocationMtx);
		}

		// Constructs p_out_handles.size() TDatas and uses the arguments from p_args as TData's constructor arguments, writing the handles to p_out_handles
		template<typename TArgs>
		auto emplace_n(std::span<HandleType> p_out_handles, InitialiserList<const TArgs> p_args) -> void
		{
			const uint32 count{static_cast<uint32>(p_out_handles.size())};
			TST_ASSERT_MSG(count > 0u, "Cannot emplace 0 elements!");

			mtx_lock(&m_allocationMtx);

			const uint32 required_new_allocations{(count > m_freeIndices.size()) ? (count - static_cast<uint32>(m_freeIndices.size())) : 0u};
			if (m_totalSlots + required_new_allocations > c_numElements)
			{
				TST_PERMA_ASSERT_MSG(false, "Pool capacity reached!");
				mtx_unlock(&m_allocationMtx);
				return;
			}

			if (required_new_allocations > 0u)
			{
				for (uint32 id{m_totalSlots}; id < m_totalSlots + required_new_allocations; ++id)
				{
					uint32 page_index{_get_page_index(id)};
					if (m_pages[page_index].load(std::memory_order_relaxed) == nullptr)
						_allocate_page(page_index);
				}
			}

			for (uint32 i{0u}; i < count; ++i)
			{
				uint32 id{0u};
				uint32 magic{1u};

				if (!m_freeIndices.empty())
				{
					id = m_freeIndices.back();
					m_freeIndices.pop_back();

					const uint32 metadata_value{
						m_pages[_get_page_index(id)].load(std::memory_order_relaxed)[_get_slot_index(id)].metadata.metadata.load(std::memory_order_relaxed)
					};
					magic = (metadata_value & c_magicMask);
				}
				else
					id = m_totalSlots++;

				PageSlot *page{m_pages[_get_page_index(id)].load(std::memory_order_relaxed)};
				PageSlot &slot{page[_get_slot_index(id)]};

				std::construct_at(std::launder(reinterpret_cast<TData *>(&slot.storage)), p_args[i]);

				slot.metadata.metadata.store(c_aliveMask | magic, std::memory_order_release);

				p_out_handles[i] = HandleType{id, magic};
			}

			mtx_unlock(&m_allocationMtx);
		}

		// Uses the provided function to construct n objects
		template<typename TFunc> requires std::is_invocable_r_v<TData, TFunc, uint32> && std::is_move_constructible_v<TData> // the uint32 is the index in the loop.
		auto emplace_n_fn(std::span<HandleType> p_out_handles, TFunc &&p_func) -> void
		{
			const uint32 count{static_cast<uint32>(p_out_handles.size())};
			TST_ASSERT_MSG(count > 0u, "Cannot emplace 0 elements!");

			mtx_lock(&m_allocationMtx);

			const uint32 required_new_allocations{(count > m_freeIndices.size()) ? (count - static_cast<uint32>(m_freeIndices.size())) : 0u};
			if (m_totalSlots + required_new_allocations > c_numElements)
			{
				TST_PERMA_ASSERT_MSG(false, "Pool capacity reached!");
				mtx_unlock(&m_allocationMtx);
				return;
			}

			if (required_new_allocations > 0u)
			{
				for (uint32 id{m_totalSlots}; id < m_totalSlots + required_new_allocations; ++id)
				{
					uint32 page_index{_get_page_index(id)};
					if (m_pages[page_index].load(std::memory_order_relaxed) == nullptr)
						_allocate_page(page_index);
				}
			}

			for (uint32 i{0u}; i < count; ++i)
			{
				uint32 id{0u};
				uint32 magic{1u};

				if (!m_freeIndices.empty())
				{
					id = m_freeIndices.back();
					m_freeIndices.pop_back();

					const uint32 metadata_value{
						m_pages[_get_page_index(id)].load(std::memory_order_relaxed)[_get_slot_index(id)].metadata.metadata.load(std::memory_order_relaxed)
					};
					magic = (metadata_value & c_magicMask);
				}
				else
					id = m_totalSlots++;

				PageSlot *page{m_pages[_get_page_index(id)].load(std::memory_order_relaxed)};
				PageSlot &slot{page[_get_slot_index(id)]};

				std::construct_at(std::launder(reinterpret_cast<TData *>(&slot.storage)), p_func(i));

				slot.metadata.metadata.store(c_aliveMask | magic, std::memory_order_release);

				p_out_handles[i] = HandleType{id, magic};
			}

			mtx_unlock(&m_allocationMtx);
		}

		#ifndef NDEBUG
		auto operator[](HandleType p_handle) -> TData &
		{
			TData *data{try_get(p_handle)};
			TST_PERMA_ASSERT_MSG(data, "Failed to get data from FixedPool!");
			return *data;
		}
		#else
		// Technically unsafe, but in release builds, speed matters more. If you are unsure about the validity of p_handle, just use try_get
		auto operator[](HandleType p_handle) -> TData &
		{
			const uint32 id{p_handle.getId()};
			const uint32 page_index{_get_page_index(id)};
			return *reinterpret_cast<TData *>(&m_pages[page_index].load(std::memory_order_acquire)[_get_slot_index(id)].storage.bytes);
		}
		#endif

		#ifndef NDEBUG
		auto operator[](HandleType p_handle) const -> const TData &
		{
			const TData *data{try_get(p_handle)};
			TST_PERMA_ASSERT_MSG(data, "Failed to get data from FixedPool!");
			return *data;
		}
		#else
		// Technically unsafe, but in release builds, speed matters more. If you are unsure about the validity of p_handle, just use try_get
		auto operator[](HandleType p_handle) const -> const TData &
		{
			const uint32 id{p_handle.getId()};
			const uint32 page_index{_get_page_index(id)};
			return *reinterpret_cast<const TData *>(&m_pages[page_index].load(std::memory_order_acquire)[_get_slot_index(id)].storage.bytes);
		}
		#endif

		// You should be using try_get most of the time
		auto try_get(HandleType p_handle) -> TData *
		{
			const uint32 id{p_handle.getId()};
			const uint32 page_index{_get_page_index(id)};

			PageSlot *page{m_pages[page_index].load(std::memory_order_acquire)};
			if (!page)
				return nullptr;

			PageSlot &   slot{page[_get_slot_index(id)]};
			const uint32 metadata_value{slot.metadata.metadata.load(std::memory_order_acquire)};
			if ((metadata_value & c_magicMask) != p_handle.getMagic() || !(metadata_value & c_aliveMask))
				return nullptr;

			return reinterpret_cast<TData *>(&slot.storage);
		}

		// You should be using try_get most of the time
		auto try_get(HandleType p_handle) const -> const TData *
		{
			return const_cast<FixedPool *>(this)->try_get(p_handle);
		}

		auto destroy(HandleType p_handle) -> void
		{
			const uint32 id{p_handle.getId()};
			const uint32 page_index{_get_page_index(id)};

			mtx_lock(&m_allocationMtx);

			PageSlot *page{m_pages[page_index].load(std::memory_order_relaxed)};
			if (!page) [[unlikely]]
			{
				mtx_unlock(&m_allocationMtx);
				return;
			}

			PageSlot &   slot{page[_get_slot_index(id)]};
			const uint32 metadata_value{slot.metadata.metadata.load(std::memory_order_relaxed)};
			if ((metadata_value & c_magicMask) != p_handle.getMagic() || !(metadata_value & c_aliveMask))
			{
				mtx_unlock(&m_allocationMtx);
				return;
			}

			const uint32 next_magic{((metadata_value & c_magicMask) + 1u) & c_magicMask}; // Increment the magic
			slot.metadata.metadata.store(next_magic, std::memory_order_release); // Ignore the alive mask because the slot is now dead

			auto data{reinterpret_cast<TData *>(&slot.storage)};
			if (m_destructorFn)
				m_destructorFn(data, m_destructorUserData);

			std::destroy_at(data);
			m_freeIndices.push_back(id);

			mtx_unlock(&m_allocationMtx);
		}

		// Destroys a range of handles
		auto destroy_n(InitialiserList<const HandleType> p_handles) -> void
		{
			mtx_lock(&m_allocationMtx);

			m_freeIndices.reserve(m_freeIndices.size() + p_handles.size());

			uint32    last_page_index{UINT32_MAX};
			PageSlot *last_page{nullptr};

			for (const HandleType handle: p_handles)
			{
				const uint32 id{handle.getId()};
				const uint32 page_index{_get_page_index(id)};

				if (page_index != last_page_index)
				{
					last_page       = m_pages[page_index].load(std::memory_order_relaxed);
					last_page_index = page_index;
				}

				if (!last_page) [[unlikely]] continue;

				PageSlot &   slot{last_page[_get_slot_index(id)]};
				const uint32 metadata_value{slot.metadata.metadata.load(std::memory_order_relaxed)};
				if ((metadata_value & c_magicMask) != handle.getMagic() || !(metadata_value & c_aliveMask))
					continue;

				const uint32 next_magic{((metadata_value & c_magicMask) + 1u) & c_magicMask}; // Increment the magic
				slot.metadata.metadata.store(next_magic, std::memory_order_release); // Ignore the alive mask because the slot is now dead

				auto data{reinterpret_cast<TData *>(&slot.storage)};
				if (m_destructorFn)
					m_destructorFn(data, m_destructorUserData);

				std::destroy_at(data);
				m_freeIndices.push_back(id);
			}

			mtx_unlock(&m_allocationMtx);
		}

		auto clear() -> void
		{
			mtx_lock(&m_allocationMtx);

			for (uint32 i{0u}; i < m_totalSlots; ++i)
			{
				uint32    page_index{_get_page_index(i)};
				PageSlot *page{m_pages[page_index].load(std::memory_order_relaxed)};

				if (page)
				{
					PageSlot &slot{page[_get_slot_index(i)]};
					if (slot.metadata.metadata.load(std::memory_order_relaxed) & c_aliveMask)
					{
						auto data{reinterpret_cast<TData *>(&page[_get_slot_index(i)].storage)};
						if (m_destructorFn)
							m_destructorFn(data, m_destructorUserData);

						std::destroy_at(data);
					}
				}
			}

			for (uint32 i{0u}; i < MaxPages; ++i)
			{
				PageSlot *page{m_pages[i].load(std::memory_order_relaxed)};
				if (page)
				{
					for (uint32 j{0u}; j < PageSize; ++j)
						std::destroy_at(&page[j].metadata);
					rpfree(page);
					m_pages[i].store(nullptr, std::memory_order_relaxed);
				}
			}

			m_freeIndices.clear();
			m_totalSlots = 0u;

			mtx_unlock(&m_allocationMtx);
		}

		auto is_valid(HandleType p_handle) const -> bool
		{
			const uint32 id{p_handle.getId()};
			const uint32 page_index{_get_page_index(id)};

			PageSlot *page{m_pages[page_index].load(std::memory_order_relaxed)};
			if (!page)
				return false;

			PageSlot &   slot{page[_get_slot_index(id)]};
			const uint32 metadata_value{slot.metadata.metadata.load(std::memory_order_relaxed)};
			if ((metadata_value & c_magicMask) != p_handle.getMagic() || !(metadata_value & c_aliveMask))
				return false;

			return true;
		}

		// Faster than manually iterating because there is a page caching setup
		template<typename TFunc> requires std::is_invocable_r_v<void, TFunc, TData &, uint32>
		auto for_each(std::span<const HandleType> p_range, TFunc &&p_func) -> void
		{
			uint32    last_page_index{UINT32_MAX};
			PageSlot *last_page{nullptr};

			for (uint32 i{0u}; i < p_range.size(); ++i)
			{
				const HandleType handle{p_range[i]};
				const uint32     id{handle.getId()};
				const uint32     page_index{_get_page_index(id)};

				if (page_index != last_page_index)
				{
					last_page       = m_pages[page_index].load(std::memory_order_relaxed);
					last_page_index = page_index;
				}

				if (!last_page)
				{
					TST_ASSERT(false);
					continue;
				}

				PageSlot &   slot{last_page[_get_slot_index(id)]};
				const uint32 metadata_value{slot.metadata.metadata.load(std::memory_order_relaxed)};
				if ((metadata_value & c_magicMask) != handle.getMagic() || !(metadata_value & c_aliveMask))
					continue;

				auto data{reinterpret_cast<TData *>(&slot.storage)};

				p_func(*data, i);
			}
		}

		template<typename TFunc> requires std::is_invocable_r_v<void, TFunc, const TData &, uint32>
		auto for_each(std::span<const HandleType> p_range, TFunc &&p_func) const -> void
		{
			uint32    last_page_index{UINT32_MAX};
			PageSlot *last_page{nullptr};

			for (uint32 i{0u}; i < p_range.size(); ++i)
			{
				const HandleType handle{p_range[i]};
				const uint32     id{handle.getId()};
				const uint32     page_index{_get_page_index(id)};

				if (page_index != last_page_index)
				{
					last_page       = m_pages[page_index].load(std::memory_order_relaxed);
					last_page_index = page_index;
				}

				if (!last_page) [[unlikely]] continue;

				const PageSlot &slot{last_page[_get_slot_index(id)]};
				const uint32    metadata_value{slot.metadata.metadata.load(std::memory_order_relaxed)};
				if ((metadata_value & c_magicMask) != handle.getMagic() || !(metadata_value & c_aliveMask))
					continue;

				const auto data{reinterpret_cast<const TData *>(&slot.storage)};

				p_func(*data, i);
			}
		}

		// This is here because it may be useful, but it is not cache efficient...
		template<typename TFunc>
		auto for_each_alive(TFunc &&p_func) -> void
		{
			mtx_lock(&m_allocationMtx);

			for (uint32 i{0u}; i < m_totalSlots; ++i)
			{
				const uint32 page_index{_get_page_index(i)};

				PageSlot *page{m_pages[page_index].load(std::memory_order_relaxed)};
				PageSlot &slot{page[_get_slot_index(i)]};

				const uint32 metadata_value{slot.metadata.metadata.load(std::memory_order_relaxed)};
				if (metadata_value & c_aliveMask)
					p_func(*reinterpret_cast<TData *>(&slot.storage.bytes), i);
			}

			mtx_unlock(&m_allocationMtx);
		}

	private:
		struct StorageSlot
		{
			alignas(alignof(TData)) uint8 bytes[sizeof(TData)];
		};

		static constexpr uint32 c_magicMask{~TST_BIT(31u)}; // 01111111111111111111111111111111
		static constexpr uint32 c_aliveMask{~c_magicMask};  // 10000000000000000000000000000000

		struct Metadata
		{
			std::atomic_uint32_t metadata{1u}; // Magic is 1, alive is false
		};

		struct PageSlot
		{
			StorageSlot storage;
			Metadata    metadata;
		};

		static constexpr uint64 c_pageSizeBytes{static_cast<uint64>(PageSize) * sizeof(PageSlot)};
		static constexpr uint64 c_metadataPageSizeBytes{static_cast<uint64>(PageSize) * sizeof(Metadata)};

		static constexpr auto _get_page_index(uint32 p_id) -> uint32 { return p_id / PageSize; }
		static constexpr auto _get_slot_index(uint32 p_id) -> uint32 { return p_id % PageSize; }

		void _allocate_page(uint32 p_page_index)
		{
			auto new_page{static_cast<PageSlot *>(rpmalloc(c_pageSizeBytes))};

			TST_PERMA_ASSERT_MSG(new_page, "Failed to allocate FixedPool page!");

			for (uint32 i{0u}; i < PageSize; ++i)
				new(std::launder(&new_page[i].metadata)) Metadata{};

			m_pages[p_page_index].store(new_page, std::memory_order_release);
		}

		mtx_t                                    m_allocationMtx;
		Array<std::atomic<PageSlot *>, MaxPages> m_pages;
		Vector<uint32>                           m_freeIndices;

		DestructorFn m_destructorFn{nullptr};
		void *       m_destructorUserData{nullptr};
		uint32       m_totalSlots{0u};
	};
}
